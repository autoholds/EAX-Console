#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <gdiplus.h>
#include <shellapi.h>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <algorithm>
#include <cstdint>
#include <cstdio>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "msimg32.lib")
#pragma comment(lib, "shell32.lib")

using namespace Gdiplus;

static constexpr int WIN_W = 500;
static constexpr int WIN_H = 368;
static constexpr int BOTTOM_Y = 332;

enum ButtonState { STATE_NORMAL = 0, STATE_HOVER, STATE_PRESSED, NUM_STATES };

static constexpr int TITLE_H = 38;
static constexpr int TAB_H = 24;
static constexpr int CONTENT_H = 238;
static constexpr int BOTTOM_H = 36;

static constexpr int CLOSE_X = 468, CLOSE_Y = 6, CLOSE_W = 24, CLOSE_H = 24;
static constexpr int MIN_X = 444, MIN_Y = 6, MIN_W = 24, MIN_H = 24;
static constexpr int HOWDOI_X = 412, HOWDOI_Y = 6, HOWDOI_W = 16, HOWDOI_H = 20;
static constexpr int HELP_X = 381, HELP_Y = 6, HELP_W = 32, HELP_H = 20;

static constexpr int DEFAULT_X = 17, DEFAULT_Y = 337, DEFAULT_W = 90, DEFAULT_H = 25;
static constexpr int APP_Y = 337, APP_W = 25, APP_H = 25;
static constexpr int APP_MIXER_X   = 122;
static constexpr int APP_SPEAKER_X = 154;
static constexpr int APP_DIAG_X    = 187;
static constexpr int APP_EQ_X      = 219;

struct TabRect { int x1, x2; };
static const TabRect g_tabBounds[5] = {
    { 24, 116 },
    { 116, 211 },
    { 211, 309 },
    { 309, 405 },
    { 405, 480 }
};

static const std::vector<std::wstring> g_hardwarePresets = {
    L"Auditorium",
    L"Ball Park",
    L"Bathroom",
    L"Cathedral",
    L"Chapel",
    L"Church",
    L"Concert Hall",
    L"Garage",
    L"Haunted Cavern",
    L"Indoor Arena",
    L"Jazz Club",
    L"Living Room",
    L"Opera House",
    L"Small Room",
    L"Theater"
};

static int g_activeTab = 0;
static bool g_enableEffects = true;
static bool g_enableKaraoke = false;

static bool g_trayClose = true;
static bool g_micEnabled = true, g_spkEnabled = true;
static std::wstring g_savedMicId, g_savedSpkId;
static bool g_enableCMSS = false;
static int g_cmssMode = -1;
static int g_cmssWidening = 0;
static bool g_isDraggingCMSSSlider = false;
static bool g_isHoverCMSSThumb = false;
static bool g_enableCleanup = false;
static bool g_enableTimeScale = false;
static bool g_micMuted = false;
static int g_micMode = 0;
static bool g_karaokeActivePreset = false;
static bool g_karaokeVocSupEnabled = true;
static bool g_karaokePitchEnabled = true;
static int  g_karaokePitchStep = 0;
static int  g_karaokeMode = 0;
static bool g_balHover = false, g_balPressed = false;
static int g_timeScaleAmt = 50;

static int g_karaokeVocSup = 50;
static int g_karaokePitch = 50;
static int g_karaokeMicVol = 17;

static int g_cleanupNoiseRem = 71;
static int g_cleanupClickRem = 89;
static int g_cleanupBandAdj = 69;
static bool g_isPressedMute = false;
static int g_radioPressed = -1;

static int g_dragSliderIdx = -1;
static bool g_isHoverMute = false;

static int g_selectedPreset = 8;

static const wchar_t* kEfxCat[6] = { L"Environment", L"Advanced EQ", L"Special FX", L"Custom", L"Studio", L"Karaoke" };
static const std::vector<std::wstring> g_advEqPresets = {
    L"EQ Acoustic", L"EQ Chamber", L"EQ Classical", L"EQ Disco", L"EQ Drum & Bass", L"EQ Flat", L"EQ Heavy Metal",
    L"EQ Jazz", L"EQ Latin", L"EQ New Age", L"EQ Pop", L"EQ Rock", L"EQ Techno", L"EQ Vocal",
};
static const std::vector<std::wstring> g_noPresets;
static int g_efxCat = 0, g_catSel[6] = { 8, 5, -1, -1, -1, -1 }, g_catTop[6] = { 3, 0, 0, 0, 0, 0 };
static int g_scrollOffset = 3;
static int g_effectsAmount = 0;
static bool g_isDraggingSlider = false;
static bool g_isDraggingScrollbar = false;
static bool g_isScrollbarHovered = false;
static int g_dragStartMouseY = 0;
static int g_dragStartScrollOffset = 0;

static ButtonState g_stateClose   = STATE_NORMAL;
static ButtonState g_stateMin     = STATE_NORMAL;
static ButtonState g_stateHowDoI  = STATE_NORMAL;
static ButtonState g_stateHelp    = STATE_NORMAL;
static ButtonState g_stateDefault = STATE_NORMAL;

enum { CH_ARROWL, CH_ARROWR, CH_CLOSE, CH_MIN, CH_HELP, CH_HOWDOI, CH_DEFAULT, CH_APP1, CH_APP2, CH_APP3, CH_APP4, CH_COUNT };
static const struct { const wchar_t* name; int x, y, w, h; } kChromeBtn[CH_COUNT] = {
    { L"arrowL", 6, 38, 32, 24 }, { L"arrowR", 462, 38, 32, 24 }, { L"close", 468, 6, 24, 22 }, { L"min", 444, 6, 24, 22 },
    { L"help", 381, 6, 32, 20 },  { L"howdoi", 413, 6, 15, 20 },  { L"default", 18, 341, 90, 22 },
    { L"app1", 118, 338, 32, 24 }, { L"app2", 150, 338, 32, 24 }, { L"app3", 182, 338, 32, 24 }, { L"app4", 214, 338, 32, 24 },
};
static std::unique_ptr<Bitmap> bmpChrome[CH_COUNT][2];
static ButtonState g_stateApp[4] = { STATE_NORMAL, STATE_NORMAL, STATE_NORMAL, STATE_NORMAL };
static int g_bottomPressed = -1;
static bool InChrome(int i, int x, int y) {
    return x >= kChromeBtn[i].x && x < kChromeBtn[i].x + kChromeBtn[i].w && y >= kChromeBtn[i].y && y < kChromeBtn[i].y + kChromeBtn[i].h;
}

static int g_tabScrollOffset = 0;

static constexpr int TAB_STRIP_W = 557;
static constexpr int MAX_TAB_SCROLL = TAB_STRIP_W - 424;
static constexpr int kTabEdge[6] = { 0, 102, 209, 320, 431, 557 };
static int TabAt(int trackX) {
    for (int t = 0; t < 5; t++) if (trackX >= kTabEdge[t] && trackX < kTabEdge[t + 1]) return t;
    return -1;
}
static bool g_tabRepeatFast = false;
static ButtonState g_stateTabEdgeLeft  = STATE_NORMAL;
static ButtonState g_stateTabEdgeRight = STATE_NORMAL;
static int g_hoverTab = -1;
static ButtonState g_stateScrlUp       = STATE_NORMAL;
static ButtonState g_stateScrlDown     = STATE_NORMAL;

static constexpr int LST_X0 = 179, LST_Y0 = 185, LST_X1 = 347, LST_Y1 = 289;
static constexpr int LST_ROW = 16, LST_PAGE = 6, SB_X = 331, SB_ARROW = 17, SB_THUMB = 28;
static const std::vector<std::wstring>& EfxList() { return g_efxCat == 0 ? g_hardwarePresets : g_efxCat == 1 ? g_advEqPresets : g_noPresets; }
static int ListMaxTop() { return std::max(0, (int)EfxList().size() - LST_PAGE); }
static int ListThumbY() {
    int mt = ListMaxTop(), travel = (LST_Y1 - LST_Y0) - 2 * SB_ARROW - SB_THUMB;
    return LST_Y0 + SB_ARROW + (mt ? (travel * g_scrollOffset + mt / 2) / mt : 0);
}
enum { SBP_NONE, SBP_UP, SBP_DOWN, SBP_THUMB, SBP_PAGEUP, SBP_PAGEDOWN };
static int SbPartAt(int x, int y) {
    if (x < SB_X || x >= LST_X1 || y < LST_Y0 || y >= LST_Y1) return SBP_NONE;
    if (y < LST_Y0 + SB_ARROW) return SBP_UP;
    if (y >= LST_Y1 - SB_ARROW) return SBP_DOWN;
    int ty = ListThumbY();
    return y < ty ? SBP_PAGEUP : (y < ty + SB_THUMB ? SBP_THUMB : SBP_PAGEDOWN);
}
static void ListScrollPart(int part) {
    int d = part == SBP_UP ? -1 : part == SBP_DOWN ? 1 : part == SBP_PAGEUP ? -LST_PAGE : part == SBP_PAGEDOWN ? LST_PAGE : 0;
    g_scrollOffset = std::clamp(g_scrollOffset + d, 0, ListMaxTop());
}
static bool g_sbRepeatFast = false;
static int  g_sbHot = SBP_NONE, g_sbPressed = SBP_NONE;
static bool g_listHover = false;
static int  g_sbDragGrabY = 0;
enum { SBS_IDLE, SBS_GHOST, SBS_EXP, SBS_HOTTHUMB, SBS_PRESSTHUMB, SBS_HOTUP, SBS_PRESSUP, SBS_HOTDOWN, SBS_PRESSDOWN, SBS_COUNT };
static const wchar_t* kSbStateNames[SBS_COUNT] = { L"idle", L"ghost", L"exp", L"hotthumb", L"pressthumb",
                                                   L"hotup", L"pressup", L"hotdown", L"pressdown" };
static std::unique_ptr<Bitmap> bmpSb[SBS_COUNT][4];

static HINSTANCE g_hInst = nullptr;
static HWND g_hWnd = nullptr;
static ULONG_PTR g_gdiplusToken = 0;

static HCURSOR g_hCurArrow = nullptr;
static HCURSOR g_hCurHand = nullptr;
static HCURSOR g_hCurHelp = nullptr;

static std::unique_ptr<Bitmap> bmpTitlebar;
static std::unique_ptr<Bitmap> bmpBottombar;
static std::unique_ptr<Bitmap> bmpTabStrip[5];
static std::unique_ptr<Bitmap> bmpTabPano[5];
static std::unique_ptr<Bitmap> bmpContent[5];

static std::unique_ptr<Bitmap> bmpCloseHover, bmpClosePressed;
static std::unique_ptr<Bitmap> bmpMinHover, bmpMinPressed;
static std::unique_ptr<Bitmap> bmpHelpHover, bmpHelpPressed;
static std::unique_ptr<Bitmap> bmpHowDoIHover, bmpHowDoIPressed;

static std::unique_ptr<Bitmap> bmpTabEdgeLeftNorm, bmpTabEdgeLeftHover, bmpTabEdgeLeftPressed;
static std::unique_ptr<Bitmap> bmpTabEdgeRightNorm, bmpTabEdgeRightHover, bmpTabEdgeRightPressed;
static std::unique_ptr<Bitmap> bmpTabBorderLeft, bmpTabBorderRight;

static std::unique_ptr<Bitmap> bmpChkChecked;
static std::unique_ptr<Bitmap> bmpChkUnchecked;
static std::unique_ptr<Bitmap> bmpTrackEmpty;
static std::unique_ptr<Bitmap> bmpTrackBlue;
static std::unique_ptr<Bitmap> bmpThumbNorm;
static std::unique_ptr<Bitmap> bmpThumbHov;
static std::unique_ptr<Bitmap> bmpThumbPres;

static std::unique_ptr<Bitmap> bmpScrlBtnUp;
static std::unique_ptr<Bitmap> bmpScrlBtnDown;
static std::unique_ptr<Bitmap> bmpScrlThumb;
static std::unique_ptr<Bitmap> bmpScrlTrack;
static std::unique_ptr<Bitmap> bmpCMSSBoxBg;
static std::unique_ptr<Bitmap> bmpGreyBoxBg;
static std::unique_ptr<Bitmap> bmpBtnMute;
static std::unique_ptr<Bitmap> bmpBtnPlayer;
static std::unique_ptr<Bitmap> bmpBtnBal;
static std::unique_ptr<Bitmap> bmpBtnEdit;
static std::unique_ptr<Bitmap> bmpSldrHoriz;
static std::unique_ptr<Bitmap> bmpSldrVolume;
static std::unique_ptr<Bitmap> bmpChkSheet;
static std::unique_ptr<Bitmap> bmpRadioSheet;
static std::unique_ptr<Bitmap> bmpSldrVert;
static std::unique_ptr<Bitmap> bmpBtnBalH;
static std::unique_ptr<Bitmap> bmpCMSSBox;
static std::unique_ptr<Bitmap> bmpTabOn[5], bmpTabOff[5];

enum { CB_NONE, CB_DEVICE, CB_CLEANUP, CB_KVOICE, CB_KMIC };
static constexpr RECT kKvFace = { 40, 168, 224, 188 }, kKvPopup = { 40, 188, 224, 232 };

static constexpr RECT kKmFace = { 282, 168, 466, 188 }, kKmPopup = { 282, 188, 466, 232 };
static int g_kvHot = 0, g_kmHot = 0;
static std::unique_ptr<Bitmap> bmpKvList[3], bmpKvFace[3][2];
static int g_comboOpen = CB_NONE, g_comboFocus = CB_NONE;
static constexpr RECT kDevFace = { 200, 6, 370, 27 },   kDevPopup = { 200, 27, 370, 44 };
static constexpr RECT kClnFace = { 40, 154, 224, 174 }, kClnPopup = { 40, 174, 224, 190 };
static std::unique_ptr<Bitmap> bmpCbDevOpen, bmpCbDevFocus, bmpCbClnOpen, bmpCbClnFocus, bmpAboutBox;
#include "portable_paths.h"
static const std::wstring kEaxDir = Creative::Dir(L"EAX");
static std::unique_ptr<Bitmap> bmpRadioChecked;
static std::unique_ptr<Bitmap> bmpRadioUnchecked;
static std::unique_ptr<Bitmap> bmpCMSSTrack;
static std::unique_ptr<Bitmap> bmpCMSSThumb;
static std::unique_ptr<Bitmap> bmpCMSSThumbHover;

static std::wstring GetExecutableDir() {
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(nullptr, path, MAX_PATH);
    std::wstring s(path);
    size_t pos = s.find_last_of(L"\\/");
    return (pos != std::wstring::npos) ? s.substr(0, pos) : L".";
}

static std::unique_ptr<Bitmap> LoadPng(const std::wstring& relPath) {
    std::wstring full = GetExecutableDir() + L"\\" + relPath;
    auto bmp = std::make_unique<Bitmap>(full.c_str());
    if (bmp->GetLastStatus() != Ok) {
        bmp = std::make_unique<Bitmap>(relPath.c_str());
    }
    return bmp;
}

