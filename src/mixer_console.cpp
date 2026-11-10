#ifndef UNICODE
#define UNICODE
#endif
#include <windows.h>
#include <windowsx.h>
#include <gdiplus.h>
#include <shellapi.h>
#include <commctrl.h>
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
static const std::wstring kRoot = Creative::Root();
static const std::wstring kMixDir = Creative::Dir(L"Surround Mixer");

struct Slider { int tab, x, y, h, travel, scale; int val, pct; int sheet; bool unlocked; };

static Slider g_sl[] = {
    { 0, 36, 117, 124, 100, 65536, 65536, 100, 0, false },
    { 0, 191, 117, 124, 100, 65536, 65536, 100, 0, false },
    { 0, 275, 117, 124, 100, 65536, 65536, 100, 0, false },
    { 1, 132, 101, 141, 117, 65535, 65535, 100, 3, false },
    { 1, 194, 101, 141, 117, 65535, 65535, 100, 3, false },
    { 0, 86, 117, 124, 100, 65536, 32768, 50, 1, true },
    { 0, 126, 117, 124, 100, 65536, 32768, 50, 1, true },
    { 0, 233, 117, 124, 100, 65536, 32768, 50, 0, true },
    { 0, 317, 117, 124, 100, 65536, 32768, 50, 0, true },
    { 0, 359, 117, 124, 100, 65536, 32768, 50, 0, true },
    { 0, 399, 117, 124, 100, 65536, 32768, 50, 0, true },
    { 0, 459, 117, 124, 100, 65536, 32768, 50, 2, true },
    { 1, 404, 101, 141, 117, 65535, 32488, 49, 3, true },
};
static constexpr int NSL = (int)(sizeof(g_sl) / sizeof(g_sl[0]));
enum { SL_BASS = 5, SL_TREBLE = 6, SL_SUB = 12 };

static constexpr RECT kBal[2] = { { 101, 173, 106, 183 }, { 141, 173, 146, 183 } };
static int g_balPressed = -1;
static constexpr RECT kMute2 = { 230, 241, 250, 261 };
static bool g_mute2 = false;
static int ThumbOff(const Slider& s) {
    long long q = ((long long)std::max(0, s.scale - s.val) * s.travel + s.scale - 1) / s.scale;
    return (int)std::min<long long>(q, s.travel);
}
static void SliderFromMouse(Slider& s, int my) {
    int d = std::clamp(my - 12 - s.y, 0, s.travel);
    s.val = (int)((long long)(s.travel - d) * s.scale / s.travel);
    s.pct = (s.travel - d) * 100 / s.travel;
}
static void SliderSetPct(Slider& s, int pct) { s.pct = pct; s.val = (int)((long long)pct * s.scale / 100); }

static constexpr RECT kMute[2] = { { 32, 241, 52, 261 }, { 188, 241, 208, 261 } };
static constexpr RECT kTab[2] = { { 7, 39, 238, 61 }, { 238, 39, 493, 61 } };
static constexpr RECT kDevFace = { 200, 6, 370, 27 };
static RECT kLogo = { 352, 272, 470, 297 };

static int g_tab = 0;
static bool g_muted = false;
static int g_mutePressed = -1;
static bool g_logoPressed = false;
static int g_drag = -1;
static int g_tabHot = -1;
static bool g_devOpen = false, g_devFocus = false;
static bool g_optPlace = false, g_optLoad = true;
enum { BT_NONE, BT_CLOSE, BT_MIN, BT_HELP, BT_HOWDOI, BT_DEFAULT, BT_APP1, BT_APP2, BT_APP3, BT_APP4, BT_COUNT };
static int g_btnPressed = BT_NONE, g_btnHot = BT_NONE;
static const struct { int x, y, w, h; const wchar_t* name; const wchar_t* tip; } kBtn[BT_COUNT] = {
    { 0, 0, 0, 0, nullptr, nullptr }, { 470, 6, 22, 21, L"close", L"Close" }, { 446, 6, 20, 21, L"min", L"Minimize" },
    { 381, 6, 32, 20, L"help", L"Help Contents" }, { 413, 6, 15, 20, L"howdoi", L"How Do I..." }, { 18, 275, 90, 22, L"default", nullptr },
    { 118, 272, 32, 24, L"app1", L"EAX Console" }, { 150, 272, 32, 24, L"app2", L"Speaker Settings" },
    { 182, 272, 32, 24, L"app3", L"Diagnostics" }, { 214, 272, 32, 24, L"app4", L"Graphic Equalizer" },
};
static bool InBtn(int i, int x, int y) { return x >= kBtn[i].x && x < kBtn[i].x + kBtn[i].w && y >= kBtn[i].y && y < kBtn[i].y + kBtn[i].h; }
static bool InR(const RECT& r, int x, int y) { return x >= r.left && x < r.right && y >= r.top && y < r.bottom; }

static HINSTANCE g_hInst; static HWND g_hWnd; static ULONG_PTR g_gdipToken;

struct Px { int w = 0, h = 0; std::vector<uint32_t> d; uint32_t& at(int x, int y) { return d[y * w + x]; } uint32_t at(int x, int y) const { return d[y * w + x]; } };
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

static std::unique_ptr<Bitmap> bmpBase[2], bmpSheet[4], bmpMute, bmpIcon, bmpIconArw, bmpBass, bmpTreble, bmpBal, bmpBtn[BT_COUNT][2], bmpTabHover[2], bmpDevOpen, bmpDevFocus, bmpAbout;
static Px g_cur[2];
static HCURSOR g_curHand;
static Px g_tFace[8];
static Px g_mFace[5][4];
static Px g_sFace[4];
static RECT g_titleBox = { 0, 0, 0, 0 };

