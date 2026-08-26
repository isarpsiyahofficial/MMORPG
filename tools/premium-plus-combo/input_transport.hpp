#pragma once

#include <windows.h>
#include <array>
#include <atomic>
#include "bridge_protocol.hpp"

namespace ppc_input {

inline std::atomic<unsigned long long> g_sendFailures{0};
inline HANDLE g_bridgeMap{};
inline ppc_bridge::SharedState* g_bridge{};
inline SRWLOCK g_bridgeWriteLock = SRWLOCK_INIT;

inline ppc_bridge::SharedState* EnsureBridgeMapping() noexcept {
    if (g_bridge) return g_bridge;

    HANDLE map = CreateFileMappingW(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0,
                                    static_cast<DWORD>(sizeof(ppc_bridge::SharedState)),
                                    ppc_bridge::kMappingName);
    if (!map) return nullptr;

    void* view = MapViewOfFile(map, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(ppc_bridge::SharedState));
    if (!view) {
        CloseHandle(map);
        return nullptr;
    }

    auto* shared = static_cast<ppc_bridge::SharedState*>(view);
    if (!ppc_bridge::IsHeaderValid(shared)) {
        ZeroMemory(shared, sizeof(*shared));
        shared->magic = ppc_bridge::kMagic;
        shared->version = ppc_bridge::kVersion;
        MemoryBarrier();
    }

    // This executable is one translation unit, but the compare/exchange also makes
    // lazy initialization safe when combo/R/cure workers race on first use.
    if (InterlockedCompareExchangePointer(reinterpret_cast<PVOID volatile*>(&g_bridge), shared, nullptr) == nullptr) {
        g_bridgeMap = map;
        return shared;
    }

    UnmapViewOfFile(shared);
    CloseHandle(map);
    return g_bridge;
}

inline bool BridgeConnected() noexcept {
    auto* shared = EnsureBridgeMapping();
    if (!ppc_bridge::IsHeaderValid(shared)) return false;
    const ULONGLONG now = GetTickCount64();
    const LONG64 hb = InterlockedCompareExchange64(&shared->gameHeartbeatMs, 0, 0);
    return hb > 0 && now >= static_cast<ULONGLONG>(hb)
        && (now - static_cast<ULONGLONG>(hb)) <= ppc_bridge::kHeartbeatTimeoutMs;
}

inline int VirtualKeyFromInput(const INPUT& in) noexcept {
    if (in.type != INPUT_KEYBOARD) return 0;
    if (in.ki.wVk != 0) return static_cast<int>(in.ki.wVk);
    if ((in.ki.dwFlags & KEYEVENTF_SCANCODE) != 0 && in.ki.wScan != 0)
        return static_cast<int>(MapVirtualKeyW(in.ki.wScan, MAPVK_VSC_TO_VK_EX));
    return 0;
}

inline bool PublishBridgeBatch(UINT count, const INPUT* inputs) noexcept {
    auto* shared = EnsureBridgeMapping();
    if (!shared || !BridgeConnected()) return false;

    for (UINT i = 0; i < count; ++i)
        if (inputs[i].type != INPUT_KEYBOARD || VirtualKeyFromInput(inputs[i]) == 0)
            return false;

    AcquireSRWLockExclusive(&g_bridgeWriteLock);
    for (UINT i = 0; i < count; ++i) {
        const INPUT& in = inputs[i];
        const int vk = VirtualKeyFromInput(in);
        const UINT scEx = in.ki.wScan != 0
            ? static_cast<UINT>(in.ki.wScan)
            : MapVirtualKeyW(static_cast<UINT>(vk), MAPVK_VK_TO_VSC_EX);

        const LONG64 next = InterlockedCompareExchange64(&shared->writeSequence, 0, 0) + 1;
        ppc_bridge::Event& e = shared->events[static_cast<std::size_t>(next) % ppc_bridge::kRingSize];
        e.virtualKey = static_cast<std::uint32_t>(vk);
        e.scanCode = scEx & 0xFFu;
        e.flags = (in.ki.dwFlags & KEYEVENTF_KEYUP) ? ppc_bridge::KeyUp : ppc_bridge::KeyDown;
        if ((in.ki.dwFlags & KEYEVENTF_EXTENDEDKEY) != 0 || (scEx & 0xFF00u) != 0)
            e.flags |= ppc_bridge::Extended;
        e.reserved = 0;
        MemoryBarrier();
        InterlockedExchange64(&e.sequence, next);
        MemoryBarrier();
        InterlockedExchange64(&shared->writeSequence, next);
    }
    ReleaseSRWLockExclusive(&g_bridgeWriteLock);
    return true;
}

inline bool ConvertKeyboardInputToScanCode(const INPUT& source, INPUT& out) noexcept {
    out = source;
    if (source.type != INPUT_KEYBOARD) return true;

    // Already scan-code or Unicode input: keep it untouched.
    if ((source.ki.dwFlags & (KEYEVENTF_SCANCODE | KEYEVENTF_UNICODE)) != 0 || source.ki.wVk == 0)
        return true;

    const UINT scEx = MapVirtualKeyW(source.ki.wVk, MAPVK_VK_TO_VSC_EX);
    if (scEx == 0) return false;

    out.ki.wVk = 0;
    out.ki.wScan = static_cast<WORD>(scEx & 0xFFu);
    out.ki.dwFlags |= KEYEVENTF_SCANCODE;

    const UINT prefix = scEx & 0xFF00u;
    if (prefix == 0xE000u || prefix == 0xE100u)
        out.ki.dwFlags |= KEYEVENTF_EXTENDEDKEY;

    return true;
}

// Preferred path: when the official game-side bridge is present, deliver every
// down/up event to the client directly. This preserves 120/240 attempts per key
// even when the game renders at a much lower frame rate because the client can
// drain multiple queued actions during one frame.
//
// Compatibility path: if the client has not integrated the bridge yet, emit
// physical scan-code input for DirectInput instead of the old virtual-key stream.
inline UINT SendInputScanCodeCompatible(UINT count, LPINPUT inputs, int cbSize) noexcept {
    if (!inputs || count == 0 || cbSize != sizeof(INPUT))
        return ::SendInput(count, inputs, cbSize);

    if (PublishBridgeBatch(count, inputs))
        return count;

    // Current combo batches are at most 48 INPUT records. Keep conversion allocation-free.
    if (count > 64)
        return ::SendInput(count, inputs, cbSize);

    std::array<INPUT, 64> converted{};
    for (UINT i = 0; i < count; ++i) {
        if (!ConvertKeyboardInputToScanCode(inputs[i], converted[i])) {
            ++g_sendFailures;
            return 0;
        }
    }

    const UINT sent = ::SendInput(count, converted.data(), sizeof(INPUT));
    if (sent != count) ++g_sendFailures;
    return sent;
}

inline unsigned long long SendFailureCount() noexcept {
    return g_sendFailures.load(std::memory_order_relaxed);
}

} // namespace ppc_input
