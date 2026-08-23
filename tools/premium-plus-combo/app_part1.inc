#define NOMINMAX
#include <windows.h>
#include <windowsx.h>
#include <cstring>
#include <gdiplus.h>
#include <mmsystem.h>
#include <atomic>
#include <array>
#include <string>
#include <thread>
#include <mutex>
#include <memory>
#include <algorithm>
#include <cwctype>
#include <sstream>
#include "core.hpp"
#include "resource.h"

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "winmm.lib")

using namespace Gdiplus;
using ppc::Mode;
using ppc::Settings;

namespace {
constexpr int W = 820;
constexpr int H = 560;
constexpr UINT WM_APP_CUREKEY = WM_APP + 1;
constexpr UINT_PTR TIMER_UI = 1;
constexpr int HOTKEY_START = 0x5101;
constexpr int HOTKEY_STOP = 0x5102;
constexpr ULONG_PTR EXTRA_TAG = 0x5050434F4D424FULL;

HWND g_hwnd{};
HHOOK g_hook{};
HBRUSH g_brushCream{};
HFONT g_font{}, g_fontBold{};
ULONG_PTR g_gdiplusToken{};
std::unique_ptr<Bitmap> g_karus;
std::unique_ptr<Bitmap> g_human;

std::atomic<bool> g_quit{false};
std::atomic<bool> g_active{false};
std::atomic<bool> g_captureCure{false};
std::atomic<int> g_currentBar{1};
std::atomic<long long> g_rSuppressUntil{0};
std::atomic<bool> g_cureBusy{false};
std::atomic<Mode> g_mode{Mode::Maximum};
HANDLE g_wakeEvent{};

Settings g_settings{};
std::mutex g_settingsMutex;

HWND hSeq1{}, hSeq2{}, hSeq3{};
HWND hRCheck{}, hRMax{}, hRTurbo{};
HWND hCureCheck{}, hCureBar{}, hCureSlot{}, hCureKey{};

RECT rcStart{195,75,395,125};
RECT rcStop{425,75,625,125};
RECT rcMax{195,145,395,235};
RECT rcTurbo{425,145,625,235};
RECT rcInfoMax{365,154,388,177};
RECT rcInfoTurbo{595,154,618,177};
RECT rcSave{300,492,520,530};
RECT rcMin{748,14,778,42};
RECT rcClose{782,14,812,42};

long long QpcNow() {
    LARGE_INTEGER v{}; QueryPerformanceCounter(&v); return v.QuadPart;
}
long long QpcFreq() {
    static long long f = []{ LARGE_INTEGER v{}; QueryPerformanceFrequency(&v); return v.QuadPart; }();
    return f;
}

bool PtIn(const RECT& r, int x, int y) { POINT p{x,y}; return PtInRect(&r,p) != FALSE; }

std::wstring VkName(int vk) {
    if (vk >= '0' && vk <= '9') return std::wstring(1, wchar_t(vk));
    if (vk >= 'A' && vk <= 'Z') return std::wstring(1, wchar_t(vk));
    if (vk >= VK_F1 && vk <= VK_F12) return L"F" + std::to_wstring(vk - VK_F1 + 1);
    switch (vk) {
        case VK_SPACE: return L"SPACE";
        case VK_SHIFT: return L"SHIFT";
        case VK_CONTROL: return L"CTRL";
        case VK_MENU: return L"ALT";
        case VK_TAB: return L"TAB";
        case VK_CAPITAL: return L"CAPS";
        case VK_XBUTTON1: return L"MOUSE4";
        case VK_XBUTTON2: return L"MOUSE5";
        default: return L"VK" + std::to_wstring(vk);
    }
}

int ParseVk(std::wstring s) {
    s.erase(std::remove_if(s.begin(), s.end(), [](wchar_t c){ return iswspace(c); }), s.end());
    std::transform(s.begin(), s.end(), s.begin(), [](wchar_t c){ return static_cast<wchar_t>(towupper(c)); });
    if (s.size() == 1 && ((s[0] >= L'0' && s[0] <= L'9') || (s[0] >= L'A' && s[0] <= L'Z'))) return int(s[0]);
    if (s.size() >= 2 && s[0] == L'F') {
        int n = _wtoi(s.c_str()+1); if (n >= 1 && n <= 12) return VK_F1+n-1;
    }
    if (s == L"SPACE") return VK_SPACE;
    if (s == L"SHIFT") return VK_SHIFT;
    if (s == L"CTRL" || s == L"CONTROL") return VK_CONTROL;
    if (s == L"ALT") return VK_MENU;
    return 0;
}

void SendKey(int vk) {
    INPUT in[2]{};
    in[0].type = INPUT_KEYBOARD; in[0].ki.wVk = WORD(vk); in[0].ki.dwExtraInfo = EXTRA_TAG;
    in[1].type = INPUT_KEYBOARD; in[1].ki.wVk = WORD(vk); in[1].ki.dwFlags = KEYEVENTF_KEYUP; in[1].ki.dwExtraInfo = EXTRA_TAG;
    SendInput(2, in, sizeof(INPUT));
}
