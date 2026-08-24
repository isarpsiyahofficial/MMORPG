void SendCombo(const std::array<int, 3>& keys, int repeats) {
    repeats = std::clamp(repeats, 1, 4);
    INPUT in[24]{};
    int count = 0;
    for (int r = 0; r < repeats; ++r) {
        for (int i = 0; i < 3; ++i) {
            in[count].type = INPUT_KEYBOARD;
            in[count].ki.wVk = WORD(keys[i]);
            in[count].ki.dwExtraInfo = EXTRA_TAG;
            ++count;
            in[count].type = INPUT_KEYBOARD;
            in[count].ki.wVk = WORD(keys[i]);
            in[count].ki.dwFlags = KEYEVENTF_KEYUP;
            in[count].ki.dwExtraInfo = EXTRA_TAG;
            ++count;
        }
    }
    if (SendInput(static_cast<UINT>(count), in, sizeof(INPUT)) == static_cast<UINT>(count)) {
        g_comboBursts.fetch_add(static_cast<unsigned long long>(repeats), std::memory_order_relaxed);
    }
}

void PreciseWaitUntil(long long target) {
    const long long freq = QpcFreq();
    while (!g_quit.load(std::memory_order_relaxed) && g_active.load(std::memory_order_relaxed)) {
        const long long now = QpcNow();
        const long long rem = target - now;
        if (rem <= 0) break;
        const double ms = static_cast<double>(rem) * 1000.0 / static_cast<double>(freq);
        if (ms > 2.0) Sleep(1);
        else if (ms > 0.35) SwitchToThread();
        else YieldProcessor();
    }
}

void ComboWorker() {
    const long long freq = QpcFreq();
    long long next = QpcNow();
    while (!g_quit.load(std::memory_order_relaxed)) {
        if (!g_active.load(std::memory_order_relaxed)) {
            WaitForSingleObject(g_wakeEvent, 50);
            next = QpcNow();
            continue;
        }

        Settings s;
        {
            std::lock_guard<std::mutex> lk(g_settingsMutex);
            s = g_settings;
        }

        // Two/four complete cycles at 60 scheduler ticks keep the requested
        // average while reducing system-call overhead: 2*60=120 and 4*60=240
        // presses/sec for EACH combo key. Every generated event carries EXTRA_TAG,
        // so the tiny Caps-only control hook never mistakes macro output for a
        // physical control-key press.
        const int targetHz = ppc::minorRate(g_mode.load(std::memory_order_relaxed));
        const int repeats = targetHz >= 240 ? 4 : 2;
        constexpr int schedulerHz = 60;
        const long long step = std::max<long long>(1, freq / schedulerHz);
        if (!g_active.load(std::memory_order_relaxed)) continue;

        SendCombo(s.comboVk, repeats);
        next += step;
        const long long now = QpcNow();
        if (now - next > step * 2) next = now + step;
        PreciseWaitUntil(next);
    }
}

void RWorker() {
    const long long freq = QpcFreq();
    long long next = QpcNow();
    while (!g_quit.load(std::memory_order_relaxed)) {
        if (!g_active.load(std::memory_order_relaxed)) {
            Sleep(10);
            next = QpcNow();
            continue;
        }

        Settings s;
        {
            std::lock_guard<std::mutex> lk(g_settingsMutex);
            s = g_settings;
        }
        if (!s.rEnabled || QpcNow() < g_rSuppressUntil.load(std::memory_order_relaxed)) {
            Sleep(1);
            next = QpcNow();
            continue;
        }

        const int hz = ppc::rRate(s, g_mode.load(std::memory_order_relaxed));
        const long long step = std::max<long long>(1, freq / hz);
        if (!g_active.load(std::memory_order_relaxed)) continue;

        if (SendKey('R')) g_rPresses.fetch_add(1, std::memory_order_relaxed);
        next += step;
        const long long now = QpcNow();
        if (now - next > step * 2) next = now + step;
        PreciseWaitUntil(next);
    }
}

