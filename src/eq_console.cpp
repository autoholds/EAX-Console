#ifndef UNICODE
#define UNICODE
#endif
#include <windows.h>
#include <windowsx.h>
#include <gdiplus.h>
#include <shellapi.h>
#include <commctrl.h>
#include <cmath>
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>
#include "shared_state.h"
using namespace Gdiplus;

static constexpr int WIN_W = 500, WIN_H = 302;
#include "portable_paths.h"
static const std::wstring kEqDir = Creative::Dir(L"Equalizer");

static constexpr int BAND_X[7] = { 79, 124, 170, 217, 263, 309, 354 };
static constexpr int BAND_Y = 104, BAND_H = 134, BAND_TRAVEL = 110;
static constexpr int LVL_X = 449, LVL_Y = 113, LVL_H = 118, LVL_TRAVEL = 94;
static constexpr int BAL_X[7] = { 96, 141, 187, 234, 280, 326, 372 };
static constexpr int BAL_Y = 166;
static constexpr RECT kCheck = { 13, 71, 112, 85 };
static constexpr RECT kSave = { 421, 67, 453, 89 }, kDelete = { 456, 67, 488, 89 };
static constexpr RECT kFace = { 258, 68, 418, 88 }, kList = { 257, 88, 417, 216 };
static constexpr RECT kDevFace = { 200, 6, 370, 27 };
static constexpr int LIST_ROWS = 9, ROW_H = 14;

struct Preset { std::wstring name; int off[7]; bool user; };
static std::vector<Preset> g_presets = {
    { L"EQ Acoustic",    { 39, 39, 55, 55, 55, 39, 30 }, false }, { L"EQ Chamber",     { 26, 39, 55, 55, 47, 39, 30 }, false },
    { L"EQ Classical",   { 26, 39, 55, 55, 55, 39, 39 }, false }, { L"EQ Disco",       { 39, 55, 55, 55, 47, 39, 30 }, false },
    { L"EQ Drum & Bass", { 39, 47, 55, 64, 47, 32, 26 }, false }, { L"EQ Flat",        { 55, 55, 55, 55, 55, 55, 55 }, false },
    { L"EQ Heavy Metal", { 39, 55, 55, 55, 39, 39, 30 }, false }, { L"EQ Jazz",        { 55, 39, 39, 39, 47, 39, 26 }, false },
    { L"EQ Latin",       { 39, 55, 39, 39, 55, 39, 26 }, false }, { L"EQ New Age",     { 32, 47, 55, 55, 55, 55, 39 }, false },
    { L"EQ Pop",         { 55, 39, 39, 55, 55, 39, 30 }, false }, { L"EQ Rock",        { 55, 39, 39, 47, 55, 55, 30 }, false },
    { L"EQ Techno",      { 39, 47, 63, 64, 48, 39, 24 }, false }, { L"EQ Vocal",       { 55, 30, 32, 32, 55, 55, 39 }, false },
};

static bool g_enabled = false;
static int g_band[7] = { 55, 55, 55, 55, 55, 55, 55 };
static int g_levelS = 0;
static int LevelPos(int s) { return (s * 94 + 99) / 100; }
#define g_level LevelPos(g_levelS)
static int g_preset = 5;
static int g_drag = -1, g_dragGrab = 0;
static int g_balPressed = -1;
static bool g_savePressed = false, g_deletePressed = false;
static bool g_listOpen = false, g_comboFocus = false;
static int g_listTop = 0, g_listHot = 0;

static bool g_hasCustom = false;
static int g_customOff[7] = { 55, 55, 55, 55, 55, 55, 55 };
static bool g_devOpen = false, g_devFocus = false;
enum { BT_NONE, BT_CLOSE, BT_MIN, BT_HELP, BT_HOWDOI, BT_DEFAULT, BT_APP1, BT_APP2, BT_APP3, BT_APP4, BT_COUNT };
static int g_btnPressed = BT_NONE, g_btnHot = BT_NONE;
static const struct { int x, y, w, h; const wchar_t* name; } kBtn[BT_COUNT] = {
    { 0, 0, 0, 0, nullptr }, { 470, 6, 22, 21, L"close" }, { 446, 6, 20, 21, L"min" }, { 381, 6, 32, 20, L"help" },
    { 413, 6, 15, 20, L"howdoi" }, { 18, 275, 90, 22, L"default" }, { 118, 272, 32, 24, L"app1" }, { 150, 272, 32, 24, L"app2" },
    { 182, 272, 32, 24, L"app3" }, { 214, 272, 32, 24, L"app4" },
};
static bool InBtn(int i, int x, int y) { return x >= kBtn[i].x && x < kBtn[i].x + kBtn[i].w && y >= kBtn[i].y && y < kBtn[i].y + kBtn[i].h; }
static bool InR(const RECT& r, int x, int y) { return x >= r.left && x < r.right && y >= r.top && y < r.bottom; }

static HINSTANCE g_hInst; static HWND g_hWnd; static ULONG_PTR g_gdipToken;

struct Px { int w = 0, h = 0; std::vector<uint32_t> d; uint32_t& at(int x, int y) { return d[y * w + x]; } };
static std::wstring ExeDir() { wchar_t p[MAX_PATH]; GetModuleFileNameW(nullptr, p, MAX_PATH); std::wstring s(p); return s.substr(0, s.find_last_of(L"\\/")); }
static std::unique_ptr<Bitmap> Load(const std::wstring& path) {
    std::wstring full = path.find(L':') != std::wstring::npos ? path : ExeDir() + L"\\" + path;
    auto b = std::make_unique<Bitmap>(full.c_str());
    if (b->GetLastStatus() != Ok) b = std::make_unique<Bitmap>(path.c_str());
    return b;
}
static Px ToPx(Bitmap* b) {
    Px p; if (!b || b->GetLastStatus() != Ok) return p;
    p.w = (int)b->GetWidth(); p.h = (int)b->GetHeight(); p.d.resize(p.w * p.h);
    Rect r(0, 0, p.w, p.h); BitmapData bd{}; bd.Width = p.w; bd.Height = p.h; bd.Stride = p.w * 4; bd.PixelFormat = PixelFormat32bppARGB; bd.Scan0 = p.d.data();
    b->LockBits(&r, ImageLockModeRead | ImageLockModeUserInputBuf, PixelFormat32bppARGB, &bd); b->UnlockBits(&bd);
    return p;
}
static std::unique_ptr<Bitmap> FromPx(Px& p) {
    auto b = std::make_unique<Bitmap>(p.w, p.h, PixelFormat32bppARGB);
    Rect r(0, 0, p.w, p.h); BitmapData bd{}; bd.Width = p.w; bd.Height = p.h; bd.Stride = p.w * 4; bd.PixelFormat = PixelFormat32bppARGB; bd.Scan0 = p.d.data();
    b->LockBits(&r, ImageLockModeWrite | ImageLockModeUserInputBuf, PixelFormat32bppARGB, &bd); b->UnlockBits(&bd);
    return b;
}

static std::unique_ptr<Bitmap> Q565(std::unique_ptr<Bitmap> b) {
    Px p = ToPx(b.get()); if (!p.w) return b;
    for (auto& c : p.d) {
        auto q5 = [](int v) { int q = (v * 31 + 127) / 255; return (q >> 3) * 64 + (q & 7) * 9; };
        auto q6 = [](int v) { int q = (v * 63 + 127) / 255; return (q >> 2) * 16 + (q & 3) * 5; };
        c = (c & 0xFF000000) | (q5((c >> 16) & 255) << 16) | (q6((c >> 8) & 255) << 8) | q5(c & 255);
    }
    return FromPx(p);
}