static void LoadAssets() {

    g_hCurArrow = LoadCursor(nullptr, IDC_ARROW);

    std::wstring curDir = GetExecutableDir() + L"\\assets\\Cursors\\";
    g_hCurHand = LoadCursorFromFileW((curDir + L"HandBlue.cur").c_str());
    g_hCurHelp = LoadCursorFromFileW((curDir + L"SelectHelp.cur").c_str());
    if (!g_hCurHand) g_hCurHand = LoadCursor(nullptr, IDC_HAND);
    if (!g_hCurHelp) g_hCurHelp = LoadCursor(nullptr, IDC_HELP);

    bmpTitlebar  = LoadPng(L"assets\\Official_Live\\titlebar_500x38.png");
    bmpBottombar = LoadPng(L"assets\\Official_Live\\bottombar_500x36.png");

    bmpTabStrip[0] = LoadPng(L"assets\\Official_Live\\tabstrip_effects.png");
    bmpTabStrip[1] = LoadPng(L"assets\\Official_Live\\tabstrip_karaoke.png");
    bmpTabStrip[2] = LoadPng(L"assets\\Official_Live\\tabstrip_cmss3d.png");
    bmpTabStrip[3] = LoadPng(L"assets\\Official_Live\\tabstrip_cleanup.png");
    bmpTabStrip[4] = LoadPng(L"assets\\Official_Live\\tabstrip_timescale.png");

    bmpTabPano[0] = LoadPng(L"assets\\Official_Live\\tab_panorama_0.png");
    bmpTabPano[1] = LoadPng(L"assets\\Official_Live\\tab_panorama_1.png");
    bmpTabPano[2] = LoadPng(L"assets\\Official_Live\\tab_panorama_2.png");
    bmpTabPano[3] = LoadPng(L"assets\\Official_Live\\tab_panorama_3.png");
    bmpTabPano[4] = LoadPng(L"assets\\Official_Live\\tab_panorama_4.png");

    bmpContent[0]  = LoadPng(L"assets\\Official_Live\\clean_content_effects.png");
    bmpContent[1]  = LoadPng(L"assets\\Official_Live\\content_karaoke.png");
    bmpContent[2]  = LoadPng(L"assets\\Official_Live\\content_cmss3d.png");
    bmpContent[3]  = LoadPng(L"assets\\Official_Live\\content_cleanup.png");
    bmpContent[4]  = LoadPng(L"assets\\Official_Live\\content_timescale.png");

    bmpCloseHover    = LoadPng(L"assets\\Official_Live\\btn_close_hover.png");
    bmpClosePressed  = LoadPng(L"assets\\Official_Live\\btn_close_pressed.png");

    bmpMinHover      = LoadPng(L"assets\\Official_Live\\btn_min_hover.png");
    bmpMinPressed    = LoadPng(L"assets\\Official_Live\\btn_min_pressed.png");

    bmpHelpHover     = LoadPng(L"assets\\Official_Live\\btn_help_hover.png");
    bmpHelpPressed   = LoadPng(L"assets\\Official_Live\\btn_help_pressed.png");

    bmpHowDoIHover   = LoadPng(L"assets\\Official_Live\\btn_howdoi_hover.png");
    bmpHowDoIPressed = LoadPng(L"assets\\Official_Live\\btn_howdoi_pressed.png");

    bmpTabEdgeLeftNorm    = LoadPng(L"assets\\Official_Live\\btn_tab_left_32_norm.png");
    bmpTabEdgeLeftHover   = LoadPng(L"assets\\Official_Live\\btn_tab_left_32_hover.png");
    bmpTabEdgeLeftPressed = LoadPng(L"assets\\Official_Live\\btn_tab_left_32_pressed.png");

    bmpTabEdgeRightNorm    = LoadPng(L"assets\\Official_Live\\btn_tab_right_32_norm.png");
    bmpTabEdgeRightHover   = LoadPng(L"assets\\Official_Live\\btn_tab_right_32_hover.png");
    bmpTabEdgeRightPressed = LoadPng(L"assets\\Official_Live\\btn_tab_right_32_pressed.png");

    bmpTabBorderLeft  = LoadPng(L"assets\\Official_Live\\tab_edge_border_left.png");
    bmpTabBorderRight = LoadPng(L"assets\\Official_Live\\tab_edge_border_right.png");

    bmpChkChecked   = LoadPng(L"assets\\States\\chk_checked_normal.png");
    bmpChkUnchecked = LoadPng(L"assets\\States\\chk_unchecked_normal.png");

    bmpTrackEmpty   = LoadPng(L"assets\\Official_Live\\track_empty.png");
    bmpTrackBlue    = LoadPng(L"assets\\Official_Live\\track_blue.png");
    bmpThumbNorm    = LoadPng(L"assets\\Official_Live\\thumb_norm.png");
    bmpThumbHov     = LoadPng(L"assets\\Official_Live\\thumb_hov.png");
    bmpThumbPres    = LoadPng(L"assets\\Official_Live\\thumb_pres.png");

    bmpScrlBtnUp   = LoadPng(L"assets\\Official_Live\\scrl_btn_up.png");
    bmpScrlBtnDown = LoadPng(L"assets\\Official_Live\\scrl_btn_down.png");
    bmpScrlThumb   = LoadPng(L"assets\\Official_Live\\scrl_thumb.png");
    bmpScrlTrack   = LoadPng(L"assets\\Official_Live\\scrl_track.png");

    bmpCMSSBoxBg       = LoadPng(L"assets\\CMSS\\cmss_box_bg.png");
    bmpGreyBoxBg       = LoadPng(L"assets_original\\Theme\\Default\\EAX_Spr_GreyBoxBG.bmp");
    bmpBtnMute         = LoadPng(L"assets_original\\Theme\\Default\\EAX_Bttn_Mute.bmp");
    bmpBtnPlayer       = LoadPng(L"assets_original\\Theme\\Default\\EAX_Bttn_Player.bmp");
    bmpBtnBal          = LoadPng(L"assets_original\\Theme\\Default\\EAX_Bttn_Bal.bmp");
    bmpBtnEdit         = LoadPng(L"assets_original\\Theme\\Default\\EAX_Bttn_BG_RBG242_ChroBttn22.bmp");
    bmpSldrHoriz       = LoadPng(L"assets_original\\Theme\\Default\\EAX_Sldr_Horizontal14.bmp");
    bmpSldrVolume      = LoadPng(L"assets_original\\Theme\\Default\\EAX_Sldr_VolumeH.bmp");
    bmpChkSheet        = LoadPng(L"assets_original\\Theme\\Default\\EAX_Bttn_Checkbox.bmp");
    for (int i = 0; i < CH_COUNT; i++) {
        bmpChrome[i][0] = LoadPng(std::wstring(L"assets\\Official_Live\\btn\\") + kChromeBtn[i].name + L"_hover.png");
        bmpChrome[i][1] = LoadPng(std::wstring(L"assets\\Official_Live\\btn\\") + kChromeBtn[i].name + L"_pressed.png");
    }
    for (int i = 0; i < 3; i++) {
        bmpKvList[i]    = LoadPng(L"assets\\Official_Live\\combo\\kvoice_list_hl" + std::to_wstring(i + 1) + L"_40_188.png");
        bmpKvFace[i][0] = LoadPng(L"assets\\Official_Live\\combo\\kvoice_face" + std::to_wstring(i) + L"_normal_40_168.png");
        bmpKvFace[i][1] = LoadPng(L"assets\\Official_Live\\combo\\kvoice_face" + std::to_wstring(i) + L"_focused_40_168.png");
    }
    bmpCbDevOpen       = LoadPng(L"assets\\Official_Live\\combo\\device_open_200_27.png");
    bmpCbDevFocus      = LoadPng(L"assets\\Official_Live\\combo\\device_focus_203_9.png");
    bmpCbClnOpen       = LoadPng(L"assets\\Official_Live\\combo\\cleanup_open_40_174.png");
    bmpCbClnFocus      = LoadPng(L"assets\\Official_Live\\combo\\cleanup_focus_43_157.png");
    bmpAboutBox        = LoadPng(L"assets\\Official_Live\\combo\\about_box.png");
    for (int s = 0; s < SBS_COUNT; s++) {
        const wchar_t* part[4] = { L"_top.png", L"_bot.png", L"_mid.png", L"_thumb.png" };
        for (int p = 0; p < 4; p++)
            bmpSb[s][p] = LoadPng(std::wstring(L"assets\\Official_Live\\listsb\\") + kSbStateNames[s] + part[p]);
    }
    bmpRadioSheet      = LoadPng(L"assets_original\\Theme\\Default\\EAX_Bttn_Radio.bmp");
    bmpSldrVert        = LoadPng(L"assets_original\\Theme\\Default\\EAX_Sldr_Vertical14.bmp");
    bmpBtnBalH         = LoadPng(L"assets_original\\Theme\\Default\\EAX_Bttn_BalH.bmp");
    bmpCMSSBox         = LoadPng(L"assets_original\\Theme\\Default\\EAX_Spr_CMSSBG2.bmp");
    bmpRadioChecked    = LoadPng(L"assets\\CMSS\\radio_checked_normal.png");
    bmpRadioUnchecked  = LoadPng(L"assets\\CMSS\\radio_unchecked_normal.png");
    bmpCMSSTrack       = LoadPng(L"assets\\CMSS\\siledr_track.bmp");
    bmpCMSSThumb       = LoadPng(L"assets\\CMSS\\sur_siledr.bmp");
    bmpCMSSThumbHover  = LoadPng(L"assets\\CMSS\\siledr_blue_wide.bmp");
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
    STARTUPINFOW si = { sizeof(si) };
    PROCESS_INFORMATION pi = {};
    std::wstring cmd = L"\"" + exePath + L"\"";
    std::vector<wchar_t> cmdBuf(cmd.begin(), cmd.end());
    cmdBuf.push_back(L'\0');
    if (CreateProcessW(nullptr, cmdBuf.data(), nullptr, nullptr, FALSE, 0, nullptr, workDir.c_str(), &si, &pi)) {
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
        return true;
    }
    return true;
}

static void LaunchCreativeTool(const wchar_t* exeName, const wchar_t* fallbackMsg) {
    std::wstring dir = GetAppDirectory();
    std::vector<std::wstring> localCandidates;

    if (wcsstr(exeName, L"diagnos3") != nullptr) {
        localCandidates.push_back(dir + L"diagnos3.exe");
        localCandidates.push_back(dir + L"CTCplFW.exe");
        localCandidates.push_back(dir + L"diag_console.exe");
    } else if (wcsstr(exeName, L"SurMixer") != nullptr) {
        localCandidates.push_back(dir + L"SurMixer.exe");
        localCandidates.push_back(dir + L"mixer_console.exe");
        localCandidates.push_back(dir + L"surmixer_clone.exe");
    } else if (wcsstr(exeName, L"CTEQ") != nullptr) {
        localCandidates.push_back(dir + L"CTEQ.exe");
        localCandidates.push_back(dir + L"eq_console.exe");
    } else if (wcsstr(exeName, L"EAX") != nullptr) {
        localCandidates.push_back(dir + L"EAX.exe");
        localCandidates.push_back(dir + L"eax_console.exe");
    } else if (wcsstr(exeName, L"SpkSet") != nullptr) {
        localCandidates.push_back(dir + L"SpkSet.exe");
        localCandidates.push_back(dir + L"spk_console.exe");
    } else {
        localCandidates.push_back(dir + exeName);
    }

    for (const auto& path : localCandidates) {
        if (LaunchLocalExecutable(path, dir)) return;
    }

    const wchar_t* candidatePaths[] = {
        L"C:\\Program Files (x86)\\Creative\\USB SBAudigy2 NX\\Surround Mixer\\SurMixer.exe",
        L"C:\\Program Files (x86)\\Creative\\USB SBAudigy2 NX\\Speaker Settings\\SpkSet.exe",
        L"C:\\Program Files (x86)\\Creative\\USB SBAudigy2 NX\\Diagnostics\\diagnos3.exe",
        L"C:\\Program Files (x86)\\Creative\\USB SBAudigy2 NX\\Equalizer\\CTEQ.exe"
    };

    bool launched = false;
    for (const wchar_t* p : candidatePaths) {
        if (wcsstr(p, exeName) != nullptr) {
            if (GetFileAttributesW(p) != INVALID_FILE_ATTRIBUTES) {
                ShellExecuteW(nullptr, L"open", p, nullptr, nullptr, SW_SHOWNORMAL);
                launched = true;
                break;
            }
        }
    }

    if (!launched) {
        MessageBoxW(g_hWnd, fallbackMsg, L"Creative EAX Console", MB_OK | MB_ICONINFORMATION);
    }
}

struct SliderDef { int x, y, w; int* value; };

static std::vector<SliderDef> GetTabSliders() {
    std::vector<SliderDef> v;
    if (g_activeTab == 1) {
        v.push_back({ 72,  192, 120, &g_karaokeVocSup });
        v.push_back({ 72,  264, 120, &g_karaokePitch });
        v.push_back({ 314, 264, 120, &g_karaokeMicVol });
    } else if (g_activeTab == 3) {
        v.push_back({ 66,  208, 120, &g_cleanupNoiseRem });
        v.push_back({ 306, 208, 120, &g_cleanupBandAdj });
        v.push_back({ 66,  252, 120, &g_cleanupClickRem });
    } else if (g_activeTab == 4) {
        v.push_back({ 124, 218, 256, &g_timeScaleAmt });
    }
    return v;
}

static int SliderTravel(int w) { return w - 24; }
static int SliderValueAt(const SliderDef& s, int mx) {
    int v = ((mx - s.x - 12) * 100) / SliderTravel(s.w);
    return v < 0 ? 0 : (v > 100 ? 100 : v);
}

static int PitchStepAt(int mx) {
    int s = (mx - 72 - 12 + 6) / 12;
    if (mx - 72 - 12 + 6 < 0) s = 0;
    return std::clamp(s, 0, 8) - 4;
}

static void Draw9Slice(Graphics& g, Bitmap* bmp, int x, int y, int w, int h, int l, int t, int r, int b) {
    if (!bmp || bmp->GetLastStatus() != Ok) {

        SolidBrush brushBg(Color(255, 232, 232, 232));
        g.FillRectangle(&brushBg, x, y, w, h);
        Pen penDark(Color(255, 160, 160, 160), 1.0f);
        Pen penLight(Color(255, 255, 255, 255), 1.0f);
        g.DrawRectangle(&penDark, x, y, w-1, h-1);
        g.DrawLine(&penLight, x+1, y+1, x+w-2, y+1);
        g.DrawLine(&penLight, x+1, y+1, x+1, y+h-2);
        return;
    }
    int bw = (int)bmp->GetWidth();
    int bh = (int)bmp->GetHeight();
    int mw = bw - l - r;
    int mh = bh - t - b;
    int dw = w - l - r;
    int dh = h - t - b;
    if (dw < 1) dw = 1;
    if (dh < 1) dh = 1;
    g.DrawImage(bmp, Rect(x,     y,     l,  t),  0,   0,   l,  t,  UnitPixel);
    g.DrawImage(bmp, Rect(x+l,   y,     dw, t),  l,   0,   mw, t,  UnitPixel);
    g.DrawImage(bmp, Rect(x+w-r, y,     r,  t),  bw-r,0,   r,  t,  UnitPixel);
    g.DrawImage(bmp, Rect(x,     y+t,   l,  dh), 0,   t,   l,  mh, UnitPixel);
    g.DrawImage(bmp, Rect(x+l,   y+t,   dw, dh), l,   t,   mw, mh, UnitPixel);
    g.DrawImage(bmp, Rect(x+w-r, y+t,   r,  dh), bw-r,t,   r,  mh, UnitPixel);
    g.DrawImage(bmp, Rect(x,     y+h-b, l,  b),  0,   bh-b,l,  b,  UnitPixel);
    g.DrawImage(bmp, Rect(x+l,   y+h-b, dw, b),  l,   bh-b,mw, b,  UnitPixel);
    g.DrawImage(bmp, Rect(x+w-r, y+h-b, r,  b),  bw-r,bh-b,r,  b,  UnitPixel);
}

static HFONT SknFont(bool bold = false) {
    static HFONT f[2] = {
        CreateFontW(-11, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                    CLIP_DEFAULT_PRECIS, NONANTIALIASED_QUALITY, DEFAULT_PITCH, L"Tahoma"),
        CreateFontW(-11, 0, 0, 0, FW_BOLD, 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                    CLIP_DEFAULT_PRECIS, NONANTIALIASED_QUALITY, DEFAULT_PITCH, L"Tahoma"),
    };
    return f[bold ? 1 : 0];
}

static void DrawSknText(HDC dc, const wchar_t* s, int x, int y, int w, int h, UINT fmt, COLORREF col, bool bold = false) {
    HGDIOBJ old = SelectObject(dc, SknFont(bold));
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, col);
    RECT rc = { x, y, x + w, y + h };
    if ((fmt & DT_WORDBREAK) && (fmt & (DT_CENTER | DT_VCENTER))) {

        RECT cr = rc;
        DrawTextW(dc, s, -1, &cr, (fmt & ~(DT_CENTER | DT_VCENTER)) | DT_CALCRECT | DT_NOPREFIX);
        int bh = cr.bottom - cr.top;
        if (fmt & DT_VCENTER) rc.top = y + (h - bh) / 2;
        rc.right = cr.right;
        rc.bottom = rc.top + bh;
        fmt &= ~DT_VCENTER;
    }
    DrawTextW(dc, s, -1, &rc, fmt | DT_NOPREFIX);
    SelectObject(dc, old);
}

static void DrawCreativeSlider(Graphics& g, Bitmap* sheet, int x, int y, int w, int value, bool enabled, bool dragging,
                               int thumbOffset = -1 ) {
    if (!sheet || sheet->GetLastStatus() != Ok) return;
    auto track = [&](int frame, int clipL, int clipR) {
        if (clipR <= clipL) return;
        Region old; g.GetClip(&old);
        g.SetClip(Rect(clipL, y, clipR - clipL, 14));
        int sx = frame * 96;
        g.DrawImage(sheet, Rect(x,          y, 12,     14), sx,      0, 12, 14, UnitPixel);
        g.DrawImage(sheet, Rect(x + 12,     y, w - 24, 14), sx + 12, 0, 72, 14, UnitPixel);
        g.DrawImage(sheet, Rect(x + w - 12, y, 12,     14), sx + 84, 0, 12, 14, UnitPixel);
        g.SetClip(&old);
    };
    int tx = x + (thumbOffset >= 0 ? thumbOffset : (value * SliderTravel(w)) / 100);
    int thumbFrame;
    if (enabled) {
        track(0, x, x + w);
        track(1, x, tx + 12);
        thumbFrame = dragging ? 2 : 0;
    } else {
        track(2, x, x + w);
        thumbFrame = 3;
    }
    ImageAttributes ia;
    ia.SetColorKey(Color(240, 0, 240), Color(255, 16, 255));
    g.DrawImage(sheet, Rect(tx, y, 24, 14), 288 + thumbFrame * 24, 0, 24, 14, UnitPixel, &ia);
}

static void DrawCreativeCheck(Graphics& g, int x, int y, bool checked, bool enabled) {
    if (!bmpChkSheet || bmpChkSheet->GetLastStatus() != Ok) return;
    g.DrawImage(bmpChkSheet.get(), Rect(x, y, 15, 14), (enabled ? 0 : 3) * 15, checked ? 14 : 0, 15, 14, UnitPixel);
}

static void DrawSpriteFrame(Graphics& g, Bitmap* bmp, int destX, int destY, int destW, int destH,
                             int frameW, int frameH, int frame, bool useChroma = false) {
    if (!bmp || bmp->GetLastStatus() != Ok) return;
    if (useChroma) {
        ImageAttributes ia;
        ia.SetColorKey(Color(200, 0, 200), Color(255, 30, 255));
        g.DrawImage(bmp, Rect(destX, destY, destW, destH),
                    frameW * frame, 0, frameW, frameH, UnitPixel, &ia);
    } else {
        g.DrawImage(bmp, Rect(destX, destY, destW, destH),
                    frameW * frame, 0, frameW, frameH, UnitPixel);
    }
}

