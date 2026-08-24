void Panel(Graphics& g, RectF r, Color fill = Color(245, 250, 241, 226),
           Color border = Color(255, 182, 132, 51), float radius = 8.0f) {
    GraphicsPath p;
    const float d = radius * 2.0f;
    p.AddArc(r.X, r.Y, d, d, 180, 90);
    p.AddArc(r.GetRight() - d, r.Y, d, d, 270, 90);
    p.AddArc(r.GetRight() - d, r.GetBottom() - d, d, d, 0, 90);
    p.AddArc(r.X, r.GetBottom() - d, d, d, 90, 90);
    p.CloseFigure();
    SolidBrush b(fill);
    Pen pen(border, 1.2f);
    g.FillPath(&b, &p);
    g.DrawPath(&pen, &p);
}

void Button(Graphics& g, const RECT& rr, const std::wstring& text, Color c1, Color c2, bool selected = false) {
    RectF r(static_cast<REAL>(rr.left), static_cast<REAL>(rr.top),
            static_cast<REAL>(rr.right - rr.left), static_cast<REAL>(rr.bottom - rr.top));
    GraphicsPath p;
    const float d = 16.0f;
    p.AddArc(r.X, r.Y, d, d, 180, 90);
    p.AddArc(r.GetRight() - d, r.Y, d, d, 270, 90);
    p.AddArc(r.GetRight() - d, r.GetBottom() - d, d, d, 0, 90);
    p.AddArc(r.X, r.GetBottom() - d, d, d, 90, 90);
    p.CloseFigure();

    LinearGradientBrush br(r, c1, c2, LinearGradientModeVertical);
    g.FillPath(&br, &p);
    Pen pen(selected ? Color(255, 255, 205, 80) : Color(255, 158, 106, 30), selected ? 2.1f : 1.2f);
    g.DrawPath(&pen, &p);
    DrawTextG(g, text, r.X + 5, r.Y, r.Width - 10, r.Height, 14, true,
              Color(255, 255, 248, 229), StringAlignmentCenter);
}

void DrawModeCard(Graphics& g, const RECT& rr, bool turbo, bool selected) {
    RectF r(static_cast<REAL>(rr.left), static_cast<REAL>(rr.top),
            static_cast<REAL>(rr.right - rr.left), static_cast<REAL>(rr.bottom - rr.top));
    const Color fill = selected ? (turbo ? Color(250, 105, 22, 20) : Color(248, 111, 67, 19)) : Color(242, 250, 243, 225);
    const Color txt = selected ? Color(255, 255, 235, 164) : Color(255, 72, 43, 20);
    Panel(g, r, fill, selected ? Color(255, 246, 184, 60) : Color(255, 175, 129, 47), 9.0f);
    DrawTextG(g, turbo ? L"Turbo Mod" : L"Maximum Mod", r.X + 10, r.Y + 5, r.Width - 38, 22, 14, true, txt);
    DrawTextG(g,
              turbo ? L"~240 bas\u0131m/sn / her tu\u015F" : L"~120 bas\u0131m/sn / her tu\u015F",
              r.X + 8, r.Y + 29, r.Width - 16, 18, 10, false, txt, StringAlignmentCenter);
    DrawTextG(g, L"i", r.GetRight() - 27, r.Y + 5, 18, 18, 12, true,
              selected ? Color(255, 255, 205, 77) : Color(255, 112, 70, 24), StringAlignmentCenter);
}

