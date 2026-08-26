#include "input_transport.hpp"
#include "game_bridge_sdk.hpp"
#include "core.hpp"
#include <cassert>
#include <array>
#include <iostream>

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

static std::array<INPUT, 2> Tap1(char vk) {
    std::array<INPUT, 2> in{};
    in[0].type = INPUT_KEYBOARD;
    in[0].ki.wVk = static_cast<WORD>(vk);
    in[1].type = INPUT_KEYBOARD;
    in[1].ki.wVk = static_cast<WORD>(vk);
    in[1].ki.dwFlags = KEYEVENTF_KEYUP;
    return in;
}

int main() {
    auto* shared = ppc_input::EnsureBridgeMapping();
    assert(shared != nullptr);

    ppc_game::Receiver receiver;
    assert(receiver.Open());
    assert(receiver.Connected());
    assert(ppc_input::BridgeConnected());

    auto runCombo = [&](int cycles) {
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

    // One scheduler cycle contains exactly one tap for every configured combo key.
    // Therefore 120 cycles/s == 120 taps/s for each of 8,9,0 and 240 cycles/s
    // == 240 taps/s for each key. The bridge must not collapse events to frame rate.
    runCombo(ppc::minorRate(ppc::Mode::Maximum));
    runCombo(ppc::minorRate(ppc::Mode::Turbo));

    auto runR = [&](int count) {
        int seen = 0;
        auto tap = Tap1('R');
        for (int i = 0; i < count; ++i) {
            assert(ppc_input::PublishBridgeBatch(static_cast<UINT>(tap.size()), tap.data()));
            receiver.DrainKeyTaps([&](int vk) { if (vk == 'R') ++seen; });
        }
        assert(seen == count);
    };

    ppc::Settings settings;
    runR(ppc::rRate(settings, ppc::Mode::Maximum));
    runR(ppc::rRate(settings, ppc::Mode::Turbo));

    receiver.Close();
    std::cout << "Official game bridge event/rate preservation tests: PASS\n";
    return 0;
}
