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
    Pen pen(border, 1.4f);
    g.FillPath(&b, &p);
    g.DrawPath(&pen, &p);
}

void Button(Graphics& g, const RECT& rr, const std::wstring& text, Color c1, Color c2, bool selected = false) {
    RectF r(static_cast<REAL>(rr.left), static_cast<REAL>(rr.top),
            static_cast<REAL>(rr.right - rr.left), static_cast<REAL>(rr.bottom - rr.top));
    GraphicsPath p;
    const float d = 18.0f;
    p.AddArc(r.X, r.Y, d, d, 180, 90);
    p.AddArc(r.GetRight() - d, r.Y, d, d, 270, 90);
    p.AddArc(r.GetRight() - d, r.GetBottom() - d, d, d, 0, 90);
    p.AddArc(r.X, r.GetBottom() - d, d, d, 90, 90);
    p.CloseFigure();

    LinearGradientBrush br(r, c1, c2, LinearGradientModeVertical);
    g.FillPath(&br, &p);
    Pen pen(selected ? Color(255, 255, 205, 80) : Color(255, 158, 106, 30), selected ? 2.5f : 1.4f);
    g.DrawPath(&pen, &p);
    DrawTextG(g, text, r.X + 5, r.Y, r.Width - 10, r.Height, 15, true, Color(255, 255, 248, 229), StringAlignmentCenter);
}

void DrawModeCard(Graphics& g, const RECT& rr, bool turbo, bool selected) {
    RectF r(static_cast<REAL>(rr.left), static_cast<REAL>(rr.top),
            static_cast<REAL>(rr.right - rr.left), static_cast<REAL>(rr.bottom - rr.top));
    const Color fill = selected ? (turbo ? Color(250, 104, 18, 17) : Color(248, 111, 67, 19)) : Color(242, 248, 236, 216);
    const Color txt = selected ? Color(255, 255, 235, 164) : Color(255, 72, 43, 20);
    Panel(g, r, fill, selected ? Color(255, 246, 184, 60) : Color(255, 175, 129, 47), 10.0f);
    DrawTextG(g, turbo ? L"Turbo Mod" : L"Maximum Mod", r.X + 12, r.Y + 5, r.Width - 42, 26, 16, true, txt);
    DrawTextG(g, turbo ? L"Minor: ~240 basım/sn (her tuş)" : L"Minor: ~120 basım/sn (her tuş)",
              r.X + 10, r.Y + 34, r.Width - 20, 26, 11, false, txt, StringAlignmentCenter);
    DrawTextG(g, L"i", r.GetRight() - 29, r.Y + 7, 20, 20, 13, true,
              selected ? Color(255, 255, 205, 77) : Color(255, 112, 70, 24), StringAlignmentCenter);
}

void DrawPortraitBackground(Graphics& g, Bitmap* image, const Rect& dest) {
    if (!image || image->GetLastStatus() != Ok || image->GetWidth() == 0 || image->GetHeight() == 0) return;

    const REAL dw = static_cast<REAL>(dest.Width);
    const REAL dh = static_cast<REAL>(dest.Height);
    const REAL targetRatio = dw / dh;
    const REAL sw = static_cast<REAL>(image->GetWidth());
    const REAL sh = static_cast<REAL>(image->GetHeight());
    const REAL sourceRatio = sw / sh;

    INT sx = 0;
    INT sy = 0;
    INT cw = static_cast<INT>(sw);
    INT ch = static_cast<INT>(sh);
    if (sourceRatio > targetRatio) {
        cw = static_cast<INT>(sh * targetRatio);
        sx = (static_cast<INT>(sw) - cw) / 2;
    } else {
        ch = static_cast<INT>(sw / targetRatio);
        sy = (static_cast<INT>(sh) - ch) / 2;
    }

    ColorMatrix matrix = {
        1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.88f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f, 1.0f
    };
    ImageAttributes attrs;
    attrs.SetColorMatrix(&matrix, ColorMatrixFlagsDefault, ColorAdjustTypeBitmap);

    g.SetInterpolationMode(InterpolationModeHighQualityBicubic);
    g.DrawImage(image, dest, sx, sy, cw, ch, UnitPixel, &attrs);
    LinearGradientBrush veil(dest, Color(15, 248, 236, 212), Color(95, 248, 236, 212), LinearGradientModeVertical);
    g.FillRectangle(&veil, dest);
}