static std::unique_ptr<Bitmap> bmpBase[2], bmpFaceBlank[2], bmpSldr, bmpSldrW, bmpChk, bmpSave, bmpDelete, bmpBal;
static std::unique_ptr<Bitmap> bmpBtn[BT_COUNT][2], bmpDevOpen, bmpDevFocus, bmpAbout, bmpRowHl, bmpRow[2], bmpSbTrack, bmpSbThumb;
static Px g_menuFace[3];
static Px g_sysFace[4];
static Px g_listOpenPx;

static Px g_handMask[2];
static HCURSOR g_curHand;

static void LoadAssets() {
    std::wstring A = L"eq_assets\\", T = std::wstring(kEqDir) + L"\\Theme\\Default\\";
    bmpBase[0] = Load(A + L"base_disabled.png"); bmpBase[1] = Load(A + L"base_enabled.png");
    bmpSldr = Q565(Load(T + L"EQ_Sldr_Vertical14.bmp")); bmpSldrW = Q565(Load(T + L"EQ_Sldr_Vertical14_whitebg.bmp"));
    bmpChk = Q565(Load(T + L"EQ_Bttn_Checkbox.bmp")); bmpSave = Q565(Load(T + L"EQ_Bttn_Save.bmp"));
    bmpDelete = Q565(Load(T + L"EQ_Bttn_Delete.bmp")); bmpBal = Q565(Load(T + L"EQ_Bttn_VolBal2.bmp"));
    const wchar_t* pos[BT_COUNT] = { nullptr, L"_470_6", L"_446_6", L"_381_6", L"_413_6", L"_18_275", L"_118_272", L"_150_272", L"_182_272", L"_214_272" };
    for (int i = 1; i < BT_COUNT; i++) {
        bmpBtn[i][0] = Load(A + kBtn[i].name + L"_hover" + pos[i] + L".png");
        bmpBtn[i][1] = Load(A + kBtn[i].name + L"_pressed" + pos[i] + L".png");
    }
    bmpDevOpen = Load(A + L"device_open_200_27.png"); bmpDevFocus = Load(A + L"device_focus_203_9.png");
    bmpAbout = Load(A + L"about_box.png");
    g_handMask[0] = ToPx(Load(A + L"cursor_hand_off.png").get());
    g_handMask[1] = ToPx(Load(A + L"cursor_hand_on.png").get());
    g_curHand = (HCURSOR)LoadImageW(nullptr, (Creative::ThemeDir() + L"\\1033\\Default\\HandBlue.cur").c_str(), IMAGE_CURSOR, 0, 0, LR_LOADFROMFILE | LR_DEFAULTSIZE);
    if (!g_curHand) g_curHand = (HCURSOR)LoadImageW(nullptr, (ExeDir() + L"\\assets\\Cursors\\HandBlue.cur").c_str(), IMAGE_CURSOR, 0, 0, LR_LOADFROMFILE | LR_DEFAULTSIZE);
    const wchar_t* mn[3] = { L"menu_none.png", L"menu_howdoi.png", L"menu_about.png" };
    for (int i = 0; i < 3; i++) g_menuFace[i] = ToPx(Load(A + mn[i]).get());
    const wchar_t* sn[4] = { L"sysmenu_none.png", L"sysmenu_move.png", L"sysmenu_minimize.png", L"sysmenu_close.png" };
    for (int i = 0; i < 4; i++) g_sysFace[i] = ToPx(Load(A + sn[i]).get());

    bmpFaceBlank[0] = Load(A + L"face_enabled.png");
    bmpFaceBlank[1] = Load(A + L"face_disabled.png");

    Px lo = ToPx(Load(A + L"list_open_257_88.png").get());
    g_listOpenPx = lo;
    if (lo.w == 160) {
        auto strip = [&](int row, bool hl) {
            Px s; s.w = 140; s.h = ROW_H; s.d.resize(140 * ROW_H);
            for (int y = 0; y < ROW_H; y++) for (int x = 0; x < 140; x++) {
                uint32_t c = lo.at(1 + x, 1 + row * ROW_H + y), k = c & 0xFFFFFF;
                bool interior = x > 0 && x < 139 && y > 0 && y < 12;
                if (!hl && k == 0x404040) c = lo.at(1 + 139, 1 + row * ROW_H + y);
                if (hl && interior && k == 0xFFFFFF) c = 0xFF404040;
                if (hl && y <= 12 && (y == 0 || y == 12 || x == 0 || x == 139)) c = 0xFF404040;
                s.at(x, y) = c;
            }
            return FromPx(s);
        };
        bmpRowHl = strip(0, true);
        bmpRow[0] = strip(1, false);
        bmpRow[1] = strip(2, false);
        Px tr; tr.w = 18; tr.h = 126; tr.d.resize(18 * 126);
        Px th; th.w = 18; th.h = 59; th.d.resize(18 * 59);
        for (int y = 0; y < 126; y++) for (int x = 0; x < 18; x++) tr.at(x, y) = lo.at(141 + x, 1 + y < 50 ? 1 + y : 1 + 10);
        for (int y = 0; y < 59; y++) for (int x = 0; x < 18; x++) th.at(x, y) = lo.at(141 + x, 51 + y);
        bmpSbTrack = FromPx(tr); bmpSbThumb = FromPx(th);
    }
}

static void Text(HDC dc, const wchar_t* s, int x, int y, int w, int h, UINT fmt, COLORREF c) {
    static HFONT f = CreateFontW(-11, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, 0, 0, NONANTIALIASED_QUALITY, 0, L"Tahoma");
    HGDIOBJ o = SelectObject(dc, f); SetBkMode(dc, TRANSPARENT); SetTextColor(dc, c);
    RECT r = { x, y, x + w, y + h }; DrawTextW(dc, s, -1, &r, fmt | DT_NOPREFIX); SelectObject(dc, o);
}
static std::wstring PresetName() { return g_preset < 0 ? L"<Custom>" : g_presets[g_preset].name; }
static int CustomRows() { return g_hasCustom ? 1 : 0; }
static int DispCount() { return CustomRows() + (int)g_presets.size(); }
static int DispCurrent() { return g_preset < 0 ? 0 : g_preset + CustomRows(); }
static std::wstring DispName(int d) { return d < CustomRows() ? L"<Custom>" : g_presets[d - CustomRows()].name; }
static int ListMaxTop() { return std::max(0, DispCount() - LIST_ROWS); }

static void FocusXor(HDC dc, int x, int y, int w, int h) {
    auto px = [&](int dx, int dy) {
        COLORREF c = GetPixel(dc, x + dx, y + dy); int m = ((dx + dy) & 1) ? 0x40 : 0xFF;
        SetPixel(dc, x + dx, y + dy, RGB(GetRValue(c) ^ m, GetGValue(c) ^ m, GetBValue(c) ^ m));
    };
    for (int dx = 0; dx < w; dx++) { px(dx, 0); px(dx, h - 1); }
    for (int dy = 1; dy < h - 1; dy++) { px(0, dy); px(w - 1, dy); }
}

static void Blit(Graphics& g, Bitmap* b, int x, int y, int sx = 0, int sy = 0, int w = -1, int h = -1) {
    if (!b || b->GetLastStatus() != Ok) return;
    if (w < 0) { w = (int)b->GetWidth(); h = (int)b->GetHeight(); }
    g.DrawImage(b, Rect(x, y, w, h), sx, sy, w, h, UnitPixel);
}