void CureWorker() {
    if (g_cureBusy.exchange(true, std::memory_order_acq_rel)) return;

    Settings s;
    {
        std::lock_guard<std::mutex> lk(g_settingsMutex);
        s = g_settings;
    }
    if (!g_active.load(std::memory_order_relaxed) || !s.cureEnabled) {
        g_cureBusy.store(false, std::memory_order_release);
        return;
    }

    g_cureRuns.fetch_add(1, std::memory_order_relaxed);
    const long long freq = QpcFreq();
    if (s.rEnabled) g_rSuppressUntil.store(QpcNow() + freq * 2, std::memory_order_relaxed);

    const int previous = g_currentBar.load(std::memory_order_relaxed);
    SendKey(VK_F1 + (s.cureBar - 1));
    g_currentBar.store(s.cureBar, std::memory_order_relaxed);
    Sleep(28);

    if (g_active.load(std::memory_order_relaxed)) SendKey('0' + s.cureSlot);
    Sleep(45);

    if (g_active.load(std::memory_order_relaxed) && previous >= 1 && previous <= 8 && previous != s.cureBar) {
        SendKey(VK_F1 + (previous - 1));
        g_currentBar.store(previous, std::memory_order_relaxed);
    }
    g_cureBusy.store(false, std::memory_order_release);
}

void SetIntegerProp(const wchar_t* name, unsigned long long value) {
    if (!g_hwnd) return;
    SetPropW(g_hwnd, name, reinterpret_cast<HANDLE>(static_cast<ULONG_PTR>(value)));
}

void UpdateDiagnosticMarkers() {
    if (!g_hwnd) return;
    SetIntegerProp(L"PPC_ACTIVE", g_active.load(std::memory_order_relaxed) ? 1ULL : 0ULL);
    SetIntegerProp(L"PPC_MODE", g_mode.load(std::memory_order_relaxed) == Mode::Turbo ? 1ULL : 0ULL);
    SetIntegerProp(L"PPC_COMBO_BURSTS", g_comboBursts.load(std::memory_order_relaxed));
    SetIntegerProp(L"PPC_R_PRESSES", g_rPresses.load(std::memory_order_relaxed));
    SetIntegerProp(L"PPC_CURE_RUNS", g_cureRuns.load(std::memory_order_relaxed));
}

// Caps Lock is special: RegisterHotKey receives it, but Windows also changes the
// keyboard lock state. When Caps is reserved as Start/Stop (or active Cure), this
// hook consumes ONLY Caps itself. Every other physical key is passed through
// untouched, so opening the program cannot turn C into 6 or remap normal typing.
LRESULT CALLBACK CapsControlProc(int code, WPARAM wp, LPARAM lp) {
    if (code != HC_ACTION || !lp) return CallNextHookEx(g_capsControlHook, code, wp, lp);

    const auto* k = reinterpret_cast<const KBDLLHOOKSTRUCT*>(lp);
    if (k->dwExtraInfo == EXTRA_TAG) return CallNextHookEx(g_capsControlHook, code, wp, lp);
    if (static_cast<int>(k->vkCode) != VK_CAPITAL) return CallNextHookEx(g_capsControlHook, code, wp, lp);

    // While assigning a new key, the dedicated capture hook owns the keystroke.
    if (g_captureTarget.load(std::memory_order_acquire) != static_cast<int>(CaptureTarget::None)) {
        return CallNextHookEx(g_capsControlHook, code, wp, lp);
    }

    const bool down = wp == WM_KEYDOWN || wp == WM_SYSKEYDOWN;
    const bool up = wp == WM_KEYUP || wp == WM_SYSKEYUP;
    if (up) {
        g_capsPhysicalDown.store(false, std::memory_order_release);
        return 1;
    }
    if (!down) return 1;

    // Suppress keyboard auto-repeat. One physical press means one control action.
    if (g_capsPhysicalDown.exchange(true, std::memory_order_acq_rel)) return 1;

    Settings s;
    {
        std::lock_guard<std::mutex> lk(g_settingsMutex);
        s = g_settings;
    }

    const bool active = g_active.load(std::memory_order_relaxed);
    int action = 0;
    if (s.startHotkey == VK_CAPITAL && s.stopHotkey == VK_CAPITAL) {
        action = HOTKEY_ID_START; // same key acts as an on/off toggle
    } else if (active && s.stopHotkey == VK_CAPITAL) {
        action = HOTKEY_ID_STOP;
    } else if (!active && s.startHotkey == VK_CAPITAL) {
        action = HOTKEY_ID_START;
    } else if (active && s.cureEnabled && s.cureHotkey == VK_CAPITAL) {
        action = HOTKEY_ID_CURE;
    }

    if (action != 0 && g_hwnd) PostMessageW(g_hwnd, WM_APP_CONTROL_KEY, static_cast<WPARAM>(action), 0);
    return 1;
}

