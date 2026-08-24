#pragma once

#include <windows.h>
#include <array>
#include <atomic>

namespace ppc_input {

inline std::atomic<unsigned long long> g_sendFailures{0};

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

// DirectInput-oriented transport. The previous build emitted virtual-key events.
// Games polling DirectInput keyboard state commonly expect physical scan-code input.
inline UINT SendInputScanCodeCompatible(UINT count, LPINPUT inputs, int cbSize) noexcept {
    if (!inputs || count == 0 || cbSize != sizeof(INPUT))
        return ::SendInput(count, inputs, cbSize);

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
