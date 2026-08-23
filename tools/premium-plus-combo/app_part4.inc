LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
        case WM_CREATE: {
            if (!MakeChildControls(hwnd)) {
                MessageBoxW(hwnd, L"Arayüz kontrolleri oluşturulamadı. Program başlatılmadı.", L"Premium Plus Combo", MB_OK | MB_ICONERROR);
                return -1;
            }
            SetPropW(hwnd, L"PPC_CONTROLS", reinterpret_cast<HANDLE>(static_cast<INT_PTR>(10)));
            SetTimer(hwnd, TIMER_UI, 100, nullptr);
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
                            L"Maximum Mod, giriş dizisindeki her tuş için yaklaşık 120 basım/sn; Turbo Mod ise yaklaşık 240 basım/sn hedefler.\n\n"
                            L"Örnek: 8 > 9 > 0 dizisinde Maximum Mod açıkken 8, 9 ve 0 tuşlarının her biri kendi 120/sn hedefinde sırayla gönderilir.",
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
                                L"Ayarlar kaydedildi. Giriş dizisi, R Combo ve Cure ayarları bir sonraki açılışta geri yüklenir. Program her açılışta kapalı ve Maximum Mod hazır durumda başlar.",
                                L"Premium Plus Combo", MB_OK | MB_ICONINFORMATION);
                }
                return 0;
            }
            break;
        }

        case WM_COMMAND:
            if (LOWORD(wp) == 1204 && HIWORD(wp) == BN_CLICKED) {
                StopMacro();
                g_captureCure.store(true, std::memory_order_release);
                SetWindowTextW(hCureKey, L"Tuşa bas...");
                return 0;
            }
            break;

        case WM_APP_CUREKEY:
            if (lp == 2) {
                g_captureCure.store(true, std::memory_order_release);
                MessageBoxW(hwnd,
                            L"Bu tuş giriş dizisi, R Combo veya başlat/durdur kontrolü tarafından kullanılıyor. Cure için farklı bir tuş seçin.",
                            L"Cure Tuşu", MB_OK | MB_ICONWARNING);
                SetWindowTextW(hCureKey, L"Tuşa bas...");
            } else {
                SetWindowTextW(hCureKey, VkName(static_cast<int>(wp)).c_str());
            }
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;

        case WM_TIMER:
            if (wp == TIMER_UI) {
                UpdateDiagnosticMarkers();
                InvalidateRect(hwnd, nullptr, FALSE);
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

        case WM_PAINT: {
            PAINTSTRUCT ps{};
            HDC dc = BeginPaint(hwnd, &ps);
            DrawUi(hwnd, dc);
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
    wc.style = CS_HREDRAW | CS_VREDRAW;
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
                             WS_POPUP | WS_CLIPCHILDREN, sx, sy, W, H, nullptr, nullptr, hInst, nullptr);
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

    std::thread combo(ComboWorker);
    std::thread rworker(RWorker);
    std::thread input(InputWorker);

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
    if (input.joinable()) input.join();
    for (int i = 0; i < 500 && g_cureBusy.load(std::memory_order_acquire); ++i) Sleep(1);

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
