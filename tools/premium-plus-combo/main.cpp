#include "input_transport.hpp"

// Keep the user-verified known-good application/UI exactly as it was.  Only the
// synthetic game-facing keyboard transport is changed here.
//
// The old engine submitted 8/9/0 as one zero-duration batch and submitted R/Cure
// as zero-duration down+up pairs.  Some game input paths can repeatedly observe
// only the first combo slot or miss a single-key pulse completely.  The fallback
// below emits one scan-code state at a time with a short real down/released window.
// It remains fast enough for the requested 240 combo cycles/sec producer target.
static void PpcGamePulseWaitMicros(int microseconds) noexcept {
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
        const long long remain = target - now.QuadPart;
        if (remain > frequency.QuadPart / 3000LL)
            SwitchToThread();
        else
            YieldProcessor();
    }
}

static UINT PpcKnownGoodGameSendInput(UINT count, LPINPUT inputs, int cbSize) noexcept {
    if (!inputs || count == 0 || cbSize != sizeof(INPUT))
        return ppc_input::SendInputScanCodeCompatible(count, inputs, cbSize);

    // The official game-side bridge already queues each action losslessly, so its
    // original batch/rate path stays untouched.
    if (ppc_input::BridgeConnected())
        return ppc_input::SendInputScanCodeCompatible(count, inputs, cbSize);

    for (UINT i = 0; i < count; ++i) {
        if (inputs[i].type != INPUT_KEYBOARD)
            return ppc_input::SendInputScanCodeCompatible(count, inputs, cbSize);
    }

    // 1.10 ms down + 0.10 ms released gap gives each key a distinct observable
    // state while a 3-key 8->9->0 cycle still fits comfortably below 4.17 ms.
    // R and Cure use the same physical-looking tap instead of their old instant
    // down+up pair; their configured 25/40 Hz schedules remain the bottleneck.
    constexpr int kDownHoldUs = 1100;
    constexpr int kReleasedGapUs = 100;

    UINT delivered = 0;
    for (UINT i = 0; i < count; ++i) {
        const UINT sent = ppc_input::SendInputScanCodeCompatible(1, inputs + i, cbSize);
        if (sent != 1) return delivered;
        ++delivered;

        const bool keyUp = (inputs[i].ki.dwFlags & KEYEVENTF_KEYUP) != 0;
        PpcGamePulseWaitMicros(keyUp ? kReleasedGapUs : kDownHoldUs);
    }
    return delivered;
}

// Route only the macro engine's synthetic keyboard output through the fixed
// transport.  UI, Turkish strings, key capture and physical hotkeys stay native.
#define SendInput PpcKnownGoodGameSendInput
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