void DrawPortraitCard(Graphics& g, Bitmap* image, const Rect& dest) {
    Panel(g, RectF(static_cast<REAL>(dest.X - 3), static_cast<REAL>(dest.Y - 3),
                   static_cast<REAL>(dest.Width + 6), static_cast<REAL>(dest.Height + 6)),
          Color(255, 241, 223, 187), Color(255, 164, 108, 31), 8.0f);
    if (!image || image->GetLastStatus() != Ok || image->GetWidth() == 0 || image->GetHeight() == 0) return;

    const REAL targetRatio = static_cast<REAL>(dest.Width) / static_cast<REAL>(dest.Height);
    const REAL sw = static_cast<REAL>(image->GetWidth());
    const REAL sh = static_cast<REAL>(image->GetHeight());
    const REAL sourceRatio = sw / sh;
    INT sx = 0, sy = 0, cw = static_cast<INT>(sw), ch = static_cast<INT>(sh);
    if (sourceRatio > targetRatio) {
        cw = static_cast<INT>(sh * targetRatio);
        sx = (static_cast<INT>(sw) - cw) / 2;
    } else {
        ch = static_cast<INT>(sw / targetRatio);
        sy = (static_cast<INT>(sh) - ch) / 2;
    }
    g.SetInterpolationMode(InterpolationModeHighQualityBicubic);
    g.DrawImage(image, dest, sx, sy, cw, ch, UnitPixel);
    LinearGradientBrush veil(dest, Color(0, 251, 241, 220), Color(58, 244, 222, 183), LinearGradientModeVertical);
    g.FillRectangle(&veil, dest);
}

void SaveRegistry() {
    Settings s;
    {
        std::lock_guard<std::mutex> lk(g_settingsMutex);
        s = g_settings;
    }
    HKEY key{};
    if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\PremiumPlusCombo\\RogueV2", 0, nullptr, 0,
                        KEY_WRITE, nullptr, &key, nullptr) != ERROR_SUCCESS) return;
    auto dw = [&](const wchar_t* n, DWORD v) { RegSetValueExW(key, n, 0, REG_DWORD, reinterpret_cast<BYTE*>(&v), sizeof(v)); };
    dw(L"Seq1", s.comboVk[0]);
    dw(L"Seq2", s.comboVk[1]);
    dw(L"Seq3", s.comboVk[2]);
    dw(L"REnabled", s.rEnabled ? 1 : 0);
    dw(L"RMax", s.rRateMaximum);
    dw(L"RTurbo", s.rRateTurbo);
    dw(L"CureEnabled", s.cureEnabled ? 1 : 0);
    dw(L"CureBar", s.cureBar);
    dw(L"CureSlot", s.cureSlot);
    dw(L"CureHotkey", s.cureHotkey);
    dw(L"StartHotkey", s.startHotkey);
    dw(L"StopHotkey", s.stopHotkey);
    RegCloseKey(key);
}

void LoadRegistry() {
    Settings s;
    HKEY key{};
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\PremiumPlusCombo\\RogueV2", 0, KEY_READ, &key) == ERROR_SUCCESS) {
        auto rd = [&](const wchar_t* n, DWORD def) {
            DWORD v = def;
            DWORD sz = sizeof(v);
            DWORD t = REG_DWORD;
            if (RegQueryValueExW(key, n, nullptr, &t, reinterpret_cast<BYTE*>(&v), &sz) != ERROR_SUCCESS || t != REG_DWORD) v = def;
            return v;
        };
        s.comboVk[0] = static_cast<int>(rd(L"Seq1", '8'));
        s.comboVk[1] = static_cast<int>(rd(L"Seq2", '9'));
        s.comboVk[2] = static_cast<int>(rd(L"Seq3", '0'));
        s.rEnabled = rd(L"REnabled", 1) != 0;
        s.rRateMaximum = static_cast<int>(rd(L"RMax", 25));
        s.rRateTurbo = static_cast<int>(rd(L"RTurbo", 40));
        s.cureEnabled = rd(L"CureEnabled", 1) != 0;
        s.cureBar = static_cast<int>(rd(L"CureBar", 2));
        s.cureSlot = static_cast<int>(rd(L"CureSlot", 6));
        s.cureHotkey = static_cast<int>(rd(L"CureHotkey", 'C'));
        s.startHotkey = static_cast<int>(rd(L"StartHotkey", VK_TAB));
        s.stopHotkey = static_cast<int>(rd(L"StopHotkey", VK_CAPITAL));
        RegCloseKey(key);
    }
    s = ppc::sanitize(s);
    std::lock_guard<std::mutex> lk(g_settingsMutex);
    g_settings = s;
}

std::wstring GetText(HWND h) {
    wchar_t b[64]{};
    GetWindowTextW(h, b, 64);
    return b;
}

