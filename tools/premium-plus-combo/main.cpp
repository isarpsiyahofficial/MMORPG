#include "input_transport.hpp"

// Keep the user-verified known-good application/UI intact.  Only synthetic
// game-facing keyboard delivery is changed here.
static UINT PpcKnownGoodGameSendInput(UINT count, LPINPUT inputs, int cbSize) noexcept;

#define SendInput PpcKnownGoodGameSendInput
#include "app_part1.inc"

// app_part1.inc defines g_mode, so the fallback dwell can preserve both requested
// speed profiles while still exposing a real key-down state to DirectInput-like
// game polling.  The official game bridge keeps the original batched path.
static void PpcGamePulseWaitMicros(int microseconds) noexcept {
    if (microseconds <= 0) return;

    const long long freq = QpcFreq();
    const long long start = QpcNow();
    const long long delta = std::max<long long>(
        1LL, (freq * static_cast<long long>(microseconds)) / 1000000LL);
    const long long target = start + delta;

    for (;;) {
        const long long now = QpcNow();
        if (now >= target) break;
        const long long remain = target - now;
        if (remain > freq / 3000LL)
            SwitchToThread();
        else
            YieldProcessor();
    }
}

static UINT PpcKnownGoodGameSendInput(UINT count, LPINPUT inputs, int cbSize) noexcept {
    if (!inputs || count == 0 || cbSize != sizeof(INPUT))
        return ppc_input::SendInputScanCodeCompatible(count, inputs, cbSize);

    // When the game-side bridge exists, preserve the original batch and full
    // producer rate.  The bridge queues each down/up action losslessly.
    if (ppc_input::BridgeConnected())
        return ppc_input::SendInputScanCodeCompatible(count, inputs, cbSize);

    // This wrapper is scoped only around the macro engine.  If a future caller
    // submits non-keyboard input, keep the legacy transport unchanged.
    for (UINT i = 0; i < count; ++i) {
        if (inputs[i].type != INPUT_KEYBOARD)
            return ppc_input::SendInputScanCodeCompatible(count, inputs, cbSize);
    }

    // Stock-client fallback: do not submit 8/9/0 (or R/Cure) down+up events in
    // one zero-duration batch.  Emit one scan-code state at a time so the game
    // cannot repeatedly see only the first key.  Maximum uses a longer state
    // window while staying above its 120-cycle/s target; Turbo uses a shorter
    // state window that still fits the 240-cycle/s target.
    const bool turbo = g_mode.load(std::memory_order_relaxed) == Mode::Turbo;
    const int downHoldUs = turbo ? 1100 : 2250;
    const int releasedGapUs = turbo ? 120 : 300;

    UINT delivered = 0;
    for (UINT i = 0; i < count; ++i) {
        const UINT sent = ppc_input::SendInputScanCodeCompatible(1, inputs + i, cbSize);
        if (sent != 1) return delivered;
        ++delivered;

        const bool keyUp = (inputs[i].ki.dwFlags & KEYEVENTF_KEYUP) != 0;
        PpcGamePulseWaitMicros(keyUp ? releasedGapUs : downHoldUs);
    }
    return delivered;
}

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
