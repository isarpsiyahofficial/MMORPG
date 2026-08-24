#include "core.hpp"
#include <cassert>
#include <iostream>

int main() {
    using namespace ppc;
    assert(minorRate(Mode::Maximum) == 120);
    assert(minorRate(Mode::Turbo) == 240);

    Settings s;
    assert(rRate(s, Mode::Maximum) == 25);
    assert(rRate(s, Mode::Turbo) == 40);
    assert(s.startHotkey == kVkCapsLock);
    assert(s.stopHotkey == kVkCapsLock);

    // Default: one Caps Lock press starts, the next stops.
    assert(hotkeyAction(kVkCapsLock, false, s) == 1);
    assert(hotkeyAction(kVkCapsLock, true, s) == -1);

    // Start and stop can later be assigned independently.
    s.startHotkey = 0x78; // F9
    s.stopHotkey = 0x79;  // F10
    assert(hotkeyAction(0x78, false, s) == 1);
    assert(hotkeyAction(0x78, true, s) == 1);
    assert(hotkeyAction(0x79, true, s) == -1);
    assert(hotkeyAction(0x79, false, s) == -1);
    assert(hotkeyAction('A', false, s) == 0);

    s.rRateMaximum = -8;
    s.rRateTurbo = 999;
    s.cureBar = 0;
    s.cureSlot = 99;
    s.startHotkey = -1;
    s.stopHotkey = 999;
    s = sanitize(s);
    assert(s.rRateMaximum == 1);
    assert(s.rRateTurbo == 100);
    assert(s.cureBar == 1);
    assert(s.cureSlot == 8);
    assert(s.startHotkey == kVkCapsLock);
    assert(s.stopHotkey == kVkCapsLock);

    const std::int64_t freq = 10'000'000;
    const std::int64_t now = 5 * freq;
    auto gate = beginCure(true, true, now, freq);
    assert(gate.rWasEligible);
    assert(!rMayFire(true, true, now + freq, gate));
    assert(rMayFire(true, true, now + 2 * freq, gate));

    auto gate2 = beginCure(true, false, now, freq);
    assert(!gate2.rWasEligible);
    assert(!rMayFire(true, false, now + 3 * freq, gate2));

    std::cout << "Premium Plus Combo core tests: PASS\n";
    return 0;
}