static void LoadAssets() {
    std::wstring A = L"mix_assets\\", T = std::wstring(kMixDir) + L"\\Theme\\Default\\";
    bmpBase[0] = Load(A + L"base_basic.png"); bmpBase[1] = Load(A + L"base_adv.png");
    bmpSheet[0] = Q565(Load(T + L"Mixer_Sldr_Volume.bmp")); bmpSheet[1] = Q565(Load(T + L"Mixer_Sldr_Vertical14.bmp"));
    bmpSheet[2] = Q565(Load(T + L"Mixer_Sldr_Record.bmp")); bmpSheet[3] = Q565(Load(std::wstring(kMixDir) + L"\\Theme\\Speaker21\\Mixer_Sldr_Volume.bmp"));
    bmpIcon = Q565(Load(T + L"Mixer_Bttn_Icon.bmp")); bmpIconArw = Q565(Load(T + L"Mixer_Bttn_IconArw.bmp"));
    bmpBass = Q565(Load(T + L"Mixer_Bttn_IconBass.bmp")); bmpTreble = Q565(Load(T + L"Mixer_Bttn_IconTreble.bmp")); bmpBal = Q565(Load(T + L"Mixer_Bttn_VolBal.bmp")); bmpMute = Q565(Load(T + L"Mixer_Bttn_Mute.bmp"));
    const wchar_t* posH[BT_COUNT] = { nullptr, L"_469_6", nullptr, nullptr, nullptr, nullptr, L"_118_272", L"_150_272", L"_182_272", L"_214_272" };
    const wchar_t* posP[BT_COUNT] = { nullptr, L"_470_6", L"_446_6", L"_381_6", L"_413_6", L"_18_275", L"_118_272", L"_150_272", L"_182_272", L"_214_272" };
    for (int i = 1; i < BT_COUNT; i++) {
        if (posH[i]) bmpBtn[i][0] = Load(A + kBtn[i].name + L"_hover" + posH[i] + L".png");
        bmpBtn[i][1] = Load(A + kBtn[i].name + L"_pressed" + posP[i] + L".png");
    }
    bmpTabHover[0] = Load(A + L"tab_adv_hover_238_39.png");
    bmpTabHover[1] = Load(A + L"adv_tab_basic_hover_7_39.png");
    bmpDevOpen = Load(A + L"combo_open_200_27.png"); bmpDevFocus = Load(A + L"combo_focus_203_9.png");
    bmpAbout = Load(A + L"about.png");
    g_cur[0] = ToPx(Load(A + L"cursor_basic.png").get()); g_cur[1] = ToPx(Load(A + L"cursor_advanced.png").get());
    g_curHand = (HCURSOR)LoadImageW(nullptr, L"C:\\Program Files (x86)\\Creative\\Theme\\1033\\Default\\HandBlue.cur", IMAGE_CURSOR, 0, 0, LR_LOADFROMFILE | LR_DEFAULTSIZE);
    for (int i = 0; i < 8; i++) g_tFace[i] = ToPx(Load(A + L"tmenu_" + std::to_wstring(i) + L".png").get());
    const wchar_t* hot[5] = { L"howdoi", L"place", L"load", L"none", L"about" };
    for (int i = 0; i < 5; i++) for (int c = 0; c < 4; c++)
        g_mFace[i][c] = ToPx(Load(A + L"mmenu_" + hot[i] + L"_" + std::to_wstring(c >> 1) + std::to_wstring(c & 1) + L".png").get());
    for (int i = 0; i < 4; i++) g_sFace[i] = ToPx(Load(A + L"smenu_" + std::to_wstring(i) + L".png").get());

    Px& c = g_cur[0];
    if (c.w == WIN_W) {
        RECT b = { WIN_W, WIN_H, 0, 0 };
        for (int y = 0; y < 34; y++) for (int x = 0; x < 200; x++)
            if (((c.at(x, y) >> 16) & 255) > 128) { b.left = std::min<LONG>(b.left, x); b.top = std::min<LONG>(b.top, y); b.right = std::max<LONG>(b.right, x + 1); b.bottom = std::max<LONG>(b.bottom, y + 1); }
        if (b.right > b.left) g_titleBox = b;
        RECT lg = { WIN_W, WIN_H, 0, 0 };
        for (int y = 262; y < WIN_H; y++) for (int x = 340; x < WIN_W; x++)
            if (((c.at(x, y) >> 16) & 255) > 128) { lg.left = std::min<LONG>(lg.left, x); lg.top = std::min<LONG>(lg.top, y); lg.right = std::max<LONG>(lg.right, x + 1); lg.bottom = std::max<LONG>(lg.bottom, y + 1); }
        if (lg.right > lg.left) kLogo = lg;
    }

    auto copyHand = [](Px& m, int sx0, int sy0, int sx1, int sy1, int dx) {
        if (m.w != WIN_W) return;
        for (int y = sy0; y < sy1; y++) for (int x = sx0; x < sx1; x++)
            if (((m.at(x, y) >> 16) & 255) > 128 && x + dx >= 0 && x + dx < WIN_W) m.at(x + dx, y) |= 0xFF0000;
    };
    for (int i = 5; i < NSL; i++) {
        const Slider& s = g_sl[i];
        if (s.tab == 0) copyHand(g_cur[0], 30, 105, 58, 241, s.x - 36); else copyHand(g_cur[1], 126, 95, 152, 250, s.x - 132);
    }
    copyHand(g_cur[0], 26, 241, 58, 262, kMute2.left - 32);
    for (RECT r : kBal) for (int y = r.top; y < r.bottom; y++) for (int x = r.left; x < r.right; x++) g_cur[0].at(x, y) |= 0xFF0000;
    {
    }
}
static int CursorKind(int x, int y) {
    Px& c = g_cur[g_tab];
    if (c.w != WIN_W || x < 0 || y < 0 || x >= WIN_W || y >= WIN_H) return 0;
    uint32_t p = c.at(x, y);
    return ((p >> 16) & 255) > 128 ? 1 : ((p >> 8) & 255) > 128 ? 2 : 0;
}

static HFONT TahomaFont() { static HFONT f = CreateFontW(-11, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, 0, 0, NONANTIALIASED_QUALITY, 0, L"Tahoma"); return f; }
static void Text(HDC dc, const wchar_t* s, int x, int y, int w, int h, UINT fmt, COLORREF c) {
    HGDIOBJ o = SelectObject(dc, TahomaFont()); SetBkMode(dc, TRANSPARENT); SetTextColor(dc, c);
    RECT r = { x, y, x + w, y + h }; DrawTextW(dc, s, -1, &r, fmt | DT_NOPREFIX); SelectObject(dc, o);
}