struct Px { int w = 0, h = 0; std::vector<uint32_t> d; uint32_t& at(int x, int y) { return d[y * w + x]; } };

static Px ToPx(Bitmap* b) {
    Px p;
    if (!b || b->GetLastStatus() != Ok) return p;
    p.w = (int)b->GetWidth(); p.h = (int)b->GetHeight(); p.d.resize(p.w * p.h);
    Rect r(0, 0, p.w, p.h);
    BitmapData bd{}; bd.Width = p.w; bd.Height = p.h; bd.Stride = p.w * 4;
    bd.PixelFormat = PixelFormat32bppARGB; bd.Scan0 = p.d.data();
    b->LockBits(&r, ImageLockModeRead | ImageLockModeUserInputBuf, PixelFormat32bppARGB, &bd);
    b->UnlockBits(&bd);
    return p;
}

static std::unique_ptr<Bitmap> FromPx(Px& p) {
    auto b = std::make_unique<Bitmap>(p.w, p.h, PixelFormat32bppARGB);
    Rect r(0, 0, p.w, p.h);
    BitmapData bd{}; bd.Width = p.w; bd.Height = p.h; bd.Stride = p.w * 4;
    bd.PixelFormat = PixelFormat32bppARGB; bd.Scan0 = p.d.data();
    b->LockBits(&r, ImageLockModeWrite | ImageLockModeUserInputBuf, PixelFormat32bppARGB, &bd);
    b->UnlockBits(&bd);
    return b;
}

static void RecolorText(Px& p, int x, int y, int w, int h, uint32_t fg) {
    std::unordered_map<uint32_t, int> hist;
    for (int j = y; j < y + h; j++) for (int i = x; i < x + w; i++) hist[p.at(i, j)]++;
    uint32_t bg = 0; int best = -1;
    for (auto& kv : hist) if (kv.second > best) { best = kv.second; bg = kv.first; }
    for (int j = y; j < y + h; j++) for (int i = x; i < x + w; i++) if (p.at(i, j) != bg) p.at(i, j) = fg;
}

static void CopyPx(Px& dst, int dx, int dy, Px& src, int sx, int sy, int w, int h) {
    for (int j = 0; j < h; j++) for (int i = 0; i < w; i++) dst.at(dx + i, dy + j) = src.at(sx + i, sy + j);
}

struct StateRegion { int x, y, w, h, group; bool text; };

enum { KG_MASTER, KG_VOICE, KG_PITCH, KG_MIC, KG_MICFX };
static const StateRegion kKaraokeRegions[] = {
    { 40,  86, 184, 16, KG_MASTER, true },
    { 282, 86, 120, 16, KG_MIC,    true },
    { 40, 182, 184, 16, KG_MASTER, true },
    { 264,182, 202, 16, KG_MIC,    true },
    { 294,202,  18, 16, KG_MIC,    true },
    { 438,202,  20, 16, KG_MIC,    true },
    { 52, 130,  18, 16, KG_VOICE,  true },
    { 196,130,  20, 16, KG_VOICE,  true },
    { 36, 202,  36, 16, KG_PITCH,  true },
    { 193,202,  36, 16, KG_PITCH,  true },
    { 40, 106, 184, 20, KG_VOICE,  false },
    { 282,106, 184, 20, KG_MICFX,  false },
};
static const StateRegion kCleanupRegions[] = {
    { 40,  76, 200, 16, 0, true }, { 40, 130, 200, 16, 0, true }, { 284, 130, 200, 16, 0, true },
    { 40, 174, 200, 16, 0, true }, { 284,174, 200, 16, 0, true },
    { 50, 144,  10, 16, 0, true }, { 202,144,  20, 16, 0, true }, { 290, 144, 10, 16, 0, true },
    { 439,144,  20, 16, 0, true }, { 50, 188,  10, 16, 0, true }, { 202, 188, 20, 16, 0, true },
    { 12, 218, 454, 16, 0, true },
    { 40,  92, 184, 20, 0, false },
};
static const StateRegion kTimeScaleRegions[] = {
    { 176, 96, 160, 14, 0, true },
    { 89, 154,  30, 16, 0, true },
    { 390,154,  30, 16, 0, true },
    { 172,182, 160, 16, 0, true },
    { 12, 218, 454, 16, 0, true },
};

static const StateRegion kEffectsRegions[] = {
    { 56,  86, 120, 16, 0, true },
    { 178, 86, 120, 16, 0, true },
    { 352,104,  18, 16, 0, true },
    { 464,104,  20, 16, 0, true },
};

struct TabIconPos { int x, y, xSel, ySel; bool bakedOn; };
static const TabIconPos kTabIcons[5] = {
    { 8,   0, 8,   1, true  },
    { 110, 0, 110, 1, false },
    { 217, 0, 217, 1, false },
    { 328, 0, 328, 1, false },
    { 439, 0, 439, 1, false },
};
static std::unique_ptr<Bitmap> bmpTabHover[5][5][2];
static const wchar_t* kTabIconNames[5] = { L"Effects", L"Karaoke", L"CMSS", L"CleanUp", L"TimeScale" };
static std::unique_ptr<Bitmap> bmpTabIconOn[5][5];

static constexpr uint32_t PX_BLACK = 0xFF000000, PX_GRAY = 0xFF808080;
static constexpr uint32_t PX_COMBO_TEXT = 0xFF404040, PX_COMBO_FIELD = 0xFFF5F5F5;

static void MatchCaptureColorDepth(std::unique_ptr<Bitmap>& bmp) {
    Px p = ToPx(bmp.get());
    if (!p.w) return;
    uint8_t lut5[256], lut6[256];
    for (int v = 0; v < 256; v++) {
        int q5 = (v * 31 + 127) / 255, q6 = (v * 63 + 127) / 255;
        lut5[v] = (uint8_t)((q5 >> 3) * 64 + (q5 & 7) * 9);
        lut6[v] = (uint8_t)((q6 >> 2) * 16 + (q6 & 3) * 5);
    }
    for (auto& c : p.d) {
        uint32_t r = lut5[(c >> 16) & 0xFF], g = lut6[(c >> 8) & 0xFF], b = lut5[c & 0xFF];
        c = (c & 0xFF000000) | (r << 16) | (g << 8) | b;
    }
    bmp = FromPx(p);
}

static void BuildStateVariants() {
    for (auto* b : { &bmpChkSheet, &bmpSldrVolume, &bmpSldrHoriz, &bmpBtnMute, &bmpBtnBal, &bmpBtnPlayer,
                     &bmpRadioSheet, &bmpSldrVert, &bmpBtnBalH, &bmpCMSSBox, &bmpContent[2] })
        MatchCaptureColorDepth(*b);

    Px kOff = ToPx(bmpContent[1].get());

    {
        Px cm = ToPx(bmpContent[2].get());
        if (cm.w == 500 && kOff.w == 500 && cm.h == kOff.h) {
            for (int j = 0; j < cm.h; j++)
                for (int i = 0; i < 500; i++)
                    if (i < 12 || i >= 488 || ((i == 12 || i == 487) && j >= 74))
                        cm.at(i, j) = kOff.at(i, j);
            Px bottom = ToPx(LoadPng(L"assets\\Official_Live\\combo\\effects_bottom_rows.png").get());
            if (bottom.w == 500 && bottom.h == 5) CopyPx(cm, 0, 233, bottom, 0, 0, 500, 5);
            bmpContent[2] = FromPx(cm);
        }
    }
    Px cOn  = ToPx(bmpContent[3].get());
    Px tOff = ToPx(bmpContent[4].get());
    if (kOff.w != 500 || cOn.w != 500 || tOff.w != 500) return;

    Px comboOn; comboOn.w = 184; comboOn.h = 20; comboOn.d.resize(184 * 20);
    CopyPx(comboOn, 0, 0, cOn, 40, 92, 184, 20);
    for (int j = 2; j < 18; j++) for (int i = 2; i < 166; i++)
        if (comboOn.at(i, j) == PX_COMBO_TEXT) comboOn.at(i, j) = PX_COMBO_FIELD;

    Px kOn = kOff;
    for (auto& r : kKaraokeRegions) {
        if (r.text) RecolorText(kOn, r.x, r.y, r.w, r.h, PX_BLACK);
        else        CopyPx(kOn, r.x, r.y, comboOn, 0, 0, 184, 20);
    }

    Px cOff = cOn;
    for (auto& r : kCleanupRegions) {
        if (r.text) { RecolorText(cOff, r.x, r.y, r.w, r.h, PX_GRAY); continue; }
        Px dis = ToPx(LoadPng(L"assets\\Official_Live\\combo\\cleanup_disabled_40_154.png").get());
        if (dis.w == 184 && dis.h == 20) CopyPx(cOff, r.x, r.y, dis, 0, 0, 184, 20);
    }

    Px tOn = tOff;
    for (auto& r : kTimeScaleRegions) RecolorText(tOn, r.x, r.y, r.w, r.h, PX_BLACK);
    for (Px* p : { &tOn, &tOff })
        for (int j = 112; j < 126; j++) for (int i = 236; i < 268; i++) p->at(i, j) = 0xFFFFFFFF;

    Px eOn = ToPx(bmpContent[0].get());
    if (eOn.w == 500) {
        for (int j = 86; j < 102; j++) for (int i = 356; i < 482; i++) eOn.at(i, j) = 0xFFF6F0F6;
        for (int j = 106; j < 120; j++) for (int i = 366; i < 464; i++) eOn.at(i, j) = 0xFFF6F0F6;
        for (int j = 122; j < 230; j++) for (int i = 177; i < 349; i++) eOn.at(i, j) = 0xFFF6F0F6;
        Px bottom = ToPx(LoadPng(L"assets\\Official_Live\\combo\\effects_bottom_rows.png").get());
        if (bottom.w == 500 && bottom.h == 5) CopyPx(eOn, 0, 233, bottom, 0, 0, 500, 5);
        Px eOff = eOn;
        for (auto& r : kEffectsRegions) RecolorText(eOff, r.x, r.y, r.w, r.h, 0xFFC0C0C0);
        bmpTabOn[0] = FromPx(eOn); bmpTabOff[0] = FromPx(eOff);
    }

    Px masks[5];
    for (int t = 0; t < 5; t++)
        masks[t] = ToPx(LoadPng(std::wstring(L"assets_original\\Theme\\Icons\\Tab_Icon_") + kTabIconNames[t] + L"_On.bmp").get());
    auto applyIcon = [&](Px& dst, int t, int dx, int dy, int sign) {
        const Px& m = masks[t];
        if (m.w != 20 || m.h != 44) return;
        const int on[3] = { 0, 70, 255 };
        for (int j = 0; j < 22 && dy + j < dst.h; j++)
            for (int i = 0; i < 20 && dx + i < dst.w; i++) {
                uint32_t a = m.d[(22 + j) * 20 + i], c = dst.at(dx + i, dy + j);
                int ch[3] = { (int)((c >> 16) & 0xFF), (int)((c >> 8) & 0xFF), (int)(c & 0xFF) };
                int al[3] = { (int)((a >> 16) & 0xFF), (int)((a >> 8) & 0xFF), (int)(a & 0xFF) };
                for (int k = 0; k < 3; k++) {
                    int v = ch[k] + sign * (al[k] * on[k] + 127) / 255;
                    ch[k] = v < 0 ? 0 : (v > 255 ? 255 : v);
                }
                dst.at(dx + i, dy + j) = 0xFF000000u | (ch[0] << 16) | (ch[1] << 8) | ch[2];
            }
    };
    for (int p = 0; p < 5; p++) {
        Px pano = ToPx(bmpTabPano[p].get());
        if (pano.w != TAB_STRIP_W) continue;
        for (int t = 0; t < 5; t++) {
            const auto& ip = kTabIcons[t];
            int ox = (p == t) ? ip.xSel : ip.x, oy = (p == t) ? ip.ySel : ip.y;
            if (ip.bakedOn) applyIcon(pano, t, ox, oy, -1);
            int ph = 24 - oy < 22 ? 24 - oy : 22;
            Px patch; patch.w = 20; patch.h = ph; patch.d.resize(20 * ph);
            CopyPx(patch, 0, 0, pano, ox, oy, 20, ph);
            applyIcon(patch, t, 0, 0, +1);
            bmpTabIconOn[p][t] = FromPx(patch);
        }
        bmpTabPano[p] = FromPx(pano);
    }
    for (int s = 0; s < 5; s++)
        for (int t = 0; t < 5; t++) {
            if (s == t) continue;
            Px hov = ToPx(LoadPng(L"assets\\Official_Live\\tabhover\\s" + std::to_wstring(s) + L"_h" + std::to_wstring(t) + L".png").get());
            if (hov.w != kTabEdge[t + 1] - kTabEdge[t] || hov.h != 24) continue;
            int ix = kTabIcons[t].x - kTabEdge[t], iy = kTabIcons[t].y;
            if (kTabIcons[t].bakedOn) applyIcon(hov, t, ix, iy, -1);
            bmpTabHover[s][t][0] = FromPx(hov);
            applyIcon(hov, t, ix, iy, +1);
            bmpTabHover[s][t][1] = FromPx(hov);
        }

    bmpTabOn[1] = FromPx(kOn); bmpTabOff[1] = FromPx(kOff);
    bmpTabOn[3] = FromPx(cOn); bmpTabOff[3] = FromPx(cOff);
    bmpTabOn[4] = FromPx(tOn); bmpTabOff[4] = FromPx(tOff);
}

template <size_t N>
static void DrawStatePage(Graphics& g, int tab, const StateRegion (&regions)[N], const bool* groupOn) {
    Bitmap* off = bmpTabOff[tab].get();
    Bitmap* on  = bmpTabOn[tab].get();
    if (!off || !on) return;
    g.DrawImage(off, Rect(0, TITLE_H + TAB_H, WIN_W, CONTENT_H), 0, 0, WIN_W, CONTENT_H, UnitPixel);
    for (auto& r : regions)
        if (groupOn[r.group])
            g.DrawImage(on, Rect(r.x, TITLE_H + TAB_H + r.y, r.w, r.h), r.x, r.y, r.w, r.h, UnitPixel);
}

#include "media_bar.h"
#include "shared_state.h"
#include "../engine/OctopusEAX/src/engine_api.h"

