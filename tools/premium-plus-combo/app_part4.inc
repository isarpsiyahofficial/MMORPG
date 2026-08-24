void BeginCapture(CaptureTarget target, HWND button) {
    StopMacro();
    UnregisterRuntimeHotkeys();
    if (g_keyboardHook) {
        UnhookWindowsHookEx(g_keyboardHook);
        g_keyboardHook = nullptr;
    }
    g_captureSwallowVk.store(0, std::memory_order_release);
    g_captureTarget.store(static_cast<int>(target), std::memory_order_release);
    g_keyboardHook = SetWindowsHookExW(WH_KEYBOARD_LL, KeyboardProc, GetModuleHandleW(nullptr), 0);
    if (!g_keyboardHook) {
        g_captureTarget.store(static_cast<int>(CaptureTarget::None), std::memory_order_release);
        SyncControls();
        RefreshRuntimeHotkeys(false);
        MessageBoxW(g_hwnd, L"Tu\u015F atama ekran\u0131 ba\u015Flat\u0131lamad\u0131.",
                    L"Premium Plus Combo", MB_OK | MB_ICONERROR);
        return;
    }
    SetWindowTextW(button, L"Tu\u015Fa bas...");
}

HWND CaptureButton(CaptureTarget target) {
    if (target == CaptureTarget::Start) return hStartKey;
    if (target == CaptureTarget::Stop) return hStopKey;
    if (target == CaptureTarget::Cure) return hCureKey;
    return nullptr;
}