static void Blit(Graphics& g, Bitmap* b, int x, int y, int sx = 0, int sy = 0, int w = -1, int h = -1) {
    if (!b || b->GetLastStatus() != Ok) return;
    if (w < 0) { w = (int)b->GetWidth(); h = (int)b->GetHeight(); }
    g.DrawImage(b, Rect(x, y, w, h), sx, sy, w, h, UnitPixel);
}

static void DrawSlider(Graphics& g, const Slider& s, bool pressed) {

    Bitmap* sh = bmpSheet[s.sheet].get();
    int ty = s.y + s.tab, off = ThumbOff(s), mid = s.y + off + 12, bot = s.y + s.h;
    auto seg = [&](int sheet, int y0, int y1, bool topCap, bool botCap) {
        int y = y0, e = y1;
        if (topCap) { int n = std::min(12, e - y); g.DrawImage(sh, Rect(s.x, y, 14, n), 0, sheet, 14, n, UnitPixel); y += n; }
        int bc = botCap ? std::min(12, e - y) : 0;
        while (y < e - bc) { int n = std::min(72, e - bc - y); g.DrawImage(sh, Rect(s.x, y, 14, n), 0, sheet + 12, 14, n, UnitPixel); y += n; }
        if (bc) g.DrawImage(sh, Rect(s.x, e - bc, 14, bc), 0, sheet + 96 - bc, 14, bc, UnitPixel);
    };
    seg(0, ty, mid, true, false);
    seg(96, mid, bot, false, true);
    ImageAttributes ia; ia.SetColorKey(Color(240, 0, 240), Color(255, 16, 255));
    g.DrawImage(sh, Rect(s.x, s.y + off, 14, 24), 0, 288 + (pressed ? 2 : 0) * 24, 14, 24, UnitPixel, &ia);
}

static int g_lblOx = 0, g_lblOy = 4;
static void LabelV(HDC dc, const wchar_t* s, int bx, int by, COLORREF c) {
    static HFONT f = CreateFontW(-11, 0, 900, 900, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, 0, 0, NONANTIALIASED_QUALITY, 0, L"Tahoma");
    HGDIOBJ o = SelectObject(dc, f); SetBkMode(dc, TRANSPARENT); SetTextColor(dc, c);
    TextOutW(dc, bx + g_lblOx, by + 160 - g_lblOy, s, (int)wcslen(s)); SelectObject(dc, o);
}
static void SubLabel(HDC dc) { LabelV(dc, L"Subwoofer", 382, 96, RGB(0, 0, 0)); }

static void Paint(HDC hdc) {
    HDC mem = CreateCompatibleDC(hdc); HBITMAP bm = CreateCompatibleBitmap(hdc, WIN_W, WIN_H); HGDIOBJ old = SelectObject(mem, bm);
    {
        Graphics g(mem);
        g.SetInterpolationMode(InterpolationModeNearestNeighbor); g.SetPixelOffsetMode(PixelOffsetModeNone);
        Blit(g, bmpBase[g_tab].get(), 0, 0);
        if (g_tab == 0) {
            SolidBrush bg(Color(255, 246, 240, 246));
            for (RECT r : { RECT{ 229, 94, 259, 117 }, RECT{ 448, 94, 478, 117 }, RECT{ 88, 96, 102, 111 }, RECT{ 129, 95, 139, 114 }, kBal[0], kBal[1], kMute2 })
                g.FillRectangle(&bg, (INT)r.left, (INT)r.top, (INT)(r.right - r.left), (INT)(r.bottom - r.top));
        } else {
            SolidBrush bg(Color(255, 246, 240, 246)); g.FillRectangle(&bg, 382, 96, 16, 160);
        }
        for (int i = 0; i < NSL; i++) if (g_sl[i].tab == g_tab) DrawSlider(g, g_sl[i], g_drag == i);
        if (g_tab == 0) {
            ImageAttributes ia; ia.SetColorKey(Color(240, 0, 240), Color(255, 16, 255));
            auto key = [&](Bitmap* b, int x, int y, int sx, int sy, int w, int h) { g.DrawImage(b, Rect(x, y, w, h), sx, sy, w, h, UnitPixel, &ia); };
            key(bmpIcon.get(), 229, 94, 0, 0, 30, 24);
            key(bmpIconArw.get(), 448, 94, 0, 0, 30, 24);
            key(bmpBass.get(), 88, 96, 0, 0, 14, 15);
            key(bmpTreble.get(), 129, 95, 0, 0, 10, 19);
            for (int b = 0; b < 2; b++) key(bmpBal.get(), kBal[b].left, kBal[b].top, (g_balPressed == b ? 2 : 0) * 5, 0, 5, 10);
            int col = 0;
            if (g_mutePressed == 2) { POINT c; GetCursorPos(&c); ScreenToClient(g_hWnd, &c); if (InR(kMute2, c.x, c.y)) col = 2; }
            Blit(g, bmpMute.get(), kMute2.left, kMute2.top, col * 20, g_mute2 ? 20 : 0, 20, 20);
        } else {
            HDC dc = g.GetHDC(); SubLabel(dc); g.ReleaseHDC(dc);
        }
        if (g_tab == 0) Blit(g, bmpBase[0].get(), 191, 117, 191, 117, 14, 1);
        if (g_tab == 0)
            for (int m = 0; m < 2; m++) {
                int col = 0;
                if (g_mutePressed == m) { POINT c; GetCursorPos(&c); ScreenToClient(g_hWnd, &c); if (InR(kMute[m], c.x, c.y)) col = 2; }
                Blit(g, bmpMute.get(), kMute[m].left, kMute[m].top, col * 20, g_muted ? 20 : 0, 20, 20);
            }
        if (g_tabHot >= 0 && g_tabHot != g_tab && g_drag < 0) Blit(g, bmpTabHover[g_tab].get(), g_tab == 0 ? 238 : 7, 39);
        for (int i = 1; i < BT_COUNT; i++) {
            if (g_btnPressed == i && g_btnHot == i) Blit(g, bmpBtn[i][1].get(), i == BT_CLOSE ? 470 : kBtn[i].x, kBtn[i].y);
            else if (g_btnPressed == BT_NONE && g_btnHot == i && bmpBtn[i][0]) Blit(g, bmpBtn[i][0].get(), i == BT_CLOSE ? 469 : kBtn[i].x, kBtn[i].y);
        }
        if (g_devOpen) Blit(g, bmpDevOpen.get(), 200, 27);
        else if (g_devFocus) Blit(g, bmpDevFocus.get(), 203, 9);
    }
    BitBlt(hdc, 0, 0, WIN_W, WIN_H, mem, 0, 0, SRCCOPY);
    SelectObject(mem, old); DeleteObject(bm); DeleteDC(mem);
}

