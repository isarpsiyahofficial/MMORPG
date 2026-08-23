void SendCombo(const std::array<int, 3>& keys) {
    INPUT in[6]{};
    for (int i = 0; i < 3; ++i) {
        const int j = i * 2;
        in[j].type = INPUT_KEYBOARD;
        in[j].ki.wVk = WORD(keys[i]);
        in[j].ki.dwExtraInfo = EXTRA_TAG;
        in[j + 1].type = INPUT_KEYBOARD;
        in[j + 1].ki.wVk = WORD(keys[i]);
        in[j + 1].ki.dwFlags = KEYEVENTF_KEYUP;
        in[j + 1].ki.dwExtraInfo = EXTRA_TAG;
    }
    if (SendInput(6, in, sizeof(INPUT)) == 6) {
        g_comboBursts.fetch_add(1, std::memory_order_relaxed);
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
        const int hz = ppc::minorRate(g_mode.load(std::memory_order_relaxed));
        const long long step = std::max<long long>(1, freq / hz);
        if (!g_active.load(std::memory_order_relaxed)) continue;

        SendCombo(s.comboVk);
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
        SetEvent(g_wakeEvent);
    }
    UpdateStateTitle();
    if (g_hwnd) InvalidateRect(g_hwnd, nullptr, FALSE);
}

void StopMacro() {
    g_active.store(false, std::memory_order_release);
    ResetEvent(g_wakeEvent);
    g_rSuppressUntil.store(0, std::memory_order_relaxed);
    UpdateStateTitle();
    if (g_hwnd) InvalidateRect(g_hwnd, nullptr, FALSE);
}

bool IsKeyDown(int vk) {
    return (GetAsyncKeyState(vk) & 0x8000) != 0;
}

bool IsReservedCureKey(int vk, const Settings& s) {
    if (vk == VK_TAB || vk == VK_CAPITAL || vk == 'R') return true;
    return vk == s.comboVk[0] || vk == s.comboVk[1] || vk == s.comboVk[2];
}

void InputWorker() {
    std::array<bool, 256> previous{};
    for (int vk = 0; vk < 256; ++vk) previous[vk] = IsKeyDown(vk);

    ppc::StartStopLatch control;
    control.prime(previous[VK_TAB], previous[VK_CAPITAL]);

    while (!g_quit.load(std::memory_order_relaxed)) {
        const bool tabDown = IsKeyDown(VK_TAB);
        const bool capsDown = IsKeyDown(VK_CAPITAL);
        const int action = control.update(tabDown, capsDown);
        if (action > 0) StartMacro();
        else if (action < 0) StopMacro();
        previous[VK_TAB] = tabDown;
        previous[VK_CAPITAL] = capsDown;

        Settings s;
        {
            std::lock_guard<std::mutex> lk(g_settingsMutex);
            s = g_settings;
        }

        bool capture = g_captureCure.load(std::memory_order_relaxed);
        bool capturedThisPass = false;
        for (int vk = VK_BACK; vk <= 0xFE; ++vk) {
            if (vk == VK_TAB || vk == VK_CAPITAL) continue;
            const bool down = IsKeyDown(vk);
            const bool rising = down && !previous[vk];
            previous[vk] = down;
            if (!rising) continue;

            if (vk >= VK_F1 && vk <= VK_F8) g_currentBar.store(vk - VK_F1 + 1, std::memory_order_relaxed);

            if (capture && !capturedThisPass) {
                if (IsReservedCureKey(vk, s)) {
                    PostMessageW(g_hwnd, WM_APP_CUREKEY, WPARAM(s.cureHotkey), 2);
                } else if (g_captureCure.exchange(false, std::memory_order_acq_rel)) {
                    {
                        std::lock_guard<std::mutex> lk(g_settingsMutex);
                        g_settings.cureHotkey = vk;
                    }
                    PostMessageW(g_hwnd, WM_APP_CUREKEY, WPARAM(vk), 0);
                }
                capturedThisPass = true;
                capture = false;
                continue;
            }

            if (g_active.load(std::memory_order_relaxed) && s.cureEnabled && vk == s.cureHotkey) {
                std::thread(CureWorker).detach();
            }
        }
        Sleep(1);
    }
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