void SaveRegistry() {
    Settings s;
    {
        std::lock_guard<std::mutex> lk(g_settingsMutex);
        s = g_settings;
    }
    HKEY key{};
    if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\PremiumPlusCombo", 0, nullptr, 0, KEY_WRITE, nullptr, &key, nullptr) != ERROR_SUCCESS) return;
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
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\PremiumPlusCombo", 0, KEY_READ, &key) == ERROR_SUCCESS) {
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
        s.startHotkey = static_cast<int>(rd(L"StartHotkey", VK_CAPITAL));
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
        if (showError) MessageBoxW(g_hwnd, L"Giriş dizisinde geçersiz bir tuş var. 0-9, A-Z veya F1-F12 kullanabilirsiniz.", L"Giriş Dizisi", MB_OK | MB_ICONWARNING);
        return false;
    }
    if (k1 == k2 || k1 == k3 || k2 == k3) {
        if (showError) MessageBoxW(g_hwnd, L"Giriş dizisindeki üç tuş birbirinden farklı olmalıdır.", L"Giriş Dizisi", MB_OK | MB_ICONWARNING);
        return false;
    }
    if (k1 == 'R' || k2 == 'R' || k3 == 'R') {
        if (showError) MessageBoxW(g_hwnd, L"R tuşu ayrı R Combo motoru tarafından kullanılır. Giriş dizisi için farklı bir tuş seçin.", L"Giriş Dizisi", MB_OK | MB_ICONWARNING);
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
        if (showError) MessageBoxW(g_hwnd, L"Açma/Kapatma tuşları giriş dizisi, R Combo veya Cure tuşuyla çakışamaz. Açma ve Kapatma tuşlarının birbiriyle aynı olması ise serbesttir.", L"Kontrol Tuşları", MB_OK | MB_ICONWARNING);
        return false;
    }
    if (!CanAssignHotkey(CaptureTarget::Cure, s.cureHotkey, s)) {
        if (showError) MessageBoxW(g_hwnd, L"Cure tuşu giriş dizisi, R Combo veya Açma/Kapatma tuşlarıyla çakışıyor. Cure için farklı bir tuş seçin.", L"Cure Tuşu", MB_OK | MB_ICONWARNING);
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

    hStartKey = mkButton(1301, L"CAPS LOCK", 365, 126, 110, 28);
    hStopKey = mkButton(1302, L"CAPS LOCK", 575, 126, 110, 28);

    hSeq1 = mkEdit(1001, 330, 293, 60, 30);
    hSeq2 = mkEdit(1002, 430, 293, 60, 30);
    hSeq3 = mkEdit(1003, 530, 293, 60, 30);
    hRCheck = mkCheck(1101, L"R Combo", 300, 379, 105, 28);
    hRMax = mkEdit(1102, 475, 377, 58, 30, ES_NUMBER);
    hRTurbo = mkEdit(1103, 615, 377, 58, 30, ES_NUMBER);
    hCureCheck = mkCheck(1201, L"Cure Al", 300, 458, 110, 28);
    hCureBar = CreateWindowExW(0, L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST | WS_VSCROLL,
                               420, 456, 76, 190, w, reinterpret_cast<HMENU>(static_cast<INT_PTR>(1202)), inst, nullptr);
    hCureSlot = CreateWindowExW(0, L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST | WS_VSCROLL,
                                520, 456, 66, 190, w, reinterpret_cast<HMENU>(static_cast<INT_PTR>(1203)), inst, nullptr);
    hCureKey = mkButton(1204, L"C", 610, 455, 78, 31);

    const std::array<HWND, 12> controls{{hSeq1, hSeq2, hSeq3, hRCheck, hRMax, hRTurbo,
                                         hCureCheck, hCureBar, hCureSlot, hCureKey, hStartKey, hStopKey}};
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

std::wstring ElapsedText() {
    if (!g_active.load(std::memory_order_relaxed)) return L"00:00";
    const long long started = g_startedTicks.load(std::memory_order_relaxed);
    if (started <= 0) return L"00:00";
    const long long seconds = std::max<long long>(0, (QpcNow() - started) / QpcFreq());
    const long long minutes = seconds / 60;
    const long long rem = seconds % 60;
    std::wostringstream ss;
    ss << std::setfill(L'0') << std::setw(2) << minutes << L":" << std::setw(2) << rem;
    return ss.str();
}

void DrawUi(HWND, HDC hdc) {
    Graphics g(hdc);
    g.SetSmoothingMode(SmoothingModeAntiAlias);
    g.SetTextRenderingHint(TextRenderingHintClearTypeGridFit);
    g.SetCompositingQuality(CompositingQualityHighQuality);

    LinearGradientBrush bg(Rect(0, 0, W, H), Color(255, 252, 246, 232), Color(255, 224, 196, 151), LinearGradientModeVertical);
    g.FillRectangle(&bg, 0, 0, W, H);

    // The supplied Rogue reference visuals are used as the actual side backgrounds.
    // No separate KARUS/HUMAN labels or placeholder boxes are drawn.
    DrawPortraitBackground(g, g_karus.get(), Rect(0, 55, 255, 515));
    DrawPortraitBackground(g, g_human.get(), Rect(725, 55, 255, 515));

    Panel(g, RectF(255, 55, 470, 525), Color(238, 250, 241, 222), Color(255, 153, 98, 24), 12.0f);
    Pen outer(Color(255, 137, 85, 21), 2.0f);
    g.DrawRectangle(&outer, 2, 2, W - 5, H - 5);
    Pen inner(Color(150, 212, 172, 96), 1.0f);
    g.DrawRectangle(&inner, 8, 8, W - 17, H - 17);

    DrawTextG(g, L"Premium Plus Combo", 290, 13, 400, 38, 26, true, Color(255, 116, 64, 16), StringAlignmentCenter);
    DrawTextG(g, L"Rogue", 28, 18, 120, 28, 18, true, Color(255, 55, 34, 20));
    DrawTextG(g, L"—", 910, 12, 30, 30, 22, true, Color(255, 74, 43, 17), StringAlignmentCenter);
    DrawTextG(g, L"×", 945, 12, 30, 30, 23, true, Color(255, 74, 43, 17), StringAlignmentCenter);

    Settings s;
    {
        std::lock_guard<std::mutex> lk(g_settingsMutex);
        s = g_settings;
    }

    Button(g, rcStart, L"Başlat (" + VkName(s.startHotkey) + L")", Color(255, 128, 78, 20), Color(255, 79, 41, 12), g_active.load(std::memory_order_relaxed));
    Button(g, rcStop, L"Durdur (" + VkName(s.stopHotkey) + L")", Color(255, 151, 28, 26), Color(255, 91, 13, 18), !g_active.load(std::memory_order_relaxed));

    DrawTextG(g, L"Açma Tuşu", 285, 126, 76, 28, 11, true, Color(255, 76, 46, 25), StringAlignmentCenter);
    DrawTextG(g, L"Kapatma Tuşu", 488, 126, 83, 28, 11, true, Color(255, 76, 46, 25), StringAlignmentCenter);

    DrawTextG(g, L"Mod Seçimi", 285, 157, 415, 22, 14, true, Color(255, 77, 44, 18), StringAlignmentCenter);
    DrawModeCard(g, rcMax, false, g_mode.load(std::memory_order_relaxed) == Mode::Maximum);
    DrawModeCard(g, rcTurbo, true, g_mode.load(std::memory_order_relaxed) == Mode::Turbo);

    Panel(g, RectF(275, 262, 435, 75), Color(235, 250, 242, 224), Color(255, 184, 136, 55), 8.0f);
    DrawTextG(g, L"Giriş Dizisi", 290, 263, 130, 24, 14, true);
    DrawTextG(g, L">", 398, 293, 22, 30, 18, true, Color(255, 116, 75, 24), StringAlignmentCenter);
    DrawTextG(g, L">", 498, 293, 22, 30, 18, true, Color(255, 116, 75, 24), StringAlignmentCenter);
    DrawTextG(g, L"Her tuş kendi hedef hızında tekrarlanır.", 600, 290, 98, 38, 10, false, Color(255, 83, 61, 38), StringAlignmentCenter);

    Panel(g, RectF(275, 345, 435, 69), Color(235, 250, 242, 224), Color(255, 181, 132, 51), 8.0f);
    DrawTextG(g, L"Rogue Ayarları", 290, 346, 140, 23, 14, true);
    DrawTextG(g, L"Maximum R/sn", 452, 351, 105, 20, 11, false, Color(255, 82, 53, 31), StringAlignmentCenter);
    DrawTextG(g, L"Turbo R/sn", 592, 351, 105, 20, 11, false, Color(255, 82, 53, 31), StringAlignmentCenter);

    Panel(g, RectF(275, 423, 435, 102), Color(235, 250, 242, 224), Color(255, 181, 132, 51), 8.0f);
    DrawTextG(g, L"Cure Ayarları", 290, 424, 140, 23, 14, true);
    DrawTextG(g, L"Bar", 418, 438, 78, 18, 11, false, Color(255, 82, 53, 31), StringAlignmentCenter);
    DrawTextG(g, L"Slot", 518, 438, 68, 18, 11, false, Color(255, 82, 53, 31), StringAlignmentCenter);
    DrawTextG(g, L"Cure Tuşu", 602, 438, 94, 18, 11, false, Color(255, 82, 53, 31), StringAlignmentCenter);
    DrawTextG(g, L"Cure sırasında R Combo 2 saniye durur. R kapalıysa program R'yi açmaz.",
              295, 490, 405, 26, 10, false, Color(255, 91, 55, 28), StringAlignmentCenter);

    Button(g, rcSave, L"Ayarlarımı Kaydet", Color(255, 136, 28, 21), Color(255, 82, 12, 15), false);

    Panel(g, RectF(10, 585, 960, 27), Color(245, 249, 238, 221), Color(210, 165, 115, 42), 7.0f);
    const std::wstring status = g_active.load(std::memory_order_relaxed) ? L"Durum: Çalışıyor" : L"Durum: Hazır";
    DrawTextG(g, status, 22, 586, 170, 25, 12, true,
              g_active.load(std::memory_order_relaxed) ? Color(255, 27, 112, 40) : Color(255, 88, 68, 41));
    DrawTextG(g, g_mode.load(std::memory_order_relaxed) == Mode::Turbo ? L"Mod: Turbo" : L"Mod: Maximum",
              215, 586, 160, 25, 11, true, Color(255, 91, 45, 17), StringAlignmentCenter);

    const std::wstring combo = L"Combo: " + VkName(s.comboVk[0]) + L" > " + VkName(s.comboVk[1]) + L" > " + VkName(s.comboVk[2]);
    DrawTextG(g, combo, 390, 586, 195, 25, 11, true, Color(255, 84, 55, 28), StringAlignmentCenter);
    DrawTextG(g, L"Cure: F" + std::to_wstring(s.cureBar) + L" / " + std::to_wstring(s.cureSlot) + L" / " + VkName(s.cureHotkey),
              600, 586, 190, 25, 11, true, Color(255, 84, 55, 28), StringAlignmentCenter);
    DrawTextG(g, L"Süre: " + ElapsedText(), 805, 586, 145, 25, 11, true, Color(255, 84, 55, 28), StringAlignmentCenter);
}