static void OpenHelp(const wchar_t* topic = nullptr) {
    typedef HWND (WINAPI *HH)(HWND, LPCWSTR, UINT, DWORD_PTR);
    static HMODULE m = LoadLibraryW(L"hhctrl.ocx"); static HH hh = m ? (HH)GetProcAddress(m, "HtmlHelpW") : nullptr;
    std::wstring chm = std::wstring(kMixDir) + L"\\Help\\SurMixer.chm";
    if (topic) chm += std::wstring(L"::") + topic;
    if (hh) hh(GetDesktopWindow(), chm.c_str(), 0, 0); else ShellExecuteW(nullptr, L"open", L"hh.exe", chm.c_str(), nullptr, SW_SHOWNORMAL);
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
    if (!reg) { WNDCLASSEXW wc = { sizeof(wc) }; wc.lpfnWndProc = AboutProc; wc.hInstance = g_hInst; wc.lpszClassName = L"MixAbout"; wc.hCursor = LoadCursor(nullptr, IDC_ARROW); RegisterClassExW(&wc); reg = true; }
    int x = (GetSystemMetrics(SM_CXSCREEN) - 472) / 2, y = (GetSystemMetrics(SM_CYSCREEN) - 322) / 2;
    HWND a = CreateWindowExW(WS_EX_TOOLWINDOW, L"MixAbout", L"", WS_POPUP | WS_VISIBLE, x, y, 472, 322, g_hWnd, nullptr, g_hInst, nullptr);
    EnableWindow(g_hWnd, FALSE); SetFocus(a);
}

struct LWin { HWND h = nullptr; int x = 0, y = 0, w = 0, hh = 0; Px bg; const Px* face = nullptr; };
static LRESULT CALLBACK LWinProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    if (m == WM_PAINT) {
        PAINTSTRUCT ps; HDC dc = BeginPaint(h, &ps);
        auto* lw = (LWin*)GetWindowLongPtrW(h, GWLP_USERDATA);
        if (lw && lw->face && lw->face->w == lw->w && lw->face->h == lw->hh && (int)lw->bg.d.size() == lw->w * lw->hh) {
            std::vector<uint32_t> out(lw->w * lw->hh);
            for (int i = 0; i < lw->w * lw->hh; i++) {
                uint32_t f = lw->face->d[i], b = lw->bg.d[i], a = f >> 24, ia = 255 - a;
                out[i] = (((((f >> 16) & 255) * a + ((b >> 16) & 255) * ia) / 255) << 16) | (((((f >> 8) & 255) * a + ((b >> 8) & 255) * ia) / 255) << 8) | (((f & 255) * a + (b & 255) * ia) / 255);
            }
            BITMAPINFO bi = {}; bi.bmiHeader.biSize = sizeof(bi.bmiHeader); bi.bmiHeader.biWidth = lw->w; bi.bmiHeader.biHeight = -lw->hh; bi.bmiHeader.biPlanes = 1; bi.bmiHeader.biBitCount = 32;
            SetDIBitsToDevice(dc, 0, 0, lw->w, lw->hh, 0, 0, 0, lw->hh, out.data(), &bi, DIB_RGB_COLORS);
        }
        EndPaint(h, &ps); return 0;
    }
    if (m == WM_MOUSEACTIVATE) return MA_NOACTIVATE;
    return DefWindowProcW(h, m, w, l);
}
static uint32_t LBlend(uint32_t f, uint32_t b) {
    uint32_t a = f >> 24, ia = 255 - a;
    return (((((f >> 16) & 255) * a + ((b >> 16) & 255) * ia) / 255) << 16) | (((((f >> 8) & 255) * a + ((b >> 8) & 255) * ia) / 255) << 8) | (((f & 255) * a + (b & 255) * ia) / 255);
}

static void LOpen(LWin& lw, int x, int y, const Px* face, const LWin* under = nullptr) {
    static bool reg = false;
    if (!reg) { WNDCLASSEXW wc = { sizeof(wc) }; wc.lpfnWndProc = LWinProc; wc.hInstance = g_hInst; wc.lpszClassName = L"MixLayerMenu"; wc.hCursor = LoadCursor(nullptr, IDC_ARROW); RegisterClassExW(&wc); reg = true; }
    lw.x = x; lw.y = y; lw.w = face->w; lw.hh = face->h; lw.face = face;
    lw.bg.w = lw.w; lw.bg.h = lw.hh; lw.bg.d.assign(lw.w * lw.hh, 0);
    HDC scr = GetDC(nullptr), mem = CreateCompatibleDC(scr);
    BITMAPINFO bi = {}; bi.bmiHeader.biSize = sizeof(bi.bmiHeader); bi.bmiHeader.biWidth = lw.w; bi.bmiHeader.biHeight = -lw.hh; bi.bmiHeader.biPlanes = 1; bi.bmiHeader.biBitCount = 32;
    void* bits = nullptr; HBITMAP dib = CreateDIBSection(scr, &bi, DIB_RGB_COLORS, &bits, nullptr, 0); HGDIOBJ o = SelectObject(mem, dib);
    BitBlt(mem, 0, 0, lw.w, lw.hh, scr, x, y, SRCCOPY); GdiFlush(); memcpy(lw.bg.d.data(), bits, lw.w * lw.hh * 4);
    SelectObject(mem, o); DeleteObject(dib); DeleteDC(mem); ReleaseDC(nullptr, scr);
    if (under && under->h && under->face)
        for (int yy = 0; yy < lw.hh; yy++) for (int xx = 0; xx < lw.w; xx++) {
            int ux = x + xx - under->x, uy = y + yy - under->y;
            if (ux >= 0 && uy >= 0 && ux < under->w && uy < under->hh) lw.bg.at(xx, yy) = LBlend(under->face->at(ux, uy), under->bg.at(ux, uy));
        }
    lw.h = CreateWindowExW(WS_EX_TOOLWINDOW | WS_EX_TOPMOST | WS_EX_NOACTIVATE, L"MixLayerMenu", L"", WS_POPUP, x, y, lw.w, lw.hh, g_hWnd, nullptr, g_hInst, nullptr);
    SetWindowLongPtrW(lw.h, GWLP_USERDATA, (LONG_PTR)&lw);
    ShowWindow(lw.h, SW_SHOWNOACTIVATE); UpdateWindow(lw.h);
}
static void LFace(LWin& lw, const Px* f) { if (lw.h && lw.face != f) { lw.face = f; InvalidateRect(lw.h, nullptr, FALSE); UpdateWindow(lw.h); } }
static void LClose(LWin& lw) { if (lw.h) { DestroyWindow(lw.h); lw.h = nullptr; } }
static bool LIn(const LWin& lw, POINT p) { return lw.h && p.x >= lw.x && p.x < lw.x + lw.w && p.y >= lw.y && p.y < lw.y + lw.hh; }

