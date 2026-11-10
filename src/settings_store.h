

#pragma once
#include <shlobj.h>
#include <sstream>
#include <map>

static int g_micBase = 100;

namespace ST {

inline std::string W2U(const std::wstring& w) { if (w.empty()) return ""; int n = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, nullptr, 0, nullptr, nullptr); std::string s(n - 1, 0); WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, s.data(), n, nullptr, nullptr); return s; }
inline std::wstring U2W(const std::string& s) { if (s.empty()) return L""; int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0); std::wstring w(n - 1, 0); MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, w.data(), n); return w; }
inline std::wstring Dir() {
    wchar_t p[MAX_PATH] = L""; SHGetFolderPathW(nullptr, CSIDL_APPDATA, nullptr, 0, p);
    std::wstring d = std::wstring(p) + L"\\CreativeEAXClone"; CreateDirectoryW(d.c_str(), nullptr); return d;
}
inline std::wstring SetupsDir() { std::wstring d = Dir() + L"\\Setups"; CreateDirectoryW(d.c_str(), nullptr); return d; }
inline bool ReadAll(const std::wstring& path, std::string& out) {
    FILE* f = _wfopen(path.c_str(), L"rb"); if (!f) return false;
    char buf[4096]; size_t n; out.clear(); while ((n = fread(buf, 1, sizeof buf, f)) > 0) out.append(buf, n);
    fclose(f); return true;
}
inline bool WriteAll(const std::wstring& path, const std::string& data) {
    const std::wstring tmp = path + L".part"; FILE* f = _wfopen(tmp.c_str(), L"wb"); if (!f) return false;
    const bool ok = fwrite(data.data(), 1, data.size(), f) == data.size(); fclose(f);
    return ok && MoveFileExW(tmp.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING) != 0;
}

struct EaxVar { const char* k; int* i; bool* b; };
#define EI(n) { #n, &n, nullptr }
#define EB(n) { #n, nullptr, &n }
inline std::vector<EaxVar>& EaxVars() {
    static std::vector<EaxVar> v = {
        EB(g_enableEffects), EI(g_selectedPreset), EI(g_effectsAmount), EI(g_scrollOffset), EB(g_enableCMSS), EI(g_cmssMode), EI(g_cmssWidening),
        EB(g_enableKaraoke), EB(g_karaokeVocSupEnabled), EB(g_karaokePitchEnabled), EB(g_karaokeActivePreset), EI(g_karaokeVocSup), EI(g_karaokePitch),
        EI(g_karaokePitchStep), EI(g_karaokeMode), EI(g_karaokeMicVol), EB(g_micMuted), EI(g_micMode),
        EB(g_enableCleanup), EI(g_cleanupNoiseRem), EI(g_cleanupClickRem), EI(g_cleanupBandAdj), EB(g_enableTimeScale), EI(g_timeScaleAmt), EI(g_micBase), EB(g_trayClose),
        EB(g_micEnabled), EB(g_spkEnabled),
    };
    return v;
}

inline const char* kEngineKeys[] = {
    "monitor", "rawMic", "cardMic", "wide", "echoOn", "changeOn", "isolate", "isoMode", "isoLevel", "gateLevel", "gateLo", "gateMid", "gateHi", "gateOut",
    "toneOn", "toneLo", "toneMid", "toneHi", "tonePadX", "tonePadY", "toneBaseLo", "toneBaseMid", "toneBaseHi",
    "clipGain", "clipHear", "clipToMic", "appHear", "appOnly", "appMute", "monMute", "appGain", "clipSpeed", "clipKeepPitch", "clipFollow",
    "pitchHz", "voiceSize", "voiceMacro", "voiceRatio", "voiceMask", "voiceSpeed", "voiceKeepPitch", "glitchArmed", "glitchKey", "glitchStyle",
};
inline std::wstring lastSetup;

