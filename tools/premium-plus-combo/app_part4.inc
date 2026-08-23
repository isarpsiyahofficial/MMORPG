    Panel(g,RectF(195,320,430,60),Color(240,248,238,220),Color(255,181,132,51),8);
    DrawTextG(g,L"Rogue Ayarları",205,320,140,22,14,true);
    DrawTextG(g,L"Maximum R /sn",300,333,90,30,12,false);
    DrawTextG(g,L"Turbo R /sn",455,333,78,30,12,false);

    Panel(g,RectF(195,392,430,83),Color(240,248,238,220),Color(255,181,132,51),8);
    DrawTextG(g,L"Cure Ayarları",205,391,140,22,14,true);
    DrawTextG(g,L"Bar",292,405,27,29,12,false); DrawTextG(g,L"Slot",396,405,36,29,12,false); DrawTextG(g,L"Cure Tuşu",494,405,56,29,12,false);
    DrawTextG(g,L"Cure alınırken R Combo 2 saniye durur; R kapalıysa program R'yi açmaz.",208,438,405,27,11,false,Color(255,94,57,31),StringAlignmentCenter);

    Button(g,rcSave,L"Ayarlarımı Kaydet",Color(255,134,29,22),Color(255,83,13,16),false);
    std::wstring st = g_active.load()?L"● Çalışıyor":L"● Hazır";
    DrawTextG(g,st,24,526,140,25,14,true,g_active.load()?Color(255,33,119,43):Color(255,88,73,47));
    DrawTextG(g,g_mode.load()==Mode::Turbo?L"Mod: Turbo":L"Mod: Maximum",340,526,140,25,13,true,Color(255,98,48,18),StringAlignmentCenter);
    Settings s; { std::lock_guard<std::mutex> lk(g_settingsMutex); s=g_settings; }
    DrawTextG(g,L"Cure: "+VkName(s.cureHotkey),650,526,135,25,12,true,Color(255,96,61,30),StringAlignmentFar);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch(msg) {
        case WM_CREATE: MakeChildControls(hwnd); SetTimer(hwnd,TIMER_UI,250,nullptr); return 0;
        case WM_NCHITTEST: {
            POINT p{GET_X_LPARAM(lp),GET_Y_LPARAM(lp)}; ScreenToClient(hwnd,&p);
            if(p.y<58 && !PtIn(rcMin,p.x,p.y) && !PtIn(rcClose,p.x,p.y)) return HTCAPTION;
            return HTCLIENT;
        }
        case WM_LBUTTONUP: {
            int x=GET_X_LPARAM(lp),y=GET_Y_LPARAM(lp);
            if(PtIn(rcClose,x,y)){ DestroyWindow(hwnd); return 0; }
            if(PtIn(rcMin,x,y)){ ShowWindow(hwnd,SW_MINIMIZE); return 0; }
            if(PtIn(rcStart,x,y)){ if(ApplyControls(true)) StartMacro(); return 0; }
            if(PtIn(rcStop,x,y)){ StopMacro(); return 0; }
            if(PtIn(rcInfoMax,x,y)||PtIn(rcInfoTurbo,x,y)){
                MessageBoxW(hwnd,L"120/sn ve 240/sn gibi değerler, giriş dizisindeki HER BİR tuşa ayrı ayrı bir saniye içinde gönderilmesi hedeflenen ortalama basım sayısını ifade eder.\n\nÖrnek: Maximum Mod 120/sn ise varsayılan dizide 8 yaklaşık 120, 9 yaklaşık 120 ve 0 yaklaşık 120 basım/sn hedeflenir. Turbo Mod 240/sn ise her tuş ayrı ayrı yaklaşık 240 basım/sn hedeflenir.",L"Hız Bilgisi",MB_OK|MB_ICONINFORMATION); return 0;
            }
            if(PtIn(rcMax,x,y)){ g_mode.store(Mode::Maximum); InvalidateRect(hwnd,nullptr,FALSE); return 0; }
            if(PtIn(rcTurbo,x,y)){ g_mode.store(Mode::Turbo); InvalidateRect(hwnd,nullptr,FALSE); return 0; }
            if(PtIn(rcSave,x,y)){
                if(ApplyControls(true)){ SaveRegistry(); MessageBoxW(hwnd,L"Ayarlar kaydedildi. Bir sonraki açılışta giriş dizisi, R Combo ve Cure ayarları otomatik yüklenir. Program her açılışta Maximum Mod ile başlar.",L"Premium Plus Combo",MB_OK|MB_ICONINFORMATION); }
                return 0;
            }
            break;
        }
        case WM_COMMAND:
            if(LOWORD(wp)==1204 && HIWORD(wp)==BN_CLICKED){ g_captureCure.store(true); SetWindowTextW(hCureKey,L"Tuşa bas..."); return 0; }
            break;
        case WM_APP_CUREKEY: SetWindowTextW(hCureKey,VkName((int)wp).c_str()); InvalidateRect(hwnd,nullptr,FALSE); return 0;
        case WM_TIMER: if(wp==TIMER_UI){ InvalidateRect(hwnd,nullptr,FALSE); return 0; } break;
        case WM_CTLCOLORSTATIC:
        case WM_CTLCOLOREDIT:
        case WM_CTLCOLORBTN: {
            HDC dc=(HDC)wp; SetBkColor(dc,RGB(245,235,214)); SetTextColor(dc,RGB(70,42,22)); return (LRESULT)g_brushCream;
        }
        case WM_PAINT: { PAINTSTRUCT ps{}; HDC dc=BeginPaint(hwnd,&ps); DrawUi(hwnd,dc); EndPaint(hwnd,&ps); return 0; }
        case WM_DESTROY: g_quit.store(true); g_active.store(false); SetEvent(g_wakeEvent); KillTimer(hwnd,TIMER_UI); PostQuitMessage(0); return 0;
    }
    return DefWindowProcW(hwnd,msg,wp,lp);
}

} // namespace