void HandleControlAction(int id) {
    if (id == HOTKEY_ID_START) {
        Settings s;
        {
            std::lock_guard<std::mutex> lk(g_settingsMutex);
            s = g_settings;
        }
        if (s.startHotkey == s.stopHotkey && g_active.load(std::memory_order_relaxed)) StopMacro();
        else StartMacro();
        return;
    }
    if (id == HOTKEY_ID_STOP) {
        StopMacro();
        return;
    }
    if (id == HOTKEY_ID_CURE) {
        if (g_active.load(std::memory_order_relaxed)) std::thread(CureWorker).detach();
    }
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
        case WM_CREATE: {
            if (!MakeChildControls(hwnd)) {
                MessageBoxW(hwnd, L"Aray\u00FCz kontrolleri olu\u015Fturulamad\u0131. Program ba\u015Flat\u0131lmad\u0131.",
                            L"Premium Plus Combo", MB_OK | MB_ICONERROR);
                return -1;
            }
            SetPropW(hwnd, L"PPC_CONTROLS", reinterpret_cast<HANDLE>(static_cast<INT_PTR>(13)));
            UpdateDiagnosticMarkers();
            return 0;
        }

        case WM_NCHITTEST: {
            POINT p{GET_X_LPARAM(lp), GET_Y_LPARAM(lp)};
            ScreenToClient(hwnd, &p);
            if (p.y < 50 && !PtIn(rcMin, p.x, p.y) && !PtIn(rcClose, p.x, p.y)) return HTCAPTION;
            return HTCLIENT;
        }

        case WM_LBUTTONUP: {
            const int x = GET_X_LPARAM(lp);
            const int y = GET_Y_LPARAM(lp);
            if (PtIn(rcClose, x, y)) {
                DestroyWindow(hwnd);
                return 0;
            }
            if (PtIn(rcMin, x, y)) {
                ShowWindow(hwnd, SW_MINIMIZE);
                return 0;
            }
            if (PtIn(rcStart, x, y)) {
                if (ApplyControls(true)) StartMacro();
                return 0;
            }
            if (PtIn(rcStop, x, y)) {
                StopMacro();
                return 0;
            }
            if (PtIn(rcInfoMax, x, y) || PtIn(rcInfoTurbo, x, y)) {
                MessageBoxW(hwnd,
                            L"120/sn ve 240/sn de\u011Ferleri giri\u015F dizisindeki HER B\u0130R tu\u015Fun saniyedeki yakla\u015F\u0131k bas\u0131lma hedefidir.\n\n"
                            L"8 > 9 > 0 dizisinde Maximum Mod se\u00E7iliyken 8, 9 ve 0 tu\u015Flar\u0131n\u0131n her biri yakla\u015F\u0131k 120 bas\u0131m/sn; Turbo Modda yakla\u015F\u0131k 240 bas\u0131m/sn hedefiyle g\u00F6nderilir.\n\n"
                            L"R h\u0131z\u0131 ayr\u0131d\u0131r ve Rogue Ayarlar\u0131 b\u00F6l\u00FCm\u00FCnden belirlenir.",
                            L"H\u0131z Bilgisi", MB_OK | MB_ICONINFORMATION);
                return 0;
            }
            if (PtIn(rcMax, x, y)) {
                g_mode.store(Mode::Maximum, std::memory_order_relaxed);
                UpdateDiagnosticMarkers();
                InvalidateRect(hwnd, nullptr, FALSE);
                return 0;
            }
            if (PtIn(rcTurbo, x, y)) {
                g_mode.store(Mode::Turbo, std::memory_order_relaxed);
                UpdateDiagnosticMarkers();
                InvalidateRect(hwnd, nullptr, FALSE);
                return 0;
            }
            if (PtIn(rcSave, x, y)) {
                if (ApplyControls(true) && RefreshRuntimeHotkeys(true)) {
                    SaveRegistry();
                    MessageBoxW(hwnd,
                                L"Ayarlar kaydedildi. Giri\u015F dizisi, A\u00E7ma/Kapatma tu\u015Flar\u0131, R Combo ve Cure ayarlar\u0131 sonraki a\u00E7\u0131l\u0131\u015Fta geri y\u00FCklenir.",
                                L"Premium Plus Combo", MB_OK | MB_ICONINFORMATION);
                }
                return 0;
            }
            break;
        }

        case WM_COMMAND:
            if (HIWORD(wp) == BN_CLICKED) {
                if (LOWORD(wp) == 1204) {
                    BeginCapture(CaptureTarget::Cure, hCureKey);
                    return 0;
                }
                if (LOWORD(wp) == 1301) {
                    BeginCapture(CaptureTarget::Start, hStartKey);
                    return 0;
                }
                if (LOWORD(wp) == 1302) {
                    BeginCapture(CaptureTarget::Stop, hStopKey);
                    return 0;
                }
            }
            break;

        case WM_APP_CAPTURED_KEY: {
            const int vk = static_cast<int>(wp);
            const CaptureTarget target = static_cast<CaptureTarget>(lp);
            Settings s;
            {
                std::lock_guard<std::mutex> lk(g_settingsMutex);
                s = g_settings;
            }

            if (!CanAssignHotkey(target, vk, s)) {
                SyncControls();
                RefreshRuntimeHotkeys(false);
                MessageBoxW(hwnd,
                            target == CaptureTarget::Cure
                                ? L"Bu tu\u015F giri\u015F dizisi, R Combo veya A\u00E7ma/Kapatma tu\u015Flar\u0131ndan biri taraf\u0131ndan kullan\u0131l\u0131yor."
                                : L"Bu tu\u015F giri\u015F dizisi, R Combo veya Cure taraf\u0131ndan kullan\u0131l\u0131yor.",
                            L"Tu\u015F Atama", MB_OK | MB_ICONWARNING);
                return 0;
            }

            {
                std::lock_guard<std::mutex> lk(g_settingsMutex);
                if (target == CaptureTarget::Cure) g_settings.cureHotkey = vk;
                else if (target == CaptureTarget::Start) g_settings.startHotkey = vk;
                else if (target == CaptureTarget::Stop) g_settings.stopHotkey = vk;
            }
            HWND targetButton = CaptureButton(target);
            if (targetButton) SetWindowTextW(targetButton, VkName(vk).c_str());
            if (!RefreshRuntimeHotkeys(true)) SyncControls();
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }

        case WM_HOTKEY:
            HandleControlAction(static_cast<int>(wp));
            return 0;

        case WM_APP_CONTROL_KEY:
            HandleControlAction(static_cast<int>(wp));
            return 0;

        case WM_APP_CURE_TRIGGER:
            if (g_active.load(std::memory_order_relaxed)) std::thread(CureWorker).detach();
            return 0;

        case WM_CTLCOLORSTATIC:
        case WM_CTLCOLOREDIT:
        case WM_CTLCOLORBTN:
        case WM_CTLCOLORLISTBOX: {
            HDC dc = reinterpret_cast<HDC>(wp);
            SetBkColor(dc, RGB(250, 243, 228));
            SetTextColor(dc, RGB(65, 39, 20));
            SetBkMode(dc, OPAQUE);
            return reinterpret_cast<LRESULT>(msg == WM_CTLCOLOREDIT || msg == WM_CTLCOLORLISTBOX ? g_brushInput : g_brushCream);
        }

        case WM_ERASEBKGND:
            return 1;

        case WM_PRINTCLIENT:
            DrawUi(hwnd, reinterpret_cast<HDC>(wp));
            return 0;

        case WM_PAINT: {
            PAINTSTRUCT ps{};
            HDC dc = BeginPaint(hwnd, &ps);
            HDC mem = CreateCompatibleDC(dc);
            HBITMAP bmp = CreateCompatibleBitmap(dc, W, H);
            HGDIOBJ old = nullptr;
            if (mem && bmp) {
                old = SelectObject(mem, bmp);
                DrawUi(hwnd, mem);
                BitBlt(dc, 0, 0, W, H, mem, 0, 0, SRCCOPY);
                SelectObject(mem, old);
            } else {
                DrawUi(hwnd, dc);
            }
            if (bmp) DeleteObject(bmp);
            if (mem) DeleteDC(mem);
            EndPaint(hwnd, &ps);
            g_paintCount.fetch_add(1, std::memory_order_relaxed);
            UpdateDiagnosticMarkers();
            return 0;
        }

        case WM_DESTROY:
            RemovePropW(hwnd, L"PPC_CONTROLS");
            RemovePropW(hwnd, L"PPC_ACTIVE");
            RemovePropW(hwnd, L"PPC_MODE");
            RemovePropW(hwnd, L"PPC_COMBO_BURSTS");
            RemovePropW(hwnd, L"PPC_R_PRESSES");
            RemovePropW(hwnd, L"PPC_CURE_RUNS");
            RemovePropW(hwnd, L"PPC_PAINTS");
            UnregisterRuntimeHotkeys();
            g_captureTarget.store(static_cast<int>(CaptureTarget::None), std::memory_order_release);
            if (g_keyboardHook) {
                UnhookWindowsHookEx(g_keyboardHook);
                g_keyboardHook = nullptr;
            }
            g_quit.store(true, std::memory_order_release);
            g_active.store(false, std::memory_order_release);
            if (g_wakeEvent) SetEvent(g_wakeEvent);
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

} // namespace

