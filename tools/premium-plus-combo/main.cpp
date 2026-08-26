#include "input_transport.hpp"

// Keep the last known-good application/UI exactly as it was.  The only change
// here is the transport used when the legacy combo engine submits a multi-key
// 8 -> 9 -> 0 batch.  The old batch put every down/up record into one SendInput
// call; some game input paths can observe only the first slot from that burst.
// Single-key R/Cure output (two records) is deliberately left untouched.
static void PpcComboPulseWaitMicros(int microseconds) noexcept {
    if (microseconds <= 0) return;

    LARGE_INTEGER frequency{};
    LARGE_INTEGER start{};
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&start);

    const long long delta =
        (frequency.QuadPart * static_cast<long long>(microseconds)) / 1000000LL;
    const long long target = start.QuadPart + (delta > 0 ? delta : 1LL);

    for (;;) {
        LARGE_INTEGER now{};
        QueryPerformanceCounter(&now);
        if (now.QuadPart >= target) break;
        if (target - now.QuadPart > frequency.QuadPart / 3000LL)
            SwitchToThread();
        else
            YieldProcessor();
    }
}

static UINT PpcKnownGoodComboSendInput(UINT count, LPINPUT inputs, int cbSize) noexcept {
    // Preserve the exact known-good path for R, Cure, hotkeys and every ordinary
    // two-record key tap.  Only a true multi-key combo batch is split.
    if (!inputs || count <= 2 || cbSize != sizeof(INPUT))
        return ppc_input::SendInputScanCodeCompatible(count, inputs, cbSize);

    UINT delivered = 0;
    for (UINT i = 0; i < count; ++i) {
        const UINT sent = ppc_input::SendInputScanCodeCompatible(1, inputs + i, cbSize);
        if (sent != 1) return delivered;
        ++delivered;

        if (inputs[i].type == INPUT_KEYBOARD) {
            const bool keyUp = (inputs[i].ki.dwFlags & KEYEVENTF_KEYUP) != 0;
            // A short non-zero state window prevents 9/0 from being collapsed
            // behind the first key while still fitting the legacy 120/240 target
            // scheduler.  No UI, registry, hotkey or R/Cure timing is changed.
            PpcComboPulseWaitMicros(keyUp ? 120 : 900);
        }
    }
    return delivered;
}

// Route only the macro engine's synthetic keyboard output through the
// DirectInput-compatible / official game-bridge transport. UI/hotkey capture stays native.
#define SendInput PpcKnownGoodComboSendInput
#include "app_part1.inc"

#define StartMacro PpcLegacyStartMacro
#define StopMacro PpcLegacyStopMacro
#define RefreshRuntimeHotkeys PpcLegacyRefreshRuntimeHotkeys
#include "app_part2.inc"
#undef RefreshRuntimeHotkeys
#undef StopMacro
#undef StartMacro
#undef SendInput

#define MakeChildControls PpcLegacyMakeChildControls
#define DrawUi PpcLegacyDrawUi
#define LoadRegistry PpcLegacyLoadRegistry
#include "app_part3.inc"
#undef LoadRegistry
#undef DrawUi
#undef MakeChildControls

#include "app_power.inc"
#include "app_part4.inc"
