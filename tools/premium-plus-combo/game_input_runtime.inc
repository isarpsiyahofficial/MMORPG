// Game-facing keyboard runtime.
//
// Important: the game input path evaluates hotkeys as an ordered state chain, so
// several synthetic keys being down in the same sampling window can cause only
// the first slot to win. The old implementation sent 8/9/0 down+up events as a
// single SendInput batch, which made that failure mode very easy to hit.
//
// This layer guarantees that every synthetic tap is non-overlapping: one key is
// pressed, held briefly, released, and only then may the next combo/R/Cure key
// begin. The combo scheduler cycles 8 -> 9 -> 0 (or the configured three keys)
// instead of retrying one overdue batch. If the official in-game bridge is
// connected, the same individual down/up events are queued there without loss.

std::mutex g_gameTapMutex;
std::array<std::atomic<unsigned long long>, 256> g_gameTapCounts{};

void WaitInputMicros(int microseconds) {
    if (microseconds <= 0) return;
    const long long freq = QpcFreq();
    const long long start = QpcNow();
    const long long delta = std::max<long long>(1, (freq * static_cast<long long>(microseconds)) / 1'000'000LL);
    const long long target = start + delta;
    for (;;) {
        const long long now = QpcNow();
        if (now >= target) break;
        const long long remain = target - now;
        if (remain > freq / 2000) {
            SwitchToThread();
        } else {
            YieldProcessor();
        }
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

int GameTapHoldMicros() {
    // Long enough to avoid the old zero-duration down/up pulse, while remaining
    // comfortably below the event spacing needed for 120/key and 240/key modes.
    return g_mode.load(std::memory_order_relaxed) == Mode::Turbo ? 450 : 950;
}

bool SendKey(int vk) {
    if (vk <= 0 || vk >= 256) return false;
    std::lock_guard<std::mutex> guard(g_gameTapMutex);

    if (!SendKeyState(vk, false)) return false;
    WaitInputMicros(GameTapHoldMicros());

    bool upOk = SendKeyState(vk, true);
    if (!upOk) {
        // A failed key-up must never be allowed to leave a synthetic key stuck.
        Sleep(1);
        upOk = SendKeyState(vk, true);
    }

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
