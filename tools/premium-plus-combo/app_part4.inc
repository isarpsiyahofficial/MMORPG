void BeginCapture(CaptureTarget target, HWND button) {
    StopMacro();
    g_captureTarget.store(static_cast<int>(target), std::memory_order_release);
    SetWindowTextW(button, L"Tuşa bas...");
}

HWND CaptureButton(CaptureTarget target) {
    if (target == CaptureTarget::Start) return hStartKey;
    if (target == CaptureTarget::Stop) return hStopKey;
    if (target == CaptureTarget::Cure) return hCureKey;
    return nullptr;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
        case WM_CREATE: {
            if (!MakeChildControls(hwnd)) {
                MessageBoxW(hwnd, L"Arayüz kontrolleri oluşturulamadı. Program başlatılmadı.", L"Premium Plus Combo", MB_OK | MB_ICONERROR);
                return -1;
            }
            SetPropW(hwnd, L"PPC_CONTROLS", reinterpret_cast<HANDLE>(static_cast<INT_PTR>(12)));
            SetTimer(hwnd, TIMER_UI, 500, nullptr);
            return 0;
        }

        case WM_NCHITTEST: {
            POINT p{GET_X_LPARAM(lp), GET_Y_LPARAM(lp)};
            ScreenToClient(hwnd, &p);
            if (p.y < 55 && !PtIn(rcMin, p.x, p.y) && !PtIn(rcClose, p.x, p.y)) return HTCAPTION;
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
                            L"120/sn ve 240/sn değerleri, giriş dizisindeki HER BİR tuşun bir saniyedeki yaklaşık basılma hedefini ifade eder.\n\n"
                            L"Örnek: 8 > 9 > 0 dizisinde Maximum Mod seçiliyken 8, 9 ve 0 tuşlarının her biri yaklaşık 120 basım/sn hedefiyle gönderilir. Turbo Modda bu hedef yaklaşık 240 basım/sn olur.\n\n"
                            L"R hızı bu değerlerden bağımsızdır; Rogue Ayarları bölümündeki Maximum R/sn ve Turbo R/sn alanlarından belirlenir.",
                            L"Hız Bilgisi", MB_OK | MB_ICONINFORMATION);
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
                if (ApplyControls(true)) {
                    SaveRegistry();
                    MessageBoxW(hwnd,
                                L"Ayarlar kaydedildi. Giriş dizisi, Açma/Kapatma tuşları, R Combo ve Cure ayarları sonraki açılışta geri yüklenir. Program her açılışta kapalı ve Maximum Mod hazır durumda başlar.",
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
                HWND targetButton = CaptureButton(target);
                if (targetButton) SetWindowTextW(targetButton, L"Tuşa bas...");
                g_captureTarget.store(static_cast<int>(target), std::memory_order_release);
                MessageBoxW(hwnd,
                            target == CaptureTarget::Cure
                                ? L"Bu tuş giriş dizisi, R Combo veya Açma/Kapatma tuşlarından biri tarafından kullanılıyor. Cure için farklı bir tuş seçin."
                                : L"Bu tuş giriş dizisi, R Combo veya Cure tarafından kullanılıyor. Açma/Kapatma için farklı bir tuş seçin. Açma ve Kapatma tuşlarının birbirleriyle aynı olması serbesttir.",
                            L"Tuş Atama", MB_OK | MB_ICONWARNING);
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
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }

        case WM_APP_HOTKEY_ACTION: {
            const int action = static_cast<int>(static_cast<INT_PTR>(wp));
            if (action > 0) {
                if (ApplyControls(false)) StartMacro();
            } else if (action < 0) {
                StopMacro();
            }
            return 0;
        }

        case WM_APP_CURE_TRIGGER:
            if (g_active.load(std::memory_order_relaxed)) std::thread(CureWorker).detach();
            return 0;

        case WM_TIMER:
            if (wp == TIMER_UI) {
                UpdateDiagnosticMarkers();
                // No continuous full-window repaint. Only the elapsed-time/status
                // footer needs refreshing while the macro is actually active.
                if (g_active.load(std::memory_order_relaxed)) InvalidateRect(hwnd, &rcFooter, FALSE);
                return 0;
            }
            break;

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
            return 0;
        }

        case WM_DESTROY:
            RemovePropW(hwnd, L"PPC_CONTROLS");
            RemovePropW(hwnd, L"PPC_ACTIVE");
            RemovePropW(hwnd, L"PPC_MODE");
            RemovePropW(hwnd, L"PPC_COMBO_BURSTS");
            RemovePropW(hwnd, L"PPC_R_PRESSES");
            RemovePropW(hwnd, L"PPC_CURE_RUNS");
            g_captureTarget.store(static_cast<int>(CaptureTarget::None), std::memory_order_release);
            if (g_keyboardHook) {
                UnhookWindowsHookEx(g_keyboardHook);
                g_keyboardHook = nullptr;
            }
            g_quit.store(true, std::memory_order_release);
            g_active.store(false, std::memory_order_release);
            if (g_wakeEvent) SetEvent(g_wakeEvent);
            KillTimer(hwnd, TIMER_UI);
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
    g_font = CreateFontW(-16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                         OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    g_fontBold = CreateFontW(-16, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                             OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");

    if (!g_brushCream || !g_brushInput || !g_font || !g_fontBold) {
        MessageBoxW(nullptr, L"Arayüz kaynakları oluşturulamadı.", L"Premium Plus Combo", MB_OK | MB_ICONERROR);
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
    LoadRegistry();

    g_karus = LoadImageResource(IDR_KARUS);
    g_human = LoadImageResource(IDR_HUMAN);
    if (!g_karus || !g_human) {
        MessageBoxW(nullptr,
                    L"Gömülü Rogue arka plan görselleri yüklenemedi. Program hatalı görünmemesi için başlatılmadı.",
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
        MessageBoxW(nullptr, L"Çalışma motoru başlatılamadı.", L"Premium Plus Combo", MB_OK | MB_ICONERROR);
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
                             WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, sx, sy, W, H, nullptr, nullptr, hInst, nullptr);
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

    g_keyboardHook = SetWindowsHookExW(WH_KEYBOARD_LL, KeyboardProc, hInst, 0);
    if (!g_keyboardHook) {
        MessageBoxW(g_hwnd, L"Global kontrol tuşları başlatılamadı. Program güvenli şekilde kapatılacak.", L"Premium Plus Combo", MB_OK | MB_ICONERROR);
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
