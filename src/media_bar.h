

#pragma once
#include <mmsystem.h>
#include <commdlg.h>
#include <random>
#include <set>
#include "../engine/OctopusEAX/src/engine_api.h"
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "comdlg32.lib")

namespace MB {

enum Btn { B_MEDIA, B_OPEN, B_MORE, B_REC, B_RECMENU, B_PREV, B_PAUSE, B_PLAY, B_STOP, B_NEXT, B_LOOP, B_X, B_COUNT };
enum PlayState { S_STOPPED, S_PLAYING, S_PAUSED };
enum PlayMode { M_NORMAL, M_ONCE, M_SHUFFLE_REPEAT, M_SHUFFLE, M_REPEAT_TRACK };

constexpr int STRIP_Y = 300, STRIP_H = 32, FACE_Y = 306, FACE_H = 27;
constexpr UINT TIMER_ID = 200;

struct Spot { int x0, x1, y0, y1; const wchar_t* name; };
constexpr Spot kSpot[B_COUNT] = {
    { 41, 71, 309, 329, L"media" },   { 75, 103, 309, 329, L"open" },  { 111, 134, 309, 329, L"more" },
    { 222, 254, 309, 331, L"rec" },   { 254, 268, 309, 331, L"recmenu" },
    { 266, 296, 309, 331, L"prev" },  { 296, 326, 309, 331, L"pause" }, { 326, 366, 309, 331, L"play" },
    { 366, 396, 309, 331, L"stop" },  { 396, 427, 309, 331, L"next" },  { 429, 461, 309, 331, L"loop" },
    { 201, 217, 310, 326, L"x" },
};
constexpr int kFaceX[B_COUNT] = { 41, 75, 107, 222, 254, 266, 296, 326, 365, 396, 427, 0 };

struct Track { std::wstring path, title; int lenMs; };

inline std::vector<Track> tracks;
inline int cur = -1, state = S_STOPPED, mode = M_NORMAL, posMs = 0;
inline bool repeatAll = false, recording = false;
inline DWORD recStart = 0; inline int recMs = 0;
inline int pressed = -1, hot = -1;
inline std::unique_ptr<Bitmap> band[2];
inline std::unordered_map<std::wstring, std::unique_ptr<Bitmap>> faces;
inline std::mt19937 rng{ std::random_device{}() };
inline int shuffleLeft = 0;
inline HWND npWnd = nullptr;
inline bool engMonitor = true;

inline Bitmap* Face(const wchar_t* btn, const wchar_t* st) {
    std::wstring key = std::wstring(btn) + L"_" + st;
    auto it = faces.find(key);
    if (it != faces.end()) return it->second.get();
    int x = 0; for (int i = 0; i < B_COUNT; i++) if (!wcscmp(kSpot[i].name, btn)) x = kFaceX[i];
    auto b = LoadPng(L"assets\\media\\faces\\" + key + L"_" + std::to_wstring(x) + L".png");
    Bitmap* r = (b && b->GetLastStatus() == Ok) ? b.get() : nullptr;
    faces[key] = std::move(b); return r;
}
inline void RecFormatLoad();
inline void Init() {
    RecFormatLoad();
    { HKEY k; DWORD v = 1, sz = sizeof(v); if (!RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\CreativeEAXClone", 0, KEY_READ, &k)) { RegQueryValueExW(k, L"EngineMonitor", nullptr, nullptr, (BYTE*)&v, &sz); RegCloseKey(k); } engMonitor = v != 0; }
    band[0] = LoadPng(L"assets\\media\\band_normal_500x36.png"); band[1] = LoadPng(L"assets\\media\\band_rec_500x36.png");
}

inline std::wstring Mci(const std::wstring& cmd) {
    wchar_t out[256] = {}; mciSendStringW(cmd.c_str(), out, 256, nullptr); return out;
}
inline int TrackLenMs(const std::wstring& path) {
    Mci(L"close eaxlen"); mciSendStringW((L"open \"" + path + L"\" alias eaxlen").c_str(), nullptr, 0, nullptr);
    Mci(L"set eaxlen time format milliseconds"); int len = _wtoi(Mci(L"status eaxlen length").c_str()); Mci(L"close eaxlen"); return len;
}
inline void CloseTrack() { Mci(L"stop eaxm"); Mci(L"close eaxm"); }
inline bool StartTrack(int idx, int fromMs = 0) {
    CloseTrack();
    if (idx < 0 || idx >= (int)tracks.size()) return false;
    if (mciSendStringW((L"open \"" + tracks[idx].path + L"\" alias eaxm").c_str(), nullptr, 0, nullptr)) return false;
    Mci(L"set eaxm time format milliseconds");
    if (mciSendStringW(fromMs ? (L"play eaxm from " + std::to_wstring(fromMs)).c_str() : L"play eaxm", nullptr, 0, nullptr)) { Mci(L"close eaxm"); return false; }
    cur = idx; state = S_PLAYING; posMs = fromMs; return true;
}
inline void Stop() { CloseTrack(); state = S_STOPPED; posMs = 0; }
inline int Pick(int from) {
    int n = (int)tracks.size(); if (!n) return -1;
    if (mode == M_SHUFFLE_REPEAT || mode == M_SHUFFLE) return (int)(rng() % n);
    return (from + 1) % n;
}
inline void Play() {
    if (state == S_PAUSED) { Mci(L"resume eaxm"); state = S_PLAYING; return; }
    if (tracks.empty()) return;
    if (!StartTrack(cur < 0 ? 0 : cur)) state = S_STOPPED;
}
inline void Pause() {
    if (state == S_PLAYING) { Mci(L"pause eaxm"); state = S_PAUSED; }
    else if (state == S_PAUSED) { Mci(L"resume eaxm"); state = S_PLAYING; }
}
inline void Step(int dir) {
    int n = (int)tracks.size(); if (!n) return;
    int idx = cur < 0 ? 0 : (cur + dir + n) % n;
    if (state == S_STOPPED) { cur = idx; posMs = 0; } else if (!StartTrack(idx)) state = S_STOPPED;
}
inline void Tick(HWND h) {
    if (recording) { recMs = (int)(GetTickCount() - recStart); InvalidateRect(h, nullptr, FALSE); }
    if (state != S_PLAYING) return;
    posMs = _wtoi(Mci(L"status eaxm position").c_str());
    if (Mci(L"status eaxm mode") == L"stopped") {
        int len = tracks[cur].lenMs; int next = -1;
        if (mode == M_REPEAT_TRACK) next = cur;
        else if (mode == M_ONCE) next = -1;
        else if (mode == M_SHUFFLE) { if (shuffleLeft > 0 && --shuffleLeft > 0) next = Pick(cur); }
        else if (mode == M_SHUFFLE_REPEAT) next = Pick(cur);
        else if (cur + 1 < (int)tracks.size()) next = cur + 1; else if (repeatAll) next = 0;
        if (next < 0 || !StartTrack(next)) { CloseTrack(); state = S_STOPPED; posMs = len; }
    }
    InvalidateRect(h, nullptr, FALSE); if (npWnd) InvalidateRect(npWnd, nullptr, FALSE);
}
inline void AddFiles(HWND h, const std::vector<std::wstring>& files, bool playFirst) {
    int first = (int)tracks.size();
    for (auto& f : files) {
        size_t s = f.find_last_of(L"\\/"), d = f.find_last_of(L'.');
        std::wstring t = f.substr(s + 1, d != std::wstring::npos && d > s ? d - s - 1 : std::wstring::npos);
        tracks.push_back({ f, t, TrackLenMs(f) });
    }
    if (playFirst && (int)tracks.size() > first) { cur = first; if (!StartTrack(first)) state = S_STOPPED; SetTimer(h, TIMER_ID, 200, nullptr); }
}
inline void SelectContent(HWND h, bool play = true) {
    wchar_t buf[8192] = {};
    OPENFILENAMEW o = { sizeof(o) }; o.hwndOwner = h;
    o.lpstrFilter = L"All Supported Media Files\0*.mp3;*.wma;*.wav\0Multimedia Files (*.mp3;*.wma;*.wav)\0*.mp3;*.wma;*.wav\0All Files (*.*)\0*.*\0";
    o.lpstrFile = buf; o.nMaxFile = 8192; o.lpstrTitle = L"Open";
    o.Flags = OFN_EXPLORER | OFN_ALLOWMULTISELECT | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
    if (!GetOpenFileNameW(&o)) return;
    std::vector<std::wstring> files; std::wstring dir = buf; const wchar_t* p = buf + dir.size() + 1;
    if (!*p) files.push_back(dir);
    else for (; *p; p += wcslen(p) + 1) files.push_back(dir + L"\\" + p);
    AddFiles(h, files, play);
}
inline void ToggleRecord(HWND h) {
    if (!recording) {
        Stop(); Mci(L"open new type waveaudio alias eaxrec"); Mci(L"record eaxrec");
        recording = true; recStart = GetTickCount(); recMs = 0; SetTimer(h, TIMER_ID, 200, nullptr);
    } else {
        wchar_t tmp[MAX_PATH]; GetTempPathW(MAX_PATH, tmp);
        Mci(L"stop eaxrec"); Mci(std::wstring(L"save eaxrec \"") + tmp + L"EAXRecording.wav\""); Mci(L"close eaxrec"); recording = false;
    }
}
inline void CancelRecord() { if (recording) { Mci(L"stop eaxrec"); Mci(L"close eaxrec"); recording = false; } }

inline bool Enabled(int b) {
    bool any = !tracks.empty();
    switch (b) {
    case B_PREV: case B_NEXT: return any;
    case B_PAUSE: return state != S_STOPPED;
    case B_PLAY: return state != S_PLAYING;
    case B_STOP: return state != S_STOPPED || recording;
    case B_X: return recording;
    default: return true;
    }
}
inline Bitmap* FaceFor(int b) {
    bool p = pressed == b && hot == b;
    const wchar_t* n = kSpot[b].name;
    switch (b) {
    case B_MEDIA: return Face(n, p ? L"pressed" : hot == b && pressed < 0 ? L"hot" : L"normal");
    case B_OPEN:  return Face(n, p ? L"pressed" : L"normal");
    case B_MORE:  return Face(n, (p || (hot == b && pressed < 0)) ? L"hot" : L"normal");
    case B_REC:   return Face(n, recording ? (p ? L"on_pressed" : L"on") : p ? L"pressed" : (hot == b && pressed < 0) ? L"hot" : L"normal");
    case B_RECMENU: return Face(n, recording ? L"on" : p ? L"pressed" : (hot == b && pressed < 0) ? L"hot" : L"normal");
    case B_PREV: case B_NEXT: return Face(n, !Enabled(b) ? L"off" : p ? L"pressed" : L"normal");
    case B_PAUSE: return Face(n, !Enabled(b) ? L"off" : p ? L"pressed" : L"normal");
    case B_PLAY:  return Face(n, !Enabled(b) ? L"off" : p ? L"pressed" : L"normal");
    case B_STOP:  return Face(n, !Enabled(b) ? L"off" : p ? L"pressed" : L"normal");
    case B_LOOP:  return Face(n, repeatAll ? (p ? L"on_pressed" : L"on") : p ? L"off_pressed" : L"off");
    }
    return nullptr;
}

inline std::wstring Clock(int ms) { int s = ms / 1000; wchar_t b[16]; swprintf_s(b, L"%02d:%02d:%02d", s / 3600, s / 60 % 60, s % 60); return b; }
inline void Draw(Graphics& g) {
    Bitmap* bg = band[recording ? 1 : 0].get();
    if (bg && bg->GetLastStatus() == Ok) g.DrawImage(bg, Rect(0, STRIP_Y, WIN_W, STRIP_H + 3), 0, 0, WIN_W, STRIP_H + 3, UnitPixel);
    for (int b = 0; b < B_X; b++) {
        Bitmap* f = FaceFor(b); if (!f) continue;
        g.DrawImage(f, Rect(kFaceX[b], FACE_Y, (int)f->GetWidth(), (int)f->GetHeight()), 0, 0, (int)f->GetWidth(), (int)f->GetHeight(), UnitPixel);
    }
    HDC dc = g.GetHDC();
    if (recording) DrawSknText(dc, Clock(recMs).c_str(), 143, 312, 60, 14, DT_SINGLELINE | DT_LEFT | DT_TOP, RGB(255, 0, 0), true);
    else {
        if (!tracks.empty() && cur >= 0) DrawSknText(dc, std::to_wstring(cur + 1).c_str(), 143, 312, 22, 14, DT_SINGLELINE | DT_LEFT | DT_TOP, RGB(0, 0, 0));
        DrawSknText(dc, Clock(posMs).c_str(), 166, 312, 52, 14, DT_SINGLELINE | DT_LEFT | DT_TOP, RGB(0, 0, 0), true);
    }
    g.ReleaseHDC(dc);
}

inline int HitAt(int x, int y) {
    for (int b = 0; b < B_COUNT; b++) {
        if (b == B_X && !recording) continue;
        const Spot& s = kSpot[b]; if (x >= s.x0 && x < s.x1 && y >= s.y0 && y < s.y1) return b;
    }
    return -1;
}
inline bool InStrip(int y) { return y >= STRIP_Y && y < STRIP_Y + STRIP_H; }
inline bool HandAt(int x, int y) { int b = HitAt(x, y); return b >= 0 && Enabled(b); }
inline bool MouseDown(HWND h, int x, int y) {
    if (!InStrip(y)) return false;
    int b = HitAt(x, y);
    if (b >= 0 && Enabled(b)) { pressed = hot = b; SetCapture(h); InvalidateRect(h, nullptr, FALSE); }
    return true;
}
inline void MouseMove(HWND h, int x, int y) {
    int b = InStrip(y) ? HitAt(x, y) : -1; if (b >= 0 && !Enabled(b)) b = -1;
    if (b != hot) { hot = b; InvalidateRect(h, nullptr, FALSE); }
    if (pressed >= 0 || b >= 0) { TRACKMOUSEEVENT t = { sizeof(t), TME_LEAVE, h, 0 }; TrackMouseEvent(&t); }
}
inline void Leave(HWND h) { if (pressed < 0 && hot >= 0) { hot = -1; InvalidateRect(h, nullptr, FALSE); } }
inline void MoreMenu(HWND h); inline void MediaMenu(HWND h); inline void RecMenu(HWND h); inline void ToggleNowPlaying(HWND h);
inline bool MouseUp(HWND h, int x, int y) {
    if (pressed < 0) return false;
    int b = pressed; pressed = -1; ReleaseCapture();
    int over = HitAt(x, y); InvalidateRect(h, nullptr, FALSE);
    if (over != b) { hot = over; return true; }
    hot = -1; UpdateWindow(h);
    switch (b) {
    case B_MEDIA: MediaMenu(h); break;
    case B_OPEN: ToggleNowPlaying(h); break;
    case B_MORE: MoreMenu(h); break;
    case B_REC: ToggleRecord(h); break;
    case B_RECMENU: RecMenu(h); break;
    case B_PREV: Step(-1); break;
    case B_NEXT: Step(+1); break;
    case B_PLAY: Play(); break;
    case B_PAUSE: Pause(); break;
    case B_STOP: if (recording) ToggleRecord(h); else Stop(); break;
    case B_LOOP: repeatAll = !repeatAll; break;
    case B_X: CancelRecord(); break;
    }
    if (state != S_STOPPED || recording) SetTimer(h, TIMER_ID, 200, nullptr);
    InvalidateRect(h, nullptr, FALSE); return true;
}
inline void Shutdown(HWND h) { KillTimer(h, TIMER_ID); CancelRecord(); CloseTrack(); }

enum Cmd {
    C_NONE, C_SEL_LIB, C_SEL_PC, C_PLAY, C_PAUSE, C_STOP, C_RECORD, C_NEXT, C_PREV,
    C_MODE0, C_MODE1, C_MODE2, C_MODE3, C_MODE4, C_REPEAT_ALL, C_MUTE, C_EAX, C_REC_SYNC, C_ARVL, C_REC_SET,
    C_SHOW_PLAY, C_SHOW_REC, C_NOWPLAYING, C_MEDIA_LIB, C_MEDIA_PC, C_REC_FORMAT, C_MONITOR, C_ENG_RESTART, C_CABLE
};
enum MenuId { MN_L1, MN_PLAY, MN_VIEW, MN_SELECT, MN_CONTROL, MN_MODE, MN_MEDIA, MN_REC, MN_ENGINE, MN_COUNT };
struct MItem { const wchar_t* text; const wchar_t* accel; int cmd; int sub; bool sep; bool disabled; };
struct MDef { int w; std::vector<MItem> items; bool dyn = false; };
inline const MItem SEP = { nullptr, nullptr, 0, -1, true, false };
inline wchar_t engStatus[200] = L"Audio engine: off";

inline const MDef kMenu[MN_COUNT] = {
    { 95,  { { L"Play", nullptr, 0, MN_PLAY, false, false }, { L"View", nullptr, 0, MN_VIEW, false, false }, { L"Engine", nullptr, 0, MN_ENGINE, false, false } } },
    { 274, { { L"Select Content To Play", L"Ctrl+O", 0, MN_SELECT, false, false }, SEP,
             { L"Play Control", nullptr, 0, MN_CONTROL, false, false }, { L"Play Mode", L"Alt+M", 0, MN_MODE, false, false }, SEP,
             { L"Play Volume Mute", L"Ctrl+U", C_MUTE, -1, false, false }, SEP,
             { L"Enable EAX", L"Ctrl+E", C_EAX, -1, false, false }, SEP,
             { L"Recording Source", nullptr, 0, -1, false, true }, { L"Enable Record Sync", nullptr, C_REC_SYNC, -1, false, false },
             { L"Enable Auto Recording Volume Limiter (ARVL)", nullptr, C_ARVL, -1, false, false }, { L"Record Settings...", nullptr, C_REC_SET, -1, false, false } } },
    { 183, { { L"Show Playback Status", nullptr, C_SHOW_PLAY, -1, false, false }, { L"Show Recording Status", nullptr, C_SHOW_REC, -1, false, false }, SEP,
             { L"Now Playing Window", L"Alt+Q", C_NOWPLAYING, -1, false, false } } },
    { 106, { { L"PC Music Library", nullptr, C_SEL_LIB, -1, false, false }, { L"My Computer", nullptr, C_SEL_PC, -1, false, false } } },
    { 161, { { L"Play", L"Ctrl+P", C_PLAY, -1, false, false }, { L"Pause", L"Ctrl+Shift+P", C_PAUSE, -1, false, false }, { L"Stop", L"Ctrl+S", C_STOP, -1, false, false },
             { L"Record", L"Ctrl+R", C_RECORD, -1, false, false }, { L"Next", L"Ctrl+Page Down", C_NEXT, -1, false, false }, { L"Previous", L"Ctrl+Page Up", C_PREV, -1, false, false } } },
    { 120, { { L"Normal", nullptr, C_MODE0, -1, false, false }, { L"Play Track Once", nullptr, C_MODE1, -1, false, false }, { L"Shuffle Repeat", nullptr, C_MODE2, -1, false, false },
             { L"Shuffle", nullptr, C_MODE3, -1, false, false }, { L"Repeat Track", nullptr, C_MODE4, -1, false, false }, { L"Repeat All", nullptr, C_REPEAT_ALL, -1, false, false } } },
    { 156, { { L"Play From PC Music Library", nullptr, C_MEDIA_LIB, -1, false, false }, { L"Play From My Computer", nullptr, C_MEDIA_PC, -1, false, false } } },
    { 157, { { L"Show Recording Status", nullptr, C_SHOW_REC, -1, false, false }, { L"Select Recording Source", nullptr, 0, -1, false, true },
             { L"Select Recording Format...", nullptr, C_REC_FORMAT, -1, false, false }, { L"Enable Record Sync", nullptr, C_REC_SYNC, -1, false, false } }, true },
    { 330, { { engStatus, nullptr, 0, -1, false, true }, SEP, { L"Hear Myself", nullptr, C_MONITOR, -1, false, false },
             { L"Restart Audio", nullptr, C_ENG_RESTART, -1, false, false }, { L"Install VB-CABLE...", nullptr, C_CABLE, -1, false, false } } },
};
inline bool muted = false, recSync = false, arvl = false, showPlay = false, showRec = false;
inline bool nowPlayingOpen = false;
inline bool Checked(int cmd) {
    switch (cmd) {
    case C_MODE0: case C_MODE1: case C_MODE2: case C_MODE3: case C_MODE4: return mode == cmd - C_MODE0;
    case C_REPEAT_ALL: return repeatAll;
    case C_MONITOR: return engMonitor;
    case C_MUTE: return muted; case C_EAX: return g_enableEffects; case C_REC_SYNC: return recSync; case C_ARVL: return arvl;
    case C_SHOW_PLAY: return showPlay; case C_SHOW_REC: return showRec; case C_NOWPLAYING: return nowPlayingOpen;
    }
    return false;
}

inline int recFmt = 0, recBrType = 0, recKbps = 128, recFreq = 5, recBits = 16;
inline const wchar_t* kRecFormats[] = { L"MP3, Stereo", L"MP3, Mono", L"WMA, Stereo", L"WMA, Mono", L"WAV, Stereo", L"WAV, Mono" };
inline const wchar_t* kRecFreqs[] = { L"8000 Hz", L"11025 Hz", L"16000 Hz", L"22050 Hz", L"32000 Hz", L"44100 Hz", L"48000 Hz" };
inline const int kRecRates[15] = { 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320 };
inline int RecRateIdx(int kbps) { int b = 0; for (int i = 0; i < 15; i++) if (abs(kRecRates[i] - kbps) < abs(kRecRates[b] - kbps)) b = i; return b; }
enum { RF_FMT_LBL = 100, RF_FMT, RF_BR_LBL, RF_BR, RF_Q_LBL, RF_SLIDER, RF_MIN, RF_MAX };
inline bool RecIsWav() { return recFmt >= 4; }
inline void RecFmtLayout(HWND d) {
    bool wav = RecIsWav();
    SetDlgItemTextW(d, RF_BR_LBL, wav ? L"&Sampling Frequency:" : L"&Bitrate Type:");
    HWND br = GetDlgItem(d, RF_BR); SendMessageW(br, CB_RESETCONTENT, 0, 0);
    if (wav) { for (auto s : kRecFreqs) SendMessageW(br, CB_ADDSTRING, 0, (LPARAM)s); SendMessageW(br, CB_SETCURSEL, recFreq, 0); }
    else { SendMessageW(br, CB_ADDSTRING, 0, (LPARAM)L"Constant bitrate"); SendMessageW(br, CB_ADDSTRING, 0, (LPARAM)L"Variable bitrate"); SendMessageW(br, CB_SETCURSEL, recBrType, 0); }
    HWND sl = GetDlgItem(d, RF_SLIDER);
    if (wav) { SendMessageW(sl, TBM_SETRANGE, TRUE, MAKELONG(8, 16)); SendMessageW(sl, TBM_SETTICFREQ, 8, 0); SendMessageW(sl, TBM_SETPOS, TRUE, recBits); SetDlgItemTextW(d, RF_MIN, L"8"); SetDlgItemTextW(d, RF_MAX, L"16"); }
    else { SendMessageW(sl, TBM_SETRANGE, TRUE, MAKELONG(0, 14)); SendMessageW(sl, TBM_SETTICFREQ, 14, 0); SendMessageW(sl, TBM_SETPOS, TRUE, RecRateIdx(recKbps)); SetDlgItemTextW(d, RF_MIN, L"24"); SetDlgItemTextW(d, RF_MAX, L"320"); }
    wchar_t t[64]; if (wav) swprintf_s(t, L"&Bits Per Sample: %d", recBits); else swprintf_s(t, L"&Quality: %d kbps", recKbps);
    SetDlgItemTextW(d, RF_Q_LBL, t);
}
inline INT_PTR CALLBACK RecFmtProc(HWND d, UINT m, WPARAM w, LPARAM l) {
    if (m == WM_INITDIALOG) {
        INITCOMMONCONTROLSEX ic = { sizeof(ic), ICC_BAR_CLASSES }; InitCommonControlsEx(&ic);
        RECT wr, cr; GetWindowRect(d, &wr); GetClientRect(d, &cr); POINT o = { 0, 0 }; ClientToScreen(d, &o);
        SetWindowPos(d, nullptr, 0, 0, 462, 132, SWP_NOMOVE | SWP_NOZORDER);
        GetWindowRect(d, &wr); o = { 0, 0 }; ClientToScreen(d, &o); int ox = o.x - wr.left, oy = o.y - wr.top;
        auto put = [&](int id, int x, int y, int ww, int hh) { MoveWindow(GetDlgItem(d, id), x - ox, y - oy, ww, hh, TRUE); };
        put(RF_FMT_LBL, 14, 38, 120, 14); put(RF_FMT, 13, 56, 120, 140);
        put(RF_BR_LBL, 146, 38, 120, 14); put(RF_BR, 146, 56, 120, 140);
        put(RF_Q_LBL, 284, 38, 150, 14); put(RF_SLIDER, 311, 54, 116, 26);
        put(RF_MIN, 296, 58, 22, 14); put(RF_MAX, 428, 58, 30, 14);
        put(IDOK, 292, 97, 75, 23); put(IDCANCEL, 377, 97, 75, 23);
        { HWND s = GetDlgItem(d, RF_SLIDER); LONG_PTR st = GetWindowLongPtrW(s, GWL_STYLE); st = (st & ~(LONG_PTR)(TBS_NOTICKS | TBS_BOTH | TBS_TOP | TBS_VERT)) | TBS_AUTOTICKS | TBS_BOTTOM; SetWindowLongPtrW(s, GWL_STYLE, st); }
        HWND f = GetDlgItem(d, RF_FMT); for (auto s : kRecFormats) SendMessageW(f, CB_ADDSTRING, 0, (LPARAM)s); SendMessageW(f, CB_SETCURSEL, recFmt, 0);
        RecFmtLayout(d);
        RECT pr; GetWindowRect(GetParent(d), &pr); GetWindowRect(d, &wr);
        SetWindowPos(d, nullptr, pr.left + ((pr.right - pr.left) - 462) / 2, pr.top + ((pr.bottom - pr.top) - 132) / 2, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
        SetFocus(f); return FALSE;
    }
    if (m == WM_HSCROLL && (HWND)l == GetDlgItem(d, RF_SLIDER)) {
        int p = (int)SendMessageW((HWND)l, TBM_GETPOS, 0, 0); wchar_t t[64];
        if (RecIsWav()) { recBits = p < 12 ? 8 : 16; SendMessageW((HWND)l, TBM_SETPOS, TRUE, recBits); swprintf_s(t, L"&Bits Per Sample: %d", recBits); }
        else { recKbps = kRecRates[std::clamp(p, 0, 14)]; swprintf_s(t, L"&Quality: %d kbps", recKbps); }
        SetDlgItemTextW(d, RF_Q_LBL, t); return TRUE;
    }
    if (m == WM_COMMAND) {
        if (LOWORD(w) == RF_FMT && HIWORD(w) == CBN_SELCHANGE) { recFmt = (int)SendMessageW((HWND)l, CB_GETCURSEL, 0, 0); RecFmtLayout(d); return TRUE; }
        if (LOWORD(w) == RF_BR && HIWORD(w) == CBN_SELCHANGE) { int s = (int)SendMessageW((HWND)l, CB_GETCURSEL, 0, 0); if (RecIsWav()) recFreq = s; else recBrType = s; return TRUE; }
        if (LOWORD(w) == IDOK || LOWORD(w) == IDCANCEL) { EndDialog(d, LOWORD(w)); return TRUE; }
    }
    return FALSE;
}
inline void RecFormatDialog(HWND owner) {
    std::vector<WORD> t; auto w16 = [&](WORD v) { t.push_back(v); }; auto w32 = [&](DWORD v) { t.push_back((WORD)v); t.push_back((WORD)(v >> 16)); };
    auto str = [&](const wchar_t* s) { do w16(*s); while (*s++); };
    auto align = [&]() { while (t.size() % 2) w16(0); };
    int nItems = 10;
    w32(DS_MODALFRAME | DS_SETFONT | WS_POPUP | WS_CAPTION | WS_SYSMENU); w32(0); w16(nItems); w16(0); w16(0); w16(10); w16(10); w16(0); w16(0); str(L"Change Record Format"); w16(8); str(L"MS Shell Dlg");
    auto item = [&](WORD cls, DWORD style, int id, const wchar_t* text) {
        align(); w32(style | WS_CHILD | WS_VISIBLE); w32(0); w16(0); w16(0); w16(10); w16(10); w16((WORD)id); w16(0xFFFF); w16(cls); str(text); w16(0);
    };
    DWORD tab = WS_TABSTOP;
    item(0x0082, SS_LEFT, RF_FMT_LBL, L"&Format, Channel:"); item(0x0085, CBS_DROPDOWNLIST | WS_VSCROLL | tab, RF_FMT, L"");
    item(0x0082, SS_LEFT, RF_BR_LBL, L"&Bitrate Type:"); item(0x0085, CBS_DROPDOWNLIST | WS_VSCROLL | tab, RF_BR, L"");
    item(0x0082, SS_LEFT, RF_Q_LBL, L"&Quality:");

    align(); w32(TBS_AUTOTICKS | tab | WS_CHILD | WS_VISIBLE); w32(0); w16(0); w16(0); w16(10); w16(10); w16(RF_SLIDER); str(L"msctls_trackbar32"); w16(0); w16(0);
    item(0x0082, SS_LEFT, RF_MIN, L"24"); item(0x0082, SS_LEFT, RF_MAX, L"320");
    item(0x0080, BS_DEFPUSHBUTTON | tab, IDOK, L"OK"); item(0x0080, BS_PUSHBUTTON | tab, IDCANCEL, L"Cancel");
    align();
    if (DialogBoxIndirectParamW(GetModuleHandleW(nullptr), (LPCDLGTEMPLATEW)t.data(), owner, RecFmtProc, 0) == IDOK) {
        HKEY k; if (!RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\CreativeEAXClone", 0, nullptr, 0, KEY_WRITE, nullptr, &k, nullptr)) {
            DWORD v[5] = { (DWORD)recFmt, (DWORD)recBrType, (DWORD)recKbps, (DWORD)recFreq, (DWORD)recBits }; RegSetValueExW(k, L"RecordFormat", 0, REG_BINARY, (BYTE*)v, sizeof(v)); RegCloseKey(k);
        }
    }
}
inline void RecFormatLoad() {
    HKEY k; if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\CreativeEAXClone", 0, KEY_READ, &k)) return;
    DWORD v[5], sz = sizeof(v); if (!RegQueryValueExW(k, L"RecordFormat", nullptr, nullptr, (BYTE*)v, &sz) && sz == sizeof(v)) { recFmt = std::clamp((int)v[0], 0, 5); recBrType = std::clamp((int)v[1], 0, 1); recKbps = std::clamp((int)v[2], 24, 320); recFreq = std::clamp((int)v[3], 0, 6); recBits = v[4] >= 12 ? 16 : 8; }
    RegCloseKey(k);
}

inline void NowPlayingToggle(HWND h);
inline int MenuW(int id) {
    const MDef& d = kMenu[id]; if (!d.dyn) return d.w;
    for (auto& it : d.items) if (it.cmd && Checked(it.cmd)) return d.w + 15;
    return d.w;
}
inline void Run(HWND h, int cmd) {
    switch (cmd) {
    case C_MEDIA_PC: case C_SEL_PC: SelectContent(h); break;
    case C_PLAY: Play(); break; case C_PAUSE: Pause(); break; case C_STOP: if (recording) ToggleRecord(h); else Stop(); break;
    case C_RECORD: ToggleRecord(h); break; case C_NEXT: Step(+1); break; case C_PREV: Step(-1); break;
    case C_MODE0: case C_MODE1: case C_MODE2: case C_MODE3: case C_MODE4: mode = cmd - C_MODE0; shuffleLeft = (int)tracks.size(); break;
    case C_REPEAT_ALL: repeatAll = !repeatAll; break;
    case C_MUTE: muted = !muted; waveOutSetVolume(nullptr, muted ? 0 : 0xFFFFFFFF); break;
    case C_EAX: g_enableEffects = !g_enableEffects; break;
    case C_REC_SYNC: recSync = !recSync; break; case C_ARVL: arvl = !arvl; break;
    case C_REC_SET: ShellExecuteW(nullptr, L"open", L"control.exe", L"mmsys.cpl,,1", nullptr, SW_SHOWNORMAL); break;
    case C_REC_FORMAT: RecFormatDialog(h); break;
    case C_SHOW_PLAY: showPlay = !showPlay; break; case C_SHOW_REC: showRec = !showRec; break;
    case C_NOWPLAYING: NowPlayingToggle(h); break;
    case C_MONITOR: {
        engMonitor = !engMonitor; orb::SetMonitor(engMonitor); orb::Restart();
        HKEY k; if (!RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\CreativeEAXClone", 0, nullptr, 0, KEY_WRITE, nullptr, &k, nullptr)) { DWORD v = engMonitor; RegSetValueExW(k, L"EngineMonitor", 0, REG_DWORD, (BYTE*)&v, sizeof(v)); RegCloseKey(k); }
        break;
    }
    case C_ENG_RESTART: {
        std::wstring mic, spk; orb::DefaultDevices(mic, spk); orb::SetDevices(mic.c_str(), spk.c_str());
        orb::SetMonitor(engMonitor); if (orb::Running()) orb::Restart(); else orb::Start();
        break;
    }
    case C_CABLE: orb::InstallCable(h); break;
    }
    if (state != S_STOPPED || recording) SetTimer(h, TIMER_ID, 200, nullptr);
    InvalidateRect(h, nullptr, FALSE);
}

constexpr COLORREF MN_BODY = RGB(254, 254, 254), MN_HOTK = RGB(1, 128, 255), MN_BORDER = RGB(193, 193, 193), MN_DIS = RGB(127, 127, 127);
constexpr int SLOT = 18;
struct MWin { HWND h = nullptr; int id = 0, x = 0, y = 0, w = 0, hh = 0, hot = -1; std::vector<uint32_t> bg, out; };
inline std::vector<MWin*> mstack;
inline const char* kCheckPx[7] = { ".....#", "....##", "#..###", "######", "#####.", "####..", ".##..." };
inline const char* kArrowPx[8] = { "#...", "##..", "###.", "####", "####", "###.", "##..", "#..." };
inline void PutGlyph(HDC dc, const char** rows, int n, int x, int y, COLORREF c) {
    for (int j = 0; j < n; j++) for (int i = 0; rows[j][i]; i++) if (rows[j][i] == '#') SetPixel(dc, x + i, y + j, c);
}
inline void RenderFace(const MWin* mw, std::vector<uint32_t>& face) {
    const MDef& d = kMenu[mw->id];
    BITMAPINFO bi = {}; bi.bmiHeader.biSize = sizeof(bi.bmiHeader); bi.bmiHeader.biWidth = mw->w; bi.bmiHeader.biHeight = -mw->hh; bi.bmiHeader.biPlanes = 1; bi.bmiHeader.biBitCount = 32;
    HDC dc = CreateCompatibleDC(nullptr); void* bits = nullptr; HBITMAP dib = CreateDIBSection(dc, &bi, DIB_RGB_COLORS, &bits, nullptr, 0); HGDIOBJ old = SelectObject(dc, dib);
    HBRUSH bg = CreateSolidBrush(MN_BODY), hot = CreateSolidBrush(MN_HOTK), bd = CreateSolidBrush(MN_BORDER);
    RECT all = { 0, 0, mw->w, mw->hh }; FillRect(dc, &all, bg);
    for (int i = 0; i < (int)d.items.size(); i++) {
        const MItem& it = d.items[i]; int y = i * SLOT;
        if (it.sep) { RECT r = { 1, y + 9, mw->w - 1, y + 10 }; FillRect(dc, &r, bd); continue; }
        bool isHot = mw->hot == i && !it.disabled;
        if (isHot) { RECT r = { 1, std::max(y, 1), mw->w - 1, y + SLOT }; FillRect(dc, &r, hot); }
        COLORREF tc = it.disabled ? MN_DIS : isHot ? RGB(255, 255, 255) : RGB(0, 0, 0);
        int tx = d.dyn ? (mw->w > d.w ? 22 : 8) : 22;
        if (it.text) DrawSknText(dc, it.text, tx, y, mw->w - 22, SLOT, DT_SINGLELINE | DT_VCENTER | DT_LEFT, tc);
        if (it.accel) DrawSknText(dc, it.accel, 0, y, mw->w - 22, SLOT, DT_SINGLELINE | DT_VCENTER | DT_RIGHT, tc);
        if (it.sub >= 0 || (it.disabled && !it.accel && !it.cmd && i == 9)) PutGlyph(dc, kArrowPx, 8, mw->w - 16, y + 6, isHot ? RGB(255, 255, 255) : it.disabled ? MN_DIS : RGB(0, 0, 0));
        if (Checked(it.cmd)) PutGlyph(dc, kCheckPx, 7, 8, y + 6, isHot ? RGB(255, 255, 255) : RGB(0, 0, 0));
    }
    FrameRect(dc, &all, bd); GdiFlush();
    face.resize((size_t)mw->w * mw->hh);
    const uint32_t* src = (const uint32_t*)bits;
    for (size_t k = 0; k < face.size(); k++) {
        uint32_t p = src[k] & 0xFFFFFF;
        face[k] = p == 0xFEFEFE ? 0xE6FFFFFFu : p == 0x0180FF ? 0xE60080FFu : (0xFF000000u | p);
    }
    DeleteObject(bg); DeleteObject(hot); DeleteObject(bd); SelectObject(dc, old); DeleteObject(dib); DeleteDC(dc);
}
inline void PaintMenu(HDC wdc, MWin* mw) {
    std::vector<uint32_t> face; RenderFace(mw, face);
    size_t n = face.size(); mw->out.resize(n);
    for (size_t k = 0; k < n; k++) {
        uint32_t f = face[k], b = k < mw->bg.size() ? mw->bg[k] : 0xFFFFFFu, a = f >> 24, ia = 255 - a;
        mw->out[k] = (((((f >> 16) & 255) * a + ((b >> 16) & 255) * ia) / 255) << 16) | (((((f >> 8) & 255) * a + ((b >> 8) & 255) * ia) / 255) << 8) | (((f & 255) * a + (b & 255) * ia) / 255);
    }
    BITMAPINFO bi = {}; bi.bmiHeader.biSize = sizeof(bi.bmiHeader); bi.bmiHeader.biWidth = mw->w; bi.bmiHeader.biHeight = -mw->hh; bi.bmiHeader.biPlanes = 1; bi.bmiHeader.biBitCount = 32;
    SetDIBitsToDevice(wdc, 0, 0, mw->w, mw->hh, 0, 0, 0, mw->hh, mw->out.data(), &bi, DIB_RGB_COLORS);
}
inline LRESULT CALLBACK MenuProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    if (m == WM_PAINT) {
        PAINTSTRUCT ps; HDC wdc = BeginPaint(h, &ps); auto* mw = (MWin*)GetWindowLongPtrW(h, GWLP_USERDATA);
        if (mw) PaintMenu(wdc, mw);
        EndPaint(h, &ps); return 0;
    }
    if (m == WM_MOUSEACTIVATE) return MA_NOACTIVATE;
    return DefWindowProcW(h, m, w, l);
}

inline void RenderMenu(HDC dc, int id, int hot, uint32_t backdrop) {
    MWin mw; mw.id = id; mw.w = MenuW(id); mw.hh = (int)kMenu[id].items.size() * SLOT; mw.hot = hot; mw.bg.assign((size_t)mw.w * mw.hh, backdrop); PaintMenu(dc, &mw);
}
inline MWin* OpenMenu(HWND owner, int id, int x, int y) {
    static bool reg = false;
    if (!reg) { WNDCLASSEXW wc = { sizeof(wc) }; wc.lpfnWndProc = MenuProc; wc.hInstance = GetModuleHandleW(nullptr); wc.lpszClassName = L"EAXMediaMenu"; wc.hCursor = LoadCursor(nullptr, IDC_ARROW); RegisterClassExW(&wc); reg = true; }
    auto* mw = new MWin; mw->id = id; mw->x = x; mw->y = y; mw->w = MenuW(id); mw->hh = (int)kMenu[id].items.size() * SLOT;

    BITMAPINFO bi = {}; bi.bmiHeader.biSize = sizeof(bi.bmiHeader); bi.bmiHeader.biWidth = mw->w; bi.bmiHeader.biHeight = -mw->hh; bi.bmiHeader.biPlanes = 1; bi.bmiHeader.biBitCount = 32;
    HDC scr = GetDC(nullptr), mem = CreateCompatibleDC(scr); void* bits = nullptr; HBITMAP dib = CreateDIBSection(scr, &bi, DIB_RGB_COLORS, &bits, nullptr, 0); HGDIOBJ old = SelectObject(mem, dib);
    BitBlt(mem, 0, 0, mw->w, mw->hh, scr, x, y, SRCCOPY | CAPTUREBLT); GdiFlush();
    mw->bg.assign((uint32_t*)bits, (uint32_t*)bits + (size_t)mw->w * mw->hh); for (auto& p : mw->bg) p &= 0xFFFFFF;
    SelectObject(mem, old); DeleteObject(dib); DeleteDC(mem); ReleaseDC(nullptr, scr);
    for (MWin* p : mstack) for (int yy = 0; yy < mw->hh; yy++) for (int xx = 0; xx < mw->w; xx++) {
        int px = x + xx - p->x, py = y + yy - p->y;
        if (px >= 0 && py >= 0 && px < p->w && py < p->hh && !p->out.empty()) mw->bg[(size_t)yy * mw->w + xx] = p->out[(size_t)py * p->w + px];
    }
    mw->h = CreateWindowExW(WS_EX_TOOLWINDOW | WS_EX_TOPMOST | WS_EX_NOACTIVATE, L"EAXMediaMenu", L"", WS_POPUP, x, y, mw->w, mw->hh, owner, nullptr, GetModuleHandleW(nullptr), nullptr);
    SetWindowLongPtrW(mw->h, GWLP_USERDATA, (LONG_PTR)mw);
    ShowWindow(mw->h, SW_SHOWNOACTIVATE); UpdateWindow(mw->h); mstack.push_back(mw); return mw;
}
inline void CloseTo(size_t keep) { while (mstack.size() > keep) { DestroyWindow(mstack.back()->h); delete mstack.back(); mstack.pop_back(); } }
inline void SetHot(MWin* mw, int i) { if (mw->hot != i) { mw->hot = i; InvalidateRect(mw->h, nullptr, FALSE); UpdateWindow(mw->h); } }
inline void OpenSub(HWND owner, size_t level, int row) {
    MWin* p = mstack[level]; const MItem& it = kMenu[p->id].items[row]; if (it.sub < 0) return;
    if (mstack.size() > level + 1 && mstack[level + 1]->id == it.sub) return;
    CloseTo(level + 1);
    int sx = p->x + (p->id == MN_L1 ? 92 : 271), sy = p->y + (p->id == MN_L1 ? row * SLOT + 1 : row * SLOT + 1);
    OpenMenu(owner, it.sub, sx, sy);
}

inline int RunMenus(HWND owner, int id, int x, int y, int firstHot = -1) {
    OpenMenu(owner, id, x, y); if (firstHot >= 0) SetHot(mstack[0], firstHot);
    SetCapture(mstack[0]->h); int cmd = 0; bool done = false; MSG msg;
    auto rowAt = [&](MWin* mw, POINT p) { int r = (p.y - mw->y) / SLOT; return std::clamp(r, 0, (int)kMenu[mw->id].items.size() - 1); };
    auto topAt = [&](POINT p) -> int { for (int k = (int)mstack.size() - 1; k >= 0; k--) { MWin* m = mstack[k]; if (p.x >= m->x && p.x < m->x + m->w && p.y >= m->y && p.y < m->y + m->hh) return k; } return -1; };
    while (!done && GetMessageW(&msg, nullptr, 0, 0)) {
        if (msg.message == WM_MOUSEMOVE || msg.message == WM_LBUTTONDOWN || msg.message == WM_LBUTTONUP) {
            POINT p; GetCursorPos(&p); int k = topAt(p);
            if (k >= 0) {
                MWin* mw = mstack[k]; int r = rowAt(mw, p); const MItem& it = kMenu[mw->id].items[r];
                if (msg.message == WM_MOUSEMOVE || msg.message == WM_LBUTTONDOWN) {
                    SetHot(mw, it.sep ? -1 : r);
                    if (it.sub >= 0 && !it.disabled) OpenSub(owner, k, r); else CloseTo(k + 1);
                    for (size_t j = 0; j < mstack.size(); j++) if ((int)j != k && j + 1 < mstack.size()) {  }
                }
                if (msg.message == WM_LBUTTONUP || msg.message == WM_LBUTTONDOWN) {
                    if (!it.sep && !it.disabled && it.sub < 0 && it.cmd && msg.message == WM_LBUTTONUP) { cmd = it.cmd; done = true; }
                }
            } else if (msg.message == WM_LBUTTONDOWN) done = true;
            continue;
        }
        if (msg.message == WM_KEYDOWN) {
            MWin* mw = mstack.back(); int n = (int)kMenu[mw->id].items.size();
            if (msg.wParam == VK_ESCAPE) { if (mstack.size() > 1) CloseTo(mstack.size() - 1); else done = true; }
            else if (msg.wParam == VK_DOWN || msg.wParam == VK_UP) {
                int d = msg.wParam == VK_DOWN ? 1 : -1, r = mw->hot;
                for (int s = 0; s < n; s++) { r = ((r < 0 ? (d > 0 ? -1 : 0) : r) + d + n) % n; if (!kMenu[mw->id].items[r].sep && !kMenu[mw->id].items[r].disabled) break; }
                SetHot(mw, r);
            } else if (msg.wParam == VK_RIGHT && mw->hot >= 0) { size_t lv = mstack.size() - 1; if (kMenu[mw->id].items[mw->hot].sub >= 0) { OpenSub(owner, lv, mw->hot); SetHot(mstack.back(), 0); } }
            else if (msg.wParam == VK_LEFT && mstack.size() > 1) CloseTo(mstack.size() - 1);
            else if (msg.wParam == VK_RETURN && mw->hot >= 0) { const MItem& it = kMenu[mw->id].items[mw->hot]; if (it.sub >= 0) { OpenSub(owner, mstack.size() - 1, mw->hot); SetHot(mstack.back(), 0); } else if (it.cmd) { cmd = it.cmd; done = true; } }
            continue;
        }
        TranslateMessage(&msg); DispatchMessageW(&msg);
        if (GetCapture() != mstack[0]->h) { if (!mstack.empty()) SetCapture(mstack[0]->h); }
    }
    ReleaseCapture(); CloseTo(0); return cmd;
}
inline void MoreMenu(HWND h) {
    if (orb::Running()) { wchar_t st[160] = L""; orb::Status(st, 160); swprintf_s(engStatus, L"Audio engine on - %ls", st[0] ? st : L"starting..."); }
    else wcscpy_s(engStatus, L"Audio engine off - starts when you change an effect");
    RECT r; GetWindowRect(h, &r); int c = RunMenus(h, MN_L1, r.left + 128, r.top + 316); if (c) Run(h, c); }
inline void RecMenu(HWND h) { RECT r; GetWindowRect(h, &r); int c = RunMenus(h, MN_REC, r.left + 221, r.top + 328); if (c) Run(h, c); }
inline void MediaMenu(HWND h) { RECT r; GetWindowRect(h, &r); int c = RunMenus(h, MN_MEDIA, r.left + 41, r.top + 329); if (c) Run(h, c); }

inline bool Key(HWND h, UINT vk, bool alt) {
    bool ctrl = GetKeyState(VK_CONTROL) < 0, shift = GetKeyState(VK_SHIFT) < 0;
    int c = 0;
    if (ctrl && !alt) switch (vk) {
        case 'O': c = C_SEL_PC; break; case 'P': c = shift ? C_PAUSE : C_PLAY; break; case 'S': c = C_STOP; break; case 'R': c = C_RECORD; break;
        case VK_NEXT: c = C_NEXT; break; case VK_PRIOR: c = C_PREV; break; case 'U': c = C_MUTE; break; case 'E': c = C_EAX; break;
    }
    if (alt && !ctrl) { if (vk == 'M') { Run(h, mode == M_REPEAT_TRACK ? C_MODE0 : mode + C_MODE0 + 1); return true; } if (vk == 'Q') c = C_NOWPLAYING; }
    if (c) { Run(h, c); return true; }
    return false;
}

inline std::set<int> npSel; inline int npTop = 0, npAnchor = -1, npPressed = -1, npHot = -1, npW = 500, npH = 208;
inline std::unique_ptr<Bitmap> npBase[2], npIdle, npPlay, npPatch[8];
inline bool npCorner[4][12][12];
enum NpBtn { NB_ADD, NB_DELETE, NB_DELALL, NB_SAVE, NB_CLOSE, NB_COUNT };
constexpr int NP_BASE_W = 500, NP_BASE_H = 208, NP_ROW0 = 56, NP_ROW_H = 17, NP_LIST_TOP = 54, NP_LIST_BOTTOM0 = 146, NP_TB_BOTTOM = 173, NP_GRIP = 24;
inline int NpDw() { return npW - NP_BASE_W; }
inline int NpDh() { return npH - NP_BASE_H; }
inline int NpListBottom() { return NP_LIST_BOTTOM0 + NpDh(); }
inline RECT NpBtnRect(int b) {
    static const RECT r[NB_COUNT] = { { 49, 149, 108, 170 }, { 160, 149, 226, 170 }, { 268, 149, 351, 170 }, { 384, 149, 460, 170 }, { 469, 5, 491, 26 } };
    RECT o = r[b]; if (b == NB_CLOSE) { o.left += NpDw(); o.right += NpDw(); } else { o.top += NpDh(); o.bottom += NpDh(); } return o;
}
constexpr RECT kNpEnabled[3] = { { 160, 148, 228, 172 }, { 268, 148, 352, 172 }, { 384, 148, 462, 172 } };
inline bool NpHasItems() { return !tracks.empty(); }
inline bool NpEnabled(int b) { return b == NB_ADD || b == NB_CLOSE || NpHasItems(); }
inline int NpVisibleRows() { return std::max(1, (NpListBottom() - NP_ROW0) / NP_ROW_H); }
inline void NpRefresh() { if (npWnd) InvalidateRect(npWnd, nullptr, FALSE); }
inline void NpBuildCorners() {
    Px p = ToPx(npBase[0].get()); if (p.w != NP_BASE_W) return;
    auto desk = [&](int x, int y) { uint32_t c = p.at(x, y); int r = (c >> 16) & 255, g = (c >> 8) & 255, b = c & 255; return r < 0x40 && g < 0x40 && b < 0x40; };
    for (int k = 0; k < 4; k++) {
        int x0 = (k & 1) ? NP_BASE_W - 12 : 0, y0 = (k & 2) ? NP_BASE_H - 12 : 0;
        int sx = (k & 1) ? 11 : 0, sy = (k & 2) ? 11 : 0; bool seen[12][12] = {}; std::vector<std::pair<int, int>> st;
        if (desk(x0 + sx, y0 + sy)) st.push_back({ sx, sy });
        while (!st.empty()) {
            auto [x, y] = st.back(); st.pop_back(); if (x < 0 || y < 0 || x > 11 || y > 11 || seen[y][x] || !desk(x0 + x, y0 + y)) continue;
            seen[y][x] = true; st.push_back({ x + 1, y }); st.push_back({ x - 1, y }); st.push_back({ x, y + 1 }); st.push_back({ x, y - 1 });
        }
        memcpy(npCorner[k], seen, sizeof(seen));
    }
}
inline void NpApplyShape(HWND h) {
    HRGN rgn = CreateRectRgn(0, 0, npW, npH);
    for (int k = 0; k < 4; k++) for (int y = 0; y < 12; y++) for (int x = 0; x < 12; x++) if (npCorner[k][y][x]) {
        int dx = (k & 1) ? npW - 12 + x : x, dy = (k & 2) ? npH - 12 + y : y;
        HRGN px = CreateRectRgn(dx, dy, dx + 1, dy + 1); CombineRgn(rgn, rgn, px, RGN_DIFF); DeleteObject(px);
    }
    SetWindowRgn(h, rgn, TRUE);
}
inline void NpSlice(Graphics& g, Bitmap* src, int sy0, int sy1, int L, int R, int dy) {
    int hh = sy1 - sy0, dw = NpDw();
    g.DrawImage(src, Rect(0, dy, L, hh), 0, sy0, L, hh, UnitPixel);
    g.DrawImage(src, Rect(L, dy, R - L + dw, hh), L, sy0, R - L, hh, UnitPixel);
    g.DrawImage(src, Rect(R + dw, dy, NP_BASE_W - R, hh), R, sy0, NP_BASE_W - R, hh, UnitPixel);
}
inline void NpPaint(HDC wdc) {
    HDC dc = CreateCompatibleDC(wdc); HBITMAP bm = CreateCompatibleBitmap(wdc, npW, npH); HGDIOBJ old = SelectObject(dc, bm);
    {
        Graphics g(dc); g.SetInterpolationMode(InterpolationModeNearestNeighbor); g.SetPixelOffsetMode(PixelOffsetModeNone);
        bool items = NpHasItems(); int dw = NpDw(), dh = NpDh(), lb = NpListBottom();
        Bitmap* b0 = npBase[0].get();
        NpSlice(g, b0, 0, 37, 170, 440, 0);
        NpSlice(g, b0, 37, NP_LIST_TOP, 120, 250, 37);
        g.DrawImage(b0, Rect(0, NP_LIST_TOP, 5, lb - NP_LIST_TOP), 0, NP_LIST_TOP, 5, NP_LIST_BOTTOM0 - NP_LIST_TOP, UnitPixel);
        g.DrawImage(b0, Rect(npW - 5, NP_LIST_TOP, 5, lb - NP_LIST_TOP), 495, NP_LIST_TOP, 5, NP_LIST_BOTTOM0 - NP_LIST_TOP, UnitPixel);
        NpSlice(g, b0, NP_LIST_BOTTOM0, NP_TB_BOTTOM, 470, 490, lb);
        NpSlice(g, b0, NP_TB_BOTTOM, NP_BASE_H, 100, 330, NP_TB_BOTTOM + dh);
        if (items) for (int i = 0; i < 3; i++) { const RECT& r = kNpEnabled[i]; g.DrawImage(npBase[1].get(), Rect(r.left, r.top + dh, r.right - r.left, r.bottom - r.top), r.left, r.top, r.right - r.left, r.bottom - r.top, UnitPixel); }

        SolidBrush cream(Color(255, 255, 251, 240)), a(Color(255, 249, 245, 234)), gr(Color(255, 240, 240, 240)), sel(Color(255, 0, 120, 212));
        int lw = npW - 10;
        g.SetClip(Rect(5, NP_LIST_TOP, lw, lb - NP_LIST_TOP));
        g.FillRectangle(&cream, 5, NP_LIST_TOP, lw, lb - NP_LIST_TOP);
        int base = items ? NP_ROW0 : NP_ROW0 + 2, n = (int)tracks.size();
        for (int k = 0; base + k * NP_ROW_H < lb; k++) {
            int idx = npTop + k, y = base + k * NP_ROW_H; bool even = idx % 2 == 0;
            Brush* b = idx < n ? (npSel.count(idx) ? (Brush*)&sel : even ? (Brush*)&a : (Brush*)&gr) : (even ? (Brush*)&a : (Brush*)&cream);
            g.FillRectangle(b, 5, y, lw, NP_ROW_H);
        }
        g.ResetClip();
        for (int k = 0; NP_ROW0 + k * NP_ROW_H < lb && npTop + k < n; k++) {
            int idx = npTop + k, y = NP_ROW0 + k * NP_ROW_H; Bitmap* ic = (idx == cur && state != S_STOPPED) ? npPlay.get() : npIdle.get();
            if (ic && ic->GetLastStatus() == Ok && y + 17 <= lb + 4) g.DrawImage(ic, Rect(8, y, 16, 17), 0, 0, 16, 17, UnitPixel);
        }
        HDC tdc = g.GetHDC(); SaveDC(tdc); IntersectClipRect(tdc, 5, NP_LIST_TOP, npW - 5, lb);
        int titleW = 208 + dw;
        for (int k = 0; NP_ROW0 + k * NP_ROW_H < lb && npTop + k < n; k++) {
            int idx = npTop + k, y = NP_ROW0 + k * NP_ROW_H; COLORREF c = npSel.count(idx) ? RGB(255, 255, 255) : RGB(0, 0, 0);
            wchar_t no[8]; swprintf_s(no, L"%03d", idx + 1);
            DrawSknText(tdc, no, 25, y, 28, NP_ROW_H, DT_SINGLELINE | DT_VCENTER, c);
            DrawSknText(tdc, tracks[idx].title.c_str(), 56, y, titleW, NP_ROW_H, DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS, c);
            DrawSknText(tdc, L"<Unknown>", 271 + dw, y, 136, NP_ROW_H, DT_SINGLELINE | DT_VCENTER, c);
            DrawSknText(tdc, Clock(tracks[idx].lenMs).c_str(), 416 + dw, y, 76, NP_ROW_H, DT_SINGLELINE | DT_VCENTER, c);
        }
        RestoreDC(tdc, -1); g.ReleaseHDC(tdc);
        auto patch = [&](int i) {
            Bitmap* p = npPatch[i].get();
            if (p && p->GetLastStatus() == Ok) {
                static const int px[8][2] = { { 49, 149 }, { 160, 149 }, { 270, 149 }, { 384, 149 }, { 469, 5 }, { 161, 150 } };
                int x = px[i][0] + (i == NB_CLOSE ? dw : 0), y = px[i][1] + (i == NB_CLOSE ? 0 : dh);
                g.DrawImage(p, Rect(x, y, (int)p->GetWidth(), (int)p->GetHeight()), 0, 0, (int)p->GetWidth(), (int)p->GetHeight(), UnitPixel);
            }
        };
        if (npPressed >= 0 && npHot == npPressed) patch(npPressed);
        else if (npPressed < 0 && npHot == NB_DELETE && NpEnabled(NB_DELETE)) patch(5);
    }
    BitBlt(wdc, 0, 0, npW, npH, dc, 0, 0, SRCCOPY); SelectObject(dc, old); DeleteObject(bm); DeleteDC(dc);
}
inline int NpBtnAt(int x, int y) { for (int b = 0; b < NB_COUNT; b++) { RECT r = NpBtnRect(b); if (x >= r.left && x < r.right && y >= r.top && y < r.bottom) return b; } return -1; }
inline int NpRowAt(int x, int y) { if (x < 5 || x >= npW - 5 || y < NP_ROW0 || y >= NpListBottom()) return -1; int i = npTop + (y - NP_ROW0) / NP_ROW_H; return i < (int)tracks.size() ? i : -1; }
inline void NpSave(HWND h) {
    wchar_t f[MAX_PATH] = L"Playlist.m3u"; OPENFILENAMEW o = { sizeof(o) }; o.hwndOwner = h; o.lpstrFilter = L"M3U Playlist (*.m3u)\0*.m3u\0All Files (*.*)\0*.*\0"; o.lpstrFile = f; o.nMaxFile = MAX_PATH;
    o.lpstrDefExt = L"m3u"; o.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
    if (!GetSaveFileNameW(&o)) return;
    FILE* fp = nullptr; if (_wfopen_s(&fp, f, L"wb") || !fp) return;
    fputs("#EXTM3U\r\n", fp);
    for (auto& t : tracks) { int n = WideCharToMultiByte(CP_UTF8, 0, t.path.c_str(), -1, nullptr, 0, nullptr, nullptr); std::string s(n, 0); WideCharToMultiByte(CP_UTF8, 0, t.path.c_str(), -1, s.data(), n, nullptr, nullptr); fputs(s.c_str(), fp); fputs("\r\n", fp); }
    fclose(fp);
}
inline void NpDelete(const std::set<int>& del) {
    std::vector<Track> keep; int newCur = cur;
    for (int i = 0; i < (int)tracks.size(); i++) { if (del.count(i)) { if (i == cur) { Stop(); newCur = -1; } else if (i < cur) newCur--; } else keep.push_back(tracks[i]); }
    tracks = keep; cur = tracks.empty() ? -1 : newCur; npSel.clear(); npTop = std::clamp(npTop, 0, std::max(0, (int)tracks.size() - 1));
    if (cur < 0 && state == S_STOPPED) posMs = 0;
}
inline LRESULT CALLBACK NpProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    int x = GET_X_LPARAM(l), y = GET_Y_LPARAM(l);
    switch (m) {
    case WM_ERASEBKGND: return 1;
    case WM_NCCALCSIZE: return 0;
    case WM_GETMINMAXINFO: { auto* mi = (MINMAXINFO*)l; mi->ptMinTrackSize = { 360, 170 }; return 0; }
    case WM_SIZE: if (w != SIZE_MINIMIZED) { npW = LOWORD(l); npH = HIWORD(l); npTop = std::clamp(npTop, 0, std::max(0, (int)tracks.size() - NpVisibleRows())); NpApplyShape(h); InvalidateRect(h, nullptr, FALSE); } return 0;
    case WM_PAINT: { PAINTSTRUCT ps; HDC dc = BeginPaint(h, &ps); NpPaint(dc); EndPaint(h, &ps); return 0; }
    case WM_NCHITTEST: {
        POINT p = { GET_X_LPARAM(l), GET_Y_LPARAM(l) }; ScreenToClient(h, &p);
        if (p.x >= npW - NP_GRIP && p.y >= npH - NP_GRIP) return HTBOTTOMRIGHT;
        if (p.y < 36 && NpBtnAt(p.x, p.y) != NB_CLOSE) return HTCAPTION;
        return HTCLIENT;
    }
    case WM_LBUTTONDOWN: {
        SetFocus(h); int b = NpBtnAt(x, y);
        if (b >= 0 && NpEnabled(b)) { npPressed = npHot = b; SetCapture(h); NpRefresh(); return 0; }
        int r = NpRowAt(x, y); bool ctrl = GetKeyState(VK_CONTROL) < 0, shift = GetKeyState(VK_SHIFT) < 0;
        if (r >= 0) {
            if (shift && npAnchor >= 0) { npSel.clear(); for (int i = std::min(npAnchor, r); i <= std::max(npAnchor, r); i++) npSel.insert(i); }
            else if (ctrl) { if (!npSel.erase(r)) npSel.insert(r); npAnchor = r; }
            else { npSel = { r }; npAnchor = r; }
        } else if (y >= NP_LIST_TOP && y < NpListBottom()) npSel.clear();
        NpRefresh(); return 0;
    }
    case WM_LBUTTONDBLCLK: { int r = NpRowAt(x, y); if (r >= 0 && StartTrack(r)) { SetTimer(g_hWnd, TIMER_ID, 200, nullptr); InvalidateRect(g_hWnd, nullptr, FALSE); NpRefresh(); } return 0; }
    case WM_MOUSEMOVE: { int b = NpBtnAt(x, y); if (b >= 0 && !NpEnabled(b)) b = -1; if (b != npHot) { npHot = b; NpRefresh(); } TRACKMOUSEEVENT t = { sizeof(t), TME_LEAVE, h, 0 }; TrackMouseEvent(&t); return 0; }
    case WM_MOUSELEAVE: if (npPressed < 0 && npHot >= 0) { npHot = -1; NpRefresh(); } return 0;
    case WM_LBUTTONUP: {
        if (npPressed < 0) return 0;
        int b = npPressed; npPressed = -1; ReleaseCapture(); bool over = NpBtnAt(x, y) == b; NpRefresh();
        if (!over) return 0;
        if (b == NB_CLOSE) { DestroyWindow(h); return 0; }
        if (b == NB_ADD) SelectContent(g_hWnd, false);
        else if (b == NB_DELETE) { if (!npSel.empty()) NpDelete(npSel); }
        else if (b == NB_DELALL) { std::set<int> all; for (int i = 0; i < (int)tracks.size(); i++) all.insert(i); NpDelete(all); }
        else if (b == NB_SAVE) NpSave(h);
        InvalidateRect(g_hWnd, nullptr, FALSE); NpRefresh(); return 0;
    }
    case WM_MOUSEWHEEL: npTop = std::clamp(npTop - GET_WHEEL_DELTA_WPARAM(w) / WHEEL_DELTA * 3, 0, std::max(0, (int)tracks.size() - NpVisibleRows())); NpRefresh(); return 0;
    case WM_KEYDOWN:
        if (w == VK_DELETE && !npSel.empty()) { NpDelete(npSel); InvalidateRect(g_hWnd, nullptr, FALSE); NpRefresh(); return 0; }
        if (w == VK_ESCAPE) { DestroyWindow(h); return 0; }
        if (GetKeyState(VK_MENU) < 0 || GetKeyState(VK_CONTROL) < 0) { if (Key(g_hWnd, (UINT)w, GetKeyState(VK_MENU) < 0)) { NpRefresh(); return 0; } }
        break;
    case WM_DESTROY: npWnd = nullptr; nowPlayingOpen = false; return 0;
    }
    return DefWindowProcW(h, m, w, l);
}
inline void NowPlayingToggle(HWND owner) {
    if (npWnd) { DestroyWindow(npWnd); return; }
    static bool reg = false;
    if (!reg) {
        WNDCLASSEXW wc = { sizeof(wc) }; wc.style = CS_DBLCLKS; wc.lpfnWndProc = NpProc; wc.hInstance = GetModuleHandleW(nullptr); wc.lpszClassName = L"EAXNowPlaying"; wc.hCursor = LoadCursor(nullptr, IDC_ARROW); RegisterClassExW(&wc);
        npBase[0] = LoadPng(L"assets\\media\\np\\np_empty.png"); npBase[1] = LoadPng(L"assets\\media\\np\\np_one.png");
        npIdle = LoadPng(L"assets\\media\\np\\icon_idle.png"); npPlay = LoadPng(L"assets\\media\\np\\icon_play.png");
        const wchar_t* pf[6] = { L"add_pressed_49_149", L"delete_pressed_160_149", L"deleteall_pressed_270_149", L"saveas_pressed_384_149", L"close_pressed_469_5", L"delete_hover_161_150" };
        for (int i = 0; i < 6; i++) npPatch[i] = LoadPng(std::wstring(L"assets\\media\\np\\") + pf[i] + L".png");
        NpBuildCorners(); reg = true;
    }
    RECT r; GetWindowRect(owner, &r); npW = NP_BASE_W; npH = NP_BASE_H;
    npWnd = CreateWindowExW(WS_EX_APPWINDOW, L"EAXNowPlaying", L"Now Playing", WS_POPUP | WS_THICKFRAME, r.left + 115, std::max<LONG>(0, r.top - 40), npW, npH, nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
    NpApplyShape(npWnd); nowPlayingOpen = true; ShowWindow(npWnd, SW_SHOW); UpdateWindow(npWnd);
}
inline void ToggleNowPlaying(HWND h) { NowPlayingToggle(h); }

}