template <class OnMouse> static void MenuLoop(HWND cap, OnMouse onMouse, bool swallowFirstUp) {
    SetCapture(cap);
    bool done = false; MSG msg;
    while (!done && GetMessageW(&msg, nullptr, 0, 0)) {
        if (msg.message == WM_MOUSEMOVE || msg.message == WM_LBUTTONDOWN) { POINT p; GetCursorPos(&p); done = onMouse(p, msg.message == WM_LBUTTONDOWN); continue; }
        if (msg.message == WM_LBUTTONUP && swallowFirstUp) { swallowFirstUp = false; continue; }
        if (msg.message == WM_KEYDOWN && msg.wParam == VK_ESCAPE) break;
        TranslateMessage(&msg); DispatchMessageW(&msg);
        if (GetCapture() != cap) break;
    }
    ReleaseCapture();
}

static void ShowSysMenu() {
    POINT pt = { 12, 24 }; ClientToScreen(g_hWnd, &pt);
    static LWin lw; LOpen(lw, pt.x, pt.y, &g_tFace[0]);
    int action = 0;
    MenuLoop(lw.h, [&](POINT p, bool down) {
        bool in = LIn(lw, p); int row = std::min(6, (int)(p.y - lw.y) / 18);
        if (in) LFace(lw, &g_tFace[row + 1]);
        if (!down) return false;
        if (!in) return true;
        static const int act[7] = { 0, 1, 0, 2, 0, 0, 3 };
        if (act[row]) { action = act[row]; return true; }
        return false;
    }, true);
    LClose(lw);
    if (action == 1) PostMessageW(g_hWnd, WM_SYSCOMMAND, SC_MOVE, 0);
    else if (action == 2) ShowWindow(g_hWnd, SW_MINIMIZE);
    else if (action == 3) PostMessageW(g_hWnd, WM_CLOSE, 0, 0);
}

static const wchar_t* kTopic[3] = {
    L"\\html\\How_do_I_balance_my_speaker_s_output_.htm",
    L"\\html\\How_do_I_listen_to_audio_from_an_external_source_.htm",
    L"\\html\\How_to_I_select_a_speaker_configuration_.htm",
};
static void SaveState();
static void TrayUpdate();
static void ShowMenu() {
    POINT pt; GetCursorPos(&pt);
    static LWin mw, sw;
    int hot = 0;
    auto face = [&]() { return &g_mFace[hot][(g_optPlace ? 2 : 0) | (g_optLoad ? 1 : 0)]; };
    LOpen(mw, pt.x, pt.y, face());
    int action = 0, subHot = 0;
    LOpen(sw, mw.x + 212, mw.y + 1, &g_sFace[0], &mw);
    MenuLoop(mw.h, [&](POINT p, bool down) {
        if (LIn(sw, p)) {
            int r = std::min(2, (int)(p.y - sw.y) / 18); subHot = r + 1; LFace(sw, &g_sFace[subHot]);
            if (down) { action = 10 + r; return true; }
            return false;
        }
        bool in = LIn(mw, p);
        if (in) {
            int row = std::min(4, (int)(p.y - mw.y) / 18);
            static const int rowHot[5] = { 0, 1, 2, 3, 4 };
            hot = rowHot[row]; LFace(mw, face());
            if (hot == 0 && !sw.h) { subHot = 0; LOpen(sw, mw.x + 212, mw.y + 1, &g_sFace[0], &mw); }
            else if (hot != 0 && sw.h) LClose(sw);
            if (down) {
                if (hot == 1) action = 1; else if (hot == 2) action = 2; else if (hot == 4) action = 3;
                return action != 0;
            }
            return false;
        }
        return down;
    }, false);
    LClose(sw); LClose(mw);
    if (action == 1) { g_optPlace = !g_optPlace; TrayUpdate(); SaveState(); }
    else if (action == 2) { g_optLoad = !g_optLoad; SaveState(); }
    else if (action == 3) ShowAbout();
    else if (action >= 10) OpenHelp(kTopic[action - 10]);
}

static constexpr UINT WM_TRAY = WM_APP + 1;
static void TrayUpdate() {
    NOTIFYICONDATAW nd = {}; nd.cbSize = sizeof(nd); nd.hWnd = g_hWnd; nd.uID = 1;
    if (!g_optPlace) { Shell_NotifyIconW(NIM_DELETE, &nd); return; }
    nd.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE; nd.uCallbackMessage = WM_TRAY;
    std::wstring ico = std::wstring(kMixDir) + (g_muted ? L"\\Theme\\Default\\App_VolMute_TrayIcon.ico" : L"\\Theme\\Default\\App_Vol_TrayIcon.ico");
    nd.hIcon = (HICON)LoadImageW(nullptr, ico.c_str(), IMAGE_ICON, 16, 16, LR_LOADFROMFILE);
    wcscpy_s(nd.szTip, L"Volume");
    if (!Shell_NotifyIconW(NIM_MODIFY, &nd)) Shell_NotifyIconW(NIM_ADD, &nd);
}