static bool EngineCfg(const wchar_t* name, DWORD def) {
    HKEY k; DWORD v = def, sz = sizeof(v);
    if (!RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\CreativeEAXClone", 0, KEY_READ, &k)) { RegQueryValueExW(k, name, nullptr, nullptr, (BYTE*)&v, &sz); RegCloseKey(k); }
    return v;
}
static void EngineDevices() {
    if (g_savedMicId.empty() || g_savedSpkId.empty()) {
        std::wstring mic, spk; orb::DefaultDevices(mic, spk);
        HKEY k; wchar_t b[512]; DWORD sz;
        if (!RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\CreativeEAXClone", 0, KEY_READ, &k)) {
            sz = sizeof(b); if (!RegQueryValueExW(k, L"MicId", nullptr, nullptr, (BYTE*)b, &sz) && b[0]) mic = b;
            sz = sizeof(b); if (!RegQueryValueExW(k, L"SpkId", nullptr, nullptr, (BYTE*)b, &sz) && b[0]) spk = b;
            RegCloseKey(k);
        }
        if (g_savedMicId.empty()) g_savedMicId = mic;
        if (g_savedSpkId.empty()) g_savedSpkId = spk;
    }
    { std::wstring cm, cs; orb::CurrentDevices(cm, cs); if (!cm.empty() && !cs.empty()) return; }
    orb::SetDevices(g_micEnabled ? g_savedMicId.c_str() : L"", g_spkEnabled ? g_savedSpkId.c_str() : L"");
}
static void EngineStart() {
    if (orb::Running()) return;
    EngineDevices();
    orb::SetMonitor(EngineCfg(L"EngineMonitor", 1) != 0);
    orb::Start();
}
#include "settings_store.h"
#include "config_window.h"

static void TraySync(HWND h);
static void EngineSync() {
    TraySync(g_hWnd);
    orb::AppTick();
    ST::AutoSave();
    static bool seeded = false, eaxSpeed = false, eaxIso = false;
    static std::vector<long long> lA, lB, lC, lD, lE, lF;
    auto differs = [](std::vector<long long>& last, std::vector<long long> now) { bool c = now != last; last = std::move(now); return c; };
    shared::State* s = shared::Open();
    const bool eqSet = s && s->eqSet;
    bool any = false;

    if (differs(lA, { g_enableEffects, g_selectedPreset, g_effectsAmount, g_enableCMSS, g_cmssMode, g_cmssWidening })) {
        orb::SetCmssMode(g_cmssMode == 1 ? 1 : 0); orb::SetCmssWidth(g_cmssWidening); orb::SetCmssDepth((g_cmssWidening + 100) / 2);
        orb::SetEnabled(g_enableEffects); orb::SetPreset(g_selectedPreset); orb::SetAmount(g_effectsAmount); orb::SetCmss(g_enableCMSS); any = true;
    }

    int db[7] = {}, lvl = 0; bool eqOn = false;
    if (eqSet) { for (int i = 0; i < 7; i++) db[i] = (int)lround(s->eqBand[i]); lvl = (int)lround(s->eqLevel); eqOn = s->eqOn != 0; }
    if (differs(lB, { eqSet, eqOn, lvl, db[0], db[1], db[2], db[3], db[4], db[5], db[6] })) { if (eqSet) orb::SetEq(db, lvl, eqOn); any = true; }

    const int mixPct = (s && s->mixSet) ? s->masterPct : 100; const bool mixMute = s && s->mixSet && s->mixMuted;
    if (differs(lC, { g_micBase, mixPct, mixMute, g_enableKaraoke, g_micMuted, g_karaokeMicVol })) {
        long long mic = (long long)g_micBase * (mixMute ? 0 : mixPct) / 100;
        if (g_enableKaraoke) mic = mic * (g_micMuted ? 0 : std::clamp(g_karaokeMicVol, 0, 100)) / 100;
        orb::SetMicLevel((int)mic); any = true;
    }

    const bool pitchOn = g_enableKaraoke && g_karaokePitchEnabled;
    if (differs(lD, { pitchOn, g_karaokePitchStep })) { if (seeded || pitchOn) orb::SetPitchSemitones(pitchOn ? g_karaokePitchStep : 0); any = any || seeded; }

    if (differs(lE, { g_enableTimeScale, g_timeScaleAmt })) {
        if (g_enableTimeScale) { double f = g_timeScaleAmt <= 50 ? 0.5 + g_timeScaleAmt / 100.0 : 1.0 + (g_timeScaleAmt - 50) / 50.0; orb::SetI("voiceSpeed", (int)lround(f * 100.0)); eaxSpeed = true; any = true; }
        else if (eaxSpeed) { orb::SetI("voiceSpeed", 100); eaxSpeed = false; any = true; }
    }

    const bool isoOn = g_enableCleanup || (g_enableKaraoke && g_karaokeVocSupEnabled);
    if (differs(lF, { g_enableCleanup, g_cleanupNoiseRem, g_enableKaraoke, g_karaokeVocSupEnabled, g_karaokeMode, g_karaokeVocSup })) {
        if (isoOn) {
            orb::SetI("isolate", 1); orb::SetI("isoMode", g_enableCleanup ? 1 : 1 + std::clamp(g_karaokeMode, 0, 2));
            orb::SetI("isoLevel", g_enableCleanup ? g_cleanupNoiseRem : g_karaokeVocSup); eaxIso = true; any = true;
        } else if (eaxIso) { orb::SetI("isolate", 0); eaxIso = false; any = true; }
    }
    if (any && seeded && !orb::Running()) EngineStart();
    seeded = true;
}

static void DrawDoubleBuffered(HDC hdc) {
    HDC memDC = CreateCompatibleDC(hdc);
    HBITMAP memBM = CreateCompatibleBitmap(hdc, WIN_W, WIN_H);
    HGDIOBJ oldBM = SelectObject(memDC, memBM);

    Graphics g(memDC);
    g.SetInterpolationMode(InterpolationModeNearestNeighbor);
    g.SetPixelOffsetMode(PixelOffsetModeNone);
    g.SetTextRenderingHint(TextRenderingHintClearTypeGridFit);

    SolidBrush brushWinBg(Color(255, 255, 255, 255));
    g.FillRectangle(&brushWinBg, 0, 0, WIN_W, WIN_H);

    SolidBrush brushCyan(Color(255, 0, 218, 255));
    g.FillRectangle(&brushCyan, 0, TITLE_H, WIN_W, 6);

    if (bmpTitlebar && bmpTitlebar->GetLastStatus() == Ok) {
        g.DrawImage(bmpTitlebar.get(), Rect(0, 0, WIN_W, TITLE_H), 0, 0, WIN_W, TITLE_H, UnitPixel);
    }

    auto chrome = [&](int i, ButtonState st) {
        if (st == STATE_NORMAL) return;
        Bitmap* b = bmpChrome[i][st == STATE_PRESSED ? 1 : 0].get();
        if (b && b->GetLastStatus() == Ok)
            g.DrawImage(b, Rect(kChromeBtn[i].x, kChromeBtn[i].y, kChromeBtn[i].w, kChromeBtn[i].h), 0, 0, kChromeBtn[i].w, kChromeBtn[i].h, UnitPixel);
    };
    chrome(CH_CLOSE, g_stateClose);
    chrome(CH_MIN, g_stateMin);
    chrome(CH_HELP, g_stateHelp);
    chrome(CH_HOWDOI, g_stateHowDoI);

    Region oldClip;
    g.GetClip(&oldClip);
    Rect rTabViewport(38, TITLE_H, 424, TAB_H);
    g.SetClip(rTabViewport);
    if (bmpTabPano[g_activeTab] && bmpTabPano[g_activeTab]->GetLastStatus() == Ok) {
        g.DrawImage(bmpTabPano[g_activeTab].get(), Rect(38 - g_tabScrollOffset, TITLE_H, TAB_STRIP_W, TAB_H), 0, 0, TAB_STRIP_W, TAB_H, UnitPixel);
    }
    {
        const bool en[5] = { g_enableEffects, g_enableKaraoke, g_enableCMSS, g_enableCleanup, g_enableTimeScale };
        for (int t = 0; t < 5; t++) {
            Bitmap* p = bmpTabIconOn[g_activeTab][t].get();
            if (!en[t] || !p) continue;
            const auto& ip = kTabIcons[t];
            int ox = (t == g_activeTab) ? ip.xSel : ip.x, oy = (t == g_activeTab) ? ip.ySel : ip.y;
            int ph = (int)p->GetHeight();
            g.DrawImage(p, Rect(38 - g_tabScrollOffset + ox, TITLE_H + oy, 20, ph), 0, 0, 20, ph, UnitPixel);
        }
    }

    if (g_hoverTab >= 0 && g_hoverTab != g_activeTab && g_hoverTab < 5) {
        const bool en[5] = { g_enableEffects, g_enableKaraoke, g_enableCMSS, g_enableCleanup, g_enableTimeScale };
        Bitmap* hp = bmpTabHover[g_activeTab][g_hoverTab][en[g_hoverTab] ? 1 : 0].get();
        if (hp) {
            int w = kTabEdge[g_hoverTab + 1] - kTabEdge[g_hoverTab];
            g.DrawImage(hp, Rect(38 - g_tabScrollOffset + kTabEdge[g_hoverTab], TITLE_H, w, TAB_H), 0, 0, w, TAB_H, UnitPixel);
        }
    }
    g.SetClip(&oldClip);

    if (bmpTabBorderLeft && bmpTabBorderLeft->GetLastStatus() == Ok) {
        g.DrawImage(bmpTabBorderLeft.get(), 0, TITLE_H, 6, TAB_H);
    }
    if (bmpTabBorderRight && bmpTabBorderRight->GetLastStatus() == Ok) {
        g.DrawImage(bmpTabBorderRight.get(), 494, TITLE_H, 6, TAB_H);
    }

    if (bmpTabEdgeLeftNorm && bmpTabEdgeLeftNorm->GetLastStatus() == Ok) {
        g.DrawImage(bmpTabEdgeLeftNorm.get(), 6, TITLE_H, 32, TAB_H);
    }
    chrome(CH_ARROWL, g_stateTabEdgeLeft);

    if (bmpTabEdgeRightNorm && bmpTabEdgeRightNorm->GetLastStatus() == Ok) {
        g.DrawImage(bmpTabEdgeRightNorm.get(), 462, TITLE_H, 32, TAB_H);
    }
    chrome(CH_ARROWR, g_stateTabEdgeRight);

    if (!bmpTabOff[g_activeTab] && bmpContent[g_activeTab] && bmpContent[g_activeTab]->GetLastStatus() == Ok) {
        g.DrawImage(bmpContent[g_activeTab].get(), 0, TITLE_H + TAB_H, WIN_W, CONTENT_H);
    }

    FontFamily ff(L"Tahoma");
    Gdiplus::Font fontSmall(&ff, 11.0f, FontStyleRegular, UnitPixel);
    Gdiplus::Font fontBold(&ff, 11.0f, FontStyleBold, UnitPixel);
    SolidBrush brushBlack(Color(255, 0, 0, 0));
    SolidBrush brushWhite(Color(255, 255, 255, 255));
    SolidBrush brushBlue(Color(255, 0, 120, 215));
    SolidBrush brushScrollBg(Color(255, 236, 233, 216));
    SolidBrush brushScrollThumb(Color(255, 193, 193, 193));
    Pen penBorder(Color(255, 127, 157, 185), 1.0f);
    Pen penScrollBorder(Color(255, 212, 208, 200), 1.0f);

    if (g_activeTab == 0) {
        const int Y = TITLE_H + TAB_H;
        bool m = g_enableEffects;
        const bool groups[] = { m };
        DrawStatePage(g, 0, kEffectsRegions, groups);
        DrawCreativeCheck(g, 20, Y + 54, m, true);
        {
            Region oldClip; g.GetClip(&oldClip);
            g.SetClip(Rect(LST_X0, LST_Y0, LST_X1 - LST_X0, LST_Y1 - LST_Y0));
            if (!m) {
                SolidBrush dsbl(Color(255, 246, 240, 246));
                g.FillRectangle(&dsbl, LST_X0, LST_Y0, LST_X1 - LST_X0, LST_Y1 - LST_Y0);
            } else {
                SolidBrush white(Color(255, 255, 255, 255)), stripe(Color(255, 245, 245, 245)), sel(Color(255, 0, 120, 212));
                g.FillRectangle(&white, LST_X0, LST_Y0, SB_X - LST_X0, LST_Y1 - LST_Y0);
                for (int i = 0; i <= LST_PAGE; i++) {
                    int idx = g_scrollOffset + i, ry = LST_Y0 + i * LST_ROW;
                    if (idx >= (int)EfxList().size()) break;
                    if (idx % 2 == 0) g.FillRectangle(&stripe, LST_X0, ry, 151, LST_ROW);
                    if (idx == g_selectedPreset) g.FillRectangle(&sel, LST_X0, ry, 150, LST_ROW);
                }

                bool active = g_sbHot != SBP_NONE || g_sbPressed != SBP_NONE;
                int base = active ? SBS_EXP : (g_listHover ? SBS_GHOST : SBS_IDLE);
                auto pick = [&](int part, int hot, int pressed) {
                    if (g_sbPressed == part) return pressed;
                    if (g_sbPressed == SBP_NONE && g_sbHot == part) return hot;
                    return base;
                };
                int top = pick(SBP_UP, SBS_HOTUP, SBS_PRESSUP);
                int bot = pick(SBP_DOWN, SBS_HOTDOWN, SBS_PRESSDOWN);
                int thm = pick(SBP_THUMB, SBS_HOTTHUMB, SBS_PRESSTHUMB);
                auto blit = [&](int st, int part, int y, int h) {
                    if (Bitmap* b = bmpSb[st][part].get()) g.DrawImage(b, Rect(SB_X, y, 16, h), 0, 0, 16, h, UnitPixel);
                };
                for (int y = LST_Y0 + SB_ARROW; y < LST_Y1 - SB_ARROW; y++) blit(base, 2, y, 1);
                blit(top, 0, LST_Y0, SB_ARROW);
                blit(bot, 1, LST_Y1 - SB_ARROW, SB_ARROW);
                blit(thm, 3, ListThumbY(), SB_THUMB);
            }
            g.SetClip(&oldClip);
            Pen penListBorder(m ? Color(255, 0, 0, 0) : Color(255, 201, 202, 201), 1.0f);
            g.DrawRectangle(&penListBorder, LST_X0 - 1, LST_Y0 - 1, LST_X1 - LST_X0 + 1, LST_Y1 - LST_Y0 + 1);
        }

        int shownAmount = m ? g_effectsAmount : 0;
        DrawCreativeSlider(g, bmpSldrVolume.get(), 370, Y + 106, 90, shownAmount, m, g_isDraggingSlider);

        wchar_t amt[40];
        swprintf_s(amt, L"Effects Amount : %d", shownAmount);
        HDC dc = g.GetHDC();
        if (m && g_selectedPreset >= 0 && g_selectedPreset < (int)EfxList().size())
            DrawSknText(dc, EfxList()[g_selectedPreset].c_str(), 178, Y + 104, 170, 16, 32, RGB(0, 0, 0), true);
        if (m) {
            int saved = SaveDC(dc);
            IntersectClipRect(dc, LST_X0, LST_Y0, SB_X, LST_Y1);
            for (int i = 0; i <= LST_PAGE; i++) {
                int idx = g_scrollOffset + i;
                if (idx >= (int)EfxList().size()) break;
                COLORREF c = idx == g_selectedPreset ? RGB(255, 255, 255) : RGB(64, 64, 64);
                DrawSknText(dc, EfxList()[idx].c_str(), 184, LST_Y0 + i * LST_ROW, 145, LST_ROW, DT_SINGLELINE | DT_VCENTER, c);
            }
            RestoreDC(dc, saved);
        }
        DrawSknText(dc, amt, 356, Y + 86, 132, 16, 36, m ? RGB(0, 0, 0) : RGB(192, 192, 192));
        g.ReleaseHDC(dc);
    }

    if (g_activeTab == 2) {
        const int Y = TITLE_H + TAB_H;
        bool m = g_enableCMSS;
        COLORREF fg = m ? RGB(0, 0, 0) : RGB(192, 192, 192);

        DrawCreativeCheck(g, 20, Y + 54, m, true);
        if (bmpRadioSheet && bmpRadioSheet->GetLastStatus() == Ok) {
            const int ry[2] = { 82, 134 };
            for (int i = 0; i < 2; i++)
                g.DrawImage(bmpRadioSheet.get(), Rect(20, Y + ry[i], 12, 12),
                            (m ? 0 : 3) * 12, g_cmssMode == i ? 12 : 0, 12, 12, UnitPixel);
        }
        if (bmpCMSSBox && bmpCMSSBox->GetLastStatus() == Ok)
            g.DrawImage(bmpCMSSBox.get(), Rect(332, Y + 80, 150, 149), 0, 0, 150, 149, UnitPixel);

        if (bmpSldrVert && bmpSldrVert->GetLastStatus() == Ok) {
            const int sx = 402, sy = Y + 100, sh = 110;
            int sf = (m ? 0 : 2) * 96;
            g.DrawImage(bmpSldrVert.get(), Rect(sx, sy,           14, 12),      0, sf,      14, 12, UnitPixel);
            g.DrawImage(bmpSldrVert.get(), Rect(sx, sy + 12,      14, sh - 24), 0, sf + 12, 14, 72, UnitPixel);
            g.DrawImage(bmpSldrVert.get(), Rect(sx, sy + sh - 12, 14, 12),      0, sf + 84, 14, 12, UnitPixel);
            int ty = sy + ((100 - g_cmssWidening) * (sh - 24)) / 200;
            int tf = !m ? 3 : (g_isDraggingCMSSSlider ? 2 : (g_isHoverCMSSThumb ? 1 : 0));
            ImageAttributes ia;
            ia.SetColorKey(Color(240, 0, 240), Color(255, 16, 255));
            g.DrawImage(bmpSldrVert.get(), Rect(sx, ty, 14, 24), 0, 288 + tf * 24, 14, 24, UnitPixel, &ia);
        }
        if (m && bmpBtnBalH && bmpBtnBalH->GetLastStatus() == Ok)
            g.DrawImage(bmpBtnBalH.get(), Rect(417, Y + 150, 8, 14), 0, 0, 8, 14, UnitPixel);

        wchar_t pct[16];
        swprintf_s(pct, L"%d%%", g_cmssWidening);
        HDC dc = g.GetHDC();
        DrawSknText(dc, L"CMSS 3D provides several different modes for 3D enhanced playback of stereo audio over headphones or multichannel speakers.",
                    12, Y + 6, 476, 44, 16, RGB(0, 0, 0));
        DrawSknText(dc, L"Enable CMSS 3D", 40, Y + 54, 280, 16, 32, RGB(0, 0, 0));
        DrawSknText(dc, L"CMSS",   36, Y + 82,  160, 14, 32, fg);
        DrawSknText(dc, L"Stereo Surround", 36, Y + 134, 160, 14, 32, fg);
        DrawSknText(dc, L"Classic 3D upmix effect.  Ideal for audio CDs, WMAs, MP3s and other stereo audio sources.",
                    35, Y + 96, 295, 28, 16, fg);
        DrawSknText(dc, L"In multi-speaker setups, the stereo signal is played over each speaker pair (stereo x2 or stereo x3).",
                    35, Y + 148, 295, 28, 16, fg);
        DrawSknText(dc, L"High", 368, Y + 82,  80, 16, 33, fg);
        DrawSknText(dc, L"Low",  368, Y + 212, 80, 16, 33, fg);
        DrawSknText(dc, L"Stereo Widening", 335, Y + 121, 66, 32, 21, fg);
        if (m) DrawSknText(dc, pct, 334, Y + 153, 66, 16, 37, fg);
        g.ReleaseHDC(dc);
    }

    if (g_activeTab == 1 || g_activeTab == 3 || g_activeTab == 4) {
        const int Y = TITLE_H + TAB_H;
        auto dragging = [](int i) { return g_dragSliderIdx == i; };

        if (g_activeTab == 1) {
            bool m     = g_enableKaraoke;
            bool voice = m && g_karaokeVocSupEnabled;
            bool pitch = m && g_karaokePitchEnabled;
            bool micfx = m && g_karaokeActivePreset;
            const bool groups[] = { m, voice, pitch, m, micfx };
            DrawStatePage(g, 1, kKaraokeRegions, groups);

            DrawCreativeCheck(g, 20,  Y + 54,  m, true);
            DrawCreativeCheck(g, 20,  Y + 86,  voice, m);
            DrawCreativeCheck(g, 264, Y + 86,  micfx, m);
            DrawCreativeCheck(g, 20,  Y + 182, pitch, m);

            DrawCreativeSlider(g, bmpSldrVolume.get(), 72,  Y + 130, 120, g_karaokeVocSup, voice, dragging(0));
            DrawCreativeSlider(g, bmpSldrHoriz.get(),  72,  Y + 202, 120, 0, pitch, dragging(1), (g_karaokePitchStep + 4) * 12);
            DrawCreativeSlider(g, bmpSldrVolume.get(), 314, Y + 202, 120, g_karaokeMicVol, m, dragging(2));

            DrawSpriteFrame(g, bmpBtnBal.get(), 124, Y + 217, 14, 10, 14, 10, !pitch ? 3 : (g_balPressed ? 2 : (g_balHover ? 1 : 0)));

            if (bmpBtnMute && bmpBtnMute->GetLastStatus() == Ok)
                g.DrawImage(bmpBtnMute.get(), Rect(262, Y + 200, 20, 20), (!m ? 3 : g_isPressedMute ? 2 : 0) * 20, g_micMuted ? 20 : 0, 20, 20, UnitPixel);
            if (micfx) {
                Bitmap* f = bmpKvFace[g_micMode][(g_comboFocus == CB_KMIC && g_comboOpen != CB_KMIC) ? 1 : 0].get();
                if (f) g.DrawImage(f, Rect(kKmFace.left, kKmFace.top, 184, 20), 0, 0, 184, 20, UnitPixel);
            }

            if (voice) {
                Bitmap* f = bmpKvFace[g_karaokeMode][(g_comboFocus == CB_KVOICE && g_comboOpen != CB_KVOICE) ? 1 : 0].get();
                if (f) g.DrawImage(f, Rect(kKvFace.left, kKvFace.top, 184, 20), 0, 0, 184, 20, UnitPixel);
            }

            if (voice || pitch) {
                HDC dc = g.GetHDC();
                HBRUSH panel = CreateSolidBrush(RGB(246, 240, 246));
                wchar_t buf[64];
                if (voice) {
                    RECT rc = { 40, Y + 86, 224, Y + 102 }; FillRect(dc, &rc, panel);
                    swprintf_s(buf, L"Voice Suppression amount : %d%%", g_karaokeVocSup);
                    DrawSknText(dc, buf, 40, Y + 86, 184, 16, 36, RGB(0, 0, 0));
                }
                if (pitch) {
                    RECT rc = { 40, Y + 182, 224, Y + 198 }; FillRect(dc, &rc, panel);
                    swprintf_s(buf, L"Pitch : %d Semitones", g_karaokePitchStep);
                    DrawSknText(dc, buf, 40, Y + 182, 184, 16, 36, RGB(0, 0, 0));
                }
                DeleteObject(panel);
                g.ReleaseHDC(dc);
            }
        }

        if (g_activeTab == 3) {
            bool m = g_enableCleanup;
            const bool groups[] = { m };
            DrawStatePage(g, 3, kCleanupRegions, groups);
            DrawCreativeCheck(g, 20, Y + 54, m, true);
            DrawCreativeSlider(g, bmpSldrVolume.get(), 66,  Y + 146, 120, g_cleanupNoiseRem, m, dragging(0));
            DrawCreativeSlider(g, bmpSldrVolume.get(), 306, Y + 146, 120, g_cleanupBandAdj,  m, dragging(1));
            DrawCreativeSlider(g, bmpSldrVolume.get(), 66,  Y + 190, 120, g_cleanupClickRem, m, dragging(2));
            DrawSpriteFrame(g, bmpBtnPlayer.get(), 470, Y + 216, 18, 18, 18, 18, m ? 0 : 3);
        }

        if (g_activeTab == 4) {
            bool m = g_enableTimeScale;
            const bool groups[] = { m };
            DrawStatePage(g, 4, kTimeScaleRegions, groups);
            DrawCreativeCheck(g, 20, Y + 54, m, true);
            DrawCreativeSlider(g, bmpSldrHoriz.get(), 124, Y + 156, 256, g_timeScaleAmt, m, dragging(0));
            DrawSpriteFrame(g, bmpBtnBal.get(),    244, Y + 171, 14, 10, 14, 10, m ? 0 : 3);
            DrawSpriteFrame(g, bmpBtnPlayer.get(), 470, Y + 216, 18, 18, 18, 18, m ? 0 : 3);

            double factor = g_timeScaleAmt <= 50 ? 0.5 + g_timeScaleAmt / 100.0 : 1.0 + (g_timeScaleAmt - 50) / 50.0;
            wchar_t buf[16];
            swprintf_s(buf, L"%.2f", factor);
            HDC dc = g.GetHDC();
            HFONT font = CreateFontW(-11, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                                     CLIP_DEFAULT_PRECIS, NONANTIALIASED_QUALITY, DEFAULT_PITCH, L"Tahoma");
            HGDIOBJ oldFont = SelectObject(dc, font);
            SetBkMode(dc, TRANSPARENT);
            SetTextColor(dc, m ? RGB(64, 64, 64) : RGB(109, 109, 109));
            RECT rc = { 232, Y + 110, 268, Y + 128 };
            DrawTextW(dc, buf, -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            SelectObject(dc, oldFont);
            DeleteObject(font);
            g.ReleaseHDC(dc);
        }
    }

    if (bmpBottombar && bmpBottombar->GetLastStatus() == Ok) {
        MB::Draw(g);
        g.DrawImage(bmpBottombar.get(), Rect(0, BOTTOM_Y + 3, WIN_W, BOTTOM_H - 3), 0, 3, WIN_W, BOTTOM_H - 3, UnitPixel);
    }

    chrome(CH_DEFAULT, g_stateDefault);
    for (int i = 0; i < 4; i++) chrome(CH_APP1 + i, g_stateApp[i]);

    auto patch = [&](Bitmap* b, int x, int y) {
        if (b && b->GetLastStatus() == Ok)
            g.DrawImage(b, Rect(x, y, (int)b->GetWidth(), (int)b->GetHeight()), 0, 0, (int)b->GetWidth(), (int)b->GetHeight(), UnitPixel);
    };
    if (g_comboOpen == CB_DEVICE) patch(bmpCbDevOpen.get(), kDevPopup.left, kDevPopup.top);
    else if (g_comboFocus == CB_DEVICE) patch(bmpCbDevFocus.get(), 203, 9);
    if (g_activeTab == 3 && g_enableCleanup) {
        if (g_comboOpen == CB_CLEANUP) patch(bmpCbClnOpen.get(), kClnPopup.left, kClnPopup.top);
        else if (g_comboFocus == CB_CLEANUP) patch(bmpCbClnFocus.get(), 43, 157);
    }
    if (g_activeTab == 1 && g_comboOpen == CB_KVOICE) patch(bmpKvList[g_kvHot].get(), kKvPopup.left, kKvPopup.top);
    if (g_activeTab == 1 && g_comboOpen == CB_KMIC) patch(bmpKvList[g_kmHot].get(), kKmPopup.left, kKmPopup.top);

    BitBlt(hdc, 0, 0, WIN_W, WIN_H, memDC, 0, 0, SRCCOPY);

    SelectObject(memDC, oldBM);
    DeleteObject(memBM);
    DeleteDC(memDC);
}

static void OpenEaxHelp(const std::wstring& target) {
    typedef HWND (WINAPI *HtmlHelpW_t)(HWND, LPCWSTR, UINT, DWORD_PTR);
    static HMODULE hh = LoadLibraryW(L"hhctrl.ocx");
    static auto htmlHelp = hh ? (HtmlHelpW_t)GetProcAddress(hh, "HtmlHelpW") : nullptr;
    if (htmlHelp) htmlHelp(g_hWnd, target.c_str(), 0 , 0);
    else ShellExecuteW(nullptr, L"open", L"hh.exe", target.c_str(), nullptr, SW_SHOWNORMAL);
}

static std::wstring EaxChmPath() { return std::wstring(kEaxDir) + L"\\Help\\EAX.chm"; }

static LRESULT CALLBACK AboutProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    switch (m) {
    case WM_PAINT: {
        PAINTSTRUCT ps; HDC dc = BeginPaint(h, &ps);
        Graphics gg(dc);
        if (bmpAboutBox && bmpAboutBox->GetLastStatus() == Ok)
            gg.DrawImage(bmpAboutBox.get(), Rect(0, 0, 472, 322), 0, 0, 472, 322, UnitPixel);
        EndPaint(h, &ps);
        return 0;
    }
    case WM_LBUTTONUP:
        if (GET_X_LPARAM(l) >= 424 && GET_X_LPARAM(l) < 462 && GET_Y_LPARAM(l) >= 70 && GET_Y_LPARAM(l) < 96) DestroyWindow(h);
        return 0;
    case WM_KEYDOWN:
        if (w == VK_ESCAPE || w == VK_RETURN || w == VK_SPACE) DestroyWindow(h);
        return 0;
    case WM_DESTROY:
        EnableWindow(g_hWnd, TRUE);
        SetForegroundWindow(g_hWnd);
        return 0;
    }
    return DefWindowProcW(h, m, w, l);
}

static void ShowAboutBox() {
    static bool registered = false;
    if (!registered) {
        WNDCLASSEXW wc = { sizeof(wc) };
        wc.lpfnWndProc = AboutProc; wc.hInstance = g_hInst; wc.lpszClassName = L"EAXConsoleAbout";
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        RegisterClassExW(&wc);
        registered = true;
    }
    RECT r; GetWindowRect(g_hWnd, &r);
    HWND a = CreateWindowExW(WS_EX_TOOLWINDOW, L"EAXConsoleAbout", L"", WS_POPUP | WS_VISIBLE,
                             r.left, r.top, 472, 322, g_hWnd, nullptr, g_hInst, nullptr);
    EnableWindow(g_hWnd, FALSE);
    SetFocus(a);
}

struct LayerMenuWin { HWND hwnd = nullptr; int x = 0, y = 0, w = 0, h = 0; Px bg; const Px* face = nullptr; };
static Px g_lmMainFace[3], g_lmSubFace[7];
static LayerMenuWin g_lmMain, g_lmSub;

static LRESULT CALLBACK LayerMenuProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    if (m == WM_PAINT) {
        LayerMenuWin& lm = (h == g_lmMain.hwnd) ? g_lmMain : g_lmSub;
        PAINTSTRUCT ps; HDC dc = BeginPaint(h, &ps);
        if (lm.face && lm.face->w == lm.w && (int)lm.bg.d.size() == lm.w * lm.h) {
            BITMAPINFO bi = {}; bi.bmiHeader.biSize = sizeof(bi.bmiHeader);
            bi.bmiHeader.biWidth = lm.w; bi.bmiHeader.biHeight = -lm.h; bi.bmiHeader.biPlanes = 1; bi.bmiHeader.biBitCount = 32;
            std::vector<uint32_t> out(lm.w * lm.h);
            for (int i = 0; i < lm.w * lm.h; i++) {
                uint32_t f = lm.face->d[i], b = lm.bg.d[i], a = f >> 24, ia = 255 - a;

                uint32_t r = (((f >> 16) & 255) * a + ((b >> 16) & 255) * ia) / 255;
                uint32_t gg = (((f >> 8) & 255) * a + ((b >> 8) & 255) * ia) / 255;
                uint32_t bl = ((f & 255) * a + (b & 255) * ia) / 255;
                out[i] = (r << 16) | (gg << 8) | bl;
            }
            SetDIBitsToDevice(dc, 0, 0, lm.w, lm.h, 0, 0, 0, lm.h, out.data(), &bi, DIB_RGB_COLORS);
        }
        EndPaint(h, &ps);
        return 0;
    }
    if (m == WM_MOUSEACTIVATE) return MA_NOACTIVATE;
    return DefWindowProcW(h, m, w, l);
}

