}

void Panel(Graphics& g, RectF r, Color fill=Color(245,250,241,226), Color border=Color(255,182,132,51), float radius=8.0f) {
    GraphicsPath p; float d=radius*2;
    p.AddArc(r.X,r.Y,d,d,180,90); p.AddArc(r.GetRight()-d,r.Y,d,d,270,90); p.AddArc(r.GetRight()-d,r.GetBottom()-d,d,d,0,90); p.AddArc(r.X,r.GetBottom()-d,d,d,90,90); p.CloseFigure();
    SolidBrush b(fill); Pen pen(border,1.4f); g.FillPath(&b,&p); g.DrawPath(&pen,&p);
}

void Button(Graphics& g, const RECT& rr, const wchar_t* text, Color c1, Color c2, bool selected=false) {
    RectF r((REAL)rr.left,(REAL)rr.top,(REAL)(rr.right-rr.left),(REAL)(rr.bottom-rr.top));
    GraphicsPath p; const float rad=9, d=18;
    p.AddArc(r.X,r.Y,d,d,180,90); p.AddArc(r.GetRight()-d,r.Y,d,d,270,90); p.AddArc(r.GetRight()-d,r.GetBottom()-d,d,d,0,90); p.AddArc(r.X,r.GetBottom()-d,d,d,90,90); p.CloseFigure();
    LinearGradientBrush br(r,c1,c2,LinearGradientModeVertical); g.FillPath(&br,&p);
    Pen pen(selected?Color(255,255,193,64):Color(255,162,111,33), selected?2.3f:1.4f); g.DrawPath(&pen,&p);
    DrawTextG(g,text,r.X,r.Y,r.Width,r.Height,18,true,Color(255,255,248,229),StringAlignmentCenter);
}

void DrawModeCard(Graphics& g, const RECT& rr, bool turbo, bool selected) {
    RectF r((REAL)rr.left,(REAL)rr.top,(REAL)(rr.right-rr.left),(REAL)(rr.bottom-rr.top));
    Color fill = selected ? (turbo?Color(255,109,21,18):Color(255,102,65,18)) : Color(250,247,234,211);
    Color txt = selected ? Color(255,255,231,158) : Color(255,66,42,21);
    Panel(g,r,fill,selected?Color(255,242,174,57):Color(255,173,128,48),10);
    DrawTextG(g,turbo?L"⚡ Turbo Mod":L"⚡ Maximum Mod",r.X+12,r.Y+6,r.Width-42,30,18,true,txt);
    DrawTextG(g,turbo?L"Minor Hızı: ~240/sn (her tuş)":L"Minor Hızı: ~120/sn (her tuş)",r.X+12,r.Y+40,r.Width-24,30,14,false,txt);
    DrawTextG(g,L"!",r.GetRight()-31,r.Y+8,22,22,14,true,selected?Color(255,255,202,71):Color(255,116,76,25),StringAlignmentCenter);
}

void SaveRegistry() {
    Settings s; { std::lock_guard<std::mutex> lk(g_settingsMutex); s = g_settings; }
    HKEY key{}; if (RegCreateKeyExW(HKEY_CURRENT_USER,L"Software\\PremiumPlusCombo",0,nullptr,0,KEY_WRITE,nullptr,&key,nullptr)!=ERROR_SUCCESS) return;
    auto dw=[&](const wchar_t* n,DWORD v){ RegSetValueExW(key,n,0,REG_DWORD,(BYTE*)&v,sizeof(v)); };
    dw(L"Seq1",s.comboVk[0]); dw(L"Seq2",s.comboVk[1]); dw(L"Seq3",s.comboVk[2]);
    dw(L"REnabled",s.rEnabled?1:0); dw(L"RMax",s.rRateMaximum); dw(L"RTurbo",s.rRateTurbo);
    dw(L"CureEnabled",s.cureEnabled?1:0); dw(L"CureBar",s.cureBar); dw(L"CureSlot",s.cureSlot); dw(L"CureHotkey",s.cureHotkey);
    RegCloseKey(key);
}

void LoadRegistry() {
    Settings s;
    HKEY key{}; if (RegOpenKeyExW(HKEY_CURRENT_USER,L"Software\\PremiumPlusCombo",0,KEY_READ,&key)==ERROR_SUCCESS) {
        auto rd=[&](const wchar_t* n,DWORD def){ DWORD v=def,sz=sizeof(v),t=REG_DWORD; if(RegQueryValueExW(key,n,nullptr,&t,(BYTE*)&v,&sz)!=ERROR_SUCCESS) v=def; return v; };
        s.comboVk[0]=rd(L"Seq1",'8'); s.comboVk[1]=rd(L"Seq2",'9'); s.comboVk[2]=rd(L"Seq3",'0');
        s.rEnabled=rd(L"REnabled",1)!=0; s.rRateMaximum=rd(L"RMax",25); s.rRateTurbo=rd(L"RTurbo",40);
        s.cureEnabled=rd(L"CureEnabled",1)!=0; s.cureBar=rd(L"CureBar",2); s.cureSlot=rd(L"CureSlot",6); s.cureHotkey=rd(L"CureHotkey",'C');
        RegCloseKey(key);
    }
    s = ppc::sanitize(s); std::lock_guard<std::mutex> lk(g_settingsMutex); g_settings=s;
}