int WINAPI wWinMain(HINSTANCE hInst,HINSTANCE,LPWSTR,int) {
    timeBeginPeriod(1);
    GdiplusStartupInput gdip{}; GdiplusStartup(&g_gdiplusToken,&gdip,nullptr);
    g_brushCream=CreateSolidBrush(RGB(245,235,214));
    g_font=CreateFontW(-16,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,TURKISH_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Segoe UI");
    g_fontBold=CreateFontW(-16,0,0,0,FW_SEMIBOLD,FALSE,FALSE,FALSE,TURKISH_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Segoe UI");
    LoadRegistry();
    g_karus=LoadPngResource(IDR_KARUS); g_human=LoadPngResource(IDR_HUMAN);
    g_wakeEvent=CreateEventW(nullptr,TRUE,FALSE,nullptr);

    WNDCLASSEXW wc{sizeof(wc)}; wc.style=CS_HREDRAW|CS_VREDRAW; wc.lpfnWndProc=WndProc; wc.hInstance=hInst; wc.hIcon=LoadIconW(nullptr,IDI_APPLICATION); wc.hCursor=LoadCursorW(nullptr,IDC_ARROW); wc.lpszClassName=L"PremiumPlusComboWindow";
    RegisterClassExW(&wc);
    int sx=(GetSystemMetrics(SM_CXSCREEN)-W)/2, sy=(GetSystemMetrics(SM_CYSCREEN)-H)/2;
    g_hwnd=CreateWindowExW(WS_EX_APPWINDOW,L"PremiumPlusComboWindow",L"Premium Plus Combo",WS_POPUP,sx,sy,W,H,nullptr,nullptr,hInst,nullptr);
    if(!g_hwnd) return 2;
    g_hook=SetWindowsHookExW(WH_KEYBOARD_LL,KeyboardProc,hInst,0);
    if(!g_hook){ MessageBoxW(g_hwnd,L"Global klavye dinleyicisi başlatılamadı.",L"Premium Plus Combo",MB_OK|MB_ICONERROR); return 3; }

    std::thread combo(ComboWorker), rworker(RWorker);
    ShowWindow(g_hwnd,SW_SHOW); UpdateWindow(g_hwnd);
    MSG m{}; while(GetMessageW(&m,nullptr,0,0)>0){ TranslateMessage(&m); DispatchMessageW(&m); }
    g_quit.store(true); g_active.store(false); SetEvent(g_wakeEvent);
    if(combo.joinable()) combo.join(); if(rworker.joinable()) rworker.join();
    if(g_hook) UnhookWindowsHookEx(g_hook); if(g_wakeEvent) CloseHandle(g_wakeEvent);
    if(g_font) DeleteObject(g_font); if(g_fontBold) DeleteObject(g_fontBold); if(g_brushCream) DeleteObject(g_brushCream);
    g_karus.reset(); g_human.reset(); if(g_gdiplusToken) GdiplusShutdown(g_gdiplusToken); timeEndPeriod(1);
    return 0;
}
