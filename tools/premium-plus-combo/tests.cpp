#include "core.hpp"
#include "input_transport.hpp"
#include "game_bridge_sdk.hpp"
#include <cassert>
#include <iostream>
#include <array>

#pragma comment(lib, "user32.lib")

static std::array<INPUT, 6> Tap3(char a, char b, char c) {
    std::array<INPUT, 6> in{};
    const int vks[3] = {a, b, c};
    int n = 0;
    for (int vk : vks) {
        in[n].type = INPUT_KEYBOARD;
        in[n].ki.wVk = static_cast<WORD>(vk);
        ++n;
        in[n].type = INPUT_KEYBOARD;
        in[n].ki.wVk = static_cast<WORD>(vk);
        in[n].ki.dwFlags = KEYEVENTF_KEYUP;
        ++n;
    }
    return in;
}

int main() {
    using namespace ppc;
    assert(minorRate(Mode::Maximum) == 120);
    assert(minorRate(Mode::Turbo) == 240);

    Settings s;
    assert(rRate(s, Mode::Maximum) == 25);
    assert(rRate(s, Mode::Turbo) == 40);
    assert(s.startHotkey == kVkCapsLock);
    assert(s.stopHotkey == kVkCapsLock);

    // Default CAPS LOCK acts as a start/stop toggle.
    assert(hotkeyAction(kVkCapsLock, false, s) == 1);
    assert(hotkeyAction(kVkCapsLock, true, s) == -1);
    assert(hotkeyAction('A', false, s) == 0);

    // Start and stop can still be assigned independently.
    s.startHotkey = 0x78; // F9
    s.stopHotkey = 0x79;  // F10
    assert(hotkeyAction(0x78, false, s) == 1);
    assert(hotkeyAction(0x79, true, s) == -1);

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

    // DirectInput fallback must convert virtual keys into physical scan-code input.
    INPUT source{};
    source.type = INPUT_KEYBOARD;
    source.ki.wVk = '8';
    INPUT converted{};
    assert(ppc_input::ConvertKeyboardInputToScanCode(source, converted));
    assert(converted.ki.wVk == 0);
    assert(converted.ki.wScan != 0);
    assert((converted.ki.dwFlags & KEYEVENTF_SCANCODE) != 0);

    // Official in-game bridge: verify that it does not collapse high-rate events
    // to render-frame frequency. Every 120/240 scheduler cycle must retain one
    // key-down for each of 8, 9 and 0.
    auto* shared = ppc_input::EnsureBridgeMapping();
    assert(shared != nullptr);
    ppc_game::Receiver receiver;
    assert(receiver.Open());
    assert(receiver.Connected());
    assert(ppc_input::BridgeConnected());

    auto runBridgeCycles = [&](int cycles) {
        int c8 = 0, c9 = 0, c0 = 0;
        auto batch = Tap3('8', '9', '0');
        for (int i = 0; i < cycles; ++i) {
            assert(ppc_input::PublishBridgeBatch(static_cast<UINT>(batch.size()), batch.data()));
            receiver.DrainKeyTaps([&](int vk) {
                if (vk == '8') ++c8;
                else if (vk == '9') ++c9;
                else if (vk == '0') ++c0;
            });
        }
        assert(c8 == cycles);
        assert(c9 == cycles);
        assert(c0 == cycles);
    };
    runBridgeCycles(minorRate(Mode::Maximum));
    runBridgeCycles(minorRate(Mode::Turbo));
    receiver.Close();

    std::cout << "Premium Plus Combo core/input/bridge tests: PASS\n";
    return 0;
}