static const wchar_t* kRegKey = L"Software\\CreativeMixerClone";
static void SaveState() {
    HKEY k; if (RegCreateKeyExW(HKEY_CURRENT_USER, kRegKey, 0, nullptr, 0, KEY_WRITE, nullptr, &k, nullptr)) return;
    DWORD v[NSL * 2 + 5] = {};
    for (int i = 0; i < NSL; i++) { v[i * 2] = g_sl[i].val; v[i * 2 + 1] = g_sl[i].pct; }
    v[NSL * 2] = g_muted; v[NSL * 2 + 1] = g_optPlace; v[NSL * 2 + 2] = g_optLoad; v[NSL * 2 + 3] = g_mute2; v[NSL * 2 + 4] = 2;
    RegSetValueExW(k, L"State", 0, REG_BINARY, (BYTE*)v, sizeof(v)); RegCloseKey(k);
}
static void LoadState() {
    HKEY k; if (RegOpenKeyExW(HKEY_CURRENT_USER, kRegKey, 0, KEY_READ, &k)) return;
    DWORD v[NSL * 2 + 5] = {}, sz = sizeof(v);
    if (!RegQueryValueExW(k, L"State", nullptr, nullptr, (BYTE*)v, &sz) && sz == sizeof(v) && v[NSL * 2 + 4] == 2) {
        for (int i = 0; i < NSL; i++) { g_sl[i].val = std::clamp((int)v[i * 2], 0, g_sl[i].scale); g_sl[i].pct = std::clamp((int)v[i * 2 + 1], 0, 100); }
        g_muted = v[NSL * 2]; g_optPlace = v[NSL * 2 + 1]; g_optLoad = v[NSL * 2 + 2]; g_mute2 = v[NSL * 2 + 3];
    }
    RegCloseKey(k);
}

static std::wstring PctText(int i) { return std::to_wstring(g_sl[i].pct) + L"%"; }
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
        WNDCLASSEXW wc = { sizeof(wc) }; wc.lpfnWndProc = ValTipProc; wc.hInstance = g_hInst; wc.lpszClassName = L"MixValueTip"; RegisterClassExW(&wc);
        g_valTip = CreateWindowExW(WS_EX_TOOLWINDOW | WS_EX_TOPMOST | WS_EX_NOACTIVATE, L"MixValueTip", L"", WS_POPUP, 0, 0, 1, 1, g_hWnd, nullptr, g_hInst, nullptr);
    }
    std::wstring t = PctText(g_drag);
    HDC dc = GetDC(g_valTip); HGDIOBJ o = SelectObject(dc, TahomaFont()); SIZE sz; GetTextExtentPoint32W(dc, t.c_str(), (int)t.size(), &sz); SelectObject(dc, o); ReleaseDC(g_valTip, dc);
    const Slider& s = g_sl[g_drag];
    POINT p = { s.x + 16, s.y + ThumbOff(s) }; ClientToScreen(g_hWnd, &p);
    SetWindowTextW(g_valTip, t.c_str());
    SetWindowPos(g_valTip, HWND_TOPMOST, p.x, p.y, sz.cx + 8, 17, SWP_NOACTIVATE | SWP_SHOWWINDOW);
    InvalidateRect(g_valTip, nullptr, TRUE); UpdateWindow(g_valTip);
}