void UnregisterRuntimeHotkeys() {
    if (g_hwnd) {
        UnregisterHotKey(g_hwnd, HOTKEY_ID_START);
        UnregisterHotKey(g_hwnd, HOTKEY_ID_STOP);
        UnregisterHotKey(g_hwnd, HOTKEY_ID_CURE);
    }
    if (g_capsControlHook) {
        UnhookWindowsHookEx(g_capsControlHook);
        g_capsControlHook = nullptr;
    }
    g_capsPhysicalDown.store(false, std::memory_order_release);
}

bool RefreshRuntimeHotkeys(bool showError) {
    if (!g_hwnd) return false;
    UnregisterRuntimeHotkeys();

    Settings s;
    {
        std::lock_guard<std::mutex> lk(g_settingsMutex);
        s = g_settings;
    }

    const bool active = g_active.load(std::memory_order_relaxed);
    const bool needCapsHook = s.startHotkey == VK_CAPITAL ||
                              s.stopHotkey == VK_CAPITAL ||
                              (active && s.cureEnabled && s.cureHotkey == VK_CAPITAL);

    bool ok = true;
    if (needCapsHook) {
        g_capsControlHook = SetWindowsHookExW(WH_KEYBOARD_LL, CapsControlProc, GetModuleHandleW(nullptr), 0);
        ok = g_capsControlHook != nullptr;
    }

    if (ok) {
        if (s.startHotkey == s.stopHotkey) {
            if (s.startHotkey != VK_CAPITAL) {
                ok = RegisterHotKey(g_hwnd, HOTKEY_ID_START, MOD_NOREPEAT, static_cast<UINT>(s.startHotkey)) != FALSE;
            }
        } else {
            if (s.startHotkey != VK_CAPITAL) {
                ok = RegisterHotKey(g_hwnd, HOTKEY_ID_START, MOD_NOREPEAT, static_cast<UINT>(s.startHotkey)) != FALSE;
            }
            if (ok && s.stopHotkey != VK_CAPITAL) {
                ok = RegisterHotKey(g_hwnd, HOTKEY_ID_STOP, MOD_NOREPEAT, static_cast<UINT>(s.stopHotkey)) != FALSE;
            }
        }
    }

    if (ok && active && s.cureEnabled && s.cureHotkey != VK_CAPITAL) {
        ok = RegisterHotKey(g_hwnd, HOTKEY_ID_CURE, MOD_NOREPEAT, static_cast<UINT>(s.cureHotkey)) != FALSE;
    }

    if (!ok) {
        UnregisterRuntimeHotkeys();
        if (showError) {
            MessageBoxW(g_hwnd,
                        L"Seçilen global tuşlardan biri Windows veya başka bir program tarafından kullanılıyor. Farklı bir Açma, Kapatma veya Cure tuşu seçin.",
                        L"Global Tuş Ataması", MB_OK | MB_ICONWARNING);
        }
        return false;
    }
    return true;
}

void UpdateStateTitle() {
    if (!g_hwnd) return;
    SetWindowTextW(g_hwnd, g_active.load(std::memory_order_relaxed) ? L"Premium Plus Combo|ACTIVE" : L"Premium Plus Combo|READY");
    UpdateDiagnosticMarkers();
}

void StartMacro() {
    bool expected = false;
    if (g_active.compare_exchange_strong(expected, true, std::memory_order_acq_rel)) {
        g_comboBursts.store(0, std::memory_order_relaxed);
        g_rPresses.store(0, std::memory_order_relaxed);
        g_cureRuns.store(0, std::memory_order_relaxed);
        g_startedTicks.store(QpcNow(), std::memory_order_relaxed);
        g_rSuppressUntil.store(0, std::memory_order_relaxed);
        if (!RefreshRuntimeHotkeys(true)) {
            g_active.store(false, std::memory_order_release);
            RefreshRuntimeHotkeys(false);
        } else {
            SetEvent(g_wakeEvent);
        }
    }
    UpdateStateTitle();
    if (g_hwnd) InvalidateRect(g_hwnd, nullptr, FALSE);
}

void StopMacro() {
    g_active.store(false, std::memory_order_release);
    ResetEvent(g_wakeEvent);
    g_rSuppressUntil.store(0, std::memory_order_relaxed);
    RefreshRuntimeHotkeys(false);
    UpdateStateTitle();
    if (g_hwnd) InvalidateRect(g_hwnd, nullptr, FALSE);
}

bool ComboContains(const Settings& s, int vk) {
    return vk == s.comboVk[0] || vk == s.comboVk[1] || vk == s.comboVk[2];
}