static void VSlider(Graphics& g, Bitmap* sheet, int x, int y, int h, int off, bool en, int thumbFrame) {
    int sf = (en ? 0 : 2) * 96;
    g.DrawImage(sheet, Rect(x, y, 14, 12), 0, sf, 14, 12, UnitPixel);
    g.DrawImage(sheet, Rect(x, y + 12, 14, h - 24), 0, sf + 12, 14, 72, UnitPixel);
    g.DrawImage(sheet, Rect(x, y + h - 12, 14, 12), 0, sf + 84, 14, 12, UnitPixel);
    ImageAttributes ia; ia.SetColorKey(Color(240, 0, 240), Color(255, 16, 255));
    g.DrawImage(sheet, Rect(x, y + off, 14, 24), 0, 288 + (en ? thumbFrame : 3) * 24, 14, 24, UnitPixel, &ia);
}

static void Paint(HDC hdc) {
    HDC mem = CreateCompatibleDC(hdc); HBITMAP bm = CreateCompatibleBitmap(hdc, WIN_W, WIN_H); HGDIOBJ old = SelectObject(mem, bm);
    {
        Graphics g(mem);
        g.SetInterpolationMode(InterpolationModeNearestNeighbor); g.SetPixelOffsetMode(PixelOffsetModeNone);
        bool e = g_enabled;
        Blit(g, bmpBase[e ? 1 : 0].get(), 0, 0);
        for (int i = 0; i < 7; i++) {
            VSlider(g, bmpSldr.get(), BAND_X[i], BAND_Y, BAND_H, g_band[i], e, g_drag == i ? 2 : 0);
            int bf = !e ? 3 : (g_balPressed == i ? 2 : 0);
            Blit(g, bmpBal.get(), BAL_X[i], BAL_Y, bf * 5, 0, 5, 10);
        }
        VSlider(g, bmpSldrW.get(), LVL_X, LVL_Y, LVL_H, g_level, e, g_drag == 7 ? 2 : 0);
        Blit(g, bmpChk.get(), kCheck.left, kCheck.top, (e ? 0 : 0) * 15, e ? 14 : 0, 15, 14);
        Blit(g, bmpSave.get(), kSave.left, kSave.top, (!e ? 3 : (g_savePressed ? 2 : 0)) * 32, 0, 32, 22);
        bool canDel = e && g_preset >= 0 && g_presets[g_preset].user;
        Blit(g, bmpDelete.get(), kDelete.left, kDelete.top, (!canDel ? 3 : (g_deletePressed ? 2 : 0)) * 32, 0, 32, 22);
        Blit(g, bmpFaceBlank[DispCurrent() & 1].get(), 260, 70);
        bool focused = g_comboFocus && !g_listOpen && e;
        {
            HDC dc = g.GetHDC();
            if (focused) { HBRUSH b = CreateSolidBrush(RGB(64, 64, 64)); RECT r = { 260, 71, 396, 84 }; FillRect(dc, &r, b); DeleteObject(b); }
            Text(dc, PresetName().c_str(), 265, 69, 131, 17, DT_SINGLELINE | DT_VCENTER, focused ? RGB(255, 255, 255) : RGB(64, 64, 64));
            if (focused) FocusXor(dc, 260, 71, 136, 13);
            g.ReleaseHDC(dc);
        }
        auto btn = [&](int i) {
            if (g_btnPressed == i && g_btnHot == i) Blit(g, bmpBtn[i][1].get(), kBtn[i].x, kBtn[i].y);
            else if (g_btnPressed == BT_NONE && g_btnHot == i) Blit(g, bmpBtn[i][0].get(), kBtn[i].x, kBtn[i].y);
        };
        for (int i = 1; i < BT_COUNT; i++) btn(i);
        if (g_devOpen) Blit(g, bmpDevOpen.get(), 200, 27);
        else if (g_devFocus) Blit(g, bmpDevFocus.get(), 203, 9);
        if (g_listOpen) {
            HDC dummy = nullptr; (void)dummy;
            Pen border(Color(255, 100, 100, 100)); g.DrawRectangle(&border, kList.left, kList.top, 159, 127);
            for (int r = 0; r < LIST_ROWS; r++) {
                int idx = g_listTop + r; if (idx >= DispCount()) break;
                Bitmap* s = idx == g_listHot ? bmpRowHl.get() : bmpRow[idx & 1].get();
                Blit(g, s, kList.left + 1, kList.top + 1 + r * ROW_H);
            }
            Blit(g, bmpSbTrack.get(), kList.left + 141, kList.top + 1);
            {
                int th = (LIST_ROWS * 92 + DispCount() / 2) / DispCount(), half = th / 2, mt = std::max(1, ListMaxTop());
                int ty = kList.top + 18 + ((92 - th) * g_listTop + mt / 2) / mt;
                Blit(g, bmpSbThumb.get(), kList.left + 141, ty, 0, 0, 18, half);
                Blit(g, bmpSbThumb.get(), kList.left + 141, ty + half, 0, 59 - (th - half), 18, th - half);
            }
            HDC dc = g.GetHDC();
            for (int r = 0; r < LIST_ROWS; r++) {
                int idx = g_listTop + r; if (idx >= DispCount()) break;
                Text(dc, DispName(idx).c_str(), kList.left + 6, kList.top + 1 + r * ROW_H, 134, 13, DT_SINGLELINE | DT_VCENTER,
                     idx == g_listHot ? RGB(255, 255, 255) : RGB(64, 64, 64));
                if (idx == g_listHot) FocusXor(dc, kList.left + 1, kList.top + 1 + r * ROW_H, 140, 13);
            }
            g.ReleaseHDC(dc);
        }
    }
    BitBlt(hdc, 0, 0, WIN_W, WIN_H, mem, 0, 0, SRCCOPY);
    SelectObject(mem, old); DeleteObject(bm); DeleteDC(mem);
}