static void LayerMenuOpen(LayerMenuWin& lm, int x, int y, int w, int h, const Px* face) {
    static bool registered = false;
    if (!registered) {
        WNDCLASSEXW wc = { sizeof(wc) };
        wc.lpfnWndProc = LayerMenuProc; wc.hInstance = g_hInst; wc.lpszClassName = L"EAXLayerMenu";
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        RegisterClassExW(&wc);
        registered = true;
    }
    lm.x = x; lm.y = y; lm.w = w; lm.h = h; lm.face = face;
    lm.bg.w = w; lm.bg.h = h; lm.bg.d.assign(w * h, 0);
    HDC screen = GetDC(nullptr), mem = CreateCompatibleDC(screen);
    BITMAPINFO bi = {}; bi.bmiHeader.biSize = sizeof(bi.bmiHeader);
    bi.bmiHeader.biWidth = w; bi.bmiHeader.biHeight = -h; bi.bmiHeader.biPlanes = 1; bi.bmiHeader.biBitCount = 32;
    void* bits = nullptr;
    HBITMAP dib = CreateDIBSection(screen, &bi, DIB_RGB_COLORS, &bits, nullptr, 0);
    HGDIOBJ old = SelectObject(mem, dib);
    BitBlt(mem, 0, 0, w, h, screen, x, y, SRCCOPY);
    GdiFlush();
    memcpy(lm.bg.d.data(), bits, w * h * 4);
    SelectObject(mem, old); DeleteObject(dib); DeleteDC(mem); ReleaseDC(nullptr, screen);
    lm.hwnd = CreateWindowExW(WS_EX_TOOLWINDOW | WS_EX_TOPMOST | WS_EX_NOACTIVATE, L"EAXLayerMenu", L"", WS_POPUP,
                              x, y, w, h, g_hWnd, nullptr, g_hInst, nullptr);
    ShowWindow(lm.hwnd, SW_SHOWNOACTIVATE);
    UpdateWindow(lm.hwnd);
}

static void LayerMenuClose(LayerMenuWin& lm) {
    if (lm.hwnd) DestroyWindow(lm.hwnd);
    lm.hwnd = nullptr;
}

static void LayerMenuSetFace(LayerMenuWin& lm, const Px* face) {
    if (lm.hwnd && lm.face != face) { lm.face = face; InvalidateRect(lm.hwnd, nullptr, FALSE); UpdateWindow(lm.hwnd); }
}

static void ShowHowDoIMenu(HWND hWnd) {
    std::wstring ini = std::wstring(kEaxDir) + L"\\Help\\EAX.ini";
    std::wstring html[7];
    for (int i = 1; i <= 6; i++) {
        wchar_t key[4], target[512];
        swprintf_s(key, L"%d", i);
        GetPrivateProfileStringW(L"HTML", key, L"", target, 512, ini.c_str());
        html[i] = target;
    }
    if (g_lmMainFace[0].w != 135) {
        const wchar_t* mainNames[3] = { L"main_none", L"main_howdoi", L"main_about" };
        for (int i = 0; i < 3; i++) g_lmMainFace[i] = ToPx(LoadPng(std::wstring(L"assets\\Official_Live\\layermenu\\") + mainNames[i] + L".png").get());
        g_lmSubFace[0] = ToPx(LoadPng(L"assets\\Official_Live\\layermenu\\sub_none.png").get());
        for (int i = 1; i <= 6; i++)
            g_lmSubFace[i] = ToPx(LoadPng(L"assets\\Official_Live\\layermenu\\sub_item" + std::to_wstring(i) + L".png").get());
    }
    POINT pt; GetCursorPos(&pt);
    LayerMenuOpen(g_lmMain, pt.x, pt.y, 135, 54, &g_lmMainFace[0]);

    int mainHot = 0, subHot = 0, action = 0;
    auto inWin = [](const LayerMenuWin& lm, POINT p) { return lm.hwnd && p.x >= lm.x && p.x < lm.x + lm.w && p.y >= lm.y && p.y < lm.y + lm.h; };
    auto update = [&](POINT p) {
        if (inWin(g_lmSub, p)) {
            subHot = std::clamp((int)(p.y - g_lmSub.y) / 18 + 1, 1, 6);
            mainHot = 1;
        } else if (inWin(g_lmMain, p)) {
            int ry = p.y - g_lmMain.y;
            mainHot = ry < 18 ? 1 : (ry < 36 ? 0 : 2);
            subHot = 0;
            LayerMenuSetFace(g_lmMain, &g_lmMainFace[mainHot]);
            if (mainHot == 1 && !g_lmSub.hwnd) LayerMenuOpen(g_lmSub, g_lmMain.x + 132, g_lmMain.y + 1, 310, 108, &g_lmSubFace[0]);
            if (mainHot != 1 && g_lmSub.hwnd) LayerMenuClose(g_lmSub);
        }
        LayerMenuSetFace(g_lmMain, &g_lmMainFace[mainHot]);
        if (g_lmSub.hwnd) LayerMenuSetFace(g_lmSub, &g_lmSubFace[subHot]);
    };
    update(pt);
    SetCapture(g_lmMain.hwnd);
    bool done = false;
    MSG msg;
    while (!done && GetMessageW(&msg, nullptr, 0, 0)) {
        if (msg.message == WM_MOUSEMOVE || msg.message == WM_LBUTTONDOWN || msg.message == WM_RBUTTONDOWN) {
            POINT p; GetCursorPos(&p);
            update(p);
            if (msg.message != WM_MOUSEMOVE) {
                if (inWin(g_lmSub, p)) { action = subHot; done = true; }
                else if (inWin(g_lmMain, p)) { if (mainHot == 2) { action = 7; done = true; } }
                else done = true;
            }
            continue;
        }
        if (msg.message == WM_KEYDOWN && msg.wParam == VK_ESCAPE) { done = true; continue; }
        if (msg.message == WM_CAPTURECHANGED || (msg.message == WM_ACTIVATEAPP && !msg.wParam)) done = true;
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
        if (GetCapture() != g_lmMain.hwnd) done = true;
    }
    ReleaseCapture();
    LayerMenuClose(g_lmSub);
    LayerMenuClose(g_lmMain);
    if (action >= 1 && action <= 6 && !html[action].empty()) OpenEaxHelp(std::wstring(kEaxDir) + L"\\Help\\" + html[action]);
    else if (action == 7) ShowAboutBox();
}