int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE, LPWSTR, int) {
    timeBeginPeriod(1);

    GdiplusStartupInput gdip{};
    if (GdiplusStartup(&g_gdiplusToken, &gdip, nullptr) != Ok) {
        timeEndPeriod(1);
        return 6;
    }

    g_brushCream = CreateSolidBrush(RGB(245, 235, 214));
    g_brushInput = CreateSolidBrush(RGB(252, 247, 237));
    g_font = CreateFontW(-15, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                         OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    g_fontBold = CreateFontW(-15, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                             OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");

    if (!g_brushCream || !g_brushInput || !g_font || !g_fontBold) {
        MessageBoxW(nullptr, L"Aray\u00FCz kaynaklar\u0131 olu\u015Fturulamad\u0131.", L"Premium Plus Combo", MB_OK | MB_ICONERROR);
        if (g_font) DeleteObject(g_font);
        if (g_fontBold) DeleteObject(g_fontBold);
        if (g_brushCream) DeleteObject(g_brushCream);
        if (g_brushInput) DeleteObject(g_brushInput);
        GdiplusShutdown(g_gdiplusToken);
        timeEndPeriod(1);
        return 7;
    }

    g_active.store(false, std::memory_order_relaxed);
    g_quit.store(false, std::memory_order_relaxed);
    g_mode.store(Mode::Maximum, std::memory_order_relaxed);
    g_rSuppressUntil.store(0, std::memory_order_relaxed);
    g_startedTicks.store(0, std::memory_order_relaxed);
    g_captureTarget.store(static_cast<int>(CaptureTarget::None), std::memory_order_relaxed);
    g_capsPhysicalDown.store(false, std::memory_order_relaxed);
    g_paintCount.store(0, std::memory_order_relaxed);
    LoadRegistry();

    g_karus = LoadImageResource(IDR_KARUS);
    g_human = LoadImageResource(IDR_HUMAN);
    if (!g_karus || !g_human) {
        MessageBoxW(nullptr,
                    L"G\u00F6m\u00FCl\u00FC Rogue g\u00F6rselleri y\u00FCklenemedi. Program ba\u015Flat\u0131lmad\u0131.",
                    L"Premium Plus Combo", MB_OK | MB_ICONERROR);
        g_karus.reset();
        g_human.reset();
        DeleteObject(g_font);
        DeleteObject(g_fontBold);
        DeleteObject(g_brushCream);
        DeleteObject(g_brushInput);
        GdiplusShutdown(g_gdiplusToken);
        timeEndPeriod(1);
        return 4;
    }

    g_wakeEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (!g_wakeEvent) {
        MessageBoxW(nullptr, L"\u00C7al\u0131\u015Fma motoru ba\u015Flat\u0131lamad\u0131.", L"Premium Plus Combo", MB_OK | MB_ICONERROR);
        g_karus.reset();
        g_human.reset();
        DeleteObject(g_font);
        DeleteObject(g_fontBold);
        DeleteObject(g_brushCream);
        DeleteObject(g_brushInput);
        GdiplusShutdown(g_gdiplusToken);
        timeEndPeriod(1);
        return 5;
    }

    WNDCLASSEXW wc{sizeof(wc)};
    wc.style = 0;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.hIcon = LoadIconW(hInst, MAKEINTRESOURCEW(IDI_APP));
    if (!wc.hIcon) wc.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
    wc.hIconSm = wc.hIcon;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.lpszClassName = L"PremiumPlusComboWindow";
    if (!RegisterClassExW(&wc)) {
        CloseHandle(g_wakeEvent);
        g_wakeEvent = nullptr;
        g_karus.reset();
        g_human.reset();
        DeleteObject(g_font);
        DeleteObject(g_fontBold);
        DeleteObject(g_brushCream);
        DeleteObject(g_brushInput);
        GdiplusShutdown(g_gdiplusToken);
        timeEndPeriod(1);
        return 8;
    }

    const int sx = (GetSystemMetrics(SM_CXSCREEN) - W) / 2;
    const int sy = (GetSystemMetrics(SM_CYSCREEN) - H) / 2;
    g_hwnd = CreateWindowExW(WS_EX_APPWINDOW, L"PremiumPlusComboWindow", L"Premium Plus Combo|READY",
                             WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
                             sx, sy, W, H, nullptr, nullptr, hInst, nullptr);
    if (!g_hwnd) {
        CloseHandle(g_wakeEvent);
        g_wakeEvent = nullptr;
        g_karus.reset();
        g_human.reset();
        DeleteObject(g_font);
        DeleteObject(g_fontBold);
        DeleteObject(g_brushCream);
        DeleteObject(g_brushInput);
        GdiplusShutdown(g_gdiplusToken);
        timeEndPeriod(1);
        return 2;
    }

    if (!RefreshRuntimeHotkeys(true)) {
        DestroyWindow(g_hwnd);
        CloseHandle(g_wakeEvent);
        g_wakeEvent = nullptr;
        g_karus.reset();
        g_human.reset();
        DeleteObject(g_font);
        DeleteObject(g_fontBold);
        DeleteObject(g_brushCream);
        DeleteObject(g_brushInput);
        GdiplusShutdown(g_gdiplusToken);
        timeEndPeriod(1);
        return 9;
    }

    std::thread combo(ComboWorker);
    std::thread rworker(RWorker);

    ShowWindow(g_hwnd, SW_SHOW);
    UpdateWindow(g_hwnd);
    UpdateStateTitle();

    MSG m{};
    while (GetMessageW(&m, nullptr, 0, 0) > 0) {
        TranslateMessage(&m);
        DispatchMessageW(&m);
    }

    g_quit.store(true, std::memory_order_release);
    g_active.store(false, std::memory_order_release);
    SetEvent(g_wakeEvent);
    if (combo.joinable()) combo.join();
    if (rworker.joinable()) rworker.join();
    for (int i = 0; i < 500 && g_cureBusy.load(std::memory_order_acquire); ++i) Sleep(1);

    UnregisterRuntimeHotkeys();
    if (g_keyboardHook) {
        UnhookWindowsHookEx(g_keyboardHook);
        g_keyboardHook = nullptr;
    }
    CloseHandle(g_wakeEvent);
    g_wakeEvent = nullptr;
    DeleteObject(g_font);
    DeleteObject(g_fontBold);
    DeleteObject(g_brushCream);
    DeleteObject(g_brushInput);
    g_karus.reset();
    g_human.reset();
    GdiplusShutdown(g_gdiplusToken);
    timeEndPeriod(1);
    return 0;
}