static void OpenHelp() {
    typedef HWND (WINAPI *HH)(HWND, LPCWSTR, UINT, DWORD_PTR);
    static HMODULE m = LoadLibraryW(L"hhctrl.ocx"); static HH hh = m ? (HH)GetProcAddress(m, "HtmlHelpW") : nullptr;
    std::wstring chm = std::wstring(kEqDir) + L"\\Help\\cteq.chm";
    if (hh) hh(g_hWnd, chm.c_str(), 0, 0); else ShellExecuteW(nullptr, L"open", L"hh.exe", chm.c_str(), nullptr, SW_SHOWNORMAL);
}
static LRESULT CALLBACK AboutProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    if (m == WM_PAINT) { PAINTSTRUCT ps; HDC dc = BeginPaint(h, &ps); { Graphics g(dc); Blit(g, bmpAbout.get(), 0, 0); } EndPaint(h, &ps); return 0; }
    if (m == WM_LBUTTONUP && GET_X_LPARAM(l) >= 424 && GET_X_LPARAM(l) < 462 && GET_Y_LPARAM(l) >= 70 && GET_Y_LPARAM(l) < 96) { DestroyWindow(h); return 0; }
    if (m == WM_KEYDOWN && (w == VK_ESCAPE || w == VK_RETURN || w == VK_SPACE)) { DestroyWindow(h); return 0; }
    if (m == WM_DESTROY) { EnableWindow(g_hWnd, TRUE); SetForegroundWindow(g_hWnd); return 0; }
    return DefWindowProcW(h, m, w, l);
}
static void ShowAbout() {
    static bool reg = false;
    if (!reg) { WNDCLASSEXW wc = { sizeof(wc) }; wc.lpfnWndProc = AboutProc; wc.hInstance = g_hInst; wc.lpszClassName = L"EQAbout"; wc.hCursor = LoadCursor(nullptr, IDC_ARROW); RegisterClassExW(&wc); reg = true; }
    int x = (GetSystemMetrics(SM_CXSCREEN) - 472) / 2, y = (GetSystemMetrics(SM_CYSCREEN) - 322) / 2;
    HWND a = CreateWindowExW(WS_EX_TOOLWINDOW, L"EQAbout", L"", WS_POPUP | WS_VISIBLE, x, y, 472, 322, g_hWnd, nullptr, g_hInst, nullptr);
    EnableWindow(g_hWnd, FALSE); SetFocus(a);
}
struct LMenu { HWND h = nullptr; int x = 0, y = 0, w = 142, hh = 54; Px bg; const Px* face = nullptr; } g_lm;
static LRESULT CALLBACK LMenuProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    if (m == WM_PAINT) {
        PAINTSTRUCT ps; HDC dc = BeginPaint(h, &ps);
        if (g_lm.face && g_lm.face->w == g_lm.w && (int)g_lm.bg.d.size() == g_lm.w * g_lm.hh) {
            std::vector<uint32_t> out(g_lm.w * g_lm.hh);
            for (int i = 0; i < g_lm.w * g_lm.hh; i++) {
                uint32_t f = g_lm.face->d[i], b = g_lm.bg.d[i], a = f >> 24, ia = 255 - a;
                out[i] = (((((f >> 16) & 255) * a + ((b >> 16) & 255) * ia) / 255) << 16) | (((((f >> 8) & 255) * a + ((b >> 8) & 255) * ia) / 255) << 8) | (((f & 255) * a + (b & 255) * ia) / 255);
            }
            BITMAPINFO bi = {}; bi.bmiHeader.biSize = sizeof(bi.bmiHeader); bi.bmiHeader.biWidth = g_lm.w; bi.bmiHeader.biHeight = -g_lm.hh; bi.bmiHeader.biPlanes = 1; bi.bmiHeader.biBitCount = 32;
            SetDIBitsToDevice(dc, 0, 0, g_lm.w, g_lm.hh, 0, 0, 0, g_lm.hh, out.data(), &bi, DIB_RGB_COLORS);
        }
        EndPaint(h, &ps); return 0;
    }
    if (m == WM_MOUSEACTIVATE) return MA_NOACTIVATE;
    return DefWindowProcW(h, m, w, l);
}
static void ShowMenu() {
    static bool reg = false;
    if (!reg) { WNDCLASSEXW wc = { sizeof(wc) }; wc.lpfnWndProc = LMenuProc; wc.hInstance = g_hInst; wc.lpszClassName = L"EQLayerMenu"; wc.hCursor = LoadCursor(nullptr, IDC_ARROW); RegisterClassExW(&wc); reg = true; }
    POINT pt; GetCursorPos(&pt);
    g_lm.w = 142; g_lm.hh = 54;
    g_lm.x = pt.x; g_lm.y = pt.y; g_lm.bg.w = g_lm.w; g_lm.bg.h = g_lm.hh; g_lm.bg.d.assign(g_lm.w * g_lm.hh, 0);
    HDC scr = GetDC(nullptr), mem = CreateCompatibleDC(scr);
    BITMAPINFO bi = {}; bi.bmiHeader.biSize = sizeof(bi.bmiHeader); bi.bmiHeader.biWidth = g_lm.w; bi.bmiHeader.biHeight = -g_lm.hh; bi.bmiHeader.biPlanes = 1; bi.bmiHeader.biBitCount = 32;
    void* bits = nullptr; HBITMAP dib = CreateDIBSection(scr, &bi, DIB_RGB_COLORS, &bits, nullptr, 0); HGDIOBJ o = SelectObject(mem, dib);
    BitBlt(mem, 0, 0, g_lm.w, g_lm.hh, scr, pt.x, pt.y, SRCCOPY); GdiFlush(); memcpy(g_lm.bg.d.data(), bits, g_lm.w * g_lm.hh * 4);
    SelectObject(mem, o); DeleteObject(dib); DeleteDC(mem); ReleaseDC(nullptr, scr);
    int hot = 1;
    g_lm.face = &g_menuFace[hot];
    g_lm.h = CreateWindowExW(WS_EX_TOOLWINDOW | WS_EX_TOPMOST | WS_EX_NOACTIVATE, L"EQLayerMenu", L"", WS_POPUP, pt.x, pt.y, g_lm.w, g_lm.hh, g_hWnd, nullptr, g_hInst, nullptr);
    ShowWindow(g_lm.h, SW_SHOWNOACTIVATE); UpdateWindow(g_lm.h); SetCapture(g_lm.h);
    int action = 0; bool done = false; MSG msg;
    while (!done && GetMessageW(&msg, nullptr, 0, 0)) {
        if (msg.message == WM_MOUSEMOVE || msg.message == WM_LBUTTONDOWN) {
            POINT p; GetCursorPos(&p);
            bool in = p.x >= g_lm.x && p.x < g_lm.x + g_lm.w && p.y >= g_lm.y && p.y < g_lm.y + g_lm.hh;
            if (in) { int ry = p.y - g_lm.y; hot = ry < 18 ? 1 : (ry < 36 ? 0 : 2); }
            if (g_lm.face != &g_menuFace[hot]) { g_lm.face = &g_menuFace[hot]; InvalidateRect(g_lm.h, nullptr, FALSE); UpdateWindow(g_lm.h); }
            if (msg.message == WM_LBUTTONDOWN) { if (!in) done = true; else if (hot == 2) { action = 1; done = true; } }
            continue;
        }
        if (msg.message == WM_KEYDOWN && msg.wParam == VK_ESCAPE) { done = true; continue; }
        TranslateMessage(&msg); DispatchMessageW(&msg);
        if (GetCapture() != g_lm.h) done = true;
    }
    ReleaseCapture(); DestroyWindow(g_lm.h); g_lm.h = nullptr;
    if (action == 1) ShowAbout();
}

static void ShowSysMenu() {
    static bool reg = false;
    if (!reg) { WNDCLASSEXW wc = { sizeof(wc) }; wc.lpfnWndProc = LMenuProc; wc.hInstance = g_hInst; wc.lpszClassName = L"EQLayerMenu"; wc.hCursor = LoadCursor(nullptr, IDC_ARROW); RegisterClassExW(&wc); reg = true; }
    POINT pt = { 12, 24 }; ClientToScreen(g_hWnd, &pt);
    g_lm.w = 114; g_lm.hh = 126; g_lm.x = pt.x; g_lm.y = pt.y;
    g_lm.bg.w = g_lm.w; g_lm.bg.h = g_lm.hh; g_lm.bg.d.assign(g_lm.w * g_lm.hh, 0);
    HDC scr = GetDC(nullptr), mem = CreateCompatibleDC(scr);
    BITMAPINFO bi = {}; bi.bmiHeader.biSize = sizeof(bi.bmiHeader); bi.bmiHeader.biWidth = g_lm.w; bi.bmiHeader.biHeight = -g_lm.hh; bi.bmiHeader.biPlanes = 1; bi.bmiHeader.biBitCount = 32;
    void* bits = nullptr; HBITMAP dib = CreateDIBSection(scr, &bi, DIB_RGB_COLORS, &bits, nullptr, 0); HGDIOBJ o = SelectObject(mem, dib);
    BitBlt(mem, 0, 0, g_lm.w, g_lm.hh, scr, pt.x, pt.y, SRCCOPY); GdiFlush(); memcpy(g_lm.bg.d.data(), bits, g_lm.w * g_lm.hh * 4);
    SelectObject(mem, o); DeleteObject(dib); DeleteDC(mem); ReleaseDC(nullptr, scr);
    static const int rowFace[7] = { 0, 1, 0, 2, 0, 0, 3 };
    int hot = 0;
    g_lm.face = &g_sysFace[0];
    g_lm.h = CreateWindowExW(WS_EX_TOOLWINDOW | WS_EX_TOPMOST | WS_EX_NOACTIVATE, L"EQLayerMenu", L"", WS_POPUP, pt.x, pt.y, g_lm.w, g_lm.hh, g_hWnd, nullptr, g_hInst, nullptr);
    ShowWindow(g_lm.h, SW_SHOWNOACTIVATE); UpdateWindow(g_lm.h); SetCapture(g_lm.h);
    int action = 0; bool done = false; MSG msg;
    while (!done && GetMessageW(&msg, nullptr, 0, 0)) {
        if (msg.message == WM_MOUSEMOVE || msg.message == WM_LBUTTONDOWN) {
            POINT p; GetCursorPos(&p);
            bool in = p.x >= g_lm.x && p.x < g_lm.x + g_lm.w && p.y >= g_lm.y && p.y < g_lm.y + g_lm.hh;
            if (in) hot = rowFace[std::min(6, (int)(p.y - g_lm.y) / 18)];
            if (g_lm.face != &g_sysFace[hot]) { g_lm.face = &g_sysFace[hot]; InvalidateRect(g_lm.h, nullptr, FALSE); UpdateWindow(g_lm.h); }
            if (msg.message == WM_LBUTTONDOWN) {
                if (!in) done = true;
                else { int row = std::min(6, (int)(p.y - g_lm.y) / 18); if (rowFace[row]) { action = rowFace[row]; done = true; } }
            }
            continue;
        }
        if (msg.message == WM_LBUTTONUP) continue;
        if (msg.message == WM_KEYDOWN && msg.wParam == VK_ESCAPE) { done = true; continue; }
        TranslateMessage(&msg); DispatchMessageW(&msg);
        if (GetCapture() != g_lm.h) done = true;
    }
    ReleaseCapture(); DestroyWindow(g_lm.h); g_lm.h = nullptr;
    if (action == 1) PostMessageW(g_hWnd, WM_SYSCOMMAND, SC_MOVE, 0);
    else if (action == 2) ShowWindow(g_hWnd, SW_MINIMIZE);
    else if (action == 3) PostMessageW(g_hWnd, WM_CLOSE, 0, 0);
}

