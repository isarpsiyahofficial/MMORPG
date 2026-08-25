// Game-facing keyboard runtime.
//
// Knight Online reads keyboard STATE, not a buffered text/event queue. A down+up
// pair that exists for only a few hundred microseconds can disappear completely
// between two game input samples. Sending 8/9/0 in one batch made this even worse:
// the game's ordered hotkey chain could repeatedly observe only the first slot.
//
// Production therefore has two deliberately different transports:
//   1) Official bridge connected: every down/up event is queued directly to the
//      game-side action layer; no artificial hold is necessary and the requested
//      120/240 attempts per key can be preserved by the producer.
//   2) Stock Knight Online client: one physical-looking scan-code state is held
//      long enough to cross a realistic DirectInput sampling window, then released
//      before the next key begins. This prioritises ALL configured keys being seen
//      by the game instead of producing impressive Windows-only counters.

std::mutex g_gameTapMutex;
std::array<std::atomic<unsigned long long>, 256> g_gameTapCounts{};

void WaitInputMicros(int microseconds) {
    if (microseconds <= 0) return;
    const long long freq = QpcFreq();
    const long long requestedTicks =
        (freq * static_cast<long long>(microseconds)) / 1'000'000LL;
    const long long deltaTicks = requestedTicks > 1 ? requestedTicks : 1;
    const long long target = QpcNow() + deltaTicks;

    // Do not burn a CPU core during the millisecond-scale DirectInput dwell. Sleep
    // most of the interval, then finish against QPC for a stable release edge.
    if (microseconds >= 2500) {
        const int requestedSleep = (microseconds / 1000) - 1;
        const int sleepMs = requestedSleep > 1 ? requestedSleep : 1;
        Sleep(static_cast<DWORD>(sleepMs));
    }
    for (;;) {
        const long long now = QpcNow();
        if (now >= target) break;
        const long long remain = target - now;
        if (remain > freq / 2500) SwitchToThread();
        else YieldProcessor();
    }
}

bool SendKeyState(int vk, bool keyUp) {
    if (vk <= 0 || vk >= 256) return false;
    INPUT in{};
    in.type = INPUT_KEYBOARD;
    in.ki.wVk = static_cast<WORD>(vk);
    in.ki.dwFlags = keyUp ? KEYEVENTF_KEYUP : 0;
    in.ki.dwExtraInfo = EXTRA_TAG;
    return SendInput(1, &in, sizeof(INPUT)) == 1;
}

int GameTapHoldMicros(bool bridgeConnected) {
    if (bridgeConnected) return 0;

    // Real-client profile. 12 ms is intentionally in the range that survives
    // common 60-100+ Hz game sampling far more reliably than the old 0.45/0.95 ms
    // pulse. Turbo shortens the state window, but never returns to a zero-duration
    // press. The game itself remains the authority on cooldown/minor acceptance.
    return g_mode.load(std::memory_order_relaxed) == Mode::Turbo ? 7000 : 12000;
}

int GameTapReleaseGapMicros(bool bridgeConnected) {
    if (bridgeConnected) return 0;
    // A real released interval prevents two neighbouring keys from becoming one
    // ambiguous multi-key state in DirectInput.
    return g_mode.load(std::memory_order_relaxed) == Mode::Turbo ? 1200 : 2500;
}

bool SendKey(int vk) {
    if (vk <= 0 || vk >= 256) return false;
    std::lock_guard<std::mutex> guard(g_gameTapMutex);

    const bool bridgeConnected = ppc_input::BridgeConnected();
    if (!SendKeyState(vk, false)) return false;
    WaitInputMicros(GameTapHoldMicros(bridgeConnected));

    bool upOk = SendKeyState(vk, true);
    if (!upOk) {
        // A failed key-up must never leave a synthetic key stuck.
        Sleep(1);
        upOk = SendKeyState(vk, true);
    }

    if (!bridgeConnected) WaitInputMicros(GameTapReleaseGapMicros(false));

    if (upOk)
        g_gameTapCounts[static_cast<std::size_t>(vk)].fetch_add(1, std::memory_order_relaxed);
    return upOk;
}

unsigned long long GameTapCount(int vk) {
    if (vk <= 0 || vk >= 256) return 0;
    return g_gameTapCounts[static_cast<std::size_t>(vk)].load(std::memory_order_relaxed);
}

void ResetGameTapCounts() {
    for (auto& c : g_gameTapCounts) c.store(0, std::memory_order_relaxed);
}
