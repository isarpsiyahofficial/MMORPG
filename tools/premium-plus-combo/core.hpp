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

struct RisingEdge {
    bool previous{false};

    void prime(bool currentlyDown) noexcept {
        previous = currentlyDown;
    }

    bool update(bool currentlyDown) noexcept {
        const bool rising = currentlyDown && !previous;
        previous = currentlyDown;
        return rising;
    }
};

struct StartStopLatch {
    RisingEdge tab;
    RisingEdge caps;
    bool armed{false};

    void prime(bool tabDown, bool capsDown) noexcept {
        tab.prime(tabDown);
        caps.prime(capsDown);
        armed = !tabDown && !capsDown;
    }

    // Returns +1 for start, -1 for stop, 0 for no action.
    int update(bool tabDown, bool capsDown) noexcept {
        const bool tabRise = tab.update(tabDown);
        const bool capsRise = caps.update(capsDown);
        if (!armed) {
            if (!tabDown && !capsDown) armed = true;
            return 0;
        }
        if (capsRise) return -1; // stop always has priority
        if (tabRise) return 1;
        return 0;
    }
};

} // namespace ppc
