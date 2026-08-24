// Master application gate. This layer is intentionally kept separate from the
// Rogue combo engine so OFF means the process may stay open in the background
// while every macro/hotkey function is inert.

constexpr int POWER_SWITCH_ID = 1401;
constexpr int POWER_OVERLAY_ID = 1402;
constexpr wchar_t POWER_SWITCH_CLASS[] = L"PremiumPlusPowerSwitch";
constexpr wchar_t POWER_OVERLAY_CLASS[] = L"PremiumPlusPowerOverlay";

HWND hPowerSwitch{};
HWND hPowerOverlay{};
std::atomic<bool> g_powerEnabled{true};

// One-time migration from builds where TAB/CAPS LOCK was the shipped default.
// Saved custom hotkeys remain supported; only the exact legacy default pair is migrated.
void LoadRegistry() {
    PpcLegacyLoadRegistry();

    HKEY key{};
    DWORD marker = 0;
    DWORD markerSize = sizeof(marker);
    DWORD markerType = REG_DWORD;
    bool migratedBefore = false;

    if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\PremiumPlusCombo\\RogueV2", 0, nullptr, 0,
                        KEY_READ | KEY_WRITE, nullptr, &key, nullptr) == ERROR_SUCCESS) {
        migratedBefore = RegQueryValueExW(key, L"CapsToggleDefaultV3", nullptr, &markerType,
                                          reinterpret_cast<BYTE*>(&marker), &markerSize) == ERROR_SUCCESS
                         && markerType == REG_DWORD && marker == 1;

        if (!migratedBefore) {
            {
                std::lock_guard<std::mutex> lk(g_settingsMutex);
                if (g_settings.startHotkey == VK_TAB && g_settings.stopHotkey == VK_CAPITAL) {
                    g_settings.startHotkey = VK_CAPITAL;
                    g_settings.stopHotkey = VK_CAPITAL;
                }
            }
            marker = 1;
            RegSetValueExW(key, L"CapsToggleDefaultV3", 0, REG_DWORD,
                           reinterpret_cast<const BYTE*>(&marker), sizeof(marker));
        }
        RegCloseKey(key);
    }
}

// app_part2.inc is included with these three implementation names remapped.
// The public wrappers below are what app_part4.inc sees.
void StartMacro() {
    if (!g_powerEnabled.load(std::memory_order_acquire)) return;
    PpcLegacyStartMacro();
}

void StopMacro() {
    PpcLegacyStopMacro();
    if (!g_powerEnabled.load(std::memory_order_acquire)) {
        // Legacy Stop keeps runtime hotkeys ready for the normal idle state.
        // Master OFF is stronger: no hotkey/hook may remain registered.
        UnregisterRuntimeHotkeys();
    }
}

bool RefreshRuntimeHotkeys(bool showError) {
    if (!g_powerEnabled.load(std::memory_order_acquire)) {
        UnregisterRuntimeHotkeys();
        return true;
    }
    return PpcLegacyRefreshRuntimeHotkeys(showError);
}