constexpr UINT WM_TRAY = WM_APP + 10, ID_TRAY_OPEN = 9001, ID_TRAY_EXIT = 9002;
static bool g_trayShown = false;
static UINT g_taskbarCreated = 0;
static void TrayAdd(HWND h) {
    NOTIFYICONDATAW nd = { sizeof(nd) }; nd.hWnd = h; nd.uID = 1; nd.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP; nd.uCallbackMessage = WM_TRAY;
    nd.hIcon = (HICON)LoadImageW(g_hInst, MAKEINTRESOURCEW(1), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
    wcscpy_s(nd.szTip, L"EAX Advanced HD");
    g_trayShown = Shell_NotifyIconW(NIM_ADD, &nd) != FALSE;
}
static void TrayRemove(HWND h) { if (!g_trayShown) return; NOTIFYICONDATAW nd = { sizeof(nd) }; nd.hWnd = h; nd.uID = 1; Shell_NotifyIconW(NIM_DELETE, &nd); g_trayShown = false; }
static void TraySync(HWND h) { if (g_trayClose && !g_trayShown) TrayAdd(h); else if (!g_trayClose && g_trayShown && IsWindowVisible(h)) TrayRemove(h); }
static void ShowFromTray(HWND h) { ShowWindow(h, IsIconic(h) ? SW_RESTORE : SW_SHOW); SetForegroundWindow(h); }
static void TrayMenu(HWND h) {
    HMENU m = CreatePopupMenu(); AppendMenuW(m, MF_STRING | MF_DEFAULT, ID_TRAY_OPEN, L"Open EAX Console"); AppendMenuW(m, MF_SEPARATOR, 0, nullptr); AppendMenuW(m, MF_STRING, ID_TRAY_EXIT, L"Exit");
    POINT pt; GetCursorPos(&pt); SetForegroundWindow(h);
    TrackPopupMenu(m, TPM_RIGHTBUTTON, pt.x, pt.y, 0, h, nullptr); PostMessageW(h, WM_NULL, 0, 0); DestroyMenu(m);
}

static void CloseOrHide(HWND h) {
    if (!g_trayClose) { DestroyWindow(h); return; }
    if (!g_trayShown) TrayAdd(h);
    if (!g_trayShown) { DestroyWindow(h); return; }
    if (CFG::win) ShowWindow(CFG::win, SW_HIDE);
    ST::AutoSave(true);
    ShowWindow(h, SW_HIDE);
}

static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (g_taskbarCreated && msg == g_taskbarCreated) { g_trayShown = false; TraySync(hWnd); return 0; }
    switch (msg) {
    case WM_CREATE:
        g_taskbarCreated = RegisterWindowMessageW(L"TaskbarCreated");
        g_hWnd = hWnd;
        LoadAssets();
        BuildStateVariants();
        MB::Init();
        ST::LoadCurrent();
        EngineSync();
        if (!orb::AppWanted().empty()) EngineStart();
        SetTimer(hWnd, 300, 100, nullptr);
        if (EngineCfg(L"EngineAutoStart", 0)) EngineStart();
        return 0;

    case WM_SETCURSOR: {
        if (LOWORD(lParam) != HTCLIENT) break;

        if (g_stateTabEdgeLeft == STATE_PRESSED || g_stateTabEdgeRight == STATE_PRESSED) {
            SetCursor(g_hCurHand);
            return TRUE;
        }

        POINT pt;
        GetCursorPos(&pt);
        ScreenToClient(hWnd, &pt);

        if (pt.x >= HELP_X && pt.x < HELP_X + HELP_W && pt.y >= HELP_Y && pt.y < HELP_Y + HELP_H) {
            SetCursor(g_hCurHelp);
            return TRUE;
        }

        bool isInteractive = false;
        if (pt.y >= TITLE_H && pt.y < TITLE_H + TAB_H) {

            if (pt.x >= 6 && pt.x < 494) isInteractive = true;
        }
        if (pt.y >= BOTTOM_Y || MB::HandAt(pt.x, pt.y)) isInteractive = true;
        if (g_activeTab == 0) {
            if (pt.x >= 20 && pt.x <= 130 && pt.y >= 116 && pt.y <= 130) isInteractive = true;
            if (g_enableEffects && pt.x >= 177 && pt.x <= 348 && pt.y >= 184 && pt.y <= 291) isInteractive = true;
            if (g_enableEffects && pt.x >= 370 && pt.x < 460 && pt.y >= 166 && pt.y <= 184) isInteractive = true;
        }
        if (g_activeTab == 2) {
            if (pt.x >= 20 && pt.x <= 112 && pt.y >= 116 && pt.y <= 130) isInteractive = true;
            if (g_enableCMSS) {
                if (pt.x >= 14 && pt.x <= 315 && pt.y >= 144 && pt.y <= 248) isInteractive = true;
                if (pt.x >= 385 && pt.x <= 425 && pt.y >= 158 && pt.y <= 276) isInteractive = true;
            }
        }
        if (g_activeTab == 1) {
            if (pt.x >= 20 && pt.x <= 118 && pt.y >= 116 && pt.y <= 130) isInteractive = true;
            if (g_enableKaraoke) {
                if (pt.x >= 20  && pt.x <= 170 && pt.y >= 148 && pt.y <= 162) isInteractive = true;
                if (pt.x >= 20  && pt.x <= 66  && pt.y >= 244 && pt.y <= 258) isInteractive = true;
                if (pt.x >= 264 && pt.x <= 372 && pt.y >= 148 && pt.y <= 162) isInteractive = true;
                if (pt.x >= 262 && pt.x < 282 && pt.y >= 262 && pt.y < 282) isInteractive = true;
                if (g_karaokeActivePreset && pt.x >= 282 && pt.x < 466 && pt.y >= 168 && pt.y < 188) isInteractive = true;
            }
        }
        if ((g_activeTab == 3 || g_activeTab == 4) && pt.x >= 20 && pt.x <= 138 && pt.y >= 116 && pt.y <= 130)
            isInteractive = true;
        if (g_activeTab == 1 || g_activeTab == 3 || g_activeTab == 4) {
            for (const auto& s : GetTabSliders()) {
                if (pt.x >= s.x - 4 && pt.x <= s.x + s.w + 4 && pt.y >= s.y - 4 && pt.y <= s.y + 18) { isInteractive = true; break; }
            }
        }
        if (pt.x >= CLOSE_X && pt.x < CLOSE_X + CLOSE_W && pt.y >= CLOSE_Y && pt.y < CLOSE_Y + CLOSE_H) isInteractive = true;
        if (pt.x >= MIN_X && pt.x < MIN_X + MIN_W && pt.y >= MIN_Y && pt.y < MIN_Y + MIN_H) isInteractive = true;
        if (pt.x >= HOWDOI_X && pt.x < HOWDOI_X + HOWDOI_W && pt.y >= HOWDOI_Y && pt.y < HOWDOI_Y + HOWDOI_H) isInteractive = true;

        if (isInteractive) {
            SetCursor(g_hCurHand);
            return TRUE;
        }

        SetCursor(g_hCurArrow);
        return TRUE;
    }

    case WM_ERASEBKGND:
        return 1;

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        DrawDoubleBuffered(hdc);
        EndPaint(hWnd, &ps);
        return 0;
    }

    case WM_TIMER: {
        if (wParam == MB::TIMER_ID) { MB::Tick(hWnd); return 0; }
        if (wParam == 300) { EngineSync(); return 0; }
        if (wParam == 101 || wParam == 102) {
            if (!g_tabRepeatFast) { g_tabRepeatFast = true; SetTimer(hWnd, wParam, 15, nullptr); }
            int next = wParam == 101 ? std::max(0, g_tabScrollOffset - 4) : std::min(MAX_TAB_SCROLL, g_tabScrollOffset + 4);
            if (next != g_tabScrollOffset) { g_tabScrollOffset = next; InvalidateRect(hWnd, nullptr, FALSE); }
            return 0;
        }
        if (wParam == 103) {
            if (!g_sbRepeatFast) { g_sbRepeatFast = true; SetTimer(hWnd, 103, 50, nullptr); }
            POINT p; GetCursorPos(&p); ScreenToClient(hWnd, &p);
            if (SbPartAt(p.x, p.y) == g_sbPressed) { ListScrollPart(g_sbPressed); InvalidateRect(hWnd, nullptr, FALSE); }
            return 0;
        }
        return 0;
    }

    case WM_MOUSEWHEEL: {
        POINT ptWheel;
        GetCursorPos(&ptWheel);
        ScreenToClient(hWnd, &ptWheel);
        if (ptWheel.y >= TITLE_H && ptWheel.y < TITLE_H + TAB_H) {
            int zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
            if (zDelta > 0 && g_tabScrollOffset > 0) {
                g_tabScrollOffset = std::max(0, g_tabScrollOffset - 24);
                InvalidateRect(hWnd, nullptr, FALSE);
            } else if (zDelta < 0 && g_tabScrollOffset < MAX_TAB_SCROLL) {
                g_tabScrollOffset = std::min(MAX_TAB_SCROLL, g_tabScrollOffset + 24);
                InvalidateRect(hWnd, nullptr, FALSE);
            }
            return 0;
        }

        if (g_activeTab != 0 || !g_enableEffects) return 0;
        int zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
        int nt = std::clamp(g_scrollOffset - (zDelta * 3) / WHEEL_DELTA, 0, ListMaxTop());
        g_listHover = false;
        if (g_sbPressed == SBP_NONE) g_sbHot = SBP_NONE;
        g_scrollOffset = nt;
        InvalidateRect(hWnd, nullptr, FALSE);
        return 0;
    }

    case WM_LBUTTONDOWN: {
        int x = GET_X_LPARAM(lParam);
        int y = GET_Y_LPARAM(lParam);
        if (g_comboOpen == CB_NONE && MB::InStrip(y)) { g_comboFocus = CB_NONE; if (MB::MouseDown(hWnd, x, y)) return 0; }

        {
            auto in = [&](const RECT& r) { return x >= r.left && x < r.right && y >= r.top && y < r.bottom; };
            if (g_comboOpen != CB_NONE) {
                if (g_comboOpen == CB_KVOICE && in(kKvPopup) && y >= kKvPopup.top + 1 && y < kKvPopup.top + 43)
                    g_karaokeMode = std::clamp((int)(y - kKvPopup.top - 1) / 14, 0, 2);
                if (g_comboOpen == CB_KMIC && in(kKmPopup) && y >= kKmPopup.top + 1 && y < kKmPopup.top + 43)
                    g_micMode = std::clamp((int)(y - kKmPopup.top - 1) / 14, 0, 2);
                g_comboFocus = g_comboOpen;
                g_comboOpen = CB_NONE;
                InvalidateRect(hWnd, nullptr, FALSE);
                return 0;
            }
            if (g_activeTab == 1 && g_enableKaraoke && g_karaokeActivePreset && in(kKmFace)) {
                g_comboOpen = g_comboFocus = CB_KMIC;
                g_kmHot = g_micMode;
                InvalidateRect(hWnd, nullptr, FALSE);
                return 0;
            }
            if (g_activeTab == 1 && g_enableKaraoke && g_karaokeVocSupEnabled && in(kKvFace)) {
                g_comboOpen = g_comboFocus = CB_KVOICE;
                g_kvHot = g_karaokeMode;
                InvalidateRect(hWnd, nullptr, FALSE);
                return 0;
            }
            if (in(kDevFace)) {
                g_comboOpen = g_comboFocus = CB_DEVICE;
                InvalidateRect(hWnd, nullptr, FALSE);
                return 0;
            }
            if (g_activeTab == 3 && g_enableCleanup && in(kClnFace)) {
                g_comboOpen = g_comboFocus = CB_CLEANUP;
                InvalidateRect(hWnd, nullptr, FALSE);
                return 0;
            }

            bool keepsFocus = (y >= TITLE_H && y < 136) ||
                              (g_activeTab == 1 && ((x >= 20 && x <= 170 && y >= 148 && y <= 162) ||
                                                    (x >= 264 && x <= 372 && y >= 148 && y <= 162) ||
                                                    (x >= 20 && x <= 66 && y >= 244 && y <= 258)));
            if (g_comboFocus != CB_NONE && !keepsFocus) { g_comboFocus = CB_NONE; InvalidateRect(hWnd, nullptr, FALSE); }
        }

        bool onTitleControl = (x >= kDevFace.left && x < kDevFace.right && y >= kDevFace.top && y < kDevFace.bottom) ||
                              InChrome(CH_HELP, x, y) || InChrome(CH_HOWDOI, x, y) || InChrome(CH_MIN, x, y) || InChrome(CH_CLOSE, x, y);
        if (y < TITLE_H && !onTitleControl) {
            ReleaseCapture();
            SendMessageW(hWnd, WM_NCLBUTTONDOWN, HTCAPTION, 0);
            return 0;
        }

        if (x >= CLOSE_X && x < CLOSE_X + CLOSE_W && y >= CLOSE_Y && y < CLOSE_Y + CLOSE_H) {
            g_stateClose = STATE_PRESSED;
            InvalidateRect(hWnd, nullptr, FALSE);
            return 0;
        }
        if (x >= MIN_X && x < MIN_X + MIN_W && y >= MIN_Y && y < MIN_Y + MIN_H) {
            g_stateMin = STATE_PRESSED;
            InvalidateRect(hWnd, nullptr, FALSE);
            return 0;
        }
        if (x >= HOWDOI_X && x < HOWDOI_X + HOWDOI_W && y >= HOWDOI_Y && y < HOWDOI_Y + HOWDOI_H) {
            g_stateHowDoI = STATE_PRESSED;
            InvalidateRect(hWnd, nullptr, FALSE);
            return 0;
        }
        if (x >= HELP_X && x < HELP_X + HELP_W && y >= HELP_Y && y < HELP_Y + HELP_H) {
            g_stateHelp = STATE_PRESSED;
            InvalidateRect(hWnd, nullptr, FALSE);
            return 0;
        }

        if (y >= TITLE_H && y < TITLE_H + TAB_H) {

            if ((x >= 6 && x < 38) || (x >= 462 && x < 494)) {
                bool leftArrow = x < 38;
                (leftArrow ? g_stateTabEdgeLeft : g_stateTabEdgeRight) = STATE_PRESSED;
                SetCapture(hWnd);
                SetCursor(g_hCurHand);
                g_tabScrollOffset = leftArrow ? std::max(0, g_tabScrollOffset - 24)
                                              : std::min(MAX_TAB_SCROLL, g_tabScrollOffset + 24);
                g_tabRepeatFast = false;
                SetTimer(hWnd, leftArrow ? 101 : 102, 400, nullptr);
                InvalidateRect(hWnd, nullptr, FALSE);
                return 0;
            }

            if (x >= 38 && x < 462) {
                int newTab = TabAt(x - 38 + g_tabScrollOffset);
                if (newTab >= 0 && newTab != g_activeTab) {
                    g_activeTab = newTab;
                    int bLeft = kTabEdge[g_activeTab], bRight = kTabEdge[g_activeTab + 1];
                    if (bLeft < g_tabScrollOffset)             g_tabScrollOffset = bLeft;
                    else if (bRight > g_tabScrollOffset + 424) g_tabScrollOffset = std::min(MAX_TAB_SCROLL, bRight - 424);
                }

                InvalidateRect(hWnd, nullptr, FALSE);
                return 0;
            }
        }

        if (g_activeTab == 0) {

            if (x >= 20 && x <= 130 && y >= 116 && y <= 130) {
                g_enableEffects = !g_enableEffects;
                InvalidateRect(hWnd, nullptr, FALSE);
                return 0;
            }
        }
        if (g_activeTab == 0 && g_enableEffects) {

            if (int part = SbPartAt(x, y)) {
                g_sbPressed = part;
                SetCapture(hWnd);
                if (part == SBP_THUMB) {
                    g_sbDragGrabY = y - ListThumbY();
                } else {
                    ListScrollPart(part);
                    g_sbRepeatFast = false;
                    SetTimer(hWnd, 103, 400, nullptr);
                }
                InvalidateRect(hWnd, nullptr, FALSE);
                return 0;
            }

            if (x >= LST_X0 && x < SB_X && y >= LST_Y0 && y < LST_Y1) {
                int idx = g_scrollOffset + (y - LST_Y0) / LST_ROW;
                if (idx < (int)EfxList().size()) {
                    g_selectedPreset = idx;
                    if (idx >= g_scrollOffset + LST_PAGE) g_scrollOffset = idx - LST_PAGE + 1;
                    InvalidateRect(hWnd, nullptr, FALSE);
                }
                return 0;
            }

            if (x >= 370 && x < 460 && y >= 166 && y <= 184) {
                g_isDraggingSlider = true;
                SetCapture(hWnd);
                int val = ((x - 370 - 12) * 100) / 66;
                if (val < 0) val = 0;
                if (val > 100) val = 100;
                g_effectsAmount = val;
                InvalidateRect(hWnd, nullptr, FALSE);
                return 0;
            }
        }

        if (g_activeTab == 1) {
            bool* hit = nullptr;
            if (x >= 20 && x <= 118 && y >= 116 && y <= 130) hit = &g_enableKaraoke;
            else if (g_enableKaraoke && x >= 20  && x <= 170 && y >= 148 && y <= 162) hit = &g_karaokeVocSupEnabled;
            else if (g_enableKaraoke && x >= 20  && x <= 66  && y >= 244 && y <= 258) hit = &g_karaokePitchEnabled;
            else if (g_enableKaraoke && x >= 264 && x <= 372 && y >= 148 && y <= 162) hit = &g_karaokeActivePreset;
            if (hit) { *hit = !*hit; InvalidateRect(hWnd, nullptr, FALSE); return 0; }
            if (g_enableKaraoke && x >= 262 && x < 282 && y >= 262 && y < 282) {
                g_isPressedMute = true; SetCapture(hWnd); InvalidateRect(hWnd, nullptr, FALSE); return 0;
            }

            if (g_enableKaraoke && g_karaokePitchEnabled && x >= 124 && x < 138 && y >= 279 && y < 289) {
                g_balPressed = true;
                SetCapture(hWnd);
                InvalidateRect(hWnd, nullptr, FALSE);
                return 0;
            }
        }

        if (g_activeTab == 1 || g_activeTab == 3 || g_activeTab == 4) {
            auto sliders = GetTabSliders();
            for (size_t i = 0; i < sliders.size(); i++) {
                bool en = g_activeTab == 3 ? g_enableCleanup : g_activeTab == 4 ? g_enableTimeScale : g_enableKaraoke;
                if (g_activeTab == 1 && i == 0) en = en && g_karaokeVocSupEnabled;
                if (g_activeTab == 1 && i == 1) en = en && g_karaokePitchEnabled;
                const auto& s = sliders[i];
                if (en && x >= s.x && x < s.x + s.w && y >= s.y - 2 && y < s.y + 16) {
                    g_dragSliderIdx = (int)i;
                    SetCapture(hWnd);
                    if (g_activeTab == 1 && i == 1) g_karaokePitchStep = PitchStepAt(x);
                    else *s.value = SliderValueAt(s, x);
                    InvalidateRect(hWnd, nullptr, FALSE);
                    return 0;
                }
            }
        }
        if (g_activeTab == 2) {
            if (x >= 20 && x <= 112 && y >= 116 && y <= 130) {
                g_enableCMSS = !g_enableCMSS;
                if (g_cmssMode < 0) g_cmssMode = 0;
                InvalidateRect(hWnd, nullptr, FALSE);
                return 0;
            }
            if (g_enableCMSS) {

                if (x >= 14 && x <= 315 && y >= 144 && y < 196) {
                    g_cmssMode = 0;
                    InvalidateRect(hWnd, nullptr, FALSE);
                    return 0;
                }

                if (x >= 14 && x <= 315 && y >= 196 && y <= 248) {
                    g_cmssMode = 1;
                    InvalidateRect(hWnd, nullptr, FALSE);
                    return 0;
                }

                if (x >= 385 && x <= 425 && y >= 158 && y <= 276) {
                    g_isDraggingCMSSSlider = true;
                    SetCapture(hWnd);
                    int val = (int)round((217 - y) * 100.0 / 43.0);
                    if (abs(val) <= 2) val = 0;
                    if (val < -100) val = -100;
                    if (val > 100) val = 100;
                    g_cmssWidening = val;
                    InvalidateRect(hWnd, nullptr, FALSE);
                    return 0;
                }
            }
        }
        if (g_activeTab == 3 && x >= 20 && x <= 138 && y >= 116 && y <= 130) {
            g_enableCleanup = !g_enableCleanup;
            InvalidateRect(hWnd, nullptr, FALSE);
            return 0;
        }
        if (g_activeTab == 4 && x >= 20 && x <= 138 && y >= 116 && y <= 130) {
            g_enableTimeScale = !g_enableTimeScale;
            InvalidateRect(hWnd, nullptr, FALSE);
            return 0;
        }

        for (int i = CH_DEFAULT; i <= CH_APP4; i++) {
            if (InChrome(i, x, y)) {
                g_bottomPressed = i;
                (i == CH_DEFAULT ? g_stateDefault : g_stateApp[i - CH_APP1]) = STATE_PRESSED;
                SetCapture(hWnd);
                InvalidateRect(hWnd, nullptr, FALSE);
                return 0;
            }
        }
        return 0;
    }

    case WM_MOUSEMOVE: {
        int x = GET_X_LPARAM(lParam);
        int y = GET_Y_LPARAM(lParam);
        MB::MouseMove(hWnd, x, y);

        if (g_isDraggingSlider) {
            int val = ((x - 370 - 12) * 100) / 66;
            if (val < 0) val = 0;
            if (val > 100) val = 100;
            if (val != g_effectsAmount) {
                g_effectsAmount = val;
                InvalidateRect(hWnd, nullptr, FALSE);
            }
            return 0;
        }

        if (g_isDraggingCMSSSlider) {
            int val = (int)round((217 - y) * 100.0 / 43.0);
            if (abs(val) <= 2) val = 0;
            if (val < -100) val = -100;
            if (val > 100) val = 100;
            if (val != g_cmssWidening) {
                g_cmssWidening = val;
                InvalidateRect(hWnd, nullptr, FALSE);
            }
            return 0;
        }

        if (g_dragSliderIdx >= 0) {
            auto sliders = GetTabSliders();
            if (g_dragSliderIdx < (int)sliders.size()) {
                const auto& s = sliders[g_dragSliderIdx];
                if (g_activeTab == 1 && g_dragSliderIdx == 1) {
                    int st = PitchStepAt(x);
                    if (st != g_karaokePitchStep) { g_karaokePitchStep = st; InvalidateRect(hWnd, nullptr, FALSE); }
                } else {
                    int val = SliderValueAt(s, x);
                    if (val != *s.value) {
                        *s.value = val;
                        InvalidateRect(hWnd, nullptr, FALSE);
                    }
                }
            }
            return 0;
        }

        if (g_sbPressed == SBP_THUMB) {
            int travel = (LST_Y1 - LST_Y0) - 2 * SB_ARROW - SB_THUMB, mt = ListMaxTop();
            int pos = y - g_sbDragGrabY - (LST_Y0 + SB_ARROW);
            int nt = mt ? std::clamp((pos * mt + travel / 2) / travel, 0, mt) : 0;
            if (nt != g_scrollOffset) { g_scrollOffset = nt; InvalidateRect(hWnd, nullptr, FALSE); }
            return 0;
        }

        ButtonState oldClose  = g_stateClose;
        ButtonState oldMin    = g_stateMin;
        ButtonState oldHowDoI = g_stateHowDoI;
        ButtonState oldHelp   = g_stateHelp;

        ButtonState oldEdgeLeft  = g_stateTabEdgeLeft;
        ButtonState oldEdgeRight = g_stateTabEdgeRight;
        ButtonState oldScrlUp    = g_stateScrlUp;
        ButtonState oldScrlDown  = g_stateScrlDown;
        bool oldScrlHov = g_isScrollbarHovered;
        int oldHoverTab = g_hoverTab;

        auto titleBtn = [&](ButtonState& st, int bx, int by, int bw, int bh) {
            bool in = x >= bx && x < bx + bw && y >= by && y < by + bh;
            if (st == STATE_PRESSED && (GetKeyState(VK_LBUTTON) & 0x8000)) return;
            st = in ? STATE_HOVER : STATE_NORMAL;
        };
        titleBtn(g_stateClose, CLOSE_X, CLOSE_Y, CLOSE_W, CLOSE_H);
        titleBtn(g_stateMin, MIN_X, MIN_Y, MIN_W, MIN_H);
        titleBtn(g_stateHowDoI, HOWDOI_X, HOWDOI_Y, HOWDOI_W, HOWDOI_H);
        titleBtn(g_stateHelp, HELP_X, HELP_Y, HELP_W, HELP_H);

        g_stateTabEdgeLeft  = (x >= 6 && x < 38 && y >= TITLE_H && y < TITLE_H + TAB_H) ? STATE_HOVER : STATE_NORMAL;
        g_stateTabEdgeRight = (x >= 462 && x < 494 && y >= TITLE_H && y < TITLE_H + TAB_H) ? STATE_HOVER : STATE_NORMAL;

        if (y >= TITLE_H && y < TITLE_H + TAB_H && x >= 38 && x < 462) {
            g_hoverTab = TabAt(x - 38 + g_tabScrollOffset);
        } else {
            g_hoverTab = -1;
        }

        if (g_comboOpen == CB_KMIC && x >= kKmPopup.left && x < kKmPopup.right && y >= kKmPopup.top + 1 && y < kKmPopup.top + 43) {
            int hot = std::clamp((int)(y - kKmPopup.top - 1) / 14, 0, 2);
            if (hot != g_kmHot) { g_kmHot = hot; InvalidateRect(hWnd, nullptr, FALSE); }
        }
        if (g_comboOpen == CB_KVOICE && x >= kKvPopup.left && x < kKvPopup.right && y >= kKvPopup.top + 1 && y < kKvPopup.top + 43) {
            int hot = std::clamp((int)(y - kKvPopup.top - 1) / 14, 0, 2);
            if (hot != g_kvHot) { g_kvHot = hot; InvalidateRect(hWnd, nullptr, FALSE); }
        }
        {
            bool bh = g_activeTab == 1 && x >= 124 && x < 138 && y >= 279 && y < 289;
            if (bh != g_balHover) { g_balHover = bh; InvalidateRect(hWnd, nullptr, FALSE); }
        }

        bool bottomChanged = false;
        for (int i = CH_DEFAULT; i <= CH_APP4; i++) {
            ButtonState& st = (i == CH_DEFAULT) ? g_stateDefault : g_stateApp[i - CH_APP1];
            ButtonState ns = g_bottomPressed == i ? (InChrome(i, x, y) ? STATE_PRESSED : STATE_NORMAL)
                           : (g_bottomPressed < 0 && InChrome(i, x, y) ? STATE_HOVER : STATE_NORMAL);
            if (ns != st) { st = ns; bottomChanged = true; }
        }
        if (bottomChanged) InvalidateRect(hWnd, nullptr, FALSE);

        int oldSbHot = g_sbHot;
        bool inList = g_activeTab == 0 && g_enableEffects && x >= LST_X0 - 1 && x <= LST_X1 && y >= LST_Y0 - 1 && y <= LST_Y1;
        g_listHover = inList;
        g_sbHot = inList ? SbPartAt(x, y) : SBP_NONE;
        g_isScrollbarHovered = g_listHover;

        bool oldHoverMute = g_isHoverMute;
        g_isHoverMute = (g_activeTab == 1 && x >= 262 && x < 282 && y >= 262 && y < 282);

        bool oldHoverCMSS = g_isHoverCMSSThumb;
        if (g_activeTab == 2 && g_enableCMSS) {
            int thumbY = 205 - (g_cmssWidening * 43) / 100;
            g_isHoverCMSSThumb = (x >= 394 && x <= 412 && y >= thumbY && y <= thumbY + 24);
        } else {
            g_isHoverCMSSThumb = false;
        }

        TRACKMOUSEEVENT tme;
        tme.cbSize = sizeof(TRACKMOUSEEVENT);
        tme.dwFlags = TME_LEAVE;
        tme.hwndTrack = hWnd;
        TrackMouseEvent(&tme);

        if (oldClose != g_stateClose || oldMin != g_stateMin || oldHowDoI != g_stateHowDoI || oldHelp != g_stateHelp ||
            oldEdgeLeft != g_stateTabEdgeLeft || oldEdgeRight != g_stateTabEdgeRight || oldScrlHov != g_isScrollbarHovered ||
            oldScrlUp != g_stateScrlUp || oldScrlDown != g_stateScrlDown || oldHoverTab != g_hoverTab || oldHoverCMSS != g_isHoverCMSSThumb ||
            oldHoverMute != g_isHoverMute || oldSbHot != g_sbHot) {
            InvalidateRect(hWnd, nullptr, FALSE);
        }
        return 0;
    }

    case WM_MOUSELEAVE: {
        MB::Leave(hWnd);
        g_isScrollbarHovered = false;
        g_listHover = false;
        if (g_bottomPressed < 0) { g_stateDefault = STATE_NORMAL; for (auto& s : g_stateApp) s = STATE_NORMAL; }
        if (g_sbPressed == SBP_NONE) g_sbHot = SBP_NONE;
        g_hoverTab = -1;
        g_isHoverCMSSThumb = false;
        g_isHoverMute = false;
        g_stateClose = STATE_NORMAL;
        g_stateMin = STATE_NORMAL;
        g_stateHowDoI = STATE_NORMAL;
        g_stateHelp = STATE_NORMAL;
        g_stateTabEdgeLeft = STATE_NORMAL;
        g_stateTabEdgeRight = STATE_NORMAL;
        g_stateScrlUp = STATE_NORMAL;
        g_stateScrlDown = STATE_NORMAL;
        InvalidateRect(hWnd, nullptr, FALSE);
        return 0;
    }

    case WM_LBUTTONUP: {
        int x = GET_X_LPARAM(lParam);
        int y = GET_Y_LPARAM(lParam);
        if (MB::MouseUp(hWnd, x, y)) { EngineSync(); return 0; }

        KillTimer(hWnd, 101);
        KillTimer(hWnd, 102);
        KillTimer(hWnd, 103);
        KillTimer(hWnd, 104);

        if (g_stateScrlUp == STATE_PRESSED || g_stateScrlDown == STATE_PRESSED) {
            g_stateScrlUp   = (x >= 332 && x <= 347 && y >= 185 && y <= 201) ? STATE_HOVER : STATE_NORMAL;
            g_stateScrlDown = (x >= 332 && x <= 347 && y >= 274 && y <= 290) ? STATE_HOVER : STATE_NORMAL;
            ReleaseCapture();
            InvalidateRect(hWnd, nullptr, FALSE);
        }

        if (g_stateTabEdgeLeft == STATE_PRESSED || g_stateTabEdgeRight == STATE_PRESSED) {
            g_stateTabEdgeLeft  = (x >= 6 && x < 38 && y >= TITLE_H && y < TITLE_H + TAB_H) ? STATE_HOVER : STATE_NORMAL;
            g_stateTabEdgeRight = (x >= 462 && x < 494 && y >= TITLE_H && y < TITLE_H + TAB_H) ? STATE_HOVER : STATE_NORMAL;
            ReleaseCapture();
            SetCursor(g_hCurHand);
            InvalidateRect(hWnd, nullptr, FALSE);
        }

        if (g_isDraggingSlider) {
            g_isDraggingSlider = false;
            ReleaseCapture();
        }

        if (g_isDraggingCMSSSlider) {
            g_isDraggingCMSSSlider = false;
            ReleaseCapture();
        }

        if (g_dragSliderIdx >= 0) {
            g_dragSliderIdx = -1;
            ReleaseCapture();
            InvalidateRect(hWnd, nullptr, FALSE);
        }

        if (g_isPressedMute) {
            g_isPressedMute = false;
            ReleaseCapture();
            if (x >= 262 && x < 282 && y >= 262 && y < 282) g_micMuted = !g_micMuted;
            InvalidateRect(hWnd, nullptr, FALSE);
        }

        if (g_balPressed) {
            g_balPressed = false;
            ReleaseCapture();
            if (x >= 124 && x < 138 && y >= 279 && y < 289) g_karaokePitchStep = 0;
            InvalidateRect(hWnd, nullptr, FALSE);
            return 0;
        }

        if (g_bottomPressed >= 0) {
            int b = g_bottomPressed;
            g_bottomPressed = -1;
            ReleaseCapture();
            for (int i = CH_DEFAULT; i <= CH_APP4; i++)
                ((i == CH_DEFAULT) ? g_stateDefault : g_stateApp[i - CH_APP1]) = InChrome(i, x, y) ? STATE_HOVER : STATE_NORMAL;
            InvalidateRect(hWnd, nullptr, FALSE);
            if (InChrome(b, x, y)) {
                switch (b) {
                case CH_DEFAULT:
                    g_efxCat = 0; g_selectedPreset = 8; g_scrollOffset = 3; g_effectsAmount = 100; g_enableEffects = true;
                    break;
                case CH_APP1: LaunchCreativeTool(L"SurMixer.exe", L"Launching Creative Surround Mixer"); break;
                case CH_APP2: CFG::Open(hWnd); break;
                case CH_APP3: LaunchCreativeTool(L"diagnos3.exe", L"Launching Creative Diagnostics"); break;
                case CH_APP4: LaunchCreativeTool(L"CTEQ.exe", L"Launching Graphic Equalizer"); break;
                }
            }
            return 0;
        }

        if (g_sbPressed != SBP_NONE) {
            KillTimer(hWnd, 103);
            g_sbPressed = SBP_NONE;
            g_sbHot = SbPartAt(x, y);
            ReleaseCapture();
            InvalidateRect(hWnd, nullptr, FALSE);
        }

        if (g_isDraggingScrollbar) {
            g_isDraggingScrollbar = false;
            ReleaseCapture();
        }

        if (g_stateClose == STATE_PRESSED && x >= CLOSE_X && x < CLOSE_X + CLOSE_W && y >= CLOSE_Y && y < CLOSE_Y + CLOSE_H) {
            g_stateClose = STATE_NORMAL; CloseOrHide(hWnd);
            return 0;
        }
        if (g_stateMin == STATE_PRESSED && x >= MIN_X && x < MIN_X + MIN_W && y >= MIN_Y && y < MIN_Y + MIN_H) {
            g_stateMin = STATE_NORMAL;
            ShowWindow(hWnd, SW_MINIMIZE);
            return 0;
        }
        if (g_stateHowDoI == STATE_PRESSED && x >= HOWDOI_X && x < HOWDOI_X + HOWDOI_W && y >= HOWDOI_Y && y < HOWDOI_Y + HOWDOI_H) {
            g_stateHowDoI = STATE_HOVER;
            InvalidateRect(hWnd, nullptr, FALSE);
            UpdateWindow(hWnd);
            ShowHowDoIMenu(hWnd);
            g_stateHowDoI = STATE_NORMAL;
            InvalidateRect(hWnd, nullptr, FALSE);
            return 0;
        }
        if (g_stateHelp == STATE_PRESSED && x >= HELP_X && x < HELP_X + HELP_W && y >= HELP_Y && y < HELP_Y + HELP_H) {
            g_stateHelp = STATE_NORMAL;
            InvalidateRect(hWnd, nullptr, FALSE);
            OpenEaxHelp(EaxChmPath());
            return 0;
        }

        g_stateClose  = STATE_NORMAL;
        g_stateMin    = STATE_NORMAL;
        g_stateHowDoI = STATE_NORMAL;
        g_stateHelp   = STATE_NORMAL;
        InvalidateRect(hWnd, nullptr, FALSE);
        return 0;
    }

    case WM_COMMAND: {
        int id = LOWORD(wParam);
        if (id == ID_TRAY_OPEN) { ShowFromTray(hWnd); return 0; }
        if (id == ID_TRAY_EXIT) { DestroyWindow(hWnd); return 0; }
        if (id >= 101 && id <= 106) {
            const wchar_t* topics[] = {
                L"How do I clean up noise from an audio file?",
                L"How do I convert my old tapes or vinyl records to MP3 file?",
                L"How do I enjoy audio with Surround effects?",
                L"How does Creative MediaSource help me enjoy karaoke?",
                L"How do I apply Audio Effects?",
                L"How do I change a track's playback speed?"
            };
            MessageBoxW(hWnd, topics[id - 101], L"Creative EAX Help - How Do I...", MB_OK | MB_ICONINFORMATION);
            return 0;
        }
        break;
    }

    case WM_SYSKEYDOWN:
        if (MB::Key(hWnd, (UINT)wParam, true)) return 0;
        break;

    case WM_KEYDOWN:
        if (MB::Key(hWnd, (UINT)wParam, false)) return 0;
        if (wParam == VK_ESCAPE && g_comboOpen != CB_NONE) {
            g_comboFocus = g_comboOpen;
            g_comboOpen = CB_NONE;
            InvalidateRect(hWnd, nullptr, FALSE);
            return 0;
        }
        break;

    case WM_ACTIVATE:
        if (LOWORD(wParam) == WA_INACTIVE && g_comboOpen != CB_NONE) {
            g_comboOpen = CB_NONE;
            InvalidateRect(hWnd, nullptr, FALSE);
        }
        break;

    case WM_CLOSE: CloseOrHide(hWnd); return 0;
    case WM_QUERYENDSESSION: return TRUE;
    case WM_ENDSESSION: if (wParam) { TrayRemove(hWnd); DestroyWindow(hWnd); } return 0;
    case WM_TRAY:
        if (lParam == WM_LBUTTONUP || lParam == WM_LBUTTONDBLCLK) ShowFromTray(hWnd);
        else if (lParam == WM_RBUTTONUP || lParam == WM_CONTEXTMENU) TrayMenu(hWnd);
        return 0;
    case WM_DESTROY:
        TrayRemove(hWnd);
        MB::Shutdown(hWnd);
        ST::AutoSave(true);
        orb::Stop();
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

static int RunSelfTest(const wchar_t* outDir) {
    LoadAssets();
    BuildStateVariants();
    MB::Init();
    CLSID png{};
    UINT n = 0, sz = 0;
    GetImageEncodersSize(&n, &sz);
    std::vector<BYTE> buf(sz);
    auto* enc = reinterpret_cast<ImageCodecInfo*>(buf.data());
    GetImageEncoders(n, sz, enc);
    for (UINT i = 0; i < n; i++) if (wcscmp(enc[i].MimeType, L"image/png") == 0) png = enc[i].Clsid;

    HDC screen = GetDC(nullptr);
    auto shot = [&](const wchar_t* name) {
        HDC mem = CreateCompatibleDC(screen);
        HBITMAP bm = CreateCompatibleBitmap(screen, WIN_W, WIN_H);
        HGDIOBJ old = SelectObject(mem, bm);
        DrawDoubleBuffered(mem);
        SelectObject(mem, old);
        Bitmap out(bm, nullptr);
        std::wstring path = std::wstring(outDir) + L"\\" + name + L".png";
        Status st = out.Save(path.c_str(), &png, nullptr);
        if (st != Ok) {
            FILE* f = nullptr;
            _wfopen_s(&f, (std::wstring(outDir) + L"\\selftest_error.txt").c_str(), L"a");
            if (f) { fwprintf(f, L"%ls: Save status %d, dir='%ls'\n", name, (int)st, outDir); fclose(f); }
        }
        DeleteObject(bm);
        DeleteDC(mem);
    };
    {
        struct T { const wchar_t* s; int x, y, w, h; UINT fmt; COLORREF c; };
        const T ts[] = {
            { L"Karaoke effects creates instant, karaoke background music from any CD, MP3, WMA, or Wav file, by suppressing the vocals. You can also adjust the pitch of the music and microphone effect to suit your voice.",
              12, 6, 476, 44, 16, RGB(0, 0, 0) },
            { L"Enable Karaoke",           40, 54, 180, 16, 32, RGB(0, 0, 0) },
            { L"Voice Suppression amount", 40, 86, 184, 16, 36, RGB(192, 192, 192) },
            { L"Pitch",                    40, 182, 184, 16, 36, RGB(128, 128, 128) },
            { L"Microphone Volume",        264, 182, 202, 16, 36, RGB(128, 128, 128) },
        };
        HDC mem = CreateCompatibleDC(screen);
        HBITMAP bm = CreateCompatibleBitmap(screen, WIN_W, CONTENT_H);
        HGDIOBJ old = SelectObject(mem, bm);
        {
            Graphics gg(mem);
            gg.DrawImage(bmpContent[1].get(), Rect(0, 0, WIN_W, CONTENT_H), 0, 0, WIN_W, CONTENT_H, UnitPixel);
            for (auto& t : ts) {
                Color bg; bmpContent[1]->GetPixel(t.x + t.w - 1, t.y + t.h - 1, &bg);
                SolidBrush b(bg); gg.FillRectangle(&b, t.x, t.y, t.w, t.h);
            }
        }
        for (auto& t : ts) DrawSknText(mem, t.s, t.x, t.y, t.w, t.h, t.fmt, t.c);
        SelectObject(mem, old);
        Bitmap out(bm, nullptr);
        out.Save((std::wstring(outDir) + L"\\textcheck_karaoke.png").c_str(), &png, nullptr);
        DeleteObject(bm); DeleteDC(mem);
    }
    {
        using namespace MB;
        tracks.clear(); for (int i = 0; i < 9; i++) tracks.push_back({ L"", L"", 4000 });
        auto st = [&](int c, int s, int pos, bool loop, bool pz) { cur = c; state = s; posMs = pos; repeatAll = loop; pressed = pz ? B_PLAY : -1; hot = pressed; };
        cur = -1; state = S_STOPPED; posMs = 0; tracks.clear(); shot(L"mb_empty");
        tracks.assign(9, { L"", L"", 4000 });
        st(1, S_STOPPED, 4000, false, false); shot(L"mb_stopped_2_4s");
        st(2, S_PLAYING, 1000, false, false); shot(L"mb_playing_3_1s");
        st(8, S_PLAYING, 6000, false, false); shot(L"mb_playing_9_6s");
        st(8, S_PAUSED, 7000, false, false); shot(L"mb_paused_9_7s");
        st(7, S_STOPPED, 0, true, false); shot(L"mb_loop_8");
        st(6, S_STOPPED, 0, false, true); shot(L"mb_stopped_press_play");
        pressed = hot = -1; recording = true; recMs = 2000; st(7, S_PLAYING, 0, false, false); state = S_STOPPED; shot(L"mb_rec_2s"); recording = false;
    }
    {
        struct Q { const wchar_t* n; int id, hot; };
        const Q qs[] = { { L"mn_play_hot3", MB::MN_PLAY, 3 }, { L"mn_play_hot0", MB::MN_PLAY, 0 }, { L"mn_mode", MB::MN_MODE, -1 }, { L"mn_control", MB::MN_CONTROL, -1 }, { L"mn_select", MB::MN_SELECT, -1 },
                         { L"mn_l1_hot0", MB::MN_L1, 0 }, { L"mn_l1_hot1", MB::MN_L1, 1 }, { L"mn_media_hot0", MB::MN_MEDIA, 0 }, { L"mn_rec_hot2", MB::MN_REC, 2 }, { L"mn_rec", MB::MN_REC, -1 }, { L"mn_media_hot1", MB::MN_MEDIA, 1 }, { L"mn_view", MB::MN_VIEW, -1 } };
        for (auto& q : qs) {
            int w = MB::MenuW(q.id), h = (int)MB::kMenu[q.id].items.size() * MB::SLOT;
            HDC mem = CreateCompatibleDC(screen); HBITMAP bm = CreateCompatibleBitmap(screen, w, h); HGDIOBJ old = SelectObject(mem, bm);
            MB::RenderMenu(mem, q.id, q.hot, 0x1F1F29); SelectObject(mem, old);
            Bitmap out(bm, nullptr); out.Save((std::wstring(outDir) + L"\\" + q.n + L".png").c_str(), &png, nullptr); DeleteObject(bm); DeleteDC(mem);
        }
    }
    {
        using namespace MB;
        npBase[0] = LoadPng(L"assets\\media\\np\\np_empty.png"); npBase[1] = LoadPng(L"assets\\media\\np\\np_one.png");
        npIdle = LoadPng(L"assets\\media\\np\\icon_idle.png"); npPlay = LoadPng(L"assets\\media\\np\\icon_play.png"); NpBuildCorners();
        auto np = [&](const wchar_t* name, int w, int h) {
            npW = w; npH = h; HDC mem = CreateCompatibleDC(screen); HBITMAP bm = CreateCompatibleBitmap(screen, w, h); HGDIOBJ old = SelectObject(mem, bm);
            NpPaint(mem); SelectObject(mem, old); Bitmap out(bm, nullptr); out.Save((std::wstring(outDir) + L"\\" + name + L".png").c_str(), &png, nullptr); DeleteObject(bm); DeleteDC(mem);
        };
        tracks.clear(); cur = -1; np(L"np_500_empty", 500, 208);
        tracks.assign(2, { L"", L"Door Knock", 4000 }); npSel.clear(); np(L"np_500_two", 500, 208);
        tracks.assign(8, { L"", L"Door Knock", 4000 }); npSel = { 1 }; np(L"np_big", 700, 330); np(L"np_small", 380, 180);
        tracks.clear(); npSel.clear(); npW = 500; npH = 208;
        int n = 0; for (int k = 0; k < 4; k++) for (int y = 0; y < 12; y++) for (int x = 0; x < 12; x++) n += npCorner[k][y][x];
        FILE* f = nullptr; _wfopen_s(&f, (std::wstring(outDir) + L"\\np_corners.txt").c_str(), L"w"); if (f) { fprintf(f, "corner pixels %d\n", n); fclose(f); }
    }
    if (wcsstr(outDir, L"recfmt")) { MB::RecFormatDialog(nullptr); return 0; }
    g_activeTab = 0; shot(L"t0_default");
    g_enableEffects = false; shot(L"t0_off");
    g_enableEffects = true;
    g_activeTab = 2; shot(L"t2_default");
    g_enableCMSS = false; g_cmssWidening = 40; shot(L"t2_off");
    g_enableCMSS = true;  g_cmssWidening = 0;
    g_activeTab = 1; shot(L"t1_default");
    g_enableKaraoke = true; g_karaokeVocSupEnabled = true; g_karaokePitchEnabled = true; g_karaokeActivePreset = true;
    shot(L"t1_on");
    g_activeTab = 3; shot(L"t3_default");
    g_enableCleanup = false; shot(L"t3_off");
    g_activeTab = 4; shot(L"t4_default");
    g_enableTimeScale = true; shot(L"t4_on");
    ReleaseDC(nullptr, screen);
    return 0;
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR lpCmdLine, int nCmdShow) {
    HANDLE hSingleMutex = CreateMutexW(nullptr, TRUE, L"Local\\Creative_EAX_Console_SingleInstance_Mutex");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        if (hSingleMutex) CloseHandle(hSingleMutex);
        HWND existingWnd = FindWindowW(L"CreativeEAXConsoleCleanApp", nullptr);
        if (!existingWnd) existingWnd = FindWindowW(nullptr, L"EAX Advanced HD");
        if (!existingWnd) existingWnd = FindWindowW(nullptr, L"EAX Console Application");
        if (existingWnd) {
            if (!IsWindowVisible(existingWnd)) ShowWindow(existingWnd, SW_SHOW);
            if (IsIconic(existingWnd)) ShowWindow(existingWnd, SW_RESTORE);
            SetForegroundWindow(existingWnd);
        }
        return 0;
    }
    g_hInst = hInstance;

    HMODULE hUser32 = GetModuleHandleW(L"user32.dll");
    if (hUser32) {
        typedef BOOL (WINAPI *SetProcessDpiAwarenessContextProc)(DPI_AWARENESS_CONTEXT);
        auto pfn = (SetProcessDpiAwarenessContextProc)GetProcAddress(hUser32, "SetProcessDpiAwarenessContext");
        if (pfn) {
            pfn(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
        } else {
            SetProcessDPIAware();
        }
    }

    GdiplusStartupInput gdiplusStartupInput;
    GdiplusStartup(&g_gdiplusToken, &gdiplusStartupInput, nullptr);

    if (lpCmdLine && wcsncmp(lpCmdLine, L"--selftest ", 11) == 0) {
        std::wstring dir = lpCmdLine + 11;
        while (!dir.empty() && (dir.back() == L' ' || dir.back() == L'"')) dir.pop_back();
        if (!dir.empty() && dir.front() == L'"') dir.erase(0, 1);
        return RunSelfTest(dir.c_str());
    }

    WNDCLASSEXW wc = { sizeof(WNDCLASSEXW) };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"CreativeEAXConsoleCleanApp";
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hIcon = LoadIconW(hInstance, MAKEINTRESOURCEW(1));
    wc.hIconSm = (HICON)LoadImageW(hInstance, MAKEINTRESOURCEW(1), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
    wc.hbrBackground = nullptr;
    RegisterClassExW(&wc);

    DWORD dwStyle = WS_POPUP | WS_VISIBLE | WS_SYSMENU;
    int scrW = GetSystemMetrics(SM_CXSCREEN);
    int scrH = GetSystemMetrics(SM_CYSCREEN);
    int posX = (scrW - WIN_W) / 2;
    int posY = (scrH - WIN_H) / 2;

    HWND hWnd = CreateWindowExW(
        WS_EX_APPWINDOW,
        wc.lpszClassName,
        L"EAX Advanced HD",
        dwStyle,
        posX, posY, WIN_W, WIN_H,
        nullptr, nullptr, hInstance, nullptr
    );
    if (wc.hIcon) SendMessageW(hWnd, WM_SETICON, ICON_BIG, (LPARAM)wc.hIcon);
    if (wc.hIconSm) SendMessageW(hWnd, WM_SETICON, ICON_SMALL, (LPARAM)wc.hIconSm);

    HRGN hRgn = CreateRectRgn(0, 16, WIN_W, WIN_H - 16);
    {
        Px top = ToPx(bmpTitlebar.get()), bot = ToPx(bmpBottombar.get());
        auto addRow = [&](Px& p, int py, int wy) {
            int l = 0, r = p.w;
            while (l < p.w / 2 && (p.at(l, py) & 0xFFFFFF) == 0) l++;
            while (r > p.w / 2 && (p.at(r - 1, py) & 0xFFFFFF) == 0) r--;
            HRGN row = CreateRectRgn(l, wy, r, wy + 1);
            CombineRgn(hRgn, hRgn, row, RGN_OR);
            DeleteObject(row);
        };
        for (int y = 0; y < 16; y++) {
            if (top.w == WIN_W) addRow(top, y, y);
            else { HRGN row = CreateRectRgn(0, y, WIN_W, y + 1); CombineRgn(hRgn, hRgn, row, RGN_OR); DeleteObject(row); }
            int by = BOTTOM_H - 16 + y;
            if (bot.w == WIN_W) addRow(bot, by, WIN_H - 16 + y);
            else { HRGN row = CreateRectRgn(0, WIN_H - 16 + y, WIN_W, WIN_H - 15 + y); CombineRgn(hRgn, hRgn, row, RGN_OR); DeleteObject(row); }
        }
    }
    SetWindowRgn(hWnd, hRgn, TRUE);

    ShowWindow(hWnd, SW_SHOW);
    UpdateWindow(hWnd);

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return (int)msg.wParam;
}