bool CanAssignHotkey(CaptureTarget target, int vk, const Settings& s) {
    if (vk <= 0 || vk > 0xFE) return false;
    if (vk == 'R' || ComboContains(s, vk)) return false;
    if (target == CaptureTarget::Cure) {
        return vk != s.startHotkey && vk != s.stopHotkey;
    }
    if (target == CaptureTarget::Start || target == CaptureTarget::Stop) {
        return vk != s.cureHotkey;
    }
    return false;
}

// This low-level hook exists ONLY for the short "Tuşa bas..." assignment window.
// Normal runtime typing is never inspected by this capture hook.
LRESULT CALLBACK KeyboardProc(int code, WPARAM wp, LPARAM lp) {
    if (code != HC_ACTION || !lp) return CallNextHookEx(g_keyboardHook, code, wp, lp);

    const auto* k = reinterpret_cast<const KBDLLHOOKSTRUCT*>(lp);
    const int vk = static_cast<int>(k->vkCode);
    if (vk <= 0 || vk >= 256) return CallNextHookEx(g_keyboardHook, code, wp, lp);

    const bool down = wp == WM_KEYDOWN || wp == WM_SYSKEYDOWN;
    const bool up = wp == WM_KEYUP || wp == WM_SYSKEYUP;
    const int swallowed = g_captureSwallowVk.load(std::memory_order_acquire);

    if (swallowed != 0 && vk == swallowed) {
        if (up) {
            g_captureSwallowVk.store(0, std::memory_order_release);
            HHOOK hook = g_keyboardHook;
            g_keyboardHook = nullptr;
            if (hook) UnhookWindowsHookEx(hook);
        }
        return 1;
    }

    const CaptureTarget capture = static_cast<CaptureTarget>(g_captureTarget.load(std::memory_order_acquire));
    if (capture != CaptureTarget::None && down) {
        g_captureTarget.store(static_cast<int>(CaptureTarget::None), std::memory_order_release);
        g_captureSwallowVk.store(vk, std::memory_order_release);
        if (g_hwnd) PostMessageW(g_hwnd, WM_APP_CAPTURED_KEY, static_cast<WPARAM>(vk), static_cast<LPARAM>(capture));
        return 1;
    }

    return CallNextHookEx(g_keyboardHook, code, wp, lp);
}

std::unique_ptr<Bitmap> LoadImageResource(int id) {
    HRSRC r = FindResourceW(nullptr, MAKEINTRESOURCEW(id), RT_RCDATA);
    if (!r) return {};
    const DWORD sz = SizeofResource(nullptr, r);
    HGLOBAL hg = LoadResource(nullptr, r);
    if (!hg || !sz) return {};
    void* src = LockResource(hg);
    if (!src) return {};

    HGLOBAL mem = GlobalAlloc(GMEM_MOVEABLE, sz);
    if (!mem) return {};
    void* dst = GlobalLock(mem);
    if (!dst) {
        GlobalFree(mem);
        return {};
    }
    memcpy(dst, src, sz);
    GlobalUnlock(mem);

    IStream* stream{};
    if (FAILED(CreateStreamOnHGlobal(mem, TRUE, &stream))) {
        GlobalFree(mem);
        return {};
    }

    std::unique_ptr<Bitmap> decoded(Bitmap::FromStream(stream, FALSE));
    if (!decoded || decoded->GetLastStatus() != Ok || decoded->GetWidth() == 0 || decoded->GetHeight() == 0) {
        stream->Release();
        return {};
    }

    auto stable = std::make_unique<Bitmap>(decoded->GetWidth(), decoded->GetHeight(), PixelFormat32bppARGB);
    {
        Graphics copy(stable.get());
        copy.SetInterpolationMode(InterpolationModeHighQualityBicubic);
        copy.DrawImage(decoded.get(), 0, 0, static_cast<INT>(decoded->GetWidth()), static_cast<INT>(decoded->GetHeight()));
    }
    decoded.reset();
    stream->Release();
    if (!stable || stable->GetLastStatus() != Ok) return {};
    return stable;
}

void DrawTextG(Graphics& g, const std::wstring& text, float x, float y, float w, float h, float size,
               bool bold = false, Color c = Color(255, 65, 42, 25), StringAlignment align = StringAlignmentNear) {
    FontFamily fam(L"Segoe UI");
    Font f(&fam, size, bold ? FontStyleBold : FontStyleRegular, UnitPixel);
    SolidBrush b(c);
    RectF r(x, y, w, h);
    StringFormat fmt;
    fmt.SetAlignment(align);
    fmt.SetLineAlignment(StringAlignmentCenter);
    fmt.SetTrimming(StringTrimmingEllipsisCharacter);
    g.DrawString(text.c_str(), -1, &f, r, &fmt, &b);
}