std::wstring GetText(HWND h) { wchar_t b[64]{}; GetWindowTextW(h,b,64); return b; }
int GetInt(HWND h,int def){ std::wstring s=GetText(h); int v=_wtoi(s.c_str()); return v? v:def; }

bool ApplyControls(bool showError) {
    Settings s; { std::lock_guard<std::mutex> lk(g_settingsMutex); s=g_settings; }
    int k1=ParseVk(GetText(hSeq1)), k2=ParseVk(GetText(hSeq2)), k3=ParseVk(GetText(hSeq3));
    if(!k1||!k2||!k3){ if(showError) MessageBoxW(g_hwnd,L"Giriş dizisinde geçersiz bir tuş var. 0-9, A-Z veya F1-F12 kullanabilirsiniz.",L"Giriş Dizisi",MB_OK|MB_ICONWARNING); return false; }
    s.comboVk={k1,k2,k3}; s.rEnabled=Button_GetCheck(hRCheck)==BST_CHECKED;
    s.rRateMaximum=std::clamp(GetInt(hRMax,25),1,100); s.rRateTurbo=std::clamp(GetInt(hRTurbo,40),1,100);
    s.cureEnabled=Button_GetCheck(hCureCheck)==BST_CHECKED;
    s.cureBar=ComboBox_GetCurSel(hCureBar)+1; s.cureSlot=ComboBox_GetCurSel(hCureSlot)+1;
    s=ppc::sanitize(s); { std::lock_guard<std::mutex> lk(g_settingsMutex); g_settings=s; }
    SetWindowTextW(hRMax,std::to_wstring(s.rRateMaximum).c_str()); SetWindowTextW(hRTurbo,std::to_wstring(s.rRateTurbo).c_str());
    return true;
}

void SyncControls() {
    Settings s; { std::lock_guard<std::mutex> lk(g_settingsMutex); s=g_settings; }
    SetWindowTextW(hSeq1,VkName(s.comboVk[0]).c_str()); SetWindowTextW(hSeq2,VkName(s.comboVk[1]).c_str()); SetWindowTextW(hSeq3,VkName(s.comboVk[2]).c_str());
    Button_SetCheck(hRCheck,s.rEnabled?BST_CHECKED:BST_UNCHECKED);
    SetWindowTextW(hRMax,std::to_wstring(s.rRateMaximum).c_str()); SetWindowTextW(hRTurbo,std::to_wstring(s.rRateTurbo).c_str());
    Button_SetCheck(hCureCheck,s.cureEnabled?BST_CHECKED:BST_UNCHECKED);
    ComboBox_SetCurSel(hCureBar,s.cureBar-1); ComboBox_SetCurSel(hCureSlot,s.cureSlot-1);
    SetWindowTextW(hCureKey,VkName(s.cureHotkey).c_str());
}