inline std::string Snapshot() {
    std::ostringstream o;
    for (const EaxVar& v : EaxVars()) o << "x." << v.k + 2 << "=" << (v.i ? *v.i : (*v.b ? 1 : 0)) << "\n";
    for (const char* k : kEngineKeys) o << "e." << k << "=" << orb::GetI(k) << "\n";
    if (!g_savedMicId.empty()) o << "dev.mic=" << W2U(g_savedMicId) << "\n";
    if (!g_savedSpkId.empty()) o << "dev.spk=" << W2U(g_savedSpkId) << "\n";
    o << "app=" << W2U(orb::AppWanted()) << "\n";
    o << "media.mode=" << MB::mode << "\nmedia.repeat=" << (MB::repeatAll ? 1 : 0) << "\n";
    return o.str();
}
inline void Apply(const std::string& text) {
    std::map<std::string, std::string> kv; std::istringstream in(text); std::string line;
    while (std::getline(in, line)) { if (!line.empty() && line.back() == '\r') line.pop_back(); size_t e = line.find('='); if (e != std::string::npos) kv[line.substr(0, e)] = line.substr(e + 1); }
    auto num = [&](const std::string& k, int& out) { auto it = kv.find(k); if (it == kv.end()) return false; out = atoi(it->second.c_str()); return true; };
    for (const EaxVar& v : EaxVars()) { int x; if (num(std::string("x.") + (v.k + 2), x)) { if (v.i) *v.i = x; else *v.b = x != 0; } }
    for (const char* k : kEngineKeys) { int x; if (num(std::string("e.") + k, x)) orb::SetI(k, x); }
    auto dm = kv.find("dev.mic"), ds = kv.find("dev.spk");
    if (dm != kv.end() && !dm->second.empty()) g_savedMicId = U2W(dm->second);
    if (ds != kv.end() && !ds->second.empty()) g_savedSpkId = U2W(ds->second);
    if (!g_savedMicId.empty() || !g_savedSpkId.empty())
        orb::SetDevices((g_micEnabled && !g_savedMicId.empty()) ? g_savedMicId.c_str() : L"",
                        (g_spkEnabled && !g_savedSpkId.empty()) ? g_savedSpkId.c_str() : L"");
    auto ap = kv.find("app"); if (ap != kv.end()) orb::WantProgram(U2W(ap->second));
    int mm; if (num("media.mode", mm)) MB::mode = std::clamp(mm, 0, 4);
    int rp; if (num("media.repeat", rp)) MB::repeatAll = rp != 0;
}

inline std::string saved;
inline std::string pending; inline ULONGLONG pendingAt = 0;
inline void LoadCurrent() {
    std::string t; if (ReadAll(Dir() + L"\\settings.ini", t)) {
        Apply(t); saved = t;
        std::istringstream in(t); std::string line; while (std::getline(in, line)) if (line.rfind("setup=", 0) == 0) { lastSetup = U2W(line.substr(6)); while (!lastSetup.empty() && (lastSetup.back() == L'\r')) lastSetup.pop_back(); }
    }
}
inline std::string WithSetup(const std::string& s) { return s + "setup=" + W2U(lastSetup) + "\n"; }
inline void AutoSave(bool now = false) {
    const std::string snap = WithSetup(Snapshot());
    if (snap == saved) { pending.clear(); return; }
    if (snap != pending) { pending = snap; pendingAt = GetTickCount64(); }
    if (now || GetTickCount64() - pendingAt >= 600) { if (WriteAll(Dir() + L"\\settings.ini", snap)) saved = snap; pending.clear(); }
}

inline std::wstring Clean(std::wstring n) {
    for (auto& c : n) if (wcschr(L"\\/:*?\"<>|", c)) c = L'_';
    while (!n.empty() && (n.back() == L' ' || n.back() == L'.')) n.pop_back();
    while (!n.empty() && n.front() == L' ') n.erase(n.begin());
    return n;
}
inline std::vector<std::wstring> ListSetups() {
    std::vector<std::wstring> v; WIN32_FIND_DATAW fd; HANDLE h = FindFirstFileW((SetupsDir() + L"\\*.ini").c_str(), &fd);
    if (h != INVALID_HANDLE_VALUE) { do { std::wstring n = fd.cFileName; v.push_back(n.substr(0, n.size() - 4)); } while (FindNextFileW(h, &fd)); FindClose(h); }
    std::sort(v.begin(), v.end(), [](const std::wstring& a, const std::wstring& b) { return _wcsicmp(a.c_str(), b.c_str()) < 0; });
    return v;
}
inline bool SaveSetup(const std::wstring& raw) {
    std::wstring n = Clean(raw); if (n.empty()) return false;
    if (!WriteAll(SetupsDir() + L"\\" + n + L".ini", Snapshot())) return false;
    lastSetup = n; return true;
}
inline bool LoadSetup(const std::wstring& n) {
    std::string t; if (!ReadAll(SetupsDir() + L"\\" + n + L".ini", t)) return false;
    Apply(t); lastSetup = n; return true;
}
inline bool DeleteSetup(const std::wstring& n) { if (!DeleteFileW((SetupsDir() + L"\\" + n + L".ini").c_str())) return false; if (lastSetup == n) lastSetup.clear(); return true; }

}
