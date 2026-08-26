#include "input_transport.hpp"
#include <atomic>

// Keep the user-verified known-good application/UI exactly as it was. Only the
// synthetic game-facing keyboard transport and control routing are layered here.
// No legacy UI drawing, power switch, registry layout or Rogue controls are
// replaced.
static std::atomic<unsigned long> g_ppcGameTapNextTicket{0};
static std::atomic<unsigned long> g_ppcGameTapServing{0};
static thread_local bool g_ppcGameTapOwned = false;

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

    for (UINT i = 0; i < count; ++i) {
        if (inputs[i].type != INPUT_KEYBOARD)
            return ppc_input::SendInputScanCodeCompatible(count, inputs, cbSize);
    }

    // Preserve the now-working visible key pulse, but trim the unnecessary tail
    // slightly so the real game path has more timing headroom. We deliberately
    // keep a full 1 ms key-down window so the repaired 8/9/0 visibility is not
    // traded away for benchmark-only speed.
    constexpr int kDownHoldUs = 1000;
    constexpr int kReleasedGapUs = 50;

    UINT delivered = 0;
    UINT i = 0;
    while (i < count) {
        const bool pair = (i + 1 < count) && PpcMatchingTapPair(inputs[i], inputs[i + 1]);
        const bool alreadyOwnsSequence = g_ppcGameTapOwned;
        unsigned long ticket = 0;
        if (!alreadyOwnsSequence) ticket = PpcAcquireGameTapTicket();

        const UINT first = ppc_input::SendInputScanCodeCompatible(1, inputs + i, cbSize);
        if (first != 1) {
            if (!alreadyOwnsSequence) PpcReleaseGameTapTicket(ticket);
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
                if (!alreadyOwnsSequence) PpcReleaseGameTapTicket(ticket);
                return delivered;
            }
            ++delivered;
            PpcGamePulseWaitMicros(kReleasedGapUs);
            i += 2;
        } else {
            PpcGamePulseWaitMicros(PpcIsKeyUp(inputs[i]) ? kReleasedGapUs : kDownHoldUs);
            ++i;
        }

        if (!alreadyOwnsSequence) PpcReleaseGameTapTicket(ticket);
    }

    return delivered;
}

// Route only the macro engine's synthetic keyboard output through the fixed
// transport. UI, Turkish strings and physical key capture stay native.
#define SendInput PpcKnownGoodGameSendInput
#include "app_part1.inc"

// Legacy Cure uses '0' + slot. Slot 10 represents the physical 0 key, so map the
// one synthetic ':' value back to '0' without touching any of the legacy UI code.
bool PpcEngineSendKey(int vk) {
    if (vk == ('0' + 10)) vk = '0';
    return SendKey(vk);
}

#define SendKey PpcEngineSendKey
#define CureWorker PpcLegacyCureWorker
#define StartMacro PpcLegacyStartMacro
#define StopMacro PpcLegacyStopMacro
#define RefreshRuntimeHotkeys PpcLegacyRefreshRuntimeHotkeys
#include "app_part2.inc"
#undef RefreshRuntimeHotkeys
#undef StopMacro
#undef StartMacro
#undef CureWorker
#undef SendKey
#undef SendInput

// Cure must be a coherent game action. The old worker left 28/45 ms windows in
// which the continuously running minor worker could press 8/9/0 on the Cure bar.
// Hold the same fair game-input gate for the whole F-bar -> slot -> restore
// sequence. The combo engine remains active and resumes immediately afterwards.
static std::atomic<bool> g_ppcCureSequenceBusy{false};

void CureWorker() {
    if (g_ppcCureSequenceBusy.exchange(true, std::memory_order_acq_rel)) return;

    const unsigned long ticket = PpcAcquireGameTapTicket();
    g_ppcGameTapOwned = true;
    PpcLegacyCureWorker();
    g_ppcGameTapOwned = false;
    PpcReleaseGameTapTicket(ticket);

    g_ppcCureSequenceBusy.store(false, std::memory_order_release);
}