int GetInt(HWND h, int def) {
    const std::wstring s = GetText(h);
    const int v = _wtoi(s.c_str());
    return v != 0 ? v : def;
}

bool ApplyControls(bool showError) {
    Settings s;
    {
        std::lock_guard<std::mutex> lk(g_settingsMutex);
        s = g_settings;
    }

    const int k1 = ParseVk(GetText(hSeq1));
    const int k2 = ParseVk(GetText(hSeq2));
    const int k3 = ParseVk(GetText(hSeq3));
    if (!k1 || !k2 || !k3) {
        if (showError) MessageBoxW(g_hwnd,
            L"Giri\u015F dizisinde ge\u00E7ersiz bir tu\u015F var. 0-9, A-Z veya F1-F12 kullanabilirsiniz.",
            L"Giri\u015F Dizisi", MB_OK | MB_ICONWARNING);
        return false;
    }
    if (k1 == k2 || k1 == k3 || k2 == k3) {
        if (showError) MessageBoxW(g_hwnd,
            L"Giri\u015F dizisindeki \u00FC\u00E7 tu\u015F birbirinden farkl\u0131 olmal\u0131d\u0131r.",
            L"Giri\u015F Dizisi", MB_OK | MB_ICONWARNING);
        return false;
    }
    if (k1 == 'R' || k2 == 'R' || k3 == 'R') {
        if (showError) MessageBoxW(g_hwnd,
            L"R tu\u015Fu ayr\u0131 R Combo motoru taraf\u0131ndan kullan\u0131l\u0131r. Giri\u015F dizisi i\u00E7in farkl\u0131 bir tu\u015F se\u00E7in.",
            L"Giri\u015F Dizisi", MB_OK | MB_ICONWARNING);
        return false;
    }

    s.comboVk = {k1, k2, k3};
    s.rEnabled = Button_GetCheck(hRCheck) == BST_CHECKED;
    s.rRateMaximum = std::clamp(GetInt(hRMax, 25), 1, 100);
    s.rRateTurbo = std::clamp(GetInt(hRTurbo, 40), 1, 100);
    s.cureEnabled = Button_GetCheck(hCureCheck) == BST_CHECKED;
    s.cureBar = ComboBox_GetCurSel(hCureBar) + 1;
    s.cureSlot = ComboBox_GetCurSel(hCureSlot) + 1;
    s = ppc::sanitize(s);

    if (!CanAssignHotkey(CaptureTarget::Start, s.startHotkey, s) ||
        !CanAssignHotkey(CaptureTarget::Stop, s.stopHotkey, s)) {
        if (showError) MessageBoxW(g_hwnd,
            L"A\u00E7ma/Kapatma tu\u015Flar\u0131 giri\u015F dizisi, R Combo veya Cure tu\u015Fuyla \u00E7ak\u0131\u015Famaz.",
            L"Kontrol Tu\u015Flar\u0131", MB_OK | MB_ICONWARNING);
        return false;
    }
    if (!CanAssignHotkey(CaptureTarget::Cure, s.cureHotkey, s)) {
        if (showError) MessageBoxW(g_hwnd,
            L"Cure tu\u015Fu giri\u015F dizisi, R Combo veya A\u00E7ma/Kapatma tu\u015Flar\u0131yla \u00E7ak\u0131\u015F\u0131yor.",
            L"Cure Tu\u015Fu", MB_OK | MB_ICONWARNING);
        return false;
    }

    {
        std::lock_guard<std::mutex> lk(g_settingsMutex);
        g_settings = s;
    }
    SetWindowTextW(hRMax, std::to_wstring(s.rRateMaximum).c_str());
    SetWindowTextW(hRTurbo, std::to_wstring(s.rRateTurbo).c_str());
    UpdateDiagnosticMarkers();
    return true;
}

