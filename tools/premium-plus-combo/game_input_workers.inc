// Ordered Rogue scheduler. The legacy batch worker is kept compiled under a
// private name for easy comparison, but production uses these workers.

void UpdateDiagnosticMarkers() {
    PpcLegacyUpdateDiagnosticMarkers();
    Settings s;
    {
        std::lock_guard<std::mutex> lk(g_settingsMutex);
        s = g_settings;
    }
    SetIntegerProp(L"PPC_COMBO_KEY1_TAPS", GameTapCount(s.comboVk[0]));
    SetIntegerProp(L"PPC_COMBO_KEY2_TAPS", GameTapCount(s.comboVk[1]));
    SetIntegerProp(L"PPC_COMBO_KEY3_TAPS", GameTapCount(s.comboVk[2]));
    SetIntegerProp(L"PPC_R_TAPS", GameTapCount('R'));
    SetIntegerProp(L"PPC_SEND_FAILURES", ppc_input::SendFailureCount());
}

void ComboWorker() {
    const long long freq = QpcFreq();
    bool wasActive = false;
    int comboIndex = 0;
    int lastComboHz = 0;
    int lastRHz = 0;
    bool lastREnabled = false;
    long long nextCombo = QpcNow();
    long long nextR = QpcNow();

    while (!g_quit.load(std::memory_order_relaxed)) {
        if (!g_active.load(std::memory_order_acquire)) {
            wasActive = false;
            WaitForSingleObject(g_wakeEvent, 20);
            continue;
        }

        Settings s;
        {
            std::lock_guard<std::mutex> lk(g_settingsMutex);
            s = g_settings;
        }
        const Mode mode = g_mode.load(std::memory_order_relaxed);
        const int comboHz = ppc::minorRate(mode);             // rate of EACH combo key
        const int comboEventHz = comboHz * 3;                 // 8,9,0 event stream
        const int rHz = ppc::rRate(s, mode);
        const bool rEnabledNow = s.rEnabled;
        const long long comboStep = std::max<long long>(1, freq / comboEventHz);
        const long long rStep = std::max<long long>(1, freq / (std::max)(1, rHz));
        long long now = QpcNow();

        if (!wasActive) {
            ResetGameTapCounts();
            comboIndex = 0;
            nextCombo = now;
            nextR = now + rStep;
            lastComboHz = comboHz;
            lastRHz = rHz;
            lastREnabled = rEnabledNow;
            wasActive = true;
        }

        if (comboHz != lastComboHz) {
            nextCombo = now;
            comboIndex = 0;
            lastComboHz = comboHz;
        }
        if (rHz != lastRHz || rEnabledNow != lastREnabled) {
            nextR = now + rStep;
            lastRHz = rHz;
            lastREnabled = rEnabledNow;
        }

        const bool rSuppressed = now < g_rSuppressUntil.load(std::memory_order_relaxed);
        const bool rEligible = rEnabledNow && !rSuppressed;
        if (!rEligible) nextR = now + rStep;

        // Never catch up with a burst of simultaneous taps. If Windows paused the
        // process, skip missed slots and resume the ordered stream from 'now'.
        if (now - nextCombo > comboStep * 4) nextCombo = now;
        if (rEligible && now - nextR > rStep * 2) nextR = now;

        if (nextCombo <= now && (!rEligible || nextCombo <= nextR)) {
            const int vk = s.comboVk[static_cast<std::size_t>(comboIndex)];
            const bool ok = SendKey(vk);

            // Advance even if Windows rejects one event. Retrying the same key in
            // place is exactly how a failure can degrade into "only 8 works".
            comboIndex = (comboIndex + 1) % 3;
            if (comboIndex == 0 && ok)
                g_comboBursts.fetch_add(1, std::memory_order_relaxed);

            nextCombo += comboStep;
            continue;
        }

        if (rEligible && nextR <= now) {
            if (SendKey('R'))
                g_rPresses.fetch_add(1, std::memory_order_relaxed);
            nextR += rStep;
            continue;
        }

        long long target = nextCombo;
        if (rEligible) target = (std::min)(target, nextR);
        PreciseWaitUntil(target);
    }
}

void RWorker() {
    // R is intentionally scheduled by ComboWorker so R can never overlap 8/9/0.
    // Keep the historical second thread entry point alive without generating keys.
    while (!g_quit.load(std::memory_order_relaxed)) Sleep(20);
}