// Use one low-level physical-key listener for every user-selected control key.
// RegisterHotKey is not reliable enough for arbitrary keys in fullscreen game
// sessions and can be reserved by Windows/other software. Injected macro keys are
// ignored via EXTRA_TAG, so 8/9/0/R/Cure output can never recursively control the
// application.
static std::atomic<int> g_ppcControlDownVk{0};
static std::atomic<int> g_ppcCureTriggerDownVk{0};

LRESULT CALLBACK PpcAllControlProc(int code, WPARAM wp, LPARAM lp) {
    if (code != HC_ACTION || !lp) return CallNextHookEx(g_capsControlHook, code, wp, lp);

    const auto* k = reinterpret_cast<const KBDLLHOOKSTRUCT*>(lp);
    if (k->dwExtraInfo == EXTRA_TAG) return CallNextHookEx(g_capsControlHook, code, wp, lp);

    if (g_captureTarget.load(std::memory_order_acquire) != static_cast<int>(CaptureTarget::None)) {
        return CallNextHookEx(g_capsControlHook, code, wp, lp);
    }

    const int vk = static_cast<int>(k->vkCode);
    if (vk <= 0 || vk >= 256) return CallNextHookEx(g_capsControlHook, code, wp, lp);

    const bool down = wp == WM_KEYDOWN || wp == WM_SYSKEYDOWN;
    const bool up = wp == WM_KEYUP || wp == WM_SYSKEYUP;

    if (up) {
        if (g_ppcControlDownVk.load(std::memory_order_acquire) == vk) {
            g_ppcControlDownVk.store(0, std::memory_order_release);
            return 1;
        }
        if (g_ppcCureTriggerDownVk.load(std::memory_order_acquire) == vk) {
            g_ppcCureTriggerDownVk.store(0, std::memory_order_release);
        }
        return CallNextHookEx(g_capsControlHook, code, wp, lp);
    }
    if (!down) return CallNextHookEx(g_capsControlHook, code, wp, lp);

    if (g_ppcControlDownVk.load(std::memory_order_acquire) == vk) return 1;
    if (g_ppcCureTriggerDownVk.load(std::memory_order_acquire) == vk)
        return CallNextHookEx(g_capsControlHook, code, wp, lp);

    Settings s;
    {
        std::lock_guard<std::mutex> lk(g_settingsMutex);
        s = g_settings;
    }

    const bool active = g_active.load(std::memory_order_relaxed);
    int action = 0;
    if (s.startHotkey == s.stopHotkey && vk == s.startHotkey) {
        action = HOTKEY_ID_START;
    } else if (active && vk == s.stopHotkey) {
        action = HOTKEY_ID_STOP;
    } else if (!active && vk == s.startHotkey) {
        action = HOTKEY_ID_START;
    } else if (active && s.cureEnabled && vk == s.cureHotkey) {
        action = HOTKEY_ID_CURE;
    }

    if (action == 0) return CallNextHookEx(g_capsControlHook, code, wp, lp);

    if (action == HOTKEY_ID_CURE) {
        // Cure is a companion action: the user's assigned physical key continues
        // to the foreground game while Cure is triggered once for this press.
        g_ppcCureTriggerDownVk.store(vk, std::memory_order_release);
        if (g_hwnd) PostMessageW(g_hwnd, WM_APP_CONTROL_KEY, static_cast<WPARAM>(action), 0);
        return CallNextHookEx(g_capsControlHook, code, wp, lp);
    }

    // Start/Stop controls are consumed exactly like the previous global-hotkey
    // behavior. CAPS LOCK therefore still never toggles Windows while controlling
    // the active macro.
    g_ppcControlDownVk.store(vk, std::memory_order_release);
    if (g_hwnd) PostMessageW(g_hwnd, WM_APP_CONTROL_KEY, static_cast<WPARAM>(action), 0);
    return 1;
}