void SyncControls() {
    Settings s;
    {
        std::lock_guard<std::mutex> lk(g_settingsMutex);
        s = g_settings;
    }
    SetWindowTextW(hSeq1, VkName(s.comboVk[0]).c_str());
    SetWindowTextW(hSeq2, VkName(s.comboVk[1]).c_str());
    SetWindowTextW(hSeq3, VkName(s.comboVk[2]).c_str());
    Button_SetCheck(hRCheck, s.rEnabled ? BST_CHECKED : BST_UNCHECKED);
    SetWindowTextW(hRMax, std::to_wstring(s.rRateMaximum).c_str());
    SetWindowTextW(hRTurbo, std::to_wstring(s.rRateTurbo).c_str());
    Button_SetCheck(hCureCheck, s.cureEnabled ? BST_CHECKED : BST_UNCHECKED);
    ComboBox_SetCurSel(hCureBar, s.cureBar - 1);
    ComboBox_SetCurSel(hCureSlot, s.cureSlot - 1);
    SetWindowTextW(hCureKey, VkName(s.cureHotkey).c_str());
    SetWindowTextW(hStartKey, VkName(s.startHotkey).c_str());
    SetWindowTextW(hStopKey, VkName(s.stopHotkey).c_str());
}

bool MakeChildControls(HWND w) {
    const HINSTANCE inst = GetModuleHandleW(nullptr);
    auto mkEdit = [&](int id, int x, int y, int ww, int hh, DWORD extra = 0) {
        HWND h = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_CENTER | extra,
                                 x, y, ww, hh, w, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), inst, nullptr);
        if (h) SendMessageW(h, WM_SETFONT, reinterpret_cast<WPARAM>(g_font), TRUE);
        return h;
    };
    auto mkCheck = [&](int id, const wchar_t* t, int x, int y, int ww, int hh) {
        HWND h = CreateWindowExW(0, L"BUTTON", t, WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
                                 x, y, ww, hh, w, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), inst, nullptr);
        if (h) SendMessageW(h, WM_SETFONT, reinterpret_cast<WPARAM>(g_font), TRUE);
        return h;
    };
    auto mkButton = [&](int id, const wchar_t* t, int x, int y, int ww, int hh) {
        HWND h = CreateWindowExW(0, L"BUTTON", t, WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
                                 x, y, ww, hh, w, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), inst, nullptr);
        if (h) SendMessageW(h, WM_SETFONT, reinterpret_cast<WPARAM>(g_fontBold), TRUE);
        return h;
    };

    hStartKey = mkButton(1301, L"TAB", 286, 146, 104, 25);
    hStopKey = mkButton(1302, L"CAPS LOCK", 500, 146, 104, 25);

    hSeq1 = mkEdit(1001, 287, 267, 56, 28);
    hSeq2 = mkEdit(1002, 372, 267, 56, 28);
    hSeq3 = mkEdit(1003, 457, 267, 56, 28);

    hRCheck = mkCheck(1101, L"R Combo", 205, 337, 105, 26);
    hRMax = mkEdit(1102, 423, 335, 54, 28, ES_NUMBER);
    hRTurbo = mkEdit(1103, 548, 335, 54, 28, ES_NUMBER);

    hCureCheck = mkCheck(1201, L"Cure Al", 205, 398, 100, 26);
    hCureBar = CreateWindowExW(0, L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST | WS_VSCROLL,
                               337, 397, 68, 180, w, reinterpret_cast<HMENU>(static_cast<INT_PTR>(1202)), inst, nullptr);
    hCureSlot = CreateWindowExW(0, L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST | WS_VSCROLL,
                                432, 397, 62, 180, w, reinterpret_cast<HMENU>(static_cast<INT_PTR>(1203)), inst, nullptr);
    hCureKey = mkButton(1204, L"C", 524, 396, 78, 29);

    hUnicodeSentinel = CreateWindowExW(0, L"STATIC",
        L"Ba\u015Flat|Giri\u015F Dizisi|Ayarlar\u0131m\u0131 Kaydet|\u00C7al\u0131\u015F\u0131yor",
        WS_CHILD, 0, 0, 1, 1, w, reinterpret_cast<HMENU>(static_cast<INT_PTR>(1901)), inst, nullptr);

    const std::array<HWND, 13> controls{{hSeq1, hSeq2, hSeq3, hRCheck, hRMax, hRTurbo,
                                         hCureCheck, hCureBar, hCureSlot, hCureKey, hStartKey, hStopKey,
                                         hUnicodeSentinel}};
    for (HWND h : controls) {
        if (!h || !IsWindow(h)) return false;
    }

    SendMessageW(hSeq1, EM_SETLIMITTEXT, 5, 0);
    SendMessageW(hSeq2, EM_SETLIMITTEXT, 5, 0);
    SendMessageW(hSeq3, EM_SETLIMITTEXT, 5, 0);
    SendMessageW(hRMax, EM_SETLIMITTEXT, 3, 0);
    SendMessageW(hRTurbo, EM_SETLIMITTEXT, 3, 0);
    SendMessageW(hCureBar, WM_SETFONT, reinterpret_cast<WPARAM>(g_font), TRUE);
    SendMessageW(hCureSlot, WM_SETFONT, reinterpret_cast<WPARAM>(g_font), TRUE);

    for (int i = 1; i <= 8; ++i) {
        const std::wstring f = L"F" + std::to_wstring(i);
        ComboBox_AddString(hCureBar, f.c_str());
        const std::wstring n = std::to_wstring(i);
        ComboBox_AddString(hCureSlot, n.c_str());
    }

    SyncControls();
    return true;
}

