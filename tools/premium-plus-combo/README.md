# Premium Plus Combo — Rogue

Windows x64 portable tek-EXE Rogue otomasyon aracı.

## Varsayılan çalışma
- TAB: başlat
- Caps Lock: durdur
- Maximum Mod: 8/9/0 tuşlarının her biri için hedef ~120 basım/sn
- Turbo Mod: 8/9/0 tuşlarının her biri için hedef ~240 basım/sn
- R Combo: Maximum 25/sn, Turbo 40/sn; değerler Rogue Ayarları bölümünden düzenlenebilir
- Cure Al: varsayılan F2 / Slot 6 / C tuşu
- Cure tetiklenince R Combo 2 saniye askıya alınır; R kapalıysa açılmaz
- Cure sonrası program fiziksel olarak en son gözlenen F1-F8 barına geri döner
- Ayarlar `HKCU\\Software\\PremiumPlusCombo` altında saklanır; harici config dosyası yoktur
- Mod seçimi kaydedilmez; uygulama her açılışta Maximum Mod ile başlar

## Derleme
Visual Studio Build Tools / MSVC:

```powershell
rc /nologo /fo app.res app.rc
cl /nologo /std:c++17 /O2 /MT /EHsc /W4 /DUNICODE /D_UNICODE main.cpp app.res /Fe:PremiumPlusCombo.exe /link /SUBSYSTEM:WINDOWS user32.lib gdi32.lib gdiplus.lib advapi32.lib ole32.lib winmm.lib
```
