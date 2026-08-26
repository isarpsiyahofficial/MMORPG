#include "input_transport.hpp"
#include <atomic>

// Keep the user-verified known-good application/UI exactly as it was. Only the
// synthetic game-facing keyboard transport is changed here.
//
// The old engine submitted 8/9/0 as one zero-duration batch and submitted R/Cure
// as zero-duration down+up pairs. Some game input paths can repeatedly observe
// only the first combo slot or miss a single-key pulse completely. The fallback
// below emits one complete scan-code tap at a time, so combo, R and Cure workers
// cannot overlap each other's key-down states.
//
// A plain SRWLOCK can favor the continuously busy combo worker and starve the R
// or Cure worker. Use a tiny FIFO ticket gate instead: each complete key tap gets
// its turn in arrival order, preserving serialization without starving R/Cure.
static std::atomic<unsigned long> g_ppcGameTapNextTicket{0};
static std::atomic<unsigned long> g_ppcGameTapServing{0};

static unsigned long PpcAcquireGameTapTicket() noexcept {
    const unsigned long ticket =
        g_ppcGameTapNextTicket.fetch_add(1, std::memory_order_relaxed);
    while (g_ppcGameTapServing.load(std::memory_order_acquire) != ticket) {
        SwitchToThread();
    }
    return ticket;
}

static void PpcReleaseGameTapTicket(unsigned long ticket) noexcept {
    g_ppcGameTapServing.store(ticket + 1, std::memory_order_release);
}

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

static bool PpcIsKeyUp(const INPUT& in) noexcept {
    return in.type == INPUT_KEYBOARD && (in.ki.dwFlags & KEYEVENTF_KEYUP) != 0;
}

static bool PpcMatchingTapPair(const INPUT& down, const INPUT& up) noexcept {
    if (down.type != INPUT_KEYBOARD || up.type != INPUT_KEYBOARD) return false;
    if (PpcIsKeyUp(down) || !PpcIsKeyUp(up)) return false;
    return down.ki.wVk == up.ki.wVk && down.ki.wScan == up.ki.wScan;
}

static UINT PpcKnownGoodGameSendInput(UINT count, LPINPUT inputs, int cbSize) noexcept {
    if (!inputs || count == 0 || cbSize != sizeof(INPUT))
        return ppc_input::SendInputScanCodeCompatible(count, inputs, cbSize);

    // The official game-side bridge already queues every action losslessly. Keep
    // its original full-rate batch path untouched.
    if (ppc_input::BridgeConnected())
        return ppc_input::SendInputScanCodeCompatible(count, inputs, cbSize);

    // This wrapper is scoped only around the macro engine. Preserve legacy
    // behavior if a future caller ever submits non-keyboard input here.
    for (UINT i = 0; i < count; ++i) {
        if (inputs[i].type != INPUT_KEYBOARD)
            return ppc_input::SendInputScanCodeCompatible(count, inputs, cbSize);
    }

    // 1.10 ms down + 0.10 ms released gap gives every key a distinct observable
    // state while a 3-key 8->9->0 cycle remains below the 4.17 ms budget required
    // by the requested 240-cycle/s Turbo producer. R stays governed by its 25/40
    // Hz scheduler and Cure keeps its existing sequencing/timing.
    constexpr int kDownHoldUs = 1100;
    constexpr int kReleasedGapUs = 100;

    UINT delivered = 0;
    UINT i = 0;
    while (i < count) {
        const bool pair = (i + 1 < count) && PpcMatchingTapPair(inputs[i], inputs[i + 1]);
        const unsigned long ticket = PpcAcquireGameTapTicket();

        const UINT first = ppc_input::SendInputScanCodeCompatible(1, inputs + i, cbSize);
        if (first != 1) {
            PpcReleaseGameTapTicket(ticket);
            return delivered;
        }
        ++delivered;

        if (pair) {
            PpcGamePulseWaitMicros(kDownHoldUs);

            UINT second = ppc_input::SendInputScanCodeCompatible(1, inputs + i + 1, cbSize);
            if (second != 1) {
                // Never leave a synthetic key down if Windows transiently rejects
                // the first key-up attempt.
                Sleep(1);
                second = ppc_input::SendInputScanCodeCompatible(1, inputs + i + 1, cbSize);
            }
            if (second != 1) {
                PpcReleaseGameTapTicket(ticket);
                return delivered;
            }
            ++delivered;
            PpcGamePulseWaitMicros(kReleasedGapUs);
            i += 2;
        } else {
            PpcGamePulseWaitMicros(PpcIsKeyUp(inputs[i]) ? kReleasedGapUs : kDownHoldUs);
            ++i;
        }

        PpcReleaseGameTapTicket(ticket);
    }

    return delivered;
}

// Route only the macro engine's synthetic keyboard output through the fixed
// transport. UI, Turkish strings, key capture and physical hotkeys stay native.
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