static std::wstring g_saveName;
static INT_PTR CALLBACK SaveProc(HWND d, UINT m, WPARAM w, LPARAM) {
    if (m == WM_INITDIALOG) {
        HWND cb = GetDlgItem(d, 14020);
        for (auto& p : g_presets) if (p.user) SendMessageW(cb, CB_ADDSTRING, 0, (LPARAM)p.name.c_str());
        RECT r, pr; GetWindowRect(d, &r); GetWindowRect(g_hWnd, &pr);
        SetWindowPos(d, nullptr, pr.left + (500 - (r.right - r.left)) / 2, pr.top + 81, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
        SetFocus(cb); return FALSE;
    }
    if (m == WM_COMMAND && (LOWORD(w) == IDOK || LOWORD(w) == IDCANCEL)) {
        wchar_t buf[128] = {}; GetDlgItemTextW(d, 14020, buf, 128); g_saveName = buf;
        EndDialog(d, LOWORD(w)); return TRUE;
    }
    return FALSE;
}
static void SaveDialog() {

    static HMODULE res = LoadLibraryExW((std::wstring(kEqDir) + L"\\EQRes.crl").c_str(), nullptr, LOAD_LIBRARY_AS_DATAFILE);
    HRSRC r = res ? FindResourceW(res, MAKEINTRESOURCEW(131), RT_DIALOG) : nullptr;
    if (!r) return;
    auto tpl = (LPCDLGTEMPLATEW)LockResource(LoadResource(res, r));
    if (DialogBoxIndirectParamW(g_hInst, tpl, g_hWnd, SaveProc, 0) != IDOK) return;
    std::wstring n = g_saveName;
    while (!n.empty() && n.back() == L' ') n.pop_back();
    if (n.empty() || n == L"<Custom>") { MessageBoxW(g_hWnd, L"Invalid name. Preset not saved.", L"Graphic Equalizer", MB_OK | MB_ICONEXCLAMATION); return; }
    for (int i = 0; i < (int)g_presets.size(); i++)
        if (_wcsicmp(g_presets[i].name.c_str(), n.c_str()) == 0) {
            if (!g_presets[i].user || MessageBoxW(g_hWnd, L"An EQ Preset of the same name already exists. Do you want to overwrite it?", L"Graphic Equalizer", MB_YESNO | MB_ICONQUESTION) != IDYES) return;
            std::copy(g_band, g_band + 7, g_presets[i].off); g_preset = i; return;
        }
    Preset p{ n, {}, true }; std::copy(g_band, g_band + 7, p.off);
    g_presets.push_back(p);
    std::sort(g_presets.begin(), g_presets.end(), [](const Preset& a, const Preset& b) { return _wcsicmp(a.name.c_str(), b.name.c_str()) < 0; });
    for (int i = 0; i < (int)g_presets.size(); i++) if (g_presets[i].name == n) g_preset = i;
}

static void ApplyPreset(int i) { g_preset = i; std::copy(g_presets[i].off, g_presets[i].off + 7, g_band); }
static const wchar_t* kRegKey = L"Software\\CreativeEQClone";
static void SaveCustom() {
    HKEY k; if (RegCreateKeyExW(HKEY_CURRENT_USER, kRegKey, 0, nullptr, 0, KEY_WRITE, nullptr, &k, nullptr)) return;
    DWORD has = g_hasCustom; RegSetValueExW(k, L"HasCustom", 0, REG_DWORD, (BYTE*)&has, sizeof(has));
    RegSetValueExW(k, L"Custom", 0, REG_BINARY, (BYTE*)g_customOff, sizeof(g_customOff));
    RegCloseKey(k);
}
static void LoadCustom() {
    HKEY k; if (RegOpenKeyExW(HKEY_CURRENT_USER, kRegKey, 0, KEY_READ, &k)) return;
    DWORD has = 0, sz = sizeof(has); RegQueryValueExW(k, L"HasCustom", nullptr, nullptr, (BYTE*)&has, &sz);
    sz = sizeof(g_customOff); RegQueryValueExW(k, L"Custom", nullptr, nullptr, (BYTE*)g_customOff, &sz);
    g_hasCustom = has != 0; RegCloseKey(k);
}
static void SelectDisplay(int d) {
    if (d < CustomRows()) { g_preset = -1; std::copy(g_customOff, g_customOff + 7, g_band); }
    else ApplyPreset(d - CustomRows());
}
static void BandsEdited() {
    g_preset = -1; std::copy(g_band, g_band + 7, g_customOff);
    if (!g_hasCustom) { g_hasCustom = true; }
}
static std::wstring GetAppDirectory() {
    wchar_t buf[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, buf, MAX_PATH);
    wchar_t* last = wcsrchr(buf, L'\\');
    if (last) *(last + 1) = L'\0';
    return std::wstring(buf);
}

static bool LaunchLocalExecutable(const std::wstring& exePath, const std::wstring& workDir) {
    if (GetFileAttributesW(exePath.c_str()) == INVALID_FILE_ATTRIBUTES) return false;
    HINSTANCE h = ShellExecuteW(nullptr, L"open", exePath.c_str(), nullptr, workDir.c_str(), SW_SHOWNORMAL);
    if ((INT_PTR)h > 32) return true;
    h = ShellExecuteW(nullptr, L"runas", exePath.c_str(), nullptr, workDir.c_str(), SW_SHOWNORMAL);
    if ((INT_PTR)h > 32) return true;
    return true;
}

static void Launch(const wchar_t* rel) {
    std::wstring dir = GetAppDirectory();
    std::vector<std::wstring> localCandidates;

    if (wcsstr(rel, L"SurMixer") != nullptr) {
        localCandidates.push_back(dir + L"SurMixer.exe");
        localCandidates.push_back(dir + L"mixer_console.exe");
        localCandidates.push_back(dir + L"surmixer_clone.exe");
    } else if (wcsstr(rel, L"EAX") != nullptr) {
        localCandidates.push_back(dir + L"EAX.exe");
        localCandidates.push_back(dir + L"eax_console.exe");
    } else if (wcsstr(rel, L"CTEQ") != nullptr) {
        localCandidates.push_back(dir + L"CTEQ.exe");
        localCandidates.push_back(dir + L"eq_console.exe");
    } else if (wcsstr(rel, L"SpkSet") != nullptr) {
        localCandidates.push_back(dir + L"SpkSet.exe");
        localCandidates.push_back(dir + L"spk_console.exe");
    } else if (wcsstr(rel, L"diagnos3") != nullptr) {
        localCandidates.push_back(dir + L"diagnos3.exe");
        localCandidates.push_back(dir + L"diag_console.exe");
    }

    for (const auto& path : localCandidates) {
        if (LaunchLocalExecutable(path, dir)) return;
    }

    std::wstring p = Creative::Root() + rel;
    ShellExecuteW(nullptr, L"open", p.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}

static int BandSnap(int p) { p = std::clamp(p, 0, BAND_TRAVEL); int v = (BAND_TRAVEL - p) * 100 / BAND_TRAVEL; return BAND_TRAVEL - v * BAND_TRAVEL / 100; }
static int LevelStepAt(int p) { p = std::clamp(p, 0, LVL_TRAVEL); return std::min(100, (p * 100 + LVL_TRAVEL - 1) / LVL_TRAVEL); }
static int BandStep(int pos) { for (int s = 0; s <= 100; s++) if (BAND_TRAVEL - s * BAND_TRAVEL / 100 == pos) return s; return 50; }

static std::wstring SliderValue(int i) {
    double v = 0;
    if (i < 7) { int k = BandStep(g_band[i]) - 50; if (k) v = (k > 0 ? 1 : -1) * std::pow(12.0, std::abs(k) / 50.0); }
    else if (g_levelS) v = -std::pow(24.0, g_levelS / 100.0);
    wchar_t b[16]; swprintf_s(b, L"%.2f", v); return b;
}

static HWND g_valTip;
static LRESULT CALLBACK ValTipProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    if (m == WM_PAINT) {
        PAINTSTRUCT ps; HDC dc = BeginPaint(h, &ps); RECT r; GetClientRect(h, &r);
        HBRUSH bg = CreateSolidBrush(RGB(255, 255, 225)); FillRect(dc, &r, bg); DeleteObject(bg);
        FrameRect(dc, &r, (HBRUSH)GetStockObject(BLACK_BRUSH));
        wchar_t t[32]; GetWindowTextW(h, t, 32);
        Text(dc, t, 4, 0, r.right - 4, r.bottom, DT_SINGLELINE | DT_VCENTER, RGB(0, 0, 0));
        EndPaint(h, &ps); return 0;
    }
    if (m == WM_MOUSEACTIVATE) return MA_NOACTIVATE;
    if (m == WM_NCHITTEST) return HTTRANSPARENT;
    return DefWindowProcW(h, m, w, l);
}
static void ValTipUpdate() {
    if (g_drag < 0) { if (g_valTip) ShowWindow(g_valTip, SW_HIDE); return; }
    if (!g_valTip) {
        WNDCLASSEXW wc = { sizeof(wc) }; wc.lpfnWndProc = ValTipProc; wc.hInstance = g_hInst; wc.lpszClassName = L"EQValueTip"; RegisterClassExW(&wc);
        g_valTip = CreateWindowExW(WS_EX_TOOLWINDOW | WS_EX_TOPMOST | WS_EX_NOACTIVATE, L"EQValueTip", L"", WS_POPUP, 0, 0, 1, 1, g_hWnd, nullptr, g_hInst, nullptr);
    }
    std::wstring t = SliderValue(g_drag);
    HDC dc = GetDC(g_valTip);
    static HFONT f = CreateFontW(-11, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, 0, 0, NONANTIALIASED_QUALITY, 0, L"Tahoma");
    HGDIOBJ o = SelectObject(dc, f); SIZE sz; GetTextExtentPoint32W(dc, t.c_str(), (int)t.size(), &sz); SelectObject(dc, o); ReleaseDC(g_valTip, dc);
    POINT p = g_drag < 7 ? POINT{ BAND_X[g_drag] + 16, BAND_Y + g_band[g_drag] } : POINT{ LVL_X + 16, LVL_Y + g_level };
    ClientToScreen(g_hWnd, &p);
    SetWindowTextW(g_valTip, t.c_str());
    SetWindowPos(g_valTip, HWND_TOPMOST, p.x, p.y, sz.cx + 8, 17, SWP_NOACTIVATE | SWP_SHOWWINDOW);
    InvalidateRect(g_valTip, nullptr, TRUE); UpdateWindow(g_valTip);
}

static void PublishEq() {
    shared::State* s = shared::Open(); if (!s) return;
    double b[7], lv = 0; for (int i = 0; i < 7; i++) { int k = BandStep(g_band[i]) - 50; b[i] = k ? (k > 0 ? 1 : -1) * std::pow(12.0, std::abs(k) / 50.0) : 0.0; }
    if (g_levelS) lv = std::pow(24.0, g_levelS / 100.0);
    bool same = s->eqSet && s->eqOn == (g_enabled ? 1 : 0) && s->eqLevel == lv; for (int i = 0; same && i < 7; i++) same = s->eqBand[i] == b[i];
    if (same) return;
    for (int i = 0; i < 7; i++) s->eqBand[i] = b[i];
    s->eqLevel = lv; s->eqOn = g_enabled ? 1 : 0; s->eqSet = 1; shared::Bump(s);
}
static HWND g_hoverTip;
static void HoverTipsInit() {
    INITCOMMONCONTROLSEX ic = { sizeof(ic), ICC_WIN95_CLASSES }; InitCommonControlsEx(&ic);
    g_hoverTip = CreateWindowExW(WS_EX_TOPMOST, TOOLTIPS_CLASSW, nullptr, WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP, 0, 0, 0, 0, g_hWnd, nullptr, g_hInst, nullptr);
    for (int i = 0; i < 8; i++) {
        TOOLINFOW ti = {};
        ti.cbSize = TTTOOLINFOW_V2_SIZE;
        ti.uFlags = 0;
        ti.hwnd = g_hWnd; ti.uId = i + 1; ti.lpszText = LPSTR_TEXTCALLBACKW;
        ti.rect = i < 7 ? RECT{ BAND_X[i], BAND_Y, BAND_X[i] + 14, BAND_Y + BAND_H } : RECT{ LVL_X, LVL_Y, LVL_X + 14, LVL_Y + LVL_H };
        SendMessageW(g_hoverTip, TTM_ADDTOOLW, 0, (LPARAM)&ti);
    }
}
static int BtnAt(int x, int y) { for (int i = 1; i < BT_COUNT; i++) if (InBtn(i, x, y)) return i; return BT_NONE; }
static LRESULT CALLBACK WndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    int x = GET_X_LPARAM(l), y = GET_Y_LPARAM(l);
    if (g_hoverTip && (m == WM_MOUSEMOVE || m == WM_LBUTTONDOWN || m == WM_LBUTTONUP || m == WM_RBUTTONDOWN || m == WM_RBUTTONUP)) {
        MSG msg = { h, m, w, l }; SendMessageW(g_hoverTip, TTM_RELAYEVENT, 0, (LPARAM)&msg);
    }
    switch (m) {
    case WM_TIMER: if (w == 1) PublishEq(); return 0;
    case WM_ERASEBKGND: return 1;
    case WM_SETCURSOR:
        if (LOWORD(l) == HTCLIENT) {
            POINT p; GetCursorPos(&p); ScreenToClient(h, &p);
            Px& mk = g_handMask[g_enabled ? 1 : 0];
            bool hand = g_curHand && mk.w == WIN_W && p.x >= 0 && p.y >= 0 && p.x < mk.w && p.y < mk.h && (mk.at(p.x, p.y) & 0xFF) > 128;
            SetCursor(hand ? g_curHand : LoadCursor(nullptr, IDC_ARROW)); return TRUE;
        }
        break;
    case WM_PAINT: { PAINTSTRUCT ps; HDC dc = BeginPaint(h, &ps); Paint(dc); EndPaint(h, &ps); return 0; }
    case WM_LBUTTONDOWN: {
        if (g_listOpen) {
            if (InR(kList, x, y) && x < kList.left + 141 && y > kList.top && y < kList.top + 1 + LIST_ROWS * ROW_H) {
                int idx = g_listTop + (int)(y - kList.top - 1) / ROW_H;
                if (idx < DispCount()) SelectDisplay(idx);
            }
            g_listOpen = false; InvalidateRect(h, nullptr, FALSE); return 0;
        }
        if (g_devOpen) { g_devOpen = false; g_devFocus = true; InvalidateRect(h, nullptr, FALSE); return 0; }
        if (InR(kDevFace, x, y)) { g_devOpen = g_devFocus = true; g_comboFocus = false; InvalidateRect(h, nullptr, FALSE); return 0; }
        if (g_enabled && InR(kFace, x, y)) {
            g_listOpen = g_comboFocus = true; g_devFocus = false;
            g_listHot = DispCurrent(); g_listTop = std::clamp(g_listHot, 0, ListMaxTop());
            InvalidateRect(h, nullptr, FALSE); return 0;
        }
        bool keep = (y >= 38 && y < 100);
        if (!keep) { g_comboFocus = false; g_devFocus = false; }
        if (int b = BtnAt(x, y)) { g_btnPressed = g_btnHot = b; SetCapture(h); InvalidateRect(h, nullptr, FALSE); return 0; }
        if (y < 32 && x < 200 && g_handMask[0].w == WIN_W && (g_handMask[0].at(x, y) & 0xFF) > 128) { ShowSysMenu(); return 0; }
        if (y < 38) { ReleaseCapture(); SendMessageW(h, WM_NCLBUTTONDOWN, HTCAPTION, 0); return 0; }
        if (InR(kCheck, x, y)) { g_enabled = !g_enabled; InvalidateRect(h, nullptr, FALSE); return 0; }
        if (!g_enabled) { InvalidateRect(h, nullptr, FALSE); return 0; }
        if (InR(kSave, x, y)) { g_savePressed = true; SetCapture(h); InvalidateRect(h, nullptr, FALSE); return 0; }
        if (InR(kDelete, x, y) && g_preset >= 0 && g_presets[g_preset].user) { g_deletePressed = true; SetCapture(h); InvalidateRect(h, nullptr, FALSE); return 0; }
        for (int i = 0; i < 7; i++) {
            if (x >= BAL_X[i] && x < BAL_X[i] + 5 && y >= BAL_Y && y < BAL_Y + 10) { g_balPressed = i; SetCapture(h); InvalidateRect(h, nullptr, FALSE); return 0; }
            if (x >= BAND_X[i] && x < BAND_X[i] + 14 && y >= BAND_Y && y < BAND_Y + BAND_H) {
                int ty = BAND_Y + g_band[i];
                g_dragGrab = (y >= ty && y < ty + 24) ? y - ty : 12;
                g_drag = i; g_band[i] = BandSnap(y - BAND_Y - g_dragGrab); BandsEdited(); ValTipUpdate();
                SetCapture(h); InvalidateRect(h, nullptr, FALSE); return 0;
            }
        }
        if (x >= LVL_X && x < LVL_X + 14 && y >= LVL_Y && y < LVL_Y + LVL_H) {
            int ty = LVL_Y + g_level; g_dragGrab = (y >= ty && y < ty + 24) ? y - ty : 12;
            g_drag = 7; g_levelS = LevelStepAt(y - LVL_Y - g_dragGrab); SetCapture(h); ValTipUpdate(); InvalidateRect(h, nullptr, FALSE); return 0;
        }
        InvalidateRect(h, nullptr, FALSE); return 0;
    }
    case WM_MOUSEMOVE: {
        if (g_drag >= 0 && g_drag < 7) { g_band[g_drag] = BandSnap(y - BAND_Y - g_dragGrab); BandsEdited(); ValTipUpdate(); InvalidateRect(h, nullptr, FALSE); return 0; }
        if (g_drag == 7) { g_levelS = LevelStepAt(y - LVL_Y - g_dragGrab); ValTipUpdate(); InvalidateRect(h, nullptr, FALSE); return 0; }
        if (g_listOpen && InR(kList, x, y) && x < kList.left + 141 && y > kList.top && y < kList.top + 1 + LIST_ROWS * ROW_H) {
            int hot = std::min(g_listTop + (int)(y - kList.top - 1) / ROW_H, DispCount() - 1);
            if (hot != g_listHot) { g_listHot = hot; InvalidateRect(h, nullptr, FALSE); }
        }
        int b = BtnAt(x, y);
        if (b != g_btnHot) { g_btnHot = b; InvalidateRect(h, nullptr, FALSE); }
        TRACKMOUSEEVENT t = { sizeof(t), TME_LEAVE, h, 0 }; TrackMouseEvent(&t);
        return 0;
    }
    case WM_MOUSELEAVE: if (g_btnPressed == BT_NONE && g_btnHot) { g_btnHot = BT_NONE; InvalidateRect(h, nullptr, FALSE); } return 0;
    case WM_MOUSEWHEEL:
        if (g_listOpen) { g_listTop = std::clamp(g_listTop - GET_WHEEL_DELTA_WPARAM(w) * 3 / WHEEL_DELTA, 0, ListMaxTop()); InvalidateRect(h, nullptr, FALSE); }
        return 0;
    case WM_LBUTTONUP: {
        ReleaseCapture();
        if (g_drag >= 0) { g_drag = -1; ValTipUpdate(); InvalidateRect(h, nullptr, FALSE); return 0; }
        if (g_balPressed >= 0) {
            int i = g_balPressed; g_balPressed = -1;
            if (x >= BAL_X[i] && x < BAL_X[i] + 5 && y >= BAL_Y && y < BAL_Y + 10) { g_band[i] = 55; BandsEdited(); }
            InvalidateRect(h, nullptr, FALSE); return 0;
        }
        if (g_savePressed) { g_savePressed = false; InvalidateRect(h, nullptr, FALSE); if (InR(kSave, x, y)) SaveDialog(); InvalidateRect(h, nullptr, FALSE); return 0; }
        if (g_deletePressed) {
            g_deletePressed = false; InvalidateRect(h, nullptr, FALSE);
            if (InR(kDelete, x, y) && MessageBoxW(h, L"Are you sure you want to delete this preset?", L"Graphic Equalizer", MB_YESNO | MB_ICONQUESTION) == IDYES) {
                g_presets.erase(g_presets.begin() + g_preset); ApplyPreset(5);
            }
            InvalidateRect(h, nullptr, FALSE); return 0;
        }
        if (g_btnPressed) {
            int b = g_btnPressed; g_btnPressed = BT_NONE; InvalidateRect(h, nullptr, FALSE);
            if (!InBtn(b, x, y)) return 0;
            switch (b) {
            case BT_CLOSE: DestroyWindow(h); break;
            case BT_MIN: ShowWindow(h, SW_MINIMIZE); break;
            case BT_HELP: OpenHelp(); break;
            case BT_HOWDOI: g_btnHot = BT_NONE; InvalidateRect(h, nullptr, FALSE); UpdateWindow(h); ShowMenu(); break;
            case BT_DEFAULT: ApplyPreset(5); g_levelS = 0; g_enabled = false; g_comboFocus = false; break;
            case BT_APP1: Launch(L"Surround Mixer\\SurMixer.exe"); break;
            case BT_APP2: Launch(L"Speaker Settings\\SpkSet.exe"); break;
            case BT_APP3: Launch(L"Diagnostics\\diagnos3.exe"); break;
            case BT_APP4: Launch(L"EAX\\EAX.exe"); break;
            }
            InvalidateRect(h, nullptr, FALSE);
        }
        return 0;
    }
    case WM_KEYDOWN:
        if (w == VK_ESCAPE && (g_listOpen || g_devOpen)) { g_listOpen = false; g_devOpen = false; InvalidateRect(h, nullptr, FALSE); return 0; }
        if (g_comboFocus && g_enabled && !g_listOpen && (w == VK_UP || w == VK_DOWN || w == VK_HOME || w == VK_END)) {
            int n = DispCount(), cur = DispCurrent();
            int nxt = w == VK_HOME ? 0 : w == VK_END ? n - 1 : std::clamp(cur + (w == VK_DOWN ? 1 : -1), 0, n - 1);
            SelectDisplay(nxt); InvalidateRect(h, nullptr, FALSE); return 0;
        }
        break;
    case WM_ACTIVATE: if (LOWORD(w) == WA_INACTIVE && (g_listOpen || g_devOpen)) { g_listOpen = g_devOpen = false; InvalidateRect(h, nullptr, FALSE); } break;
    case WM_NOTIFY:
        if (((NMHDR*)l)->code == TTN_GETDISPINFOW) {
            auto* di = (NMTTDISPINFOW*)l; static std::wstring s; s = SliderValue((int)di->hdr.idFrom - 1); di->lpszText = (LPWSTR)s.c_str(); return 0;
        }
        break;
    case WM_DESTROY: SaveCustom(); PostQuitMessage(0); return 0;
    }
    return DefWindowProcW(h, m, w, l);
}