bool PpcReliableRefreshRuntimeHotkeys(bool showError) {
    if (!g_hwnd) return false;
    UnregisterRuntimeHotkeys();

    const int heldControl = g_ppcControlDownVk.load(std::memory_order_acquire);
    if (heldControl != 0 && (GetAsyncKeyState(heldControl) & 0x8000) == 0)
        g_ppcControlDownVk.store(0, std::memory_order_release);
    const int heldCure = g_ppcCureTriggerDownVk.load(std::memory_order_acquire);
    if (heldCure != 0 && (GetAsyncKeyState(heldCure) & 0x8000) == 0)
        g_ppcCureTriggerDownVk.store(0, std::memory_order_release);

    g_capsControlHook = SetWindowsHookExW(WH_KEYBOARD_LL, PpcAllControlProc,
                                          GetModuleHandleW(nullptr), 0);
    if (g_capsControlHook) return true;

    if (showError) {
        MessageBoxW(g_hwnd,
                    L"Global tu\u015F dinleyicisi ba\u015Flat\u0131lamad\u0131. Se\u00E7ti\u011Finiz A\u00E7ma, Kapatma ve Cure tu\u015Flar\u0131 etkinle\u015Ftirilemedi.",
                    L"Global Tu\u015F Atamas\u0131", MB_OK | MB_ICONWARNING);
    }
    return false;
}

void PpcReliableStartMacro() {
    bool expected = false;
    if (g_active.compare_exchange_strong(expected, true, std::memory_order_acq_rel)) {
        g_comboBursts.store(0, std::memory_order_relaxed);
        g_rPresses.store(0, std::memory_order_relaxed);
        g_cureRuns.store(0, std::memory_order_relaxed);
        g_startedTicks.store(QpcNow(), std::memory_order_relaxed);
        g_rSuppressUntil.store(0, std::memory_order_relaxed);
        if (!PpcReliableRefreshRuntimeHotkeys(true)) {
            g_active.store(false, std::memory_order_release);
            PpcReliableRefreshRuntimeHotkeys(false);
        } else {
            SetEvent(g_wakeEvent);
        }
    }
    UpdateStateTitle();
    if (g_hwnd) InvalidateRect(g_hwnd, nullptr, FALSE);
}

void PpcReliableStopMacro() {
    g_active.store(false, std::memory_order_release);
    ResetEvent(g_wakeEvent);
    g_rSuppressUntil.store(0, std::memory_order_relaxed);
    PpcReliableRefreshRuntimeHotkeys(false);
    UpdateStateTitle();
    if (g_hwnd) InvalidateRect(g_hwnd, nullptr, FALSE);
}

#define MakeChildControls PpcLegacyMakeChildControls
#define DrawUi PpcLegacyDrawUi
#define LoadRegistry PpcLegacyLoadRegistry
#include "app_part3.inc"
#undef LoadRegistry
#undef DrawUi
#undef MakeChildControls

// Extend only the Cure slot list. Existing controls/layout stay exactly where
// they are; two additional entries complete the normal 1..9,0 game key row.
bool PpcEnhancedMakeChildControls(HWND w) {
    if (!PpcLegacyMakeChildControls(w)) return false;
    const int count = ComboBox_GetCount(hCureSlot);
    if (count == 8) {
        ComboBox_AddString(hCureSlot, L"9");
        ComboBox_AddString(hCureSlot, L"0");
    }
    SyncControls();
    return true;
}

// Keep the existing master power layer untouched, but route its three engine
// callbacks and legacy child-control call to the hardened implementations above.
#define PpcLegacyStartMacro PpcReliableStartMacro
#define PpcLegacyStopMacro PpcReliableStopMacro
#define PpcLegacyRefreshRuntimeHotkeys PpcReliableRefreshRuntimeHotkeys
#define PpcLegacyMakeChildControls PpcEnhancedMakeChildControls
#include "app_power.inc"
#undef PpcLegacyMakeChildControls
#undef PpcLegacyRefreshRuntimeHotkeys
#undef PpcLegacyStopMacro
#undef PpcLegacyStartMacro

#include "app_part4.inc"
