// One-time repair for the accidental intermediate build that rewrote the
// intended CAPS LOCK / CAPS LOCK toggle into TAB / CAPS LOCK.
//
// Important: this runs on top of the existing loader instead of replacing any
// UI, power, game-input or user-setting code. If a user later intentionally
// assigns TAB/CAPS again, the V5 marker means that custom choice is preserved.
void LoadRegistry() {
    PpcPowerLoadRegistry();

    HKEY key{};
    if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\PremiumPlusCombo\\RogueV2", 0, nullptr, 0,
                        KEY_READ | KEY_WRITE, nullptr, &key, nullptr) != ERROR_SUCCESS) {
        return;
    }

    DWORD marker = 0;
    DWORD markerSize = sizeof(marker);
    DWORD markerType = REG_DWORD;
    const bool repairedBefore =
        RegQueryValueExW(key, L"CapsToggleRestoreV5", nullptr, &markerType,
                         reinterpret_cast<BYTE*>(&marker), &markerSize) == ERROR_SUCCESS &&
        markerType == REG_DWORD && marker == 1;

    if (!repairedBefore) {
        bool corrected = false;
        {
            std::lock_guard<std::mutex> lk(g_settingsMutex);
            if (g_settings.startHotkey == VK_TAB && g_settings.stopHotkey == VK_CAPITAL) {
                g_settings.startHotkey = VK_CAPITAL;
                g_settings.stopHotkey = VK_CAPITAL;
                corrected = true;
            }
        }

        // Persist the repaired pair. Earlier migration code changed only memory,
        // which allowed the old pair to reappear on a later launch.
        if (corrected) {
            const DWORD caps = VK_CAPITAL;
            RegSetValueExW(key, L"StartHotkey", 0, REG_DWORD,
                           reinterpret_cast<const BYTE*>(&caps), sizeof(caps));
            RegSetValueExW(key, L"StopHotkey", 0, REG_DWORD,
                           reinterpret_cast<const BYTE*>(&caps), sizeof(caps));
        }

        marker = 1;
        RegSetValueExW(key, L"CapsToggleRestoreV5", 0, REG_DWORD,
                       reinterpret_cast<const BYTE*>(&marker), sizeof(marker));
    }

    RegCloseKey(key);
}