void MakeChildControls(HWND w) {
    auto mkEdit=[&](int id,int x,int y,int ww,int hh,DWORD extra=0){ HWND h=CreateWindowExW(0,L"EDIT",L"",WS_CHILD|WS_VISIBLE|WS_BORDER|ES_CENTER|extra,x,y,ww,hh,w,(HMENU)(INT_PTR)id,nullptr,nullptr); SendMessage(h,WM_SETFONT,(WPARAM)g_font,TRUE); return h; };
    auto mkCheck=[&](int id,const wchar_t* t,int x,int y,int ww,int hh){ HWND h=CreateWindowExW(0,L"BUTTON",t,WS_CHILD|WS_VISIBLE|BS_AUTOCHECKBOX,x,y,ww,hh,w,(HMENU)(INT_PTR)id,nullptr,nullptr); SendMessage(h,WM_SETFONT,(WPARAM)g_font,TRUE); return h; };
    hSeq1=mkEdit(1001,260,272,58,30); hSeq2=mkEdit(1002,335,272,58,30); hSeq3=mkEdit(1003,410,272,58,30);
    SendMessage(hSeq1,EM_SETLIMITTEXT,5,0); SendMessage(hSeq2,EM_SETLIMITTEXT,5,0); SendMessage(hSeq3,EM_SETLIMITTEXT,5,0);
    hRCheck=mkCheck(1101,L"R Combo",205,338,90,24);
    hRMax=mkEdit(1102,395,335,54,28,ES_NUMBER); hRTurbo=mkEdit(1103,535,335,54,28,ES_NUMBER);
    hCureCheck=mkCheck(1201,L"Cure Al",205,408,85,24);
    hCureBar=CreateWindowExW(0,L"COMBOBOX",L"",WS_CHILD|WS_VISIBLE|CBS_DROPDOWNLIST,318,405,72,180,w,(HMENU)1202,nullptr,nullptr);
    hCureSlot=CreateWindowExW(0,L"COMBOBOX",L"",WS_CHILD|WS_VISIBLE|CBS_DROPDOWNLIST,430,405,58,180,w,(HMENU)1203,nullptr,nullptr);
    hCureKey=CreateWindowExW(0,L"BUTTON",L"C",WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON,548,404,78,29,w,(HMENU)1204,nullptr,nullptr);
    SendMessage(hCureBar,WM_SETFONT,(WPARAM)g_font,TRUE); SendMessage(hCureSlot,WM_SETFONT,(WPARAM)g_font,TRUE); SendMessage(hCureKey,WM_SETFONT,(WPARAM)g_fontBold,TRUE);
    for(int i=1;i<=8;i++){ std::wstring f=L"F"+std::to_wstring(i); ComboBox_AddString(hCureBar,f.c_str()); std::wstring n=std::to_wstring(i); ComboBox_AddString(hCureSlot,n.c_str()); }
    SyncControls();
}

void DrawUi(HWND hwnd, HDC hdc) {
    Graphics g(hdc); g.SetSmoothingMode(SmoothingModeAntiAlias); g.SetTextRenderingHint(TextRenderingHintClearTypeGridFit);
    LinearGradientBrush bg(Rect(0,0,W,H),Color(255,251,244,229),Color(255,232,210,174),LinearGradientModeVertical); g.FillRectangle(&bg,0,0,W,H);
    Pen edge(Color(255,156,105,28),2); g.DrawRectangle(&edge,2,2,W-5,H-5);
    Pen inner(Color(150,205,165,92),1); g.DrawRectangle(&inner,8,8,W-17,H-17);

    DrawTextG(g,L"Premium Plus Combo",210,14,400,42,27,true,Color(255,118,68,18),StringAlignmentCenter);
    DrawTextG(g,L"Rogue",28,21,125,30,19,true,Color(255,55,36,22));
    DrawTextG(g,L"—",748,10,30,30,22,true,Color(255,83,50,20),StringAlignmentCenter);
    DrawTextG(g,L"×",782,10,30,30,23,true,Color(255,83,50,20),StringAlignmentCenter);

    if(g_karus){ Rect dest(18,86,145,190); g.DrawImage(g_karus.get(),dest); Pen p(Color(255,168,118,40),2); g.DrawRectangle(&p,dest); }
    if(g_human){ Rect dest(657,86,145,190); g.DrawImage(g_human.get(),dest); Pen p(Color(255,168,118,40),2); g.DrawRectangle(&p,dest); }
    DrawTextG(g,L"KARUS",18,278,145,22,13,true,Color(255,126,21,18),StringAlignmentCenter);
    DrawTextG(g,L"HUMAN",657,278,145,22,13,true,Color(255,126,84,18),StringAlignmentCenter);

    Button(g,rcStart,L"▶  Başlat  (TAB)",Color(255,125,78,22),Color(255,80,43,13),g_active.load());
    Button(g,rcStop,L"■  Durdur  (Caps Lock)",Color(255,151,28,26),Color(255,91,13,18),!g_active.load());

    DrawTextG(g,L"Mod Seçimi",195,127,430,20,14,true,Color(255,82,49,20),StringAlignmentCenter);
    DrawModeCard(g,rcMax,false,g_mode.load()==Mode::Maximum);
    DrawModeCard(g,rcTurbo,true,g_mode.load()==Mode::Turbo);

    Panel(g,RectF(195,246,430,67),Color(245,249,240,220),Color(255,185,138,57),8);
    DrawTextG(g,L"Giriş Dizisi",205,247,120,22,14,true);
    DrawTextG(g,L">",320,271,14,30,18,true,Color(255,118,79,26),StringAlignmentCenter);
    DrawTextG(g,L">",395,271,14,30,18,true,Color(255,118,79,26),StringAlignmentCenter);
    DrawTextG(g,L"Her tuş kendi hedef hızında tekrarlanır.",482,268,130,36,11,false,Color(255,88,65,42),StringAlignmentCenter);
