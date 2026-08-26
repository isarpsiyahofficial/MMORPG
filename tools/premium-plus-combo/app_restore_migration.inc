// One-time repair for the accidental intermediate builds that could leave
// TAB / CAPS LOCK behind after the intended CAPS LOCK / CAPS LOCK toggle
// had already been restored in memory.
//
// V6 deliberately inspects the raw registry values as well as the loaded
// settings. This fixes two real upgrade paths without touching genuine custom
// hotkeys:
//   1) a build that explicitly saved TAB/CAPS;
//   2) a fresh/legacy install where the old V3 migration changed only memory,
//      set its marker, but never persisted StartHotkey/StopHotkey.
void LoadRegistry() {
    PpcPowerLoadRegistry();

    HKEY key{};
    if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\PremiumPlusCombo\\RogueV2", 0, nullptr, 0,
                        KEY_READ | KEY_WRITE, nullptr, &key, nullptr) != ERROR_SUCCESS) {
        return;
    }

    auto readDword = [&](const wchar_t* name, DWORD& out) -> bool {
        DWORD size = sizeof(out);
        DWORD type = REG_DWORD;
        return RegQueryValueExW(key, name, nullptr, &type,
                                reinterpret_cast<BYTE*>(&out), &size) == ERROR_SUCCESS &&
               type == REG_DWORD && size == sizeof(out);
    };

    DWORD v6 = 0;
    const bool repairedV6 = readDword(L"CapsToggleRestoreV6", v6) && v6 == 1;

    if (!repairedV6) {
        DWORD rawStart = 0, rawStop = 0, v3 = 0;
        const bool hasRawStart = readDword(L"StartHotkey", rawStart);
        const bool hasRawStop = readDword(L"StopHotkey", rawStop);
        const bool hasV3Marker = readDword(L"CapsToggleDefaultV3", v3) && v3 == 1;

        bool restoreCapsToggle = false;

        // Exact accidentally shipped pair: always safe to restore.
        if (hasRawStart && hasRawStop &&
            rawStart == static_cast<DWORD>(VK_TAB) &&
            rawStop == static_cast<DWORD>(VK_CAPITAL)) {
            restoreCapsToggle = true;
        }

        // Old V3 migration could set its marker after changing only the in-memory
        // defaults. On the next launch both raw hotkeys were still absent, so the
        // loader silently fell back to TAB/CAPS again. Treat that incomplete V3
        // state as the intended CAPS/CAPS default.
        if (!hasRawStart && !hasRawStop && hasV3Marker) {
            restoreCapsToggle = true;
        }

        if (restoreCapsToggle) {
            {
                std::lock_guard<std::mutex> lk(g_settingsMutex);
                g_settings.startHotkey = VK_CAPITAL;
                g_settings.stopHotkey = VK_CAPITAL;
            }

            const DWORD caps = VK_CAPITAL;
            RegSetValueExW(key, L"StartHotkey", 0, REG_DWORD,
                           reinterpret_cast<const BYTE*>(&caps), sizeof(caps));
            RegSetValueExW(key, L"StopHotkey", 0, REG_DWORD,
                           reinterpret_cast<const BYTE*>(&caps), sizeof(caps));
        }

        const DWORD one = 1;
        RegSetValueExW(key, L"CapsToggleRestoreV6", 0, REG_DWORD,
                       reinterpret_cast<const BYTE*>(&one), sizeof(one));
    }

    RegCloseKey(key);
}
