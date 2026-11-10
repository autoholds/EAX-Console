#pragma once
#include <commctrl.h>
#include <functional>
#include <memory>

namespace CFG {

enum Kind { K_LABEL, K_CHECK, K_RADIO, K_SLIDER, K_COMBO, K_LIST, K_BUTTON, K_HOLD, K_METER, K_TEXT };
struct Ctl {
    int page = 0, kind = 0, id = 0, mn = 0, mx = 0; bool vertical = false;
    HWND h = nullptr, val = nullptr;
    std::function<int()> get;
    std::function<void(int)> set;
    std::function<std::wstring(int)> fmt;
    std::function<void()> act;
    std::function<void()> drop;
    std::function<std::wstring()> text;
    std::wstring caption; bool bold = false;
    int shown = INT_MIN;
};
struct Grp { int page, x, y, w, h; std::wstring title; };
inline HWND win = nullptr;
inline std::vector<std::unique_ptr<Ctl>> ctls; inline std::vector<Grp> grps;
inline int curPage = 0, hotTab = -1, hotBtn = -1, pressBtn = -1; inline bool building = false;
constexpr int NPAGES = 3;
inline const wchar_t* kPages[NPAGES] = { L"Audio", L"Voice", L"Program" };
inline RECT tabRc[NPAGES];
constexpr int W = 700, H = 522, TITLE = 38, TABS = 24, BOTTOM = 36, CONTENT_TOP = TITLE + TABS;
constexpr COLORREF PANEL = RGB(246, 240, 246);
inline HBRUSH panelBrush() { static HBRUSH b = CreateSolidBrush(PANEL); return b; }
inline Px frameArt;
inline std::unique_ptr<Bitmap> frameBmp;

inline LRESULT CALLBACK SliderMouse(HWND h, UINT m, WPARAM w, LPARAM l, UINT_PTR, DWORD_PTR) {
    auto follow = [&](int x, int y) {
        RECT rc; GetClientRect(h, &rc); const bool vert = (GetWindowLongW(h, GWL_STYLE) & TBS_VERT) != 0;
        const int len = (vert ? rc.bottom : rc.right) - 24, at = (vert ? y : x) - 12; if (len <= 0) return;
        const int mx = (int)SendMessageW(h, TBM_GETRANGEMAX, 0, 0), pos = std::clamp((at * mx + len / 2) / len, 0, mx);
        if (pos != (int)SendMessageW(h, TBM_GETPOS, 0, 0)) { SendMessageW(h, TBM_SETPOS, TRUE, pos); SendMessageW(GetParent(h), vert ? WM_VSCROLL : WM_HSCROLL, TB_THUMBTRACK, (LPARAM)h); }
    };
    switch (m) {
    case WM_LBUTTONDOWN: case WM_LBUTTONDBLCLK: SetFocus(h); SetCapture(h); follow(GET_X_LPARAM(l), GET_Y_LPARAM(l)); InvalidateRect(h, nullptr, FALSE); return 0;
    case WM_MOUSEMOVE: if (GetCapture() == h) { follow(GET_X_LPARAM(l), GET_Y_LPARAM(l)); return 0; } break;
    case WM_LBUTTONUP: if (GetCapture() == h) { ReleaseCapture(); InvalidateRect(h, nullptr, FALSE); return 0; } break;
    case WM_NCDESTROY: RemoveWindowSubclass(h, SliderMouse, 1); break;
    }
    return DefSubclassProc(h, m, w, l);
}
inline Ctl& Add(int page, int kind) { ctls.push_back(std::make_unique<Ctl>()); Ctl& c = *ctls.back(); c.page = page; c.kind = kind; c.id = 2000 + (int)ctls.size() - 1; return c; }
inline HWND Make(const wchar_t* cls, const wchar_t* text, DWORD style, int x, int y, int w, int h, int id, DWORD ex = 0) {
    HWND c = CreateWindowExW(ex, cls, text, WS_CHILD | style, x, y, w, h, win, (HMENU)(INT_PTR)id, GetModuleHandleW(nullptr), nullptr);
    SendMessageW(c, WM_SETFONT, (WPARAM)SknFont(false), TRUE); return c;
}
inline void Label(int p, const wchar_t* t, int x, int y, int w, bool bold = false) {
    Ctl& c = Add(p, K_LABEL); c.bold = bold; c.h = Make(L"STATIC", t, SS_LEFT, x, y, w, 16, c.id);
    if (bold) SendMessageW(c.h, WM_SETFONT, (WPARAM)SknFont(true), TRUE);
}
inline void Check(int p, const wchar_t* t, int x, int y, int w, std::function<int()> get, std::function<void(int)> set) {
    Ctl& c = Add(p, K_CHECK); c.caption = t; c.h = Make(L"BUTTON", t, BS_OWNERDRAW | WS_TABSTOP, x, y, w, 18, c.id); c.get = get; c.set = set;
}
inline void Radio(int p, const wchar_t* t, int x, int y, int w, std::function<int()> get, std::function<void(int)> set) {
    Ctl& c = Add(p, K_RADIO); c.caption = t; c.h = Make(L"BUTTON", t, BS_OWNERDRAW | WS_TABSTOP, x, y, w, 18, c.id); c.get = get; c.set = set;
}
inline void Slider(int p, int x, int y, int w, int mn, int mx, std::function<int()> get, std::function<void(int)> set, std::function<std::wstring(int)> fmt, bool vertical = false, int h = 0) {
    Ctl& c = Add(p, K_SLIDER); c.mn = mn; c.mx = mx; c.get = get; c.set = set; c.fmt = fmt; c.vertical = vertical;
    c.h = Make(L"msctls_trackbar32", L"", (vertical ? TBS_VERT : TBS_HORZ) | TBS_NOTICKS | TBS_NOTHUMB | WS_TABSTOP, x, y, vertical ? w : w, vertical ? h : 24, c.id);
    SetWindowSubclass(c.h, SliderMouse, 1, 0);
    SendMessageW(c.h, TBM_SETRANGE, TRUE, MAKELONG(0, mx - mn));
    if (!vertical && fmt) c.val = Make(L"STATIC", L"", SS_RIGHT, x + w + 4, y + 4, 70, 16, 0);
    else if (vertical && fmt) c.val = Make(L"STATIC", L"", SS_CENTER, x - 12, y + h + 2, w + 24, 16, 0);
}
inline Ctl& Combo(int p, int x, int y, int w, const std::vector<std::wstring>& items, std::function<int()> get, std::function<void(int)> set) {
    Ctl& c = Add(p, K_COMBO); c.get = get; c.set = set;
    c.h = Make(L"COMBOBOX", L"", CBS_DROPDOWNLIST | CBS_OWNERDRAWFIXED | CBS_HASSTRINGS | WS_VSCROLL | WS_TABSTOP, x, y, w, 220, c.id);
    SendMessageW(c.h, CB_SETITEMHEIGHT, (WPARAM)-1, 18); SendMessageW(c.h, CB_SETITEMHEIGHT, 0, 16);
    for (auto& s : items) SendMessageW(c.h, CB_ADDSTRING, 0, (LPARAM)s.c_str());
    return c;
}
inline void List(int p, int x, int y, int w, int h, const std::vector<std::wstring>& items, std::function<int()> get, std::function<void(int)> set) {
    Ctl& c = Add(p, K_LIST); c.get = get; c.set = set;
    c.h = Make(L"LISTBOX", L"", LBS_NOTIFY | LBS_OWNERDRAWFIXED | LBS_HASSTRINGS | WS_VSCROLL | WS_BORDER | WS_TABSTOP, x, y, w, h, c.id);
    SendMessageW(c.h, LB_SETITEMHEIGHT, 0, 16);
    for (auto& s : items) SendMessageW(c.h, LB_ADDSTRING, 0, (LPARAM)s.c_str());
}
inline void Button(int p, const wchar_t* t, int x, int y, int w, int h, std::function<void()> act) {
    Ctl& c = Add(p, K_BUTTON); c.act = act; c.caption = t; c.h = Make(L"BUTTON", t, BS_OWNERDRAW | WS_TABSTOP, x, y, w, h, c.id);
}
inline Ctl* ById(int id);
inline LRESULT CALLBACK HoldMouse(HWND, UINT, WPARAM, LPARAM, UINT_PTR, DWORD_PTR);
inline void Hold(int p, const wchar_t* t, int x, int y, int w, int h, std::function<void(int)> set) {
    Ctl& c = Add(p, K_HOLD); c.set = set; c.caption = t; c.h = Make(L"BUTTON", t, BS_OWNERDRAW | WS_TABSTOP, x, y, w, h, c.id);
    SetWindowSubclass(c.h, HoldMouse, 2, 0);
}
inline void Meter(int p, int x, int y, int w, int h) { Ctl& c = Add(p, K_METER); c.h = Make(L"STATIC", L"", SS_OWNERDRAW, x, y, w, h, c.id); }
inline void Text(int p, int x, int y, int w, int h, std::function<std::wstring()> t) { Ctl& c = Add(p, K_TEXT); c.text = t; c.h = Make(L"STATIC", L"", SS_LEFT, x, y, w, h, c.id); }
inline void Group(int p, int x, int y, int w, int h, const wchar_t* title) { grps.push_back({ p, x, y, w, h, title }); }
inline Ctl* ById(int id) { for (auto& c : ctls) if (c->id == id) return c.get(); return nullptr; }

inline HWND heldBtn = nullptr;
inline LRESULT CALLBACK HoldMouse(HWND h, UINT m, WPARAM w, LPARAM l, UINT_PTR, DWORD_PTR) {
    auto release = [&] { if (heldBtn == h) { heldBtn = nullptr; if (Ctl* c = ById(GetDlgCtrlID(h))) c->set(0); } };
    switch (m) {
    case WM_LBUTTONDOWN: case WM_LBUTTONDBLCLK:
        if (Ctl* c = ById(GetDlgCtrlID(h))) { EngineStart(); heldBtn = h; c->set(1); }
        SetCapture(h); SendMessageW(h, BM_SETSTATE, TRUE, 0); return 0;
    case WM_LBUTTONUP: if (GetCapture() == h) ReleaseCapture(); SendMessageW(h, BM_SETSTATE, FALSE, 0); release(); return 0;
    case WM_CAPTURECHANGED: case WM_KILLFOCUS: SendMessageW(h, BM_SETSTATE, FALSE, 0); release(); break;
    case WM_NCDESTROY: release(); RemoveWindowSubclass(h, HoldMouse, 2); break;
    }
    return DefSubclassProc(h, m, w, l);
}

inline std::function<int()> Gi(const char* k) { return [k] { return orb::GetI(k); }; }
inline std::function<void(int)> Si(const char* k) { return [k](int v) { orb::SetI(k, v); }; }
inline std::function<int()> Gbit(const char* k, int bit) { return [k, bit] { return (orb::GetI(k) & bit) != 0 ? 1 : 0; }; }
inline std::function<void(int)> Sbit(const char* k, int bit) { return [k, bit](int v) { int x = orb::GetI(k); orb::SetI(k, v ? (x | bit) : (x & ~bit)); }; }
inline std::wstring Pct(int v) { return std::to_wstring(v) + L"%"; }
inline std::wstring Num(int v) { return std::to_wstring(v); }

inline std::vector<orb::Device> mics, spks;
inline std::vector<orb::Program> progs;
inline void SaveDevices() {
    HKEY k; if (!RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\CreativeEAXClone", 0, nullptr, 0, KEY_WRITE, nullptr, &k, nullptr)) {
        if (!g_savedMicId.empty()) RegSetValueExW(k, L"MicId", 0, REG_SZ, (BYTE*)g_savedMicId.c_str(), (DWORD)((g_savedMicId.size() + 1) * 2));
        if (!g_savedSpkId.empty()) RegSetValueExW(k, L"SpkId", 0, REG_SZ, (BYTE*)g_savedSpkId.c_str(), (DWORD)((g_savedSpkId.size() + 1) * 2));
        RegCloseKey(k);
    }
}
inline void ApplyDeviceEnables() {
    orb::SetDevices(g_micEnabled ? g_savedMicId.c_str() : L"", g_spkEnabled ? g_savedSpkId.c_str() : L"");
    if (orb::Running()) orb::Restart();
}
inline int DevIndex(const std::vector<orb::Device>& v, const std::wstring& id) { for (size_t i = 0; i < v.size(); i++) if (v[i].id == id) return (int)i; return -1; }
inline void FillPrograms(HWND combo) {
    progs = orb::Programs(); SendMessageW(combo, CB_RESETCONTENT, 0, 0); SendMessageW(combo, CB_ADDSTRING, 0, (LPARAM)L"(none)");
    for (auto& p : progs) SendMessageW(combo, CB_ADDSTRING, 0, (LPARAM)p.name.c_str());
}

inline std::vector<std::wstring> setupNames;
inline void FillSetups(HWND c) { setupNames = ST::ListSetups(); SendMessageW(c, CB_RESETCONTENT, 0, 0); for (auto& n : setupNames) SendMessageW(c, CB_ADDSTRING, 0, (LPARAM)n.c_str()); }
inline int SetupIndex() { for (size_t i = 0; i < setupNames.size(); i++) if (setupNames[i] == ST::lastSetup) return (int)i; return -1; }
inline HWND setupCombo = nullptr;
inline std::wstring askedName;
inline INT_PTR CALLBACK NameProc(HWND d, UINT m, WPARAM w, LPARAM) {
    if (m == WM_INITDIALOG) {
        SetDlgItemTextW(d, 3001, askedName.c_str()); SendDlgItemMessageW(d, 3001, EM_SETSEL, 0, -1);
        RECT pr, dr; GetWindowRect(win, &pr); GetWindowRect(d, &dr); SetWindowPos(d, nullptr, pr.left + (W - (dr.right - dr.left)) / 2, pr.top + 120, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
        SetFocus(GetDlgItem(d, 3001)); return FALSE;
    }
    if (m == WM_COMMAND && (LOWORD(w) == IDOK || LOWORD(w) == IDCANCEL)) {
        wchar_t b[200] = L""; GetDlgItemTextW(d, 3001, b, 200); askedName = b; EndDialog(d, LOWORD(w)); return TRUE;
    }
    return FALSE;
}
inline bool AskName(std::wstring& name) {
    std::vector<WORD> t; auto w16 = [&](WORD v) { t.push_back(v); }; auto w32 = [&](DWORD v) { t.push_back((WORD)v); t.push_back((WORD)(v >> 16)); };
    auto str = [&](const wchar_t* s) { do w16(*s); while (*s++); }; auto align = [&]() { while (t.size() % 2) w16(0); };
    w32(DS_MODALFRAME | DS_SETFONT | WS_POPUP | WS_CAPTION | WS_SYSMENU); w32(0); w16(4); w16(0); w16(0); w16(190); w16(62); w16(0); w16(0); str(L"Save setup"); w16(8); str(L"MS Shell Dlg");
    auto item = [&](WORD cls, DWORD st, int id, const wchar_t* text, int x, int y, int cx, int cy) { align(); w32(st | WS_CHILD | WS_VISIBLE); w32(0); w16(x); w16(y); w16(cx); w16(cy); w16(id); w16(0xFFFF); w16(cls); str(text); w16(0); };
    item(0x0082, SS_LEFT, 3000, L"Setup name:", 8, 8, 170, 10); item(0x0081, ES_AUTOHSCROLL | WS_BORDER | WS_TABSTOP, 3001, L"", 8, 20, 174, 12);
    item(0x0080, BS_DEFPUSHBUTTON | WS_TABSTOP, IDOK, L"Save", 38, 40, 50, 14); item(0x0080, BS_PUSHBUTTON | WS_TABSTOP, IDCANCEL, L"Cancel", 98, 40, 50, 14);
    align(); askedName = name;
    if (DialogBoxIndirectParamW(GetModuleHandleW(nullptr), (LPCDLGTEMPLATEW)t.data(), win, NameProc, 0) != IDOK) return false;
    name = askedName; return true;
}

inline void BuildPages() {
    {
        Ctl& sc = Combo(-1, 330, 8, 120, {}, [] { return SetupIndex(); }, [](int i) { if (i >= 0 && i < (int)setupNames.size()) ST::LoadSetup(setupNames[i]); });
        setupCombo = sc.h; sc.drop = [&sc] { FillSetups(sc.h); SendMessageW(sc.h, CB_SETCURSEL, SetupIndex(), 0); }; FillSetups(sc.h);
        Button(-1, L"Save as...", 456, 7, 58, 22, [] {
            std::wstring n = ST::lastSetup; if (!AskName(n)) return;
            if (!ST::SaveSetup(n)) { MessageBoxW(win, L"The setup could not be saved.", L"Audio Configuration", MB_OK | MB_ICONWARNING); return; }
            FillSetups(setupCombo); SendMessageW(setupCombo, CB_SETCURSEL, SetupIndex(), 0); if (auto* c = ById(GetDlgCtrlID(setupCombo))) c->shown = INT_MIN; });
        Button(-1, L"Delete", 518, 7, 46, 22, [] {
            int i = SetupIndex(); if (i < 0) return;
            if (MessageBoxW(win, (L"Delete the setup \"" + setupNames[i] + L"\"?").c_str(), L"Audio Configuration", MB_YESNO | MB_ICONQUESTION) != IDYES) return;
            ST::DeleteSetup(setupNames[i]); FillSetups(setupCombo); SendMessageW(setupCombo, CB_SETCURSEL, SetupIndex(), 0); });
    }
    const int X = 36, Y = 90;

    mics = orb::Mics(); spks = orb::Speakers();
    std::vector<std::wstring> mn, sn; for (auto& d : mics) mn.push_back(d.name); for (auto& d : spks) sn.push_back(d.name);
    Group(0, X - 14, Y - 16, 656, 88, L"Devices");
    Label(0, L"Microphone", X, Y - 2, 90);
    Check(0, L"On", X + 94, Y - 3, 46, [] { return g_micEnabled ? 1 : 0; }, [](int v) { g_micEnabled = v != 0; ApplyDeviceEnables(); });
    Combo(0, X, Y + 14, 302, mn, [] { return DevIndex(mics, g_savedMicId); },
          [](int i) { if (i < 0 || i >= (int)mics.size()) return; g_savedMicId = mics[i].id; SaveDevices(); if (g_micEnabled) { orb::SetDevices(g_savedMicId.c_str(), g_spkEnabled ? g_savedSpkId.c_str() : L""); if (orb::Running()) orb::Restart(); } });
    Label(0, L"Speakers", X + 320, Y - 2, 70);
    Check(0, L"On", X + 394, Y - 3, 46, [] { return g_spkEnabled ? 1 : 0; }, [](int v) { g_spkEnabled = v != 0; ApplyDeviceEnables(); });
    Combo(0, X + 320, Y + 14, 302, sn, [] { return DevIndex(spks, g_savedSpkId); },
          [](int i) { if (i < 0 || i >= (int)spks.size()) return; g_savedSpkId = spks[i].id; SaveDevices(); if (g_spkEnabled) { orb::SetDevices(g_micEnabled ? g_savedMicId.c_str() : L"", g_savedSpkId.c_str()); if (orb::Running()) orb::Restart(); } });
    Check(0, L"Hear myself", X, Y + 46, 110, Gi("monitor"), Si("monitor"));
    Check(0, L"Wide echo in headphones", X + 130, Y + 46, 180, Gi("wide"), Si("wide"));
    Check(0, L"Raw mic", X + 330, Y + 46, 90, Gi("rawMic"), Si("rawMic"));
    Check(0, L"Card mic sound", X + 440, Y + 46, 140, Gi("cardMic"), Si("cardMic"));
    Check(0, L"Cable only (mute monitor)", X, Y + 66, 200, Gi("monMute"), [](int v) { orb::SetI("monMute", v); });
    Group(0, X - 14, Y + 84, 656, 188, L"Microphone");
    Label(0, L"Mic level", X, Y + 108, 80);
    Slider(0, X + 90, Y + 104, 440, 0, 1000, [] { return g_micBase; }, [](int v) { g_micBase = v; }, Pct);
    Label(0, L"Noise isolation", X, Y + 142, 100);
    Check(0, L"On", X + 100, Y + 141, 50, Gi("isolate"), Si("isolate"));
    std::vector<std::wstring> modes; for (int i = 0; i < orb::IsoModeCount(); i++) modes.push_back(orb::IsoModeName(i));
    Combo(0, X + 152, Y + 138, 140, modes, [] { int m = orb::GetI("isoMode"); for (int i = 0; i < orb::IsoModeCount(); i++) if (orb::IsoModeOfSlot(i) == m) return i; return 0; },
          [](int i) { orb::SetI("isoMode", orb::IsoModeOfSlot(i)); });
    Label(0, L"Strength", X + 320, Y + 142, 56);
    Slider(0, X + 380, Y + 138, 170, 0, 100, [] { return orb::GetI(orb::GetI("isoMode") == orb::IsoModeOfSlot(0) ? "gateLevel" : "isoLevel"); },
           [](int v) { orb::SetI(orb::GetI("isoMode") == orb::IsoModeOfSlot(0) ? "gateLevel" : "isoLevel", v); }, Num);
    Label(0, L"Echo on", X, Y + 176, 60);
    Check(0, L"Your voice", X + 100, Y + 175, 96, Gbit("echoOn", 1), Sbit("echoOn", 1));
    Check(0, L"Sound clips", X + 210, Y + 175, 96, Gbit("echoOn", 2), Sbit("echoOn", 2));
    Check(0, L"Program sound", X + 320, Y + 175, 110, Gbit("echoOn", 4), Sbit("echoOn", 4));
    const char* gk[4] = { "gateLo", "gateMid", "gateHi", "gateOut" }; const wchar_t* gl[4] = { L"Vocal low", L"Vocal mid", L"Vocal high", L"Vocal output" };
    for (int i = 0; i < 4; i++) {
        const int cx = X + (i % 2) * 320, cy = Y + 208 + (i / 2) * 30;
        Label(0, gl[i], cx, cy + 4, 78); Slider(0, cx + 78, cy, 160, -120, 120, Gi(gk[i]), Si(gk[i]), Num);
    }
    Group(0, X - 14, Y + 284, 656, 112, L"Status");
    Label(0, L"Input level", X, Y + 308, 80); Meter(0, X + 90, Y + 308, 440, 14);
    Text(0, X, Y + 332, 620, 16, [] { wchar_t st[200] = L""; orb::Status(st, 200); return std::wstring(orb::Running() ? L"Audio engine on - " : L"Audio engine off - ") + (st[0] ? st : L"starts when you change something"); });
    Button(0, L"Restart audio", X, Y + 356, 120, 24, [] { EngineDevices(); if (orb::Running()) orb::Restart(); else EngineStart(); });
    Button(0, L"Install VB-CABLE...", X + 128, Y + 356, 150, 24, [] { orb::InstallCable(win); });
    Button(0, L"Rescan devices", X + 286, Y + 356, 112, 24, [] { mics = orb::Mics(); spks = orb::Speakers(); MessageBoxW(win, L"Close and reopen this window to refresh the device lists.", L"Audio Configuration", MB_OK | MB_ICONINFORMATION); });
    Check(0, L"Keep in system tray when closed", X + 412, Y + 359, 220, [] { return g_trayClose ? 1 : 0; }, [](int v) { g_trayClose = v != 0; });

    Group(1, X - 14, Y - 16, 340, 270, L"Voice mask");
    Label(1, L"Apply to", X, Y + 4, 50);
    Check(1, L"Your voice", X + 50, Y + 3, 84, Gbit("changeOn", 1), Sbit("changeOn", 1));
    Check(1, L"Sound clips", X + 136, Y + 3, 86, Gbit("changeOn", 2), Sbit("changeOn", 2));
    Check(1, L"Program sound", X + 224, Y + 3, 104, Gbit("changeOn", 4), Sbit("changeOn", 4));
    std::vector<std::wstring> xfi, own; for (int i = 0; i < orb::XfiMaskCount(); i++) xfi.push_back(orb::MaskName(i)); for (int k = 0; k < orb::OwnMaskCount(); k++) own.push_back(orb::MaskName(orb::OwnMask(k)));
    Label(1, L"X-Fi masks", X, Y + 30, 150, true); Label(1, L"Octopus masks", X + 162, Y + 30, 150, true);
    List(1, X, Y + 50, 150, 190, xfi, [] { int m = orb::GetI("voiceMask"); return m < orb::XfiMaskCount() ? m : -1; }, [](int i) { orb::PickMask(i); });
    List(1, X + 162, Y + 50, 160, 190, own, [] { int m = orb::GetI("voiceMask"); for (int k = 0; k < orb::OwnMaskCount(); k++) if (orb::OwnMask(k) == m) return k; return -1; }, [](int k) { orb::PickMask(orb::OwnMask(k)); });
    Group(1, X - 14, Y + 266, 340, 118, L"Tone");
    Check(1, L"On", X + 272, Y + 258, 50, Gi("toneOn"), Si("toneOn"));
    Label(1, L"Deeper / sharper", X, Y + 290, 104); Slider(1, X + 106, Y + 286, 120, -100, 100, Gi("tonePadX"), [](int v) { orb::SetTonePad(v, orb::GetI("tonePadY")); }, Num);
    Label(1, L"Less / more middle", X, Y + 320, 104); Slider(1, X + 106, Y + 316, 120, -100, 100, Gi("tonePadY"), [](int v) { orb::SetTonePad(orb::GetI("tonePadX"), v); }, Num);
    Text(1, X, Y + 352, 210, 16, [] { return L"Low " + std::to_wstring(orb::GetI("toneLo") / 10) + L"  Mid " + std::to_wstring(orb::GetI("toneMid") / 10) + L"  High " + std::to_wstring(orb::GetI("toneHi") / 10) + L" dB"; });
    Button(1, L"Flat tone", X + 222, Y + 348, 100, 22, [] { orb::ToneFlat(); });
    const int SX = X + 348;
    Group(1, SX - 14, Y - 16, 308, 320, L"Fine tune");
    Label(1, L"Pitch", SX, Y + 8, 70); Slider(1, SX + 70, Y + 4, 140, -150, 150, Gi("pitchHz"), [](int v) { if (v > -6 && v < 6) v = 0; orb::SetI("pitchHz", v); }, [](int v) { return std::to_wstring(v) + L" Hz"; });
    Label(1, L"Size", SX, Y + 42, 70); Slider(1, SX + 70, Y + 38, 140, 60, 150, Gi("voiceSize"), [](int v) { if (v > 96 && v < 104) v = 100; orb::SetI("voiceSize", v); }, Pct);
    Label(1, L"Voice changer", SX, Y + 76, 70); Slider(1, SX + 70, Y + 72, 140, -100, 100, Gi("voiceMacro"), [](int v) { orb::ApplyVoiceMacro(v); }, Num);
    Label(1, L"Speed", SX, Y + 110, 70); Slider(1, SX + 70, Y + 106, 140, 50, 200, Gi("voiceSpeed"), [](int v) { v = (v + 2) / 5 * 5; orb::SetI("voiceSpeed", v < 50 ? 50 : v > 200 ? 200 : v); }, Pct);
    Check(1, L"Keep pitch when changing speed", SX, Y + 144, 270, Gi("voiceKeepPitch"), Si("voiceKeepPitch"));
    Label(1, L"Glitch", SX, Y + 182, 100, true);
    std::vector<std::wstring> gs; for (int i = 0; i < orb::GlitchStyleCount(); i++) gs.push_back(orb::GlitchStyleName(i));
    Combo(1, SX, Y + 202, 140, gs, Gi("glitchStyle"), Si("glitchStyle"));
    Hold(1, L"Hold to repeat", SX + 150, Y + 200, 120, 24, [](int down) { orb::SetI("glitchHold", down); });
    Check(1, L"Glitch key armed", SX, Y + 236, 200, Gi("glitchArmed"), Si("glitchArmed"));
    Button(1, L"Default", SX, Y + 268, 120, 24, [] { orb::ResetVoice(); });
    Button(1, L"Reset mask", SX + 130, Y + 268, 120, 24, [] { orb::PickMask(orb::GetI("voiceMask")); orb::SetI("voiceSpeed", 100); });

    Group(2, X - 14, Y - 16, 656, 236, L"Program on the mic");
    Label(2, L"Program", X, Y + 10, 60);
    Ctl& pc = Combo(2, X + 70, Y + 6, 470, { L"(none)" }, [] { return orb::ProgramSel(); }, [](int i) { orb::PickProgram(i); orb::AppRepark(); });
    pc.drop = [&pc] { FillPrograms(pc.h); SendMessageW(pc.h, CB_SETCURSEL, orb::ProgramSel(), 0); };
    FillPrograms(pc.h);
    Text(2, X + 70, Y + 36, 540, 16, [] { return orb::AppNote(); });
    Label(2, L"Level", X, Y + 74, 60);
    Slider(2, X + 70, Y + 68, 440, 0, 1000, Gi("appGain"), Si("appGain"), Num);
    for (int v : { 100, 500, 1000 }) Label(2, std::to_wstring(v).c_str(), X + 70 + 6 + 434 * v / 1000 - 12, Y + 92, 40);
    Check(2, L"Only I hear it", X + 70, Y + 126, 200, Gi("appOnly"), [](int v) { orb::SetI("appOnly", v); orb::AppRepark(); });
    Check(2, L"Mute from monitor", X + 290, Y + 126, 200, Gi("appMute"), [](int v) { orb::SetI("appMute", v); });
    Radio(2, L"Hear it through the card", X + 70, Y + 158, 220, Gi("appHear"), [](int) { orb::SetI("appHear", 1); orb::Restart(); orb::AppRepark(); });
    Radio(2, L"Hear it from the app", X + 310, Y + 158, 220, [] { return orb::GetI("appHear") ? 0 : 1; }, [](int) { orb::SetI("appHear", 0); orb::Restart(); orb::AppRepark(); });
    Text(2, X, Y + 190, 620, 30, [] { return std::wstring(L"Its sound goes into your mic. With Only I hear it, the sound stays in your headphones and never reaches the mic."); });
}

inline void BuildFrame() {
    Px ti = ToPx(bmpTitlebar.get()), bo = ToPx(bmpBottombar.get()), ct = ToPx(bmpContent[0].get());
    Px ts = ToPx(LoadPng(L"assets\\Official_Live\\tabstrip_effects.png").get());
    frameArt.w = W; frameArt.h = H; frameArt.d.assign((size_t)W * H, 0xFF000000u);
    auto lerp = [](uint32_t a, uint32_t b, double t) {
        auto ch = [&](int s) { return (uint32_t)((((a >> s) & 255) * (1 - t) + ((b >> s) & 255) * t) + 0.5); };
        return 0xFF000000u | (ch(16) << 16) | (ch(8) << 8) | ch(0);
    };
    for (int y = 0; y < TITLE && ti.w == 500; y++) for (int x = 0; x < W; x++)
        frameArt.at(x, y) = x < 195 ? ti.at(x, y) : x >= W - 130 ? ti.at(x - (W - 500), y) : ti.at(193, y);
    for (int y = 0; y < TABS && ts.w == 500; y++) for (int x = 0; x < W; x++)
        frameArt.at(x, TITLE + y) = x < 8 ? ts.at(x, y) : x >= W - 8 ? ts.at(x - (W - 500), y) : ts.at(330, y);
    const int cTop = CONTENT_TOP, cBot = H - BOTTOM, ch = cBot - cTop;
    for (int y = cTop; y < cBot; y++) for (int x = 0; x < W; x++) {
        const int sy = std::min(237, (y - cTop) * 238 / std::max(1, ch));
        uint32_t c = PANEL | 0xFF000000u;
        c = 0xFF000000u | ((PANEL & 255) << 16) | (((PANEL >> 8) & 255) << 8) | ((PANEL >> 16) & 255);
        if (ct.w == 500) { if (x < 12) c = ct.at(x, sy); else if (x >= W - 12) c = ct.at(x - (W - 500), sy); }
        frameArt.at(x, y) = c;
    }
    for (int y = 0; y < BOTTOM && bo.w == 500; y++) for (int x = 0; x < W; x++) {
        uint32_t c;
        if (x < 18) c = bo.at(x, y); else if (x >= W - 150) c = bo.at(x - (W - 500), y);
        else c = lerp(bo.at(18, y), bo.at(300, y), std::min(1.0, (x - 18) / (0.42 * W)));
        frameArt.at(x, cBot + y) = c;
    }
    frameBmp = FromPx(frameArt);
}
inline void ApplyShape() {
    HRGN rgn = CreateRectRgn(0, 16, W, H - 16);
    auto row = [&](int y) {
        int l = 0, r = W;
        while (l < W / 2 && (frameArt.at(l, y) & 0xFFFFFF) == 0) l++;
        while (r > W / 2 && (frameArt.at(r - 1, y) & 0xFFFFFF) == 0) r--;
        HRGN t = CreateRectRgn(l, y, r, y + 1); CombineRgn(rgn, rgn, t, RGN_OR); DeleteObject(t);
    };
    for (int y = 0; y < 16; y++) { row(y); row(H - 16 + y); }
    SetWindowRgn(win, rgn, TRUE);
}

inline void TabLayout() {
    HDC dc = GetDC(win); HGDIOBJ o = SelectObject(dc, SknFont(true)); int x = 14;
    for (int i = 0; i < NPAGES; i++) { SIZE sz; GetTextExtentPoint32W(dc, kPages[i], (int)wcslen(kPages[i]), &sz); tabRc[i] = { x, TITLE + 2, x + sz.cx + 36, TITLE + TABS }; x += sz.cx + 36; }
    SelectObject(dc, o); ReleaseDC(win, dc);
}
inline void PaintWindow(HDC dc) {
    HDC mem = CreateCompatibleDC(dc); HBITMAP bm = CreateCompatibleBitmap(dc, W, H); HGDIOBJ old = SelectObject(mem, bm);
    {
        Graphics g(mem); g.SetInterpolationMode(InterpolationModeNearestNeighbor); g.SetPixelOffsetMode(PixelOffsetModeNone);
        if (frameBmp) g.DrawImage(frameBmp.get(), Rect(0, 0, W, H), 0, 0, W, H, UnitPixel);
        for (int i = CH_CLOSE; i <= CH_MIN; i++) {
            int s = pressBtn == i && hotBtn == i ? 1 : (pressBtn < 0 && hotBtn == i ? 0 : -1);
            if (s >= 0 && bmpChrome[i][s]) g.DrawImage(bmpChrome[i][s].get(), Rect(kChromeBtn[i].x + W - 500, kChromeBtn[i].y, kChromeBtn[i].w, kChromeBtn[i].h), 0, 0, kChromeBtn[i].w, kChromeBtn[i].h, UnitPixel);
        }
        Pen sep(Color(255, 150, 152, 156)); SolidBrush selB(Color(255, 252, 250, 252)), hotB(Color(255, 240, 246, 252));
        for (int i = 0; i < NPAGES; i++) {
            const RECT& r = tabRc[i];
            if (i == curPage) { g.FillRectangle(&selB, (INT)r.left, (INT)r.top, (INT)(r.right - r.left), (INT)(r.bottom - r.top)); Pen top(Color(255, 0, 120, 215), 2.0f); g.DrawLine(&top, (REAL)r.left, 39.0f, (REAL)r.right, 39.0f); }
            else if (i == hotTab) g.FillRectangle(&hotB, (INT)r.left, (INT)r.top, (INT)(r.right - r.left), (INT)(r.bottom - r.top));
            g.DrawLine(&sep, (INT)r.right - 1, (INT)r.top, (INT)r.right - 1, (INT)r.bottom);
        }
        Pen box(Color(255, 188, 184, 188));
        for (const Grp& gp : grps) if (gp.page == curPage) g.DrawRectangle(&box, gp.x, gp.y, gp.w, gp.h);
        HDC t = g.GetHDC();
        DrawSknText(t, L"Audio Configuration", 205, 13, 120, 16, DT_SINGLELINE, RGB(64, 64, 64), true);
        for (int i = 0; i < NPAGES; i++) DrawSknText(t, kPages[i], tabRc[i].left + 18, tabRc[i].top + 4, tabRc[i].right - tabRc[i].left, 16, DT_SINGLELINE, i == curPage ? RGB(0, 60, 150) : RGB(0, 0, 0), i == curPage);
        for (const Grp& gp : grps) if (gp.page == curPage) {
            RECT gap = { gp.x + 8, gp.y - 1, gp.x + 16 + (int)gp.title.size() * 6, gp.y + 2 }; FillRect(t, &gap, panelBrush());
            DrawSknText(t, gp.title.c_str(), gp.x + 12, gp.y - 7, 200, 14, DT_SINGLELINE, RGB(0, 0, 0), true);
        }
        g.ReleaseHDC(t);
    }
    BitBlt(dc, 0, 0, W, H, mem, 0, 0, SRCCOPY); SelectObject(mem, old); DeleteObject(bm); DeleteDC(mem);
}
inline Ctl* BySlider(HWND h) { for (auto& c : ctls) if (c->h == h && c->kind == K_SLIDER) return c.get(); return nullptr; }

inline void DrawSliderCtl(Ctl* c, HDC dc) {
    RECT rc; GetClientRect(c->h, &rc); FillRect(dc, &rc, panelBrush());
    const int pos = (int)SendMessageW(c->h, TBM_GETPOS, 0, 0), span = std::max(1, c->mx - c->mn);
    const bool drag = GetCapture() == c->h;
    Graphics g(dc); g.SetInterpolationMode(InterpolationModeNearestNeighbor); g.SetPixelOffsetMode(PixelOffsetModeNone);
    if (!c->vertical) {
        DrawCreativeSlider(g, bmpSldrVolume.get(), 0, (rc.bottom - 14) / 2, rc.right, pos * 100 / span, true, drag);
    } else {
        Bitmap* sh = bmpSldrVert.get(); if (!sh || sh->GetLastStatus() != Ok) return;
        const int h = rc.bottom, x = (rc.right - 14) / 2, travel = h - 24, ty = pos * travel / span;
        g.DrawImage(sh, Rect(x, 0, 14, 12), 0, 0, 14, 12, UnitPixel);
        g.DrawImage(sh, Rect(x, 12, 14, h - 24), 0, 12, 14, 72, UnitPixel);
        g.DrawImage(sh, Rect(x, h - 12, 14, 12), 0, 84, 14, 12, UnitPixel);
        ImageAttributes ia; ia.SetColorKey(Color(240, 0, 240), Color(255, 16, 255));
        g.DrawImage(sh, Rect(x, ty, 14, 24), 0, 288 + (drag ? 2 : 0) * 24, 14, 24, UnitPixel, &ia);
    }
}
inline void DrawItem(DRAWITEMSTRUCT* d) {
    Ctl* c = ById((int)d->CtlID); if (!c) return;
    HDC dc = d->hDC; RECT r = d->rcItem;
    if (c->kind == K_CHECK || c->kind == K_RADIO) {
        FillRect(dc, &r, panelBrush()); Graphics g(dc); g.SetPixelOffsetMode(PixelOffsetModeNone);
        const int y0 = (r.bottom - r.top - (c->kind == K_RADIO ? 12 : 14)) / 2;
        if (c->kind == K_CHECK) DrawCreativeCheck(g, 0, y0, c->shown > 0, true);
        else if (bmpRadioSheet && bmpRadioSheet->GetLastStatus() == Ok) g.DrawImage(bmpRadioSheet.get(), Rect(0, y0, 12, 12), 0, c->shown > 0 ? 12 : 0, 12, 12, UnitPixel);
        DrawSknText(dc, c->caption.c_str(), 22, 0, r.right - 22, r.bottom, DT_SINGLELINE | DT_VCENTER, RGB(0, 0, 0));
        if (d->itemState & ODS_FOCUS) { RECT f = { 20, 1, r.right - 1, r.bottom - 1 }; DrawFocusRect(dc, &f); }
    } else if (c->kind == K_BUTTON || c->kind == K_HOLD) {
        const bool down = (d->itemState & ODS_SELECTED) != 0;
        FillRect(dc, &r, panelBrush());
        Graphics g(dc); g.SetSmoothingMode(SmoothingModeNone);
        LinearGradientBrush lb(Point(0, (INT)r.top), Point(0, (INT)r.bottom), down ? Color(255, 205, 205, 208) : Color(255, 255, 255, 255), down ? Color(255, 248, 248, 250) : Color(255, 205, 207, 212));
        g.FillRectangle(&lb, (INT)r.left + 1, (INT)r.top + 1, (INT)(r.right - r.left) - 2, (INT)(r.bottom - r.top) - 2);
        Pen bd(Color(255, 96, 98, 102)); g.DrawRectangle(&bd, (INT)r.left, (INT)r.top, (INT)(r.right - r.left) - 1, (INT)(r.bottom - r.top) - 1);
        Pen hi(Color(255, 255, 255, 255)); if (!down) g.DrawLine(&hi, (INT)r.left + 1, (INT)r.top + 1, (INT)r.right - 2, (INT)r.top + 1);
        RECT t = r; if (down) OffsetRect(&t, 1, 1);
        DrawSknText(dc, c->caption.c_str(), t.left, t.top, t.right - t.left, t.bottom - t.top, DT_SINGLELINE | DT_VCENTER | DT_CENTER, RGB(0, 0, 0));
        if (d->itemState & ODS_FOCUS) { RECT f = { r.left + 3, r.top + 3, r.right - 3, r.bottom - 3 }; DrawFocusRect(dc, &f); }
    } else if (c->kind == K_METER) {
        FillRect(dc, &r, (HBRUSH)GetStockObject(WHITE_BRUSH)); FrameRect(dc, &r, (HBRUSH)GetStockObject(GRAY_BRUSH));
        RECT f = { r.left + 1, r.top + 1, r.left + 1 + std::max<LONG>(0, (r.right - r.left - 2) * std::clamp(c->shown, 0, 100) / 100), r.bottom - 1 };
        Graphics g(dc); if (f.right > f.left) { LinearGradientBrush lb(Point((INT)f.left, 0), Point((INT)r.right, 0), Color(255, 0, 181, 255), Color(255, 0, 74, 255)); g.FillRectangle(&lb, (INT)f.left, (INT)f.top, (INT)(f.right - f.left), (INT)(f.bottom - f.top)); }
    } else if (c->kind == K_LIST || c->kind == K_COMBO) {
        if ((int)d->itemID < 0) return;
        wchar_t s[256] = L""; SendMessageW(c->h, c->kind == K_LIST ? LB_GETTEXT : CB_GETLBTEXT, d->itemID, (LPARAM)s);
        const bool sel = (d->itemState & ODS_SELECTED) != 0, inEdit = (d->itemState & ODS_COMBOBOXEDIT) != 0;
        const bool dark = c->kind == K_COMBO;
        HBRUSH b = CreateSolidBrush(sel && !(inEdit && !dark) ? (dark ? RGB(64, 64, 64) : RGB(0, 120, 212)) : (inEdit ? RGB(250, 248, 250) : (d->itemID & 1) ? RGB(249, 245, 234) : RGB(255, 251, 240)));
        FillRect(dc, &r, b); DeleteObject(b);
        DrawSknText(dc, s, r.left + 4, r.top, r.right - r.left - 6, r.bottom - r.top, DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS, sel && !(inEdit && !dark) ? RGB(255, 255, 255) : RGB(64, 64, 64));
    }
}
inline void ShowPage(int p) {
    curPage = p;
    for (auto& c : ctls) { int s = (c->page == p || c->page < 0) ? SW_SHOW : SW_HIDE; ShowWindow(c->h, s); if (c->val) ShowWindow(c->val, s); }
    InvalidateRect(win, nullptr, FALSE);
}
inline void Refresh() {
    building = true;
    for (auto& c : ctls) {
        if (c->page >= 0 && c->page != curPage) continue;
        switch (c->kind) {
        case K_CHECK: case K_RADIO: { int v = c->get(); if (v != c->shown) { c->shown = v; InvalidateRect(c->h, nullptr, FALSE); } break; }
        case K_SLIDER: {
            int v = c->get(); if (v != c->shown) { SendMessageW(c->h, TBM_SETPOS, TRUE, v - c->mn); c->shown = v; if (c->val) SetWindowTextW(c->val, c->fmt(v).c_str()); InvalidateRect(c->h, nullptr, FALSE); }
            break;
        }
        case K_COMBO: { int v = c->get(); if (v != c->shown) { SendMessageW(c->h, CB_SETCURSEL, v, 0); c->shown = v; } break; }
        case K_LIST: { int v = c->get(); if (v != c->shown) { SendMessageW(c->h, LB_SETCURSEL, v, 0); c->shown = v; } break; }
        case K_METER: {
            int peak = orb::GetI("meterPeak"); orb::SetI("meterPeak", 0); int v = std::min(100, (int)((float)peak / 10000.0f * 1.6f * 100.0f));
            int cur = std::max(0, c->shown); int nv = v > cur ? v : std::max(v, cur - 6); if (nv != c->shown) { c->shown = nv; InvalidateRect(c->h, nullptr, FALSE); } break;
        }
        case K_TEXT: { std::wstring t = c->text(); wchar_t old[400] = L""; GetWindowTextW(c->h, old, 400); if (t != old) SetWindowTextW(c->h, t.c_str()); break; }
        }
    }
    building = false;
}
inline void Touched() { EngineStart(); }
inline int TabAt(int x, int y) { for (int i = 0; i < NPAGES; i++) if (x >= tabRc[i].left && x < tabRc[i].right && y >= tabRc[i].top && y < tabRc[i].bottom) return i; return -1; }
inline int ChromeAt(int x, int y) { for (int i = CH_CLOSE; i <= CH_MIN; i++) if (x >= kChromeBtn[i].x + W - 500 && x < kChromeBtn[i].x + W - 500 + kChromeBtn[i].w && y >= kChromeBtn[i].y && y < kChromeBtn[i].y + kChromeBtn[i].h) return i; return -1; }

inline LRESULT CALLBACK Proc(HWND h, UINT m, WPARAM w, LPARAM l) {
    switch (m) {
    case WM_ERASEBKGND: return 1;
    case WM_PAINT: { PAINTSTRUCT ps; HDC dc = BeginPaint(h, &ps); PaintWindow(dc); EndPaint(h, &ps); return 0; }
    case WM_CTLCOLORSTATIC: { HDC dc = (HDC)w; SetBkColor(dc, PANEL); SetTextColor(dc, RGB(0, 0, 0)); return (LRESULT)panelBrush(); }
    case WM_CTLCOLORLISTBOX: case WM_CTLCOLOREDIT: return (LRESULT)GetStockObject(WHITE_BRUSH);
    case WM_DRAWITEM: DrawItem((DRAWITEMSTRUCT*)l); return TRUE;
    case WM_NOTIFY: {
        NMHDR* nh = (NMHDR*)l;
        if (nh->code == NM_CUSTOMDRAW) {
            if (Ctl* c = BySlider(nh->hwndFrom)) {
                NMCUSTOMDRAW* cd = (NMCUSTOMDRAW*)l;
                if (cd->dwDrawStage == CDDS_PREPAINT) { DrawSliderCtl(c, cd->hdc); return CDRF_SKIPDEFAULT; }
            }
        }
        return 0;
    }
    case WM_NCHITTEST: {
        POINT p = { GET_X_LPARAM(l), GET_Y_LPARAM(l) }; ScreenToClient(h, &p);
        if (p.y < TITLE && ChromeAt(p.x, p.y) < 0) return HTCAPTION;
        return HTCLIENT;
    }
    case WM_LBUTTONDOWN: {
        int x = GET_X_LPARAM(l), y = GET_Y_LPARAM(l), t = TabAt(x, y), b = ChromeAt(x, y);
        if (t >= 0 && t != curPage) { ShowPage(t); Refresh(); }
        if (b >= 0) { pressBtn = hotBtn = b; SetCapture(h); InvalidateRect(h, nullptr, FALSE); }
        return 0;
    }
    case WM_MOUSEMOVE: {
        int x = GET_X_LPARAM(l), y = GET_Y_LPARAM(l), t = TabAt(x, y), b = ChromeAt(x, y);
        if (t != hotTab || b != hotBtn) { hotTab = t; hotBtn = pressBtn >= 0 ? (b == pressBtn ? b : -1) : b; InvalidateRect(h, nullptr, FALSE); }
        TRACKMOUSEEVENT te = { sizeof(te), TME_LEAVE, h, 0 }; TrackMouseEvent(&te);
        return 0;
    }
    case WM_MOUSELEAVE: if (pressBtn < 0) { hotTab = -1; hotBtn = -1; InvalidateRect(h, nullptr, FALSE); } return 0;
    case WM_LBUTTONUP: {
        if (pressBtn >= 0) {
            int b = pressBtn, over = ChromeAt(GET_X_LPARAM(l), GET_Y_LPARAM(l)); pressBtn = -1; ReleaseCapture(); InvalidateRect(h, nullptr, FALSE);
            if (over == b) { if (b == CH_CLOSE) DestroyWindow(h); else ShowWindow(h, SW_MINIMIZE); }
        }
        return 0;
    }
    case WM_COMMAND: {
        if (building) return 0;
        Ctl* c = ById(LOWORD(w)); if (!c) return 0;
        const int note = HIWORD(w);
        if (c->kind == K_CHECK && note == BN_CLICKED) { Touched(); c->set(c->get() ? 0 : 1); c->shown = INT_MIN; Refresh(); }
        else if (c->kind == K_RADIO && note == BN_CLICKED) { Touched(); if (!c->get()) c->set(1); for (auto& o : ctls) if (o->kind == K_RADIO) o->shown = INT_MIN; Refresh(); }
        else if (c->kind == K_COMBO && note == CBN_DROPDOWN) { if (c->drop) c->drop(); }
        else if (c->kind == K_COMBO && note == CBN_SELCHANGE) { Touched(); c->set((int)SendMessageW(c->h, CB_GETCURSEL, 0, 0)); c->shown = INT_MIN; }
        else if (c->kind == K_LIST && note == LBN_SELCHANGE) { Touched(); c->set((int)SendMessageW(c->h, LB_GETCURSEL, 0, 0)); c->shown = INT_MIN; }
        else if (c->kind == K_BUTTON && note == BN_CLICKED) c->act();
        return 0;
    }
    case WM_HSCROLL: case WM_VSCROLL: {
        if (building) return 0;
        if (Ctl* c = BySlider((HWND)l)) { Touched(); int v = (int)SendMessageW(c->h, TBM_GETPOS, 0, 0) + c->mn; c->set(v); c->shown = INT_MIN; Refresh(); InvalidateRect(c->h, nullptr, FALSE); }
        return 0;
    }
    case WM_TIMER: Refresh(); return 0;
    case WM_CLOSE: DestroyWindow(h); return 0;
    case WM_DESTROY: KillTimer(h, 1); ctls.clear(); grps.clear(); win = nullptr; return 0;
    }
    return DefWindowProcW(h, m, w, l);
}

inline void Open(HWND owner) {
    if (win) { ShowWindow(win, SW_RESTORE); SetForegroundWindow(win); return; }
    INITCOMMONCONTROLSEX ic = { sizeof(ic), ICC_BAR_CLASSES | ICC_STANDARD_CLASSES }; InitCommonControlsEx(&ic);
    static bool reg = false;
    if (!reg) { WNDCLASSEXW wc = { sizeof(wc) }; wc.lpfnWndProc = Proc; wc.hInstance = GetModuleHandleW(nullptr); wc.lpszClassName = L"EAXConfig"; wc.hCursor = LoadCursor(nullptr, IDC_ARROW); RegisterClassExW(&wc); reg = true; }
    RECT pr; GetWindowRect(owner, &pr);
    win = CreateWindowExW(WS_EX_APPWINDOW, L"EAXConfig", L"Audio Configuration", WS_POPUP | WS_MINIMIZEBOX | WS_CLIPCHILDREN, std::max<LONG>(0, pr.left - 150), std::max<LONG>(0, pr.top - 30), W, H, nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
    BuildFrame(); ApplyShape(); TabLayout();
    { std::wstring m, s; orb::CurrentDevices(m, s); if (m.empty() || s.empty()) EngineDevices(); }
    ctls.clear(); grps.clear(); BuildPages(); ShowPage(0); Refresh();
    SetTimer(win, 1, 150, nullptr);
    ShowWindow(win, SW_SHOW); UpdateWindow(win);
}

}
