
void SendCombo(const std::array<int,3>& keys) {
    INPUT in[6]{};
    for (int i=0;i<3;i++) {
        in[i].type = INPUT_KEYBOARD; in[i].ki.wVk = WORD(keys[i]); in[i].ki.dwExtraInfo = EXTRA_TAG;
        in[i+3].type = INPUT_KEYBOARD; in[i+3].ki.wVk = WORD(keys[i]); in[i+3].ki.dwFlags = KEYEVENTF_KEYUP; in[i+3].ki.dwExtraInfo = EXTRA_TAG;
    }
    SendInput(6, in, sizeof(INPUT));
}

void PreciseWaitUntil(long long target) {
    const long long freq = QpcFreq();
    while (!g_quit.load(std::memory_order_relaxed) && g_active.load(std::memory_order_relaxed)) {
        long long now = QpcNow();
        long long rem = target - now;
        if (rem <= 0) break;
        double ms = (double)rem * 1000.0 / (double)freq;
        if (ms > 2.0) Sleep(1);
        else if (ms > 0.35) SwitchToThread();
        else YieldProcessor();
    }
}

void ComboWorker() {
    const long long freq = QpcFreq();
    long long next = QpcNow();
    while (!g_quit.load()) {
        if (!g_active.load()) { WaitForSingleObject(g_wakeEvent, 50); next = QpcNow(); continue; }
        Settings s; { std::lock_guard<std::mutex> lk(g_settingsMutex); s = g_settings; }
        int hz = ppc::minorRate(g_mode.load());
        const long long step = std::max<long long>(1, freq / hz);
        if (!g_active.load()) continue;
        SendCombo(s.comboVk);
        next += step;
        long long now = QpcNow();
        if (now - next > step*2) next = now + step;
        PreciseWaitUntil(next);
    }
}

void RWorker() {
    const long long freq = QpcFreq();
    long long next = QpcNow();
    while (!g_quit.load()) {
        if (!g_active.load()) { Sleep(10); next = QpcNow(); continue; }
        Settings s; { std::lock_guard<std::mutex> lk(g_settingsMutex); s = g_settings; }
        if (!s.rEnabled || QpcNow() < g_rSuppressUntil.load()) { Sleep(1); next = QpcNow(); continue; }
        int hz = ppc::rRate(s, g_mode.load());
        const long long step = std::max<long long>(1, freq / hz);
        if (!g_active.load()) continue;
        SendKey('R');
        next += step;
        long long now = QpcNow();
        if (now - next > step*2) next = now + step;
        PreciseWaitUntil(next);
    }
}

void CureWorker() {
    if (g_cureBusy.exchange(true)) return;
    Settings s; { std::lock_guard<std::mutex> lk(g_settingsMutex); s = g_settings; }
    if (!g_active.load() || !s.cureEnabled) { g_cureBusy.store(false); return; }

    const long long freq = QpcFreq();
    if (s.rEnabled) g_rSuppressUntil.store(QpcNow() + freq*2);

    int previous = g_currentBar.load();
    SendKey(VK_F1 + (s.cureBar-1));
    g_currentBar.store(s.cureBar);
    Sleep(28);
    if (g_active.load()) SendKey('0' + s.cureSlot);
    Sleep(45);
    if (g_active.load() && previous >= 1 && previous <= 8 && previous != s.cureBar) {
        SendKey(VK_F1 + (previous-1));
        g_currentBar.store(previous);
    }
    g_cureBusy.store(false);
}

void UpdateStateTitle() {
    if (!g_hwnd) return;
    SetWindowTextW(g_hwnd, g_active.load() ? L"Premium Plus Combo|ACTIVE" : L"Premium Plus Combo|READY");
}

void StartMacro() {
    g_active.store(true);
    SetEvent(g_wakeEvent);
    UpdateStateTitle();
    if (g_hwnd) InvalidateRect(g_hwnd,nullptr,FALSE);
}

void StopMacro() {
    g_active.store(false);
    ResetEvent(g_wakeEvent);
    g_rSuppressUntil.store(0);
    UpdateStateTitle();
    if (g_hwnd) InvalidateRect(g_hwnd,nullptr,FALSE);
}

LRESULT CALLBACK KeyboardProc(int code, WPARAM wp, LPARAM lp) {
    if (code == HC_ACTION) {
        auto* k = reinterpret_cast<KBDLLHOOKSTRUCT*>(lp);
        bool down = (wp == WM_KEYDOWN || wp == WM_SYSKEYDOWN);
        if (!(k->flags & LLKHF_INJECTED) && down) {
            int vk = int(k->vkCode);
            if (g_captureCure.exchange(false)) {
                if (vk == VK_TAB || vk == VK_CAPITAL) {
                    Settings s; { std::lock_guard<std::mutex> lk(g_settingsMutex); s = g_settings; }
                    PostMessageW(g_hwnd, WM_APP_CUREKEY, WPARAM(s.cureHotkey), 1);
                    return 1;
                }
                { std::lock_guard<std::mutex> lk(g_settingsMutex); g_settings.cureHotkey = vk; }
                PostMessageW(g_hwnd, WM_APP_CUREKEY, WPARAM(vk), 0);
                return 1;
            }
            if (vk >= VK_F1 && vk <= VK_F8) g_currentBar.store(vk - VK_F1 + 1);
            Settings s; { std::lock_guard<std::mutex> lk(g_settingsMutex); s = g_settings; }
            if (g_active.load() && s.cureEnabled && vk == s.cureHotkey) {
                std::thread(CureWorker).detach();
                return 1;
            }
        }
    }
    return CallNextHookEx(g_hook, code, wp, lp);
}

std::unique_ptr<Bitmap> LoadImageResource(int id) {
    HRSRC r = FindResourceW(nullptr, MAKEINTRESOURCEW(id), RT_RCDATA); if (!r) return {};
    DWORD sz = SizeofResource(nullptr, r); HGLOBAL hg = LoadResource(nullptr, r); if (!hg || !sz) return {};
    void* src = LockResource(hg); if (!src) return {};
    HGLOBAL mem = GlobalAlloc(GMEM_MOVEABLE, sz); if (!mem) return {};
    void* dst = GlobalLock(mem); if (!dst) { GlobalFree(mem); return {}; }
    memcpy(dst, src, sz); GlobalUnlock(mem);
    IStream* stream{}; if (FAILED(CreateStreamOnHGlobal(mem, TRUE, &stream))) { GlobalFree(mem); return {}; }

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

void DrawTextG(Graphics& g, const std::wstring& text, float x, float y, float w, float h, float size, bool bold=false, Color c=Color(255,65,42,25), StringAlignment align=StringAlignmentNear) {
    FontFamily fam(L"Segoe UI"); Font f(&fam,size,bold?FontStyleBold:FontStyleRegular,UnitPixel);
    SolidBrush b(c); RectF r(x,y,w,h); StringFormat fmt; fmt.SetAlignment(align); fmt.SetLineAlignment(StringAlignmentCenter);
    g.DrawString(text.c_str(), -1, &f, r, &fmt, &b);
}
