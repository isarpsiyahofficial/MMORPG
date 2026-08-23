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

    s.rRateMaximum = -8;
    s.rRateTurbo = 999;
    s.cureBar = 0;
    s.cureSlot = 99;
    s = sanitize(s);
    assert(s.rRateMaximum == 1);
    assert(s.rRateTurbo == 100);
    assert(s.cureBar == 1);
    assert(s.cureSlot == 8);

    const std::int64_t freq = 10'000'000;
    const std::int64_t now = 5 * freq;
    auto gate = beginCure(true, true, now, freq);
    assert(gate.rWasEligible);
    assert(!rMayFire(true, true, now + freq, gate));
    assert(rMayFire(true, true, now + 2 * freq, gate));

    auto gate2 = beginCure(true, false, now, freq);
    assert(!gate2.rWasEligible);
    assert(!rMayFire(true, false, now + 3 * freq, gate2));

    // Program must never auto-start because TAB happened to be held while
    // launching. It arms only after both control keys have been released.
    StartStopLatch latch;
    latch.prime(true, false);
    assert(!latch.armed);
    assert(latch.update(true, false) == 0);
    assert(latch.update(false, false) == 0);
    assert(latch.armed);
    assert(latch.update(true, false) == 1);
    assert(latch.update(true, false) == 0);
    assert(latch.update(false, false) == 0);
    assert(latch.update(false, true) == -1);
    assert(latch.update(false, true) == 0);

    // Stop wins if both edges somehow occur in the same sample.
    StartStopLatch priority;
    priority.prime(false, false);
    assert(priority.update(true, true) == -1);

    std::cout << "Premium Plus Combo core tests: PASS\n";
    return 0;
}
