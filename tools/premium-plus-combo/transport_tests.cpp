#include "input_transport.hpp"
#include <cassert>
#include <iostream>

static void CheckKey(WORD vk, DWORD extraFlags = 0) {
    INPUT src{};
    src.type = INPUT_KEYBOARD;
    src.ki.wVk = vk;
    src.ki.dwFlags = extraFlags;
    src.ki.dwExtraInfo = 0x5050434F4D424FULL;

    INPUT out{};
    assert(ppc_input::ConvertKeyboardInputToScanCode(src, out));
    const UINT expected = MapVirtualKeyW(vk, MAPVK_VK_TO_VSC_EX);
    assert(expected != 0);
    assert(out.type == INPUT_KEYBOARD);
    assert(out.ki.wVk == 0);
    assert(out.ki.wScan == static_cast<WORD>(expected & 0xFFu));
    assert((out.ki.dwFlags & KEYEVENTF_SCANCODE) != 0);
    assert((out.ki.dwFlags & KEYEVENTF_KEYUP) == (extraFlags & KEYEVENTF_KEYUP));
    assert(out.ki.dwExtraInfo == src.ki.dwExtraInfo);
}

int main() {
    // Rogue defaults and all synthetic paths used by combo/R/cure.
    CheckKey('8');
    CheckKey('9');
    CheckKey('0');
    CheckKey('R');
    CheckKey(VK_F2);
    CheckKey('6');
    CheckKey('8', KEYEVENTF_KEYUP);

    // Existing scan-code records must not be re-translated.
    INPUT raw{};
    raw.type = INPUT_KEYBOARD;
    raw.ki.wScan = 0x09;
    raw.ki.dwFlags = KEYEVENTF_SCANCODE;
    INPUT copy{};
    assert(ppc_input::ConvertKeyboardInputToScanCode(raw, copy));
    assert(copy.ki.wScan == raw.ki.wScan);
    assert(copy.ki.dwFlags == raw.ki.dwFlags);

    std::cout << "DirectInput scan-code transport tests: PASS\n";
    return 0;
}