void DrawUi(HWND, HDC hdc) {
    Graphics g(hdc);
    g.SetSmoothingMode(SmoothingModeAntiAlias);
    g.SetTextRenderingHint(TextRenderingHintClearTypeGridFit);
    g.SetCompositingQuality(CompositingQualityHighQuality);

    LinearGradientBrush bg(Rect(0, 0, W, H), Color(255, 252, 247, 235), Color(255, 232, 207, 166), LinearGradientModeVertical);
    g.FillRectangle(&bg, 0, 0, W, H);

    Pen outer(Color(255, 142, 89, 22), 2.0f);
    g.DrawRectangle(&outer, 2, 2, W - 5, H - 5);
    Pen inner(Color(150, 212, 172, 96), 1.0f);
    g.DrawRectangle(&inner, 8, 8, W - 17, H - 17);

    DrawPortraitCard(g, g_karus.get(), Rect(20, 54, 128, 88));
    DrawPortraitCard(g, g_human.get(), Rect(652, 54, 128, 88));

    DrawTextG(g, L"Premium Plus Combo", 205, 15, 390, 35, 24, true, Color(255, 112, 61, 16), StringAlignmentCenter);
    Panel(g, RectF(337, 54, 126, 27), Color(255, 125, 25, 24), Color(255, 218, 161, 55), 8.0f);
    DrawTextG(g, L"Rogue", 337, 54, 126, 27, 13, true, Color(255, 255, 239, 192), StringAlignmentCenter);
    DrawTextG(g, L"\u2014", 735, 10, 25, 28, 20, true, Color(255, 74, 43, 17), StringAlignmentCenter);
    DrawTextG(g, L"\u00D7", 765, 10, 27, 28, 22, true, Color(255, 74, 43, 17), StringAlignmentCenter);

    Settings s;
    {
        std::lock_guard<std::mutex> lk(g_settingsMutex);
        s = g_settings;
    }

    Button(g, rcStart, L"Ba\u015Flat (" + VkName(s.startHotkey) + L")",
           Color(255, 128, 78, 20), Color(255, 79, 41, 12), g_active.load(std::memory_order_relaxed));
    Button(g, rcStop, L"Durdur (" + VkName(s.stopHotkey) + L")",
           Color(255, 151, 28, 26), Color(255, 91, 13, 18), !g_active.load(std::memory_order_relaxed));

    DrawTextG(g, L"A\u00E7ma Tu\u015Fu", 212, 147, 72, 24, 10, true, Color(255, 76, 46, 25), StringAlignmentCenter);
    DrawTextG(g, L"Kapatma Tu\u015Fu", 410, 147, 88, 24, 10, true, Color(255, 76, 46, 25), StringAlignmentCenter);

    DrawTextG(g, L"Mod Se\u00E7imi", 200, 174, 400, 20, 13, true, Color(255, 77, 44, 18), StringAlignmentCenter);
    DrawModeCard(g, rcMax, false, g_mode.load(std::memory_order_relaxed) == Mode::Maximum);
    DrawModeCard(g, rcTurbo, true, g_mode.load(std::memory_order_relaxed) == Mode::Turbo);

    Panel(g, RectF(185, 245, 430, 62), Color(235, 250, 242, 224), Color(255, 184, 136, 55), 8.0f);
    DrawTextG(g, L"Giri\u015F Dizisi", 198, 246, 120, 20, 13, true);
    DrawTextG(g, L">", 345, 267, 22, 28, 17, true, Color(255, 116, 75, 24), StringAlignmentCenter);
    DrawTextG(g, L">", 430, 267, 22, 28, 17, true, Color(255, 116, 75, 24), StringAlignmentCenter);
    DrawTextG(g, L"Her tu\u015F kendi hedef h\u0131z\u0131nda tekrarlan\u0131r.", 520, 258, 83, 38, 9, false,
              Color(255, 83, 61, 38), StringAlignmentCenter);

    Panel(g, RectF(185, 315, 430, 50), Color(235, 250, 242, 224), Color(255, 181, 132, 51), 8.0f);
    DrawTextG(g, L"Rogue Ayarlar\u0131", 198, 316, 130, 20, 13, true);
    DrawTextG(g, L"Maximum R/sn", 405, 316, 90, 18, 9, false, Color(255, 82, 53, 31), StringAlignmentCenter);
    DrawTextG(g, L"Turbo R/sn", 535, 316, 80, 18, 9, false, Color(255, 82, 53, 31), StringAlignmentCenter);

    Panel(g, RectF(185, 372, 430, 56), Color(235, 250, 242, 224), Color(255, 181, 132, 51), 8.0f);
    DrawTextG(g, L"Cure Ayarlar\u0131", 198, 373, 130, 19, 13, true);
    DrawTextG(g, L"Bar", 337, 378, 68, 16, 9, false, Color(255, 82, 53, 31), StringAlignmentCenter);
    DrawTextG(g, L"Slot", 432, 378, 62, 16, 9, false, Color(255, 82, 53, 31), StringAlignmentCenter);
    DrawTextG(g, L"Cure Tu\u015Fu", 515, 378, 94, 16, 9, false, Color(255, 82, 53, 31), StringAlignmentCenter);

    Button(g, rcSave, L"Ayarlar\u0131m\u0131 Kaydet", Color(255, 136, 28, 21), Color(255, 82, 12, 15), false);

    Panel(g, RectF(12, 474, 776, 22), Color(245, 249, 238, 221), Color(210, 165, 115, 42), 6.0f);
    const std::wstring status = g_active.load(std::memory_order_relaxed)
        ? L"Durum: \u00C7al\u0131\u015F\u0131yor" : L"Durum: Haz\u0131r";
    DrawTextG(g, status, 22, 474, 165, 22, 10, true,
              g_active.load(std::memory_order_relaxed) ? Color(255, 27, 112, 40) : Color(255, 88, 68, 41));
    DrawTextG(g, g_mode.load(std::memory_order_relaxed) == Mode::Turbo ? L"Mod: Turbo" : L"Mod: Maximum",
              202, 474, 130, 22, 10, true, Color(255, 91, 45, 17), StringAlignmentCenter);
    const std::wstring combo = L"Combo: " + VkName(s.comboVk[0]) + L" > " + VkName(s.comboVk[1]) + L" > " + VkName(s.comboVk[2]);
    DrawTextG(g, combo, 342, 474, 180, 22, 10, true, Color(255, 84, 55, 28), StringAlignmentCenter);
    DrawTextG(g, L"R: " + std::to_wstring(ppc::rRate(s, g_mode.load(std::memory_order_relaxed))) + L"/sn",
              532, 474, 90, 22, 10, true, Color(255, 84, 55, 28), StringAlignmentCenter);
    DrawTextG(g, L"Cure: " + VkName(s.cureHotkey), 632, 474, 140, 22, 10, true,
              Color(255, 84, 55, 28), StringAlignmentCenter);
}
