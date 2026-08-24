#pragma once

#include <windows.h>
#include <cstdint>

namespace ppc_bridge {

constexpr wchar_t kMappingName[] = L"Local\\PremiumPlusCombo.Rogue.GameBridge.v1";
constexpr std::uint32_t kMagic = 0x50435042u; // 'PPCB'
constexpr std::uint32_t kVersion = 1u;
constexpr std::size_t kRingSize = 8192u;
constexpr ULONGLONG kHeartbeatTimeoutMs = 2000u;

enum EventFlags : std::uint32_t {
    KeyDown = 0x01u,
    KeyUp = 0x02u,
    Extended = 0x04u
};

struct Event {
    volatile LONG64 sequence;
    std::uint32_t virtualKey;
    std::uint32_t scanCode;
    std::uint32_t flags;
    std::uint32_t reserved;
};

struct SharedState {
    std::uint32_t magic;
    std::uint32_t version;
    volatile LONG64 writeSequence;
    volatile LONG64 gameHeartbeatMs;
    Event events[kRingSize];
};

inline bool IsHeaderValid(const SharedState* s) noexcept {
    return s && s->magic == kMagic && s->version == kVersion;
}

} // namespace ppc_bridge