static void SaveShot(const std::wstring& path) {
    HDC scr = GetDC(nullptr), mem = CreateCompatibleDC(scr); HBITMAP bm = CreateCompatibleBitmap(scr, WIN_W, WIN_H); HGDIOBJ o = SelectObject(mem, bm);
    Paint(mem); SelectObject(mem, o);
    CLSID png{}; UINT n = 0, sz = 0; GetImageEncodersSize(&n, &sz); std::vector<BYTE> buf(sz); auto* enc = (ImageCodecInfo*)buf.data(); GetImageEncoders(n, sz, enc);
    for (UINT i = 0; i < n; i++) if (!wcscmp(enc[i].MimeType, L"image/png")) png = enc[i].Clsid;
    { Bitmap b(bm, nullptr); b.Save(path.c_str(), &png, nullptr); }
    DeleteObject(bm); DeleteDC(mem); ReleaseDC(nullptr, scr);
}

int WINAPI wWinMain(HINSTANCE hi, HINSTANCE, PWSTR cmd, int) {
    HANDLE hSingleMutex = CreateMutexW(nullptr, TRUE, L"Local\\Creative_CTEQ_Equalizer_SingleInstance_Mutex");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        if (hSingleMutex) CloseHandle(hSingleMutex);
        HWND existingWnd = FindWindowW(L"CreativeEQClone", nullptr);
        if (!existingWnd) existingWnd = FindWindowW(nullptr, L"Graphic Equalizer");
        if (existingWnd) {
            if (IsIconic(existingWnd)) ShowWindow(existingWnd, SW_RESTORE);
            SetForegroundWindow(existingWnd);
        }
        return 0;
    }
    g_hInst = hi;
    SetProcessDPIAware();
    GdiplusStartupInput gsi; GdiplusStartup(&g_gdipToken, &gsi, nullptr);
    LoadAssets();
    if (cmd && wcsncmp(cmd, L"--selftest ", 11) == 0) {
        std::wstring d = cmd + 11; while (!d.empty() && (d.back() == L' ' || d.back() == L'"')) d.pop_back();
        SaveShot(d + L"\\disabled.png");
        g_enabled = true; SaveShot(d + L"\\enabled.png");
        g_comboFocus = true; SaveShot(d + L"\\focus.png"); g_comboFocus = false;
        g_listOpen = true; g_listTop = 5; g_listHot = 5; SaveShot(d + L"\\list_open.png");
        g_listTop = 0; g_listHot = 0; SaveShot(d + L"\\list_top0.png"); g_listOpen = false;
        g_comboFocus = true;
        for (int i = 0; i < 14; i++) { ApplyPreset(i); wchar_t f[32]; swprintf_s(f, L"\\preset_%02d.png", i); SaveShot(d + f); }
        g_comboFocus = false; g_hasCustom = true; ApplyPreset(5);
        SaveShot(d + L"\\custom_flat.png");
        g_comboFocus = true; g_listOpen = true; g_listTop = 6; g_listHot = 6; SaveShot(d + L"\\custom_list.png");
        return 0;
    }
    LoadCustom();
    WNDCLASSEXW wc = { sizeof(wc) }; wc.lpfnWndProc = WndProc; wc.hInstance = hi; wc.lpszClassName = L"CreativeEQClone";
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hIcon = LoadIconW(hi, MAKEINTRESOURCEW(1));
    if (!wc.hIcon) {
        std::wstring ico = std::wstring(kEqDir) + L"\\Theme\\Icons\\App_Equalizer.ico";
        wc.hIcon = (HICON)LoadImageW(nullptr, ico.c_str(), IMAGE_ICON, 0, 0, LR_LOADFROMFILE | LR_DEFAULTSIZE);
    }
    wc.hIconSm = (HICON)LoadImageW(hi, MAKEINTRESOURCEW(1), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
    RegisterClassExW(&wc);
    g_hWnd = CreateWindowExW(WS_EX_APPWINDOW, wc.lpszClassName, L"Graphic Equalizer", WS_POPUP | WS_SYSMENU | WS_MINIMIZEBOX,
                             (GetSystemMetrics(SM_CXSCREEN) - WIN_W) / 2, (GetSystemMetrics(SM_CYSCREEN) - WIN_H) / 2, WIN_W, WIN_H, nullptr, nullptr, hi, nullptr);
    if (wc.hIcon) SendMessageW(g_hWnd, WM_SETICON, ICON_BIG, (LPARAM)wc.hIcon);
    if (wc.hIconSm) SendMessageW(g_hWnd, WM_SETICON, ICON_SMALL, (LPARAM)wc.hIconSm);

    {
        Px b = ToPx(bmpBase[0].get());
        HRGN rgn = CreateRectRgn(0, 16, WIN_W, WIN_H - 16);
        auto row = [&](int y) {
            int l = 0, r = WIN_W;
            if (b.w == WIN_W) { while (l < 250 && (b.at(l, y) & 0xFFFFFF) == 0) l++; while (r > 250 && (b.at(r - 1, y) & 0xFFFFFF) == 0) r--; }
            HRGN t = CreateRectRgn(l, y, r, y + 1); CombineRgn(rgn, rgn, t, RGN_OR); DeleteObject(t);
        };
        for (int y = 0; y < 16; y++) { row(y); row(WIN_H - 16 + y); }
        SetWindowRgn(g_hWnd, rgn, TRUE);
    }
    HoverTipsInit();
    SetTimer(g_hWnd, 1, 100, nullptr); PublishEq();
    ShowWindow(g_hWnd, SW_SHOW); UpdateWindow(g_hWnd);
    MSG msg; while (GetMessageW(&msg, nullptr, 0, 0)) { TranslateMessage(&msg); DispatchMessageW(&msg); }
    return 0;
}