void UpdatePowerVisualState() {
    const bool enabled = g_powerEnabled.load(std::memory_order_acquire);

    const std::array<HWND, 12> gated{{
        hSeq1, hSeq2, hSeq3,
        hRCheck, hRMax, hRTurbo,
        hCureCheck, hCureBar, hCureSlot, hCureKey,
        hStartKey, hStopKey
    }};
    for (HWND h : gated) {
        if (h && IsWindow(h)) EnableWindow(h, enabled ? TRUE : FALSE);
    }

    if (hPowerSwitch && IsWindow(hPowerSwitch)) {
        SetWindowTextW(hPowerSwitch, enabled ? L"A\u00C7IK" : L"KAPALI");
        InvalidateRect(hPowerSwitch, nullptr, FALSE);
    }

    if (hPowerOverlay && IsWindow(hPowerOverlay)) {
        ShowWindow(hPowerOverlay, enabled ? SW_HIDE : SW_SHOW);
        if (!enabled) SetWindowPos(hPowerOverlay, HWND_TOP, 0, 0, 0, 0,
                                   SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    }

    if (g_hwnd && IsWindow(g_hwnd)) {
        SetIntegerProp(L"PPC_POWER_ENABLED", enabled ? 1ULL : 0ULL);
        InvalidateRect(g_hwnd, nullptr, FALSE);
    }
}

void SetPowerEnabled(bool enabled) {
    const bool previous = g_powerEnabled.exchange(enabled, std::memory_order_acq_rel);
    if (previous == enabled) {
        UpdatePowerVisualState();
        return;
    }

    if (!enabled) {
        // Cancel a hotkey-capture session as well. OFF must leave no keyboard hook.
        g_captureTarget.store(static_cast<int>(CaptureTarget::None), std::memory_order_release);
        g_captureSwallowVk.store(0, std::memory_order_release);
        if (g_keyboardHook) {
            UnhookWindowsHookEx(g_keyboardHook);
            g_keyboardHook = nullptr;
        }

        StopMacro();
        UnregisterRuntimeHotkeys();
    } else {
        // Powering back on does not auto-run the combo. It only restores controls
        // and the configured start/stop hotkeys; the engine remains stopped.
        g_active.store(false, std::memory_order_release);
        ResetEvent(g_wakeEvent);
        if (!RefreshRuntimeHotkeys(true)) {
            g_powerEnabled.store(false, std::memory_order_release);
            UnregisterRuntimeHotkeys();
        }
    }

    UpdateDiagnosticMarkers();
    UpdatePowerVisualState();
}

LRESULT CALLBACK PowerSwitchProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
        case WM_LBUTTONUP:
            SetPowerEnabled(!g_powerEnabled.load(std::memory_order_acquire));
            SetFocus(hwnd);
            return 0;

        case WM_KEYDOWN:
            if (wp == VK_SPACE || wp == VK_RETURN) {
                SetPowerEnabled(!g_powerEnabled.load(std::memory_order_acquire));
                return 0;
            }
            break;

        case WM_ERASEBKGND:
            return 1;

        case WM_PAINT: {
            PAINTSTRUCT ps{};
            HDC dc = BeginPaint(hwnd, &ps);
            RECT cr{};
            GetClientRect(hwnd, &cr);
            const bool enabled = g_powerEnabled.load(std::memory_order_acquire);

            Graphics g(dc);
            g.SetSmoothingMode(SmoothingModeAntiAlias);
            g.SetTextRenderingHint(TextRenderingHintClearTypeGridFit);

            RectF track(1.0f, 1.0f, static_cast<REAL>(cr.right - 2), static_cast<REAL>(cr.bottom - 2));
            Panel(g, track,
                  enabled ? Color(255, 135, 92, 24) : Color(255, 126, 31, 28),
                  enabled ? Color(255, 224, 174, 68) : Color(255, 186, 122, 48),
                  12.0f);

            const float knob = 22.0f;
            const float ky = (static_cast<float>(cr.bottom) - knob) * 0.5f;
            const float kx = enabled ? static_cast<float>(cr.right) - knob - 5.0f : 5.0f;
            SolidBrush kb(Color(255, 255, 246, 221));
            Pen kp(Color(255, 110, 67, 20), 1.0f);
            g.FillEllipse(&kb, kx, ky, knob, knob);
            g.DrawEllipse(&kp, kx, ky, knob, knob);

            DrawTextG(g, enabled ? L"A\u00C7IK" : L"KAPALI",
                      enabled ? 6.0f : 30.0f, 0.0f,
                      enabled ? 57.0f : 62.0f, static_cast<REAL>(cr.bottom),
                      11.0f, true, Color(255, 255, 246, 221), StringAlignmentCenter);

            EndPaint(hwnd, &ps);
            return 0;
        }
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

LRESULT CALLBACK PowerOverlayProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
        case WM_NCHITTEST:
            return HTCLIENT;

        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
            // Deliberately consume clicks: parent Start/Stop/mode/save hit regions
            // and all child controls underneath are unreachable while master OFF.
            return 0;

        case WM_ERASEBKGND:
            return 1;

        case WM_PAINT: {
            PAINTSTRUCT ps{};
            HDC dc = BeginPaint(hwnd, &ps);
            RECT cr{};
            GetClientRect(hwnd, &cr);

            Graphics g(dc);
            g.SetSmoothingMode(SmoothingModeAntiAlias);
            LinearGradientBrush veil(Rect(0, 0, cr.right, cr.bottom),
                                     Color(255, 249, 240, 220),
                                     Color(255, 226, 201, 158),
                                     LinearGradientModeVertical);
            g.FillRectangle(&veil, 0, 0, cr.right, cr.bottom);

            Pen border(Color(255, 178, 126, 43), 1.5f);
            g.DrawRectangle(&border, 1, 1, cr.right - 3, cr.bottom - 3);
            DrawTextG(g, L"UYGULAMA KAPALI", 20, 125, static_cast<REAL>(cr.right - 40), 38,
                      20, true, Color(255, 113, 31, 25), StringAlignmentCenter);
            DrawTextG(g, L"T\u00FCm fonksiyonlar ve global tu\u015Flar devre d\u0131\u015F\u0131.",
                      28, 170, static_cast<REAL>(cr.right - 56), 28,
                      11, false, Color(255, 78, 55, 34), StringAlignmentCenter);
            DrawTextG(g, L"Devam etmek i\u00E7in sa\u011F \u00FCstteki anahtar\u0131 A\u00C7IK konumuna getirin.",
                      28, 199, static_cast<REAL>(cr.right - 56), 36,
                      10, false, Color(255, 78, 55, 34), StringAlignmentCenter);

            EndPaint(hwnd, &ps);
            return 0;
        }
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

bool RegisterPowerClasses(HINSTANCE inst) {
    WNDCLASSEXW existing{};
    existing.cbSize = sizeof(existing);

    if (!GetClassInfoExW(inst, POWER_SWITCH_CLASS, &existing)) {
        WNDCLASSEXW wc{sizeof(wc)};
        wc.lpfnWndProc = PowerSwitchProc;
        wc.hInstance = inst;
        wc.hCursor = LoadCursorW(nullptr, IDC_HAND);
        wc.lpszClassName = POWER_SWITCH_CLASS;
        if (!RegisterClassExW(&wc)) return false;
    }

    existing = WNDCLASSEXW{};
    existing.cbSize = sizeof(existing);
    if (!GetClassInfoExW(inst, POWER_OVERLAY_CLASS, &existing)) {
        WNDCLASSEXW wc{sizeof(wc)};
        wc.lpfnWndProc = PowerOverlayProc;
        wc.hInstance = inst;
        wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        wc.lpszClassName = POWER_OVERLAY_CLASS;
        if (!RegisterClassExW(&wc)) return false;
    }
    return true;
}

bool MakeChildControls(HWND w) {
    if (!PpcLegacyMakeChildControls(w)) return false;

    const HINSTANCE inst = GetModuleHandleW(nullptr);
    if (!RegisterPowerClasses(inst)) return false;

    hPowerSwitch = CreateWindowExW(0, POWER_SWITCH_CLASS, L"A\u00C7IK",
                                   WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                                   622, 12, 101, 31, w,
                                   reinterpret_cast<HMENU>(static_cast<INT_PTR>(POWER_SWITCH_ID)),
                                   inst, nullptr);
    if (!hPowerSwitch) return false;

    hPowerOverlay = CreateWindowExW(WS_EX_LAYERED | WS_EX_NOACTIVATE,
                                    POWER_OVERLAY_CLASS, L"",
                                    WS_CHILD,
                                    175, 88, 450, 382, w,
                                    reinterpret_cast<HMENU>(static_cast<INT_PTR>(POWER_OVERLAY_ID)),
                                    inst, nullptr);
    if (!hPowerOverlay) return false;

    // A translucent blocker lets the existing Rogue UI remain visible underneath,
    // but it owns all mouse hits in that region while OFF.
    SetLayeredWindowAttributes(hPowerOverlay, 0, 210, LWA_ALPHA);

    // WM_CREATE runs before CreateWindowExW returns, so g_hwnd is not assigned yet.
    // Publish the authoritative power property directly on the parent HWND here.
    SetPropW(w, L"PPC_POWER_ENABLED",
             reinterpret_cast<HANDLE>(static_cast<ULONG_PTR>(
                 g_powerEnabled.load(std::memory_order_acquire) ? 1ULL : 0ULL)));

    UpdatePowerVisualState();
    return true;
}

void DrawUi(HWND hwnd, HDC hdc) {
    PpcLegacyDrawUi(hwnd, hdc);

    if (!g_powerEnabled.load(std::memory_order_acquire)) {
        Graphics g(hdc);
        g.SetTextRenderingHint(TextRenderingHintClearTypeGridFit);
        SolidBrush footer(Color(255, 249, 238, 221));
        g.FillRectangle(&footer, 18, 476, 173, 18);
        DrawTextG(g, L"Durum: Kapal\u0131", 22, 474, 165, 22, 10, true,
                  Color(255, 137, 31, 27), StringAlignmentNear);
    }
}