static void PublishMix() {
    shared::State* s = shared::Open(); if (!s) return;
    int mp = g_sl[1].pct, vp = g_sl[0].pct, mu = g_muted ? 1 : 0;
    if (s->mixSet && s->masterPct == mp && s->volumePct == vp && s->mixMuted == mu) return;
    s->masterPct = mp; s->volumePct = vp; s->mixMuted = mu; s->mixSet = 1; shared::Bump(s);
}
static HWND g_tips;
enum { TIP_SLIDER = 1, TIP_BTN = 40, TIP_TITLE = 60, TIP_LOGO = 61 };
static void TipRect(UINT id, RECT r) {
    TOOLINFOW ti = {}; ti.cbSize = TTTOOLINFOW_V2_SIZE; ti.hwnd = g_hWnd; ti.uId = id; ti.rect = r;
    SendMessageW(g_tips, TTM_NEWTOOLRECTW, 0, (LPARAM)&ti);
}
static void TipsForTab() {
    for (int i = 0; i < NSL; i++) {
        const Slider& s = g_sl[i];
        TipRect(TIP_SLIDER + i, s.tab == g_tab ? RECT{ s.x, s.y, s.x + 14, s.y + s.h } : RECT{ 0, 0, 0, 0 });
    }
}
static void TipsInit() {
    INITCOMMONCONTROLSEX ic = { sizeof(ic), ICC_WIN95_CLASSES }; InitCommonControlsEx(&ic);
    g_tips = CreateWindowExW(WS_EX_TOPMOST, TOOLTIPS_CLASSW, nullptr, WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP, 0, 0, 0, 0, g_hWnd, nullptr, g_hInst, nullptr);
    auto add = [](UINT id, RECT r) {
        TOOLINFOW ti = {};
        ti.cbSize = TTTOOLINFOW_V2_SIZE;
        ti.hwnd = g_hWnd; ti.uId = id; ti.lpszText = LPSTR_TEXTCALLBACKW; ti.rect = r;
        SendMessageW(g_tips, TTM_ADDTOOLW, 0, (LPARAM)&ti);
    };
    for (int i = 0; i < NSL; i++) add(TIP_SLIDER + i, RECT{ 0, 0, 0, 0 });
    for (int i = 1; i < BT_COUNT; i++) if (kBtn[i].tip) add(TIP_BTN + i, RECT{ kBtn[i].x, kBtn[i].y, kBtn[i].x + kBtn[i].w, kBtn[i].y + kBtn[i].h });
    add(TIP_TITLE, g_titleBox);
    add(TIP_LOGO, kLogo);
    TipsForTab();
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
    } else if (wcsstr(rel, L"CTEQ") != nullptr || wcsstr(rel, L"Equalizer") != nullptr) {
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

    ShellExecuteW(nullptr, L"open", (std::wstring(kRoot) + rel).c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}
static int BtnAt(int x, int y) { for (int i = 1; i < BT_COUNT; i++) if (InBtn(i, x, y)) return i; return BT_NONE; }
static int SliderAt(int x, int y) {
    for (int i = 0; i < NSL; i++) { const Slider& s = g_sl[i]; if (s.tab == g_tab && x >= s.x && x < s.x + 14 && y >= s.y && y < s.y + s.h) return i; }
    return -1;
}
static void SetTab(int t) { g_tab = t; g_tabHot = -1; TipsForTab(); InvalidateRect(g_hWnd, nullptr, FALSE); }

static LRESULT CALLBACK WndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    int x = GET_X_LPARAM(l), y = GET_Y_LPARAM(l);
    if (g_tips && (m == WM_MOUSEMOVE || m == WM_LBUTTONDOWN || m == WM_LBUTTONUP || m == WM_RBUTTONDOWN || m == WM_RBUTTONUP)) {
        MSG msg = { h, m, w, l }; SendMessageW(g_tips, TTM_RELAYEVENT, 0, (LPARAM)&msg);
    }
    switch (m) {
    case WM_TIMER: if (w == 1) PublishMix(); return 0;
    case WM_ERASEBKGND: return 1;
    case WM_SETCURSOR:
        if (LOWORD(l) == HTCLIENT) {
            POINT p; GetCursorPos(&p); ScreenToClient(h, &p);
            int k = CursorKind(p.x, p.y);
            SetCursor(k == 1 && g_curHand ? g_curHand : LoadCursor(nullptr, k ? IDC_HAND : IDC_ARROW)); return TRUE;
        }
        break;
    case WM_PAINT: { PAINTSTRUCT ps; HDC dc = BeginPaint(h, &ps); Paint(dc); EndPaint(h, &ps); return 0; }
    case WM_LBUTTONDOWN: {
        if (g_devOpen) { g_devOpen = false; InvalidateRect(h, nullptr, FALSE); return 0; }
        if (InR(kDevFace, x, y)) { g_devOpen = g_devFocus = true; InvalidateRect(h, nullptr, FALSE); return 0; }
        if (int b = BtnAt(x, y)) { g_btnPressed = g_btnHot = b; SetCapture(h); InvalidateRect(h, nullptr, FALSE); return 0; }
        if (InR(g_titleBox, x, y) && CursorKind(x, y) == 1) { ShowSysMenu(); return 0; }
        if (y < 38) { ReleaseCapture(); SendMessageW(h, WM_NCLBUTTONDOWN, HTCAPTION, 0); return 0; }
        for (int t = 0; t < 2; t++) if (t != g_tab && InR(kTab[t], x, y)) { SetTab(t); return 0; }
        if (InR(kLogo, x, y) && CursorKind(x, y)) { g_logoPressed = true; SetCapture(h); return 0; }
        if (g_tab == 0) for (int i = 0; i < 2; i++) if (InR(kMute[i], x, y)) { g_mutePressed = i; SetCapture(h); InvalidateRect(h, nullptr, FALSE); return 0; }
        if (g_tab == 0 && InR(kMute2, x, y)) { g_mutePressed = 2; SetCapture(h); InvalidateRect(h, nullptr, FALSE); return 0; }
        if (g_tab == 0) for (int b = 0; b < 2; b++) if (InR(kBal[b], x, y)) { g_balPressed = b; SetCapture(h); InvalidateRect(h, nullptr, FALSE); return 0; }
        if (int i = SliderAt(x, y); i >= 0) {
            g_drag = i; SliderFromMouse(g_sl[i], y); ValTipUpdate();
            SetCapture(h); InvalidateRect(h, nullptr, FALSE); return 0;
        }
        return 0;
    }
    case WM_MOUSEMOVE: {
        if (g_drag >= 0) { SliderFromMouse(g_sl[g_drag], y); ValTipUpdate(); InvalidateRect(h, nullptr, FALSE); return 0; }
        if (g_mutePressed >= 0) { InvalidateRect(h, nullptr, FALSE); return 0; }
        int b = BtnAt(x, y);
        if (b != g_btnHot) { g_btnHot = b; InvalidateRect(h, nullptr, FALSE); }
        int t = InR(kTab[0], x, y) ? 0 : InR(kTab[1], x, y) ? 1 : -1;
        if (t != g_tabHot) { g_tabHot = t; InvalidateRect(h, nullptr, FALSE); }
        TRACKMOUSEEVENT te = { sizeof(te), TME_LEAVE, h, 0 }; TrackMouseEvent(&te);
        return 0;
    }
    case WM_MOUSELEAVE:
        if (g_btnPressed == BT_NONE && (g_btnHot || g_tabHot >= 0)) { g_btnHot = BT_NONE; g_tabHot = -1; InvalidateRect(h, nullptr, FALSE); }
        return 0;
    case WM_LBUTTONUP: {
        ReleaseCapture();
        if (g_drag >= 0) { g_drag = -1; ValTipUpdate(); SaveState(); InvalidateRect(h, nullptr, FALSE); return 0; }
        if (g_balPressed >= 0) {
            int b = g_balPressed; g_balPressed = -1;
            if (InR(kBal[b], x, y)) { SliderSetPct(g_sl[b ? SL_TREBLE : SL_BASS], 50); SaveState(); }
            InvalidateRect(h, nullptr, FALSE); return 0;
        }
        if (g_logoPressed) {
            g_logoPressed = false;
            if (InR(kLogo, x, y) && CursorKind(x, y)) ShellExecuteW(nullptr, L"open", L"http://www.creative.com", nullptr, nullptr, SW_SHOWNORMAL);
            return 0;
        }
        if (g_mutePressed >= 0) {
            int i = g_mutePressed; g_mutePressed = -1;
            if (i == 2) { if (InR(kMute2, x, y)) { g_mute2 = !g_mute2; SaveState(); } }
            else if (InR(kMute[i], x, y)) { g_muted = !g_muted; SaveState(); TrayUpdate(); }
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
            case BT_DEFAULT: for (int i = 0; i < NSL; i++) if (g_sl[i].tab == 0) SliderSetPct(g_sl[i], 50); SaveState(); break;
            case BT_APP1: Launch(L"EAX\\EAX.exe"); break;
            case BT_APP2: Launch(L"Speaker Settings\\SpkSet.exe"); break;
            case BT_APP3: Launch(L"Diagnostics\\diagnos3.exe"); break;
            case BT_APP4: Launch(L"Equalizer\\CTEQ.exe"); break;
            }
            InvalidateRect(h, nullptr, FALSE);
        }
        return 0;
    }
    case WM_KEYDOWN:
        if (w == VK_ESCAPE && g_devOpen) { g_devOpen = false; InvalidateRect(h, nullptr, FALSE); return 0; }
        break;
    case WM_ACTIVATE: if (LOWORD(w) == WA_INACTIVE && g_devOpen) { g_devOpen = false; InvalidateRect(h, nullptr, FALSE); } break;
    case WM_NOTIFY:
        if (((NMHDR*)l)->code == TTN_GETDISPINFOW) {
            auto* di = (NMTTDISPINFOW*)l; static std::wstring s; UINT id = (UINT)di->hdr.idFrom;
            if (id >= TIP_SLIDER && id < TIP_SLIDER + NSL) s = PctText(id - TIP_SLIDER);
            else if (id >= TIP_BTN && id < TIP_BTN + BT_COUNT) s = kBtn[id - TIP_BTN].tip;
            else if (id == TIP_TITLE) s = L"System Menu";
            else if (id == TIP_LOGO) s = L"Visit Creative.com";
            di->lpszText = (LPWSTR)s.c_str(); return 0;
        }
        break;
    case WM_TRAY:
        if (l == WM_LBUTTONUP || l == WM_LBUTTONDBLCLK) { ShowWindow(h, SW_RESTORE); SetForegroundWindow(h); }
        return 0;
    case WM_DESTROY: {
        SaveState();
        NOTIFYICONDATAW nd = {}; nd.cbSize = sizeof(nd); nd.hWnd = h; nd.uID = 1; Shell_NotifyIconW(NIM_DELETE, &nd);
        PostQuitMessage(0); return 0;
    }
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
    HANDLE hSingleMutex = CreateMutexW(nullptr, TRUE, L"Local\\Creative_SurMixer_SingleInstance_Mutex");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        if (hSingleMutex) CloseHandle(hSingleMutex);
        HWND existingWnd = FindWindowW(L"CreativeMixerClone", nullptr);
        if (!existingWnd) existingWnd = FindWindowW(nullptr, L"Surround Mixer");
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
    WNDCLASSEXW wc = { sizeof(wc) }; wc.lpfnWndProc = WndProc; wc.hInstance = hi; wc.lpszClassName = L"CreativeMixerClone";
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hIcon = LoadIconW(hi, MAKEINTRESOURCEW(1));
    wc.hIconSm = (HICON)LoadImageW(hi, MAKEINTRESOURCEW(1), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
    if (!wc.hIcon) {
        std::wstring exe = std::wstring(kMixDir) + L"\\SurMixer.exe";
        ExtractIconExW(exe.c_str(), 0, &wc.hIcon, &wc.hIconSm, 1);
    }
    RegisterClassExW(&wc);
    g_hWnd = CreateWindowExW(WS_EX_APPWINDOW, wc.lpszClassName, L"Surround Mixer", WS_POPUP | WS_SYSMENU | WS_MINIMIZEBOX,
                             (GetSystemMetrics(SM_CXSCREEN) - WIN_W) / 2, (GetSystemMetrics(SM_CYSCREEN) - WIN_H) / 2, WIN_W, WIN_H, nullptr, nullptr, hi, nullptr);
    if (wc.hIcon) SendMessageW(g_hWnd, WM_SETICON, ICON_BIG, (LPARAM)wc.hIcon);
    if (wc.hIconSm) SendMessageW(g_hWnd, WM_SETICON, ICON_SMALL, (LPARAM)wc.hIconSm);
    if (cmd && wcsncmp(cmd, L"--selftest ", 11) == 0) {
        std::wstring d = cmd + 11; while (!d.empty() && (d.back() == L' ' || d.back() == L'"')) d.pop_back();
        SaveShot(d + L"\\basic.png");
        g_tab = 1; SaveShot(d + L"\\adv.png"); g_tab = 0;
        for (int i = 0; i < 3; i++) SliderSetPct(g_sl[i], 50);
        SaveShot(d + L"\\basic_default.png");
        for (int i = 0; i < 3; i++) SliderSetPct(g_sl[i], 100);
        g_muted = true; SaveShot(d + L"\\muted.png"); g_muted = false;
        g_devFocus = true; SaveShot(d + L"\\focus.png");
        g_devOpen = true; SaveShot(d + L"\\open.png");
        g_devOpen = g_devFocus = false;
        g_mute2 = true; g_balPressed = 0; SaveShot(d + L"\\unlocked.png"); g_mute2 = false; g_balPressed = -1;
        {
            HDC scr = GetDC(nullptr), mem = CreateCompatibleDC(scr); HBITMAP bm = CreateCompatibleBitmap(scr, 64, 220); HGDIOBJ o = SelectObject(mem, bm);
            RECT r = { 0, 0, 64, 220 }; FillRect(mem, &r, (HBRUSH)GetStockObject(WHITE_BRUSH));
            LabelV(mem, L"Front Left", 20, 20, RGB(0, 0, 0)); SelectObject(mem, o);
            CLSID png{}; UINT n = 0, sz = 0; GetImageEncodersSize(&n, &sz); std::vector<BYTE> buf(sz); auto* enc = (ImageCodecInfo*)buf.data(); GetImageEncoders(n, sz, enc);
            for (UINT i = 0; i < n; i++) if (!wcscmp(enc[i].MimeType, L"image/png")) png = enc[i].Clsid;
            { Bitmap b(bm, nullptr); b.Save((d + L"\\label.png").c_str(), &png, nullptr); }
            DeleteObject(bm); DeleteDC(mem); ReleaseDC(nullptr, scr);
        }
        return 0;
    }
    LoadState();
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
    TipsInit();
    SetTimer(g_hWnd, 1, 100, nullptr); PublishMix();
    TrayUpdate();
    ShowWindow(g_hWnd, SW_SHOW); UpdateWindow(g_hWnd);
    MSG msg; while (GetMessageW(&msg, nullptr, 0, 0)) { TranslateMessage(&msg); DispatchMessageW(&msg); }
    return 0;
}
