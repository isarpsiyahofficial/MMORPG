#pragma once
#include <array>
#include <algorithm>
#include <cstdint>

namespace ppc {

enum class Mode : int { Maximum = 0, Turbo = 1 };

struct Settings {
    std::array<int, 3> comboVk{{'8','9','0'}};
    bool rEnabled{true};
    int rRateMaximum{25};
    int rRateTurbo{40};
    bool cureEnabled{true};
    int cureBar{2};
    int cureSlot{6};
    int cureHotkey{'C'};
};

inline int minorRate(Mode mode) noexcept {
    return mode == Mode::Turbo ? 240 : 120;
}

inline int rRate(const Settings& s, Mode mode) noexcept {
    const int raw = mode == Mode::Turbo ? s.rRateTurbo : s.rRateMaximum;
    return std::clamp(raw, 1, 100);
}

inline Settings sanitize(Settings s) noexcept {
    for (int& k : s.comboVk) {
        if (k <= 0 || k > 0xFE) k = '8';
    }
    s.rRateMaximum = std::clamp(s.rRateMaximum, 1, 100);
    s.rRateTurbo = std::clamp(s.rRateTurbo, 1, 100);
    s.cureBar = std::clamp(s.cureBar, 1, 8);
    s.cureSlot = std::clamp(s.cureSlot, 1, 8);
    if (s.cureHotkey <= 0 || s.cureHotkey > 0xFE) s.cureHotkey = 'C';
    return s;
}

struct CureGate {
    bool rWasEligible{false};
    std::int64_t suppressUntilTicks{0};
};

inline CureGate beginCure(bool active, bool rEnabled, std::int64_t nowTicks, std::int64_t ticksPerSecond) noexcept {
    CureGate g;
    g.rWasEligible = active && rEnabled;
    g.suppressUntilTicks = nowTicks + (ticksPerSecond * 2);
    return g;
}

inline bool rMayFire(bool active, bool rEnabled, std::int64_t nowTicks, const CureGate& gate) noexcept {
    return active && rEnabled && nowTicks >= gate.suppressUntilTicks;
}

} // namespace ppc
