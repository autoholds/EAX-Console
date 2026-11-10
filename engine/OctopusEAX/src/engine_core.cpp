#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00
#endif
#define WIN32_LEAN_AND_MEAN
#define INITGUID
#include <windows.h>
#include <objidl.h>
#include <mmreg.h>
#include <xmmintrin.h>
#include <pmmintrin.h>
#include <initguid.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <functiondiscoverykeys_devpkey.h>
#include <commctrl.h>
#include <shellapi.h>
#include <shlobj.h>
#include <knownfolders.h>
#include <avrt.h>
#include <mmsystem.h>
#include <commdlg.h>
#include <aclapi.h>
#include <sddl.h>
#include <gdiplus.h>
#include <dwmapi.h>
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "msimg32.lib")
#pragma comment(lib, "uxtheme.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "comdlg32.lib")
#pragma comment(lib, "urlmon.lib")
#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "avrt.lib")
#pragma comment(lib, "mf.lib")
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfuuid.lib")

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_set>
#include <vector>
using std::max; using std::min;

#include "audio/render_feed.hpp"
#include "cable/cable_package.hpp"
#include "convolution/convolution_reverb.hpp"
#include "equalizer/bass_boost.hpp"
#include "equalizer/eq_presets.hpp"
#include "equalizer/graphic_equalizer.hpp"
#include "isolation/expander_gate.hpp"
#include "isolation/voice_isolator.hpp"
#include "mixing/amount_law.hpp"
#include "mixing/echo_tap.hpp"
#include "mixing/mic_drive.hpp"
#include "mixing/mic_tone.hpp"
#include "mixing/out_chain.hpp"
#include "mixing/send_level.hpp"
#include "mixing/soft_clip.hpp"
#include "mixing/step_back.hpp"
#include "mixing/subsonic.hpp"
#include "mixing/wet_peak.hpp"
#include "playback/hear_tap.hpp"
#include "playback/player.hpp"
#include "playback/program_tap.hpp"
#include "playback/recorder.hpp"
#include "playback/sound_board.hpp"
#include "playback/sound_edit.hpp"
#include "playback/sound_save.hpp"
#include "presets/card_presets.hpp"
#include "reverb/card_core.hpp"
#include "reverb/reverb_engine.hpp"
#include "reverb/env_reverb.hpp"
#include "ui/font_file.hpp"
#include "voice/voice_fx.hpp"
#include "voice/song_changer.hpp"
#include "voice/tempo.hpp"
#include "voice/glitch_pad.hpp"
#include "voice/voice_shaper.hpp"
#include "voice/voice_tone.hpp"
using namespace mix;

static clip::Player gClip;
static clip::ProgramTap gApp;
static clip::SoundBoard gBoard;
static clip::Recorder gRec, gRecHear, gBackVoice, gBackHear;
static clip::HearTap gHearTap;
static std::atomic<int> gMediaWork{0};

struct MediaWork {
    MediaWork() { ++gMediaWork; }
    ~MediaWork() { --gMediaWork; }
    MediaWork(const MediaWork&) = delete;
    MediaWork& operator=(const MediaWork&) = delete;
};

static bool mediaSettle(ULONGLONG ms) {
    const ULONGLONG until = GetTickCount64() + ms;
    while (gMediaWork.load() > 0 && GetTickCount64() < until) Sleep(20);
    return gMediaWork.load() == 0;
}
static clip::SoundBoard gTakeMic, gTakeEar;
static std::atomic<bool> takeHeld{false};
static std::vector<clip::Program> uiApps;
static std::wstring appNote;
static HWND hApp = nullptr;
static HWND hEqBox = nullptr;
static HWND hMaskBox = nullptr, hOwnBox = nullptr, hGlitchBox = nullptr;
static HWND hTip = nullptr;
static void tipsColour();
static std::vector<std::wstring> uiEqNames;
static bool eqOnPreset();
static void maskLabel(int i, wchar_t* txt);

static const int DEFAULT_PRESET = HAUNTED_CAVERN_INDEX;
static const int FS = 48000;
static const float CASCADE_GAIN = AmountLaw::CASCADE * powf(10.0f, (float)WetPeak::Tune().trimDb / 20.0f);

struct Dev { std::wstring name, id, desc, iface; };

static std::vector<Dev> listDevices(EDataFlow flow,
                                   DWORD states = DEVICE_STATE_ACTIVE) {
    std::vector<Dev> out;
    IMMDeviceEnumerator* en = nullptr;
    if (FAILED(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
                                __uuidof(IMMDeviceEnumerator), (void**)&en)))
        return out;
    IMMDeviceCollection* coll = nullptr;
    if (SUCCEEDED(en->EnumAudioEndpoints(flow, states, &coll))) {
        UINT n = 0; coll->GetCount(&n);
        for (UINT i = 0; i < n; ++i) {
            IMMDevice* d = nullptr;
            if (FAILED(coll->Item(i, &d))) continue;
            LPWSTR id = nullptr; d->GetId(&id);
            IPropertyStore* ps = nullptr;
            std::wstring name = L"?", desc, iface;
            if (SUCCEEDED(d->OpenPropertyStore(STGM_READ, &ps))) {
                auto text = [&](const PROPERTYKEY& key, std::wstring& dst) {
                    PROPVARIANT pv; PropVariantInit(&pv);
                    if (SUCCEEDED(ps->GetValue(key, &pv)) && pv.vt == VT_LPWSTR) dst = pv.pwszVal;
                    PropVariantClear(&pv);
                };
                text(PKEY_Device_FriendlyName, name);
                text(PKEY_Device_DeviceDesc, desc);
                text(PKEY_DeviceInterface_FriendlyName, iface);
                ps->Release();
            }
            if (id) { out.push_back({name, id, desc, iface}); CoTaskMemFree(id); }
            d->Release();
        }
        coll->Release();
    }
    en->Release();
    return out;
}

static std::wstring defaultDeviceId(EDataFlow flow, ERole role = eConsole) {
    std::wstring res;
    IMMDeviceEnumerator* en = nullptr;
    if (FAILED(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
                                __uuidof(IMMDeviceEnumerator), (void**)&en)))
        return res;
    IMMDevice* d = nullptr;
    if (SUCCEEDED(en->GetDefaultAudioEndpoint(flow, role, &d))) {
        LPWSTR id = nullptr;
        if (SUCCEEDED(d->GetId(&id)) && id) { res = id; CoTaskMemFree(id); }
        d->Release();
    }
    en->Release();
    return res;
}

static bool nameContains(const std::wstring& hay, const wchar_t* needle) {
    std::wstring a = hay, b = needle;
    for (auto& c : a) c = towlower(c);
    for (auto& c : b) c = towlower(c);
    return a.find(b) != std::wstring::npos;
}

static bool enablePrivilege(const wchar_t* name) {
    HANDLE tok = nullptr;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &tok)) return false;
    TOKEN_PRIVILEGES tp{};
    tp.PrivilegeCount = 1;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    bool ok = LookupPrivilegeValueW(nullptr, name, &tp.Privileges[0].Luid) &&
              AdjustTokenPrivileges(tok, FALSE, &tp, 0, nullptr, nullptr) && GetLastError() == ERROR_SUCCESS;
    CloseHandle(tok);
    return ok;
}

static bool grantAdminsOnKey(const std::wstring& objPath) {
    enablePrivilege(L"SeTakeOwnershipPrivilege");
    enablePrivilege(L"SeRestorePrivilege");
    PSID admins = nullptr;
    if (!ConvertStringSidToSidW(L"S-1-5-32-544", &admins)) return false;
    std::vector<wchar_t> path(objPath.begin(), objPath.end()); path.push_back(0);
    bool ok = SetNamedSecurityInfoW(path.data(), SE_REGISTRY_KEY, OWNER_SECURITY_INFORMATION,
                                    admins, nullptr, nullptr, nullptr) == ERROR_SUCCESS;
    if (ok) {
        EXPLICIT_ACCESSW ea{};
        ea.grfAccessPermissions = KEY_ALL_ACCESS;
        ea.grfAccessMode = GRANT_ACCESS;
        ea.grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
        ea.Trustee.TrusteeForm = TRUSTEE_IS_SID;
        ea.Trustee.TrusteeType = TRUSTEE_IS_GROUP;
        ea.Trustee.ptstrName = (LPWSTR)admins;
        PACL oldDacl = nullptr, newDacl = nullptr;
        PSECURITY_DESCRIPTOR sd = nullptr;
        if (GetNamedSecurityInfoW(path.data(), SE_REGISTRY_KEY, DACL_SECURITY_INFORMATION,
                                  nullptr, nullptr, &oldDacl, nullptr, &sd) == ERROR_SUCCESS) {
            if (SetEntriesInAclW(1, &ea, oldDacl, &newDacl) == ERROR_SUCCESS) {
                ok = SetNamedSecurityInfoW(path.data(), SE_REGISTRY_KEY, DACL_SECURITY_INFORMATION,
                                           nullptr, nullptr, newDacl, nullptr) == ERROR_SUCCESS;
                LocalFree(newDacl);
            } else ok = false;
            LocalFree(sd);
        } else ok = false;
    }
    LocalFree(admins);
    return ok;
}

static const wchar_t* FRIENDLY_NAME_VALUE = L"{a45c254e-df1c-4efd-8020-67d146a850e0},2";

static bool writeEndpointName(const std::wstring& key, const wchar_t* newName) {
    HKEY hk;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, key.c_str(), 0, KEY_SET_VALUE | KEY_WOW64_64KEY, &hk) != ERROR_SUCCESS)
        return false;
    LONG rc = RegSetValueExW(hk, FRIENDLY_NAME_VALUE, 0, REG_SZ, (const BYTE*)newName,
                             (DWORD)((wcslen(newName) + 1) * sizeof(wchar_t)));
    RegCloseKey(hk);
    return rc == ERROR_SUCCESS;
}

static bool renameEndpoint(const std::wstring& mmid, const wchar_t* newName) {
    size_t p = mmid.rfind(L'{');
    if (p == std::wstring::npos) return false;
    std::wstring key = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\MMDevices\\Audio\\Capture\\" +
                       mmid.substr(p) + L"\\Properties";
    if (writeEndpointName(key, newName)) return true;
    if (!grantAdminsOnKey(L"MACHINE\\" + key)) return false;
    return writeEndpointName(key, newName);
}

enum { ON_VOICE = 1, ON_CLIP = 2, ON_PROGRAM = 4, ON_ALL = 7 };
enum { STAGE_MISSING, STAGE_RESTART, STAGE_READY, STAGE_PROBLEM };
static int echoFromRoute(int route) { return route == 0 ? ON_VOICE | ON_CLIP : route == 1 ? ON_CLIP | ON_PROGRAM : ON_ALL; }

static const int AMOUNT_DEFAULT = 9, MIC_DEFAULT = 100, ISO_DEFAULT = 60, CLIP_GAIN_DEFAULT = 84, APP_GAIN_DEFAULT = 80;
static const int GATE_DEFAULT_DB = -170, GATE_OLD_LEVEL = 60, GATE_MIN_DB = -660, VOCAL_DEFAULT_LEVEL = iso::GATE_LEVEL_NORMAL;
static const int GATE_DEFAULT_TONE = 120, GATE_DEFAULT_OUT = -50, GATE_TONE_RANGE = 120;
static const int TONE_RANGE = 240, TONE_SAVED_ZERO = 120, TONE_PAD_RANGE = 100;
static const int ISO_VOCAL = iso::NUM_MODES, ISO_MODES = iso::NUM_MODES + 1;
static const int ISO_ORDER[ISO_MODES] = {ISO_VOCAL, 0, 1, 2, 3};

struct Shared {
    std::atomic<int> micLevel{MIC_DEFAULT};
    std::atomic<int> amount{AMOUNT_DEFAULT};

    std::atomic<int> preset{DEFAULT_PRESET};
    std::atomic<bool> enabled{true};
    std::atomic<int>  echoOn{7};
    std::atomic<int>  changeOn{1};
    std::atomic<bool> monitor{true};
    std::atomic<bool> rawMic{false};
    std::atomic<bool> cardMic{true};
    std::atomic<bool> cmss{false};
    std::atomic<int>  cmssMode{0}, cmssWidth{0}, cmssDepth{100};
    std::atomic<bool> wide{false};
    std::atomic<int>  eqDb[eqb::EQ_BANDS]{};
    std::atomic<int>  eqLevel{0};
    std::atomic<bool> eqOn{true};
    std::atomic<int>  eqPreset{0};
    std::atomic<int>  eqVersion{0};
    std::atomic<bool> isolate{false};
    std::atomic<int>  isoMode{ISO_VOCAL};
    std::atomic<int>  isoLevel{ISO_DEFAULT};
    std::atomic<int>  gateLevel{VOCAL_DEFAULT_LEVEL};
    std::atomic<int>  gateLo{GATE_DEFAULT_TONE};
    std::atomic<int>  gateMid{GATE_DEFAULT_TONE};
    std::atomic<int>  gateHi{GATE_DEFAULT_TONE};
    std::atomic<int>  gateOut{GATE_DEFAULT_OUT};
    std::atomic<bool> toneOn{true};
    std::atomic<int>  toneLo{0};
    std::atomic<int>  toneMid{0};
    std::atomic<int>  toneHi{0};
    std::atomic<int>  tonePadX{0};
    std::atomic<int>  tonePadY{0};
    std::atomic<int>  toneBaseLo{0};
    std::atomic<int>  toneBaseMid{0};
    std::atomic<int>  toneBaseHi{0};
    std::atomic<int>  toneVersion{0};
    std::atomic<int>  clipGain{CLIP_GAIN_DEFAULT};
    std::atomic<bool> clipHear{true};
    std::atomic<bool> clipToMic{true};
    std::atomic<bool> appHear{false};
    std::atomic<bool> appOnly{false};
    std::atomic<bool> appMute{false};
    std::atomic<int>  pitchHz{0};
    std::atomic<int>  voiceSize{100};
    std::atomic<int>  voiceHz{0};
    std::atomic<int>  voiceMacro{0};
    std::atomic<int>  voiceRatio{1000};
    std::atomic<int>  voiceMask{voice::MASK_NEUTRAL};
    std::atomic<int>  voiceTargetHz{0};
    std::atomic<int>  voiceSpeed{100};
    std::atomic<bool> voiceKeepPitch{true};
    std::atomic<int>  voiceLagMs{0};
    std::atomic<int>  clipSpeed{100};
    std::atomic<bool> clipKeepPitch{true};
    std::atomic<bool> clipFollow{true};
    std::atomic<int>  appGain{APP_GAIN_DEFAULT};
    std::atomic<int>  meterPeak{0};
    std::atomic<bool> glitchHold{false};
    std::atomic<bool> glitchKeyHeld{false};
    std::atomic<bool> glitchArmed{true};
    std::atomic<int>  glitchKey{0};
    std::atomic<int>  glitchStyle{voice::GLITCH_REPEAT};
    std::atomic<bool> reopenMic{false};
    std::atomic<int>  isoVersion{0};
    std::atomic<bool> restart{true};
    std::atomic<bool> quit{false};
    std::mutex devMx;
    std::wstring micId, spkId;
    std::atomic<bool> running{false};
    std::atomic<bool> statusWarn{false};
    std::atomic<bool> needCable{false};
    std::atomic<bool> askCable{false};
    std::atomic<int>  cableState{0};
    std::atomic<int>  cableStage{0};
    wchar_t status[256] = L"";
    wchar_t cable[256] = L"";
    wchar_t cableName[256] = L"";
};
static Shared g;

static voice::ToneGains toneGains() {
    voice::ToneGains t;
    if (!g.toneOn.load()) return t;
    t.loDb = g.toneLo.load() / 10.0;
    t.midDb = g.toneMid.load() / 10.0;
    t.hiDb = g.toneHi.load() / 10.0;
    return t;
}

static int vocalLevelOfThreshold(int threshold) {
    threshold = threshold < GATE_MIN_DB ? GATE_MIN_DB : (threshold > 0 ? 0 : threshold);
    if (threshold <= GATE_DEFAULT_DB) return (int)std::lround((threshold - GATE_MIN_DB) * (double)VOCAL_DEFAULT_LEVEL / (GATE_DEFAULT_DB - GATE_MIN_DB));
    return (int)std::lround(VOCAL_DEFAULT_LEVEL + (threshold - GATE_DEFAULT_DB) * (100.0 - VOCAL_DEFAULT_LEVEL) / -GATE_DEFAULT_DB);
}
static int vocalLevelOfOld(int old) {
    old = old < 0 ? 0 : (old > 100 ? 100 : old);
    if (old <= GATE_OLD_LEVEL) return (int)std::lround(old * (double)VOCAL_DEFAULT_LEVEL / GATE_OLD_LEVEL);
    return (int)std::lround(VOCAL_DEFAULT_LEVEL + (old - GATE_OLD_LEVEL) * (100.0 - VOCAL_DEFAULT_LEVEL) / (100 - GATE_OLD_LEVEL));
}
static iso::GateShape gateShape() {
    iso::GateShape s;
    const int level = g.gateLevel.load();
    s.thresholdDb = iso::gateLevelThresholdDb(level);
    s.slope = iso::gateLevelSlope(level);
    s.loDb = g.gateLo.load() / 10.0;
    s.midDb = g.gateMid.load() / 10.0;
    s.hiDb = g.gateHi.load() / 10.0;
    s.outDb = g.gateOut.load() / 10.0;
    return s;
}
static bool vocalOn() { return g.isolate.load() && g.isoMode.load() == ISO_VOCAL; }
static std::atomic<int>& isoFader() { return g.isoMode.load() == ISO_VOCAL ? g.gateLevel : g.isoLevel; }
static const wchar_t* isoModeName(int m) { return m == ISO_VOCAL ? L"Vocal" : iso::MODE_NAME[m < 0 ? 0 : (m >= iso::NUM_MODES ? iso::NUM_MODES - 1 : m)]; }

static const wchar_t* CABLE_SERVICE =
    L"SYSTEM\\CurrentControlSet\\Services\\VBAudioVACMME";

static bool driverPresent() {
    HKEY hk;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, CABLE_SERVICE, 0,
                      KEY_QUERY_VALUE | KEY_WOW64_64KEY, &hk) != ERROR_SUCCESS)
        return false;
    RegCloseKey(hk);
    return true;
}

static bool cableAdapter(const Dev& d) {
    return nameContains(d.iface, L"VB-Audio Virtual Cable");
}

static bool cableCapture(const Dev& d) {
    if (cableAdapter(d) || nameContains(d.name, L"CABLE Output")) return true;
    return d.iface.empty() && nameContains(d.name, L"What U Hear");
}

static bool cableRender(const Dev& d) {
    return (cableAdapter(d) && !nameContains(d.name, L"16ch")) || nameContains(d.name, L"CABLE Input");
}

static Dev findCableRender(DWORD states) {
    Dev best;
    for (auto& d : listDevices(eRender, states)) {
        if (!cableRender(d)) continue;
        if (nameContains(d.name, L"CABLE Input")) return d;
        if (best.id.empty()) best = d;
    }
    return best;
}

static std::wstring findParkRender(bool silentOk) {
    const std::wstring def = defaultDeviceId(eRender);
    const std::wstring cable = findCableRender(DEVICE_STATE_ACTIVE).id;
    std::wstring spk;
    { std::lock_guard<std::mutex> lk(g.devMx); spk = g.spkId; }
    std::wstring best;
    int bestScore = 0;
    for (const Dev& d : listDevices(eRender)) {
        if (d.id.empty() || d.id == def || d.id == cable || d.id == spk) continue;
        if (cableAdapter(d) || cableRender(d) || nameContains(d.name, L"CABLE") || nameContains(d.name, L"What U Hear")) continue;
        if (nameContains(d.name, L"Voicemeeter Input") && !nameContains(d.name, L"Aux") && !nameContains(d.name, L"VAIO3"))
            continue;
        int sc = 0;
        if (nameContains(d.name, L"Aux Input") || nameContains(d.name, L"Voicemeeter Aux")) sc = 100;
        else if (nameContains(d.name, L"VAIO3")) sc = 90;
        else if (silentOk && nameContains(d.name, L"Steam Streaming Speakers")) sc = 50;
        else continue;
        if (sc > bestScore) { bestScore = sc; best = d.id; }
    }
    return best;
}

static std::wstring renderName(const std::wstring& id) {
    if (id.empty()) return L"";
    for (const Dev& d : listDevices(eRender))
        if (d.id == id) return d.name;
    return L"";
}

static bool voicemeeterRender(const Dev& d) {
    return nameContains(d.name, L"Voicemeeter") || nameContains(d.iface, L"VB-Audio VoiceMeeter")
        || nameContains(d.name, L"VAIO");
}

static bool idIsVoicemeeter(const std::wstring& id) {
    if (id.empty()) return false;
    for (const Dev& d : listDevices(eRender))
        if (d.id == id) return voicemeeterRender(d);
    return false;
}

static std::wstring findAppHearRender() {
    const std::wstring park = findParkRender(false);
    const std::wstring cable = findCableRender(DEVICE_STATE_ACTIVE).id;
    const std::wstring def = defaultDeviceId(eRender);
    std::wstring spk;
    { std::lock_guard<std::mutex> lk(g.devMx); spk = g.spkId; }
    std::wstring best;
    int bestScore = 0;
    for (const Dev& d : listDevices(eRender)) {
        if (d.id.empty() || d.id == park || d.id == cable || d.id == spk || d.id == def) continue;
        if (cableRender(d) || nameContains(d.name, L"CABLE Input") || nameContains(d.name, L"What U Hear")) continue;
        if (nameContains(d.name, L"Voicemeeter Input") && !nameContains(d.name, L"Aux") && !nameContains(d.name, L"VAIO3"))
            continue;
        int sc = 0;
        if (nameContains(d.name, L"VAIO3")) sc = 100;
        else if (nameContains(d.name, L"Aux Input") || nameContains(d.name, L"Voicemeeter Aux")) continue;
        else if (nameContains(d.iface, L"VB-Audio VoiceMeeter") || nameContains(d.name, L"VAIO")) sc = 50;
        else continue;
        if (sc > bestScore) { bestScore = sc; best = d.id; }
    }
    return best;
}

static bool appToMonitor() { return !g.appHear.load() || g.appOnly.load(); }

static std::wstring appParkName;

static void appPick(const clip::Program& p) {
    const std::wstring park = findParkRender(appToMonitor());
    appParkName = renderName(park);
    gApp.start(p, FS, park);
}

static void appRepark() {
    if (!gApp.chosen()) return;
    if (findParkRender(appToMonitor()) != gApp.stealTo) appPick(clip::Program{gApp.pid, gApp.name});
}

static std::wstring monitorRenderId(const std::wstring& spkId) {
    const bool hearApp = gApp.chosen() && appToMonitor();
    const bool self = g.monitor.load() || g.clipHear.load() || takeHeld.load();
    if (hearApp && idIsVoicemeeter(spkId)) {
        if (self) return spkId;
        return findAppHearRender();
    }
    if (self || hearApp) return spkId;
    return {};
}

static void setStatus(const wchar_t* s, bool warn) {
    {
        std::lock_guard<std::mutex> lk(g.devMx);
        wcsncpy_s(g.cable, s, 255);
    }
    g.statusWarn = warn;
}

static bool cableDisabled() {
    const DWORD off = DEVICE_STATE_DISABLED | DEVICE_STATE_UNPLUGGED;
    for (auto& d : listDevices(eCapture, off))
        if (cableCapture(d)) return true;
    return !findCableRender(off).id.empty();
}

static int renameTries = 0;

static bool isElevated() {
    HANDLE tok = nullptr;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &tok)) return false;
    TOKEN_ELEVATION te{}; DWORD cb = sizeof te;
    const bool ok = GetTokenInformation(tok, TokenElevation, &te, sizeof te, &cb) && te.TokenIsElevated;
    CloseHandle(tok);
    return ok;
}

static bool cableReady() {
    for (auto& d : listDevices(eCapture)) {
        if (!cableCapture(d)) continue;
        if (!nameContains(d.name, L"What U Hear") && nameContains(d.name, L"CABLE Output") &&
            isElevated() && renameTries < 20 && !renameEndpoint(d.id, L"What U Hear"))
            ++renameTries;
        std::lock_guard<std::mutex> lk(g.devMx);
        wcsncpy_s(g.cableName, d.name.c_str(), 255);
        return true;
    }
    std::lock_guard<std::mutex> lk(g.devMx);
    g.cableName[0] = 0;
    return false;
}

static std::wstring exeFolder() {
    wchar_t exe[MAX_PATH];
    GetModuleFileNameW(nullptr, exe, MAX_PATH);
    std::wstring dir(exe);
    return dir.substr(0, dir.find_last_of(L"\\/"));
}

static const wchar_t* INSTALL_FLAG = L"--install-cable";
static const UINT WM_CABLE_READY = WM_APP + 21;
enum { CABLE_IDLE, CABLE_FETCH, CABLE_UNPACK, CABLE_SETUP, CABLE_FAILED };
static std::atomic<int> gCableJob{CABLE_IDLE};
static std::atomic<bool> gCableRetry{false};

static std::wstring cableSetup() { return cable::setupIn(cable::folder()); }

static int elevatedInstall(const std::wstring& setup) {
    if (!cable::isSetupName(setup) || !cable::exists(setup)) return 2;
    const HANDLE held = CreateFileW(setup.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (held == INVALID_HANDLE_VALUE) return 2;
    if (!cable::signedFile(setup, held)) {
        CloseHandle(held);
        return 2;
    }
    const std::wstring dir = setup.substr(0, setup.find_last_of(L"\\/"));
    SHELLEXECUTEINFOW se{};
    se.cbSize = sizeof(se);
    se.fMask = SEE_MASK_NOCLOSEPROCESS;
    se.lpVerb = L"open";
    se.lpFile = setup.c_str();
    se.lpDirectory = dir.c_str();
    se.nShow = SW_SHOWNORMAL;
    const bool started = ShellExecuteExW(&se) && se.hProcess;
    CloseHandle(held);
    if (!started) return 1;
    WaitForSingleObject(se.hProcess, 30 * 60 * 1000);
    CloseHandle(se.hProcess);
    const HRESULT com = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    for (int i = 0; i < 200 && !cableReady(); ++i) Sleep(100);
    if (SUCCEEDED(com)) CoUninitialize();
    return 0;
}

static void startCableDownload(HWND owner) {
    int idle = CABLE_IDLE, failed = CABLE_FAILED;
    if (!gCableJob.compare_exchange_strong(idle, CABLE_FETCH) && !gCableJob.compare_exchange_strong(failed, CABLE_FETCH)) return;
    std::thread([owner] {
        const HRESULT com = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        const std::wstring dir = cable::folder();
        cable::makeFolder(dir);
        std::wstring setup = cable::setupIn(dir);
        if (setup.empty()) {
            const std::wstring page = dir + L"\\vb-cable.htm", zip = dir + L"\\VBCABLE_Driver_Pack.zip";
            const std::vector<std::wstring> urls = cable::packUrls(cable::fetch(cable::PAGE_URL, page) ? page : L"");
            DeleteFileW(page.c_str());
            bool got = false;
            for (const std::wstring& u : urls)
                if (cable::fetch(u, zip) && cable::sizeOf(zip) > 100000) { got = true; break; }
            if (got) {
                gCableJob = CABLE_UNPACK;
                cable::unzip(zip, dir);
                setup = cable::setupIn(dir);
            }
        }
        if (SUCCEEDED(com)) CoUninitialize();
        if (setup.empty()) { gCableJob = CABLE_FAILED; return; }
        gCableJob = CABLE_SETUP;
        PostMessageW(owner, WM_CABLE_READY, 0, 0);
    }).detach();
}

static void runCableSetup(HWND owner) {
    const std::wstring setup = cableSetup();
    if (setup.empty()) { gCableJob = CABLE_FAILED; return; }
    if (isElevated()) {
        std::thread([setup] { elevatedInstall(setup); gCableJob = CABLE_IDLE; g.restart = true; }).detach();
        return;
    }
    wchar_t exe[MAX_PATH];
    GetModuleFileNameW(nullptr, exe, MAX_PATH);
    const std::wstring args = std::wstring(INSTALL_FLAG) + L" \"" + setup + L"\"";
    SHELLEXECUTEINFOW se{};
    se.cbSize = sizeof(se);
    se.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_FLAG_NO_UI;
    se.hwnd = owner;
    se.lpVerb = L"runas";
    se.lpFile = exe;
    se.lpParameters = args.c_str();
    se.nShow = SW_HIDE;
    if (!ShellExecuteExW(&se) || !se.hProcess) { gCableJob = CABLE_IDLE; return; }
    HANDLE proc = se.hProcess;
    std::thread([proc] {
        WaitForSingleObject(proc, INFINITE);
        CloseHandle(proc);
        gCableJob = CABLE_IDLE;
        g.restart = true;
    }).detach();
}

static void openCableFolder() {
    ShellExecuteW(nullptr, L"open", cable::folder().c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}

static void provisionCable() {
    const HRESULT com = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    int busyTicks = 0;
    for (;;) {
        const int job = gCableJob.load();
        if (job == CABLE_FETCH || job == CABLE_UNPACK) {
            setStatus(L"Downloading VB-CABLE...", true);
            g.needCable = true;
        } else if (job == CABLE_SETUP) {
            setStatus(L"Installing VB-CABLE. Finish the installer window, then restart your PC.", true);
            g.needCable = true;
        } else if (cableReady()) {
            const int state = g.cableState.load();
            const bool playback = !findCableRender(DEVICE_STATE_ACTIVE).id.empty();
            g.needCable = false;
            if (!playback) {
                const bool off = !findCableRender(DEVICE_STATE_DISABLED | DEVICE_STATE_UNPLUGGED).id.empty();
                if (off)
                    { setStatus(L"CABLE Input is turned off. Turn it back on in Windows Sound settings, Playback tab.", true); g.cableStage = STAGE_PROBLEM; }
                else {
                    setStatus(L"VB-CABLE didn't install completely. Click here to install it again.", true); g.cableStage = STAGE_MISSING;
                    g.needCable = true;
                    g.askCable = true;
                }
                busyTicks = 0;
            } else if (state == 3) {
                setStatus(L"Another app is holding CABLE Input (Voicemeeter or OBS?). Close it or change its output.", true); g.cableStage = STAGE_PROBLEM;
                if (++busyTicks >= 5) { busyTicks = 0; gCableRetry = true; }
            } else {
                setStatus(L"Virtual mic ready", false); g.cableStage = STAGE_READY;
                busyTicks = 0;
                if (state == 2 && g.running.load()) g.restart = true;
            }
        } else if (cableDisabled()) {
            setStatus(L"The virtual mic is turned off. Turn on CABLE Output and CABLE Input in Windows Sound settings.", true); g.cableStage = STAGE_PROBLEM;
            g.needCable = false;
        } else if (driverPresent()) {
            setStatus(L"Restart your PC to finish installing VB-CABLE.", true); g.cableStage = STAGE_RESTART;
            g.needCable = false;
        } else {
            setStatus(L"No virtual mic yet. Click here to install VB-CABLE.", true); g.cableStage = STAGE_MISSING;
            g.needCable = true;
            g.askCable = true;
        }

        for (int i = 0; i < 30 && !g.quit.load(); ++i) Sleep(100);
        if (g.quit.load()) break;
    }
    if (SUCCEEDED(com)) CoUninitialize();
}

static inline float micGainFor(int v) {
    return v <= 0 ? 0.0f : powf((float)v / 100.0f, 1.3010f);
}

static bool chordDown(int packed) {
    const int vk = packed & 0xFFFF, mods = packed >> 16;
    auto down = [](int k) { return (GetAsyncKeyState(k) & 0x8000) != 0; };
    if (!vk || !down(vk)) return false;
    if ((mods & MOD_CONTROL) && !down(VK_CONTROL)) return false;
    if ((mods & MOD_ALT) && !down(VK_MENU)) return false;
    if ((mods & MOD_SHIFT) && !down(VK_SHIFT)) return false;
    if ((mods & MOD_WIN) && !down(VK_LWIN) && !down(VK_RWIN)) return false;
    return true;
}

static int clipRate() {
    const int own = g.clipSpeed.load();
    if (!g.clipFollow.load() || !(g.changeOn.load() & ON_VOICE)) return own;
    return std::clamp(own * g.voiceSpeed.load() / 100, 25, 400);
}

static void flushDenormals() {
    _mm_setcsr(_mm_getcsr() | 0x8040);
}

static HANDLE gCapEvt = CreateEventW(nullptr, FALSE, FALSE, nullptr);

static const int RENDER_BUFFER_MS = 80;
static const int CABLE_FILL_MS = 25;
static const int CABLE_SLACK_MS = 15;
static const int MONITOR_FILL_MS = 10;
static const int MONITOR_SLACK_MS = 4;
static const int CAPTURE_BUFFER_MS = 100;

static void floatStereo(WAVEFORMATEX& fmt) {
    fmt = {};
    fmt.wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
    fmt.nChannels = 2;
    fmt.nSamplesPerSec = FS;
    fmt.wBitsPerSample = 32;
    fmt.nBlockAlign = 8;
    fmt.nAvgBytesPerSec = FS * 8;
}

static bool mixIsFloatStereo48(const WAVEFORMATEX* f) {
    if (!f || f->nChannels != 2 || f->nSamplesPerSec != (DWORD)FS || f->wBitsPerSample != 32)
        return false;
    if (f->wFormatTag == WAVE_FORMAT_IEEE_FLOAT) return true;
    if (f->wFormatTag != WAVE_FORMAT_EXTENSIBLE || f->cbSize < 22) return false;
    const auto* e = reinterpret_cast<const WAVEFORMATEXTENSIBLE*>(f);
    static const GUID kIeeeFloat =
        {0x00000003, 0x0000, 0x0010, {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}};
    return IsEqualGUID(e->SubFormat, kIeeeFloat);
}

static void applyClientProps(IAudioClient* ac, bool raw) {
    IAudioClient2* ac2 = nullptr;
    if (FAILED(ac->QueryInterface(__uuidof(IAudioClient2), (void**)&ac2))) return;
    AudioClientProperties props{};
    props.cbSize = sizeof(props);
    props.bIsOffload = FALSE;
    props.eCategory = AudioCategory_Other;
    props.Options = raw ? AUDCLNT_STREAMOPTIONS_RAW : (AUDCLNT_STREAMOPTIONS)0;
    ac2->SetClientProperties(&props);
    ac2->Release();
}

static bool initLowLatency(IAudioClient* ac, DWORD flags, HANDLE evt) {
    WAVEFORMATEX* mix = nullptr;
    if (FAILED(ac->GetMixFormat(&mix)) || !mix) return false;
    const bool okFmt = mixIsFloatStereo48(mix);
    IAudioClient3* ac3 = nullptr;
    bool ok = false;
    if (okFmt && SUCCEEDED(ac->QueryInterface(__uuidof(IAudioClient3), (void**)&ac3))) {
        UINT32 defP = 0, fundP = 0, minP = 0, maxP = 0;
        if (SUCCEEDED(ac3->GetSharedModeEnginePeriod(mix, &defP, &fundP, &minP, &maxP)) && minP) {
            if (SUCCEEDED(ac3->InitializeSharedAudioStream(flags, minP, mix, nullptr))) {
                ok = !evt || SUCCEEDED(ac->SetEventHandle(evt));
            }
        }
        ac3->Release();
    }
    CoTaskMemFree(mix);
    return ok;
}

static IAudioClient* tryOpenStream(IMMDevice* dev, bool capture, bool raw, UINT32& bufFrames) {
    IAudioClient* ac = nullptr;
    if (FAILED(dev->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void**)&ac)))
        return nullptr;
    applyClientProps(ac, raw);
    HANDLE evt = capture ? gCapEvt : nullptr;
    if (capture && raw) {
        if (initLowLatency(ac, AUDCLNT_STREAMFLAGS_EVENTCALLBACK, evt)) {
            ac->GetBufferSize(&bufFrames);
            return ac;
        }
        ac->Release();
        if (FAILED(dev->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void**)&ac)))
            return nullptr;
        applyClientProps(ac, raw);
    }

    WAVEFORMATEX fmt;
    floatStereo(fmt);
    DWORD flags = AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM;
    if (capture) flags |= AUDCLNT_STREAMFLAGS_EVENTCALLBACK;
    const int ms = capture ? CAPTURE_BUFFER_MS : RENDER_BUFFER_MS;
    if (FAILED(ac->Initialize(AUDCLNT_SHAREMODE_SHARED, flags, (REFERENCE_TIME)ms * 10000, 0, &fmt, nullptr))) {
        ac->Release();
        return nullptr;
    }
    if (evt && FAILED(ac->SetEventHandle(evt))) {
        ac->Release();
        return nullptr;
    }
    ac->GetBufferSize(&bufFrames);
    return ac;
}

static IAudioClient* openStream(IMMDevice* dev, bool capture, bool raw, UINT32& bufFrames) {
    IAudioClient* ac = tryOpenStream(dev, capture, raw, bufFrames);
    if (ac) return ac;
    if (raw) return tryOpenStream(dev, capture, false, bufFrames);
    return nullptr;
}

static IAudioClient* openCapture(IMMDevice* dev, UINT32& bufFrames) {
    return openStream(dev, true, g.rawMic.load(), bufFrames);
}

static bool openFeed(IMMDevice* dev, IAudioClient*& ac, wasapi::RenderFeed& feed, bool quick) {
    UINT32 frames = 0;
    ac = openStream(dev, false, quick, frames);
    if (!ac) return false;
    const UINT32 fill = (UINT32)(FS * (quick ? MONITOR_FILL_MS : CABLE_FILL_MS) / 1000);
    const UINT32 slack = (UINT32)(FS * (quick ? MONITOR_SLACK_MS : CABLE_SLACK_MS) / 1000);
    if (feed.attach(ac, frames, fill, slack)) return true;
    ac->Release();
    ac = nullptr;
    return false;
}

struct MicSwap {
    std::thread worker;
    std::atomic<bool> done{false};
    bool busy = false;
    IAudioClient* fresh = nullptr;

    void begin(IMMDevice* dev) {
        busy = true;
        done = false;
        worker = std::thread([this, dev] {
            const HRESULT com = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
            UINT32 frames = 0;
            fresh = openCapture(dev, frames);
            if (SUCCEEDED(com)) CoUninitialize();
            done = true;
        });
    }
    bool ready() const { return busy && done.load(); }
    IAudioClient* collect() {
        worker.join();
        busy = false;
        IAudioClient* c = fresh;
        fresh = nullptr;
        return c;
    }
    void abandon() {
        if (worker.joinable()) worker.join();
        busy = false;
        if (fresh) { fresh->Release(); fresh = nullptr; }
    }
};

struct InputTap {
    DWORD lastInput = 0;
    POINT lastPos{};
    int lastButtons = 0;
    bool primed = false;

    bool poll() {
        LASTINPUTINFO li{sizeof(li), 0};
        if (!GetLastInputInfo(&li)) return false;
        static const int BUTTONS[] = {VK_LBUTTON, VK_RBUTTON, VK_MBUTTON, VK_XBUTTON1, VK_XBUTTON2};
        int buttons = 0;
        for (int i = 0; i < 5; ++i)
            if (GetAsyncKeyState(BUTTONS[i]) & 0x8000) buttons |= 1 << i;
        POINT pos{};
        const bool havePos = GetCursorPos(&pos) != FALSE;
        const bool moved = havePos && (pos.x != lastPos.x || pos.y != lastPos.y);
        const bool fresh = primed && (buttons != lastButtons || (li.dwTime != lastInput && !moved));
        lastInput = li.dwTime;
        if (havePos) lastPos = pos;
        lastButtons = buttons;
        primed = true;
        return fresh;
    }
};

static const ULONGLONG ROOMS_KEEP_MS = 2000;
static const UINT32 MOST_LOST_FRAMES = FS * RENDER_BUFFER_MS / 1000;
static const int MIC_POS_TRUST = 8;
static const float WIDE_ECHO = 1.5f;

static inline void widen(float& l, float& r, float w) {
    const float m = (l + r) * 0.5f, s = (l - r) * 0.5f * w;
    l = m + s;
    r = m - s;
}

struct RoomBus {
    env::EnvironmentReverb rv;
    int64_t tail = 0;
    std::vector<float> feedL, feedR, wl, wr;

    void forget() {
        rv.suspend();
        tail = 0;
    }

    void run(int curPreset, int amt, const float* cL, const float* cR, bool clipIn, bool clipFx,
             const float* aL, const float* aR, bool appFx, UINT32 frames) {
        rv.setPreset(curPreset);
        rv.setAmount((clipIn && clipFx) || appFx ? (float)amt : 0.0f);
        feedL.resize(frames); feedR.resize(frames); wl.resize(frames); wr.resize(frames);
        for (UINT32 i = 0; i < frames; ++i) {
            feedL[i] = (clipIn && clipFx ? cL[i] : 0.0f) + (appFx ? aL[i] : 0.0f);
            feedR[i] = (clipIn && clipFx ? cR[i] : 0.0f) + (appFx ? aR[i] : 0.0f);
        }
        rv.processWet(feedL.data(), feedR.data(), wl.data(), wr.data(), (int)frames, (float)FS);
    }

    double decayTime() const { return rv.decayTime(); }
};

static void audioThread() {
    flushDenormals();
    DWORD mmIndex = 0;
    HANDLE mmTask = AvSetMmThreadCharacteristicsW(L"Pro Audio", &mmIndex);
    if (mmTask) AvSetMmThreadPriority(mmTask, AVRT_PRIORITY_CRITICAL);
    else SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
    const HRESULT com = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    IMMDeviceEnumerator* en = nullptr;
    if (FAILED(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
                                __uuidof(IMMDeviceEnumerator), (void**)&en)))
        en = nullptr;

    int curPreset = g.preset.load();
    int curAmount = g.amount.load();

    const auto rvBox = std::make_unique<env::EnvironmentReverb>();
    env::EnvironmentReverb& rv = *rvBox;
    rv.setPreset(curPreset);

    std::vector<float> mono, wl, wr, inter, heard, micIn;
    MicDrive drive;
    MicTone micTone;
    micTone.prepare(FS);
    iso::VoiceIsolator isolator;
    isolator.prepare(FS);
    iso::ExpanderGate presetGate;
    presetGate.prepare(FS);
    voice::VoiceTone voiceTone;
    voiceTone.prepare(FS);
    InputTap inputTap;
    voice::VoiceShaper shaper;
    const auto voicefxBox = std::make_unique<voice::VoiceFx>();
    voice::VoiceFx& voicefx = *voicefxBox;
    voice::GlitchPad glitchPad;
    std::vector<voice::SongChanger> changers(2);
    voice::SongChanger& clipChanger = changers[0];
    voice::SongChanger& appChanger = changers[1];
    bool clipChanging = false, appChanging = false, songBusWas = false;
    voice::VoiceSpeed voicePace;
    voice::ClipSpeed clipPace;
    unsigned clipCutSeen = 0;
    voicePace.prepare(FS);
    clipPace.prepare(FS);
    gBoard.prepare(FS);
    gTakeMic.prepare(FS);
    gTakeEar.prepare(FS);
    shaper.prepare(FS);
    voicefx.prepare(FS);
    glitchPad.prepare(FS, 1);
    clipChanger.prepare(FS);
    appChanger.prepare(FS);
    bool shaperWas = false;
    int64_t shaperIdle = 0;
    auto songMask = [](voice::SongChanger& ch, bool& live, bool want, std::vector<float>& l, std::vector<float>& r, int n) {
        if (!want && !live) { ch.listen(l.data(), r.data(), n); return; }
        if (!live) { ch.start(); live = true; }
        ch.set(g.pitchHz.load(), g.voiceRatio.load(), g.voiceMask.load());
        ch.hold(want && (g.glitchHold.load() || g.glitchKeyHeld.load()), g.glitchStyle.load());
        ch.process(l.data(), r.data(), n, want);
        if (ch.idle()) live = false;
    };

    const auto rooms = std::make_unique<RoomBus[]>(2);
    RoomBus& pub = rooms[0];
    RoomBus& priv = rooms[1];
    SoftClip sumCab, sumMon;
    std::vector<float> cL, cR, interMon, aL, aR;
    std::vector<float> takeL, takeR, hearL, hearR;
    int64_t songTail = 0, appTail = 0;
    bool isoWas = false, vocalWas = false;
    int isoSeen = -1, vocalSeen = -1, toneSeen = -1;
    float isoGain = 0.0f, toneGain = 0.0f;
    const auto chainBox = std::make_unique<OutChain[]>(4);
    OutChain &voiceMon = chainBox[0], &voiceCab = chainBox[1], &progMon = chainBox[2], &progCab = chainBox[3];
    OutChain* const chains[] = {&voiceMon, &voiceCab, &progMon, &progCab};
    for (OutChain* c : chains) c->prepare(FS);
    eqb::BassBoost voiceBass, progBass, appBass;
    for (eqb::BassBoost* b : {&voiceBass, &progBass, &appBass}) b->prepare(FS);
    int eqSeen = -1;
    ULONGLONG lastBlockAt = 0;
    MicSwap micSwap;

    auto forgetRooms = [&]() {
        rv.suspend();
        pub.forget();
        priv.forget();
        songTail = appTail = 0;
        isoWas = shaperWas = clipChanging = appChanging = songBusWas = false;
        voicePace.reset();
        clipPace.reset();
        g.voiceLagMs = 0;
    };

    while (!g.quit.load()) {
        if (!g.restart.exchange(false)) { Sleep(50); continue; }

        std::wstring micId, spkId;
        { std::lock_guard<std::mutex> lk(g.devMx); micId = g.micId; spkId = g.spkId; }

        const Dev cable = findCableRender(DEVICE_STATE_ACTIVE);

        IMMDevice *dMic = nullptr, *dSpk = nullptr, *dCab = nullptr;
        IAudioClient *acMic = nullptr, *acSpk = nullptr, *acCab = nullptr;
        IAudioCaptureClient* cap = nullptr;
        wasapi::RenderFeed feedCab, feedSpk;
        UINT32 bfMic = 0;
        std::wstring monOpenId;
        bool monOpenQuick = false;

        bool ok = false;
        do {
            if (micId.empty() || !en) break;
            if (FAILED(en->GetDevice(micId.c_str(), &dMic))) break;
            acMic = openCapture(dMic, bfMic);
            if (!acMic) break;
            if (FAILED(acMic->GetService(__uuidof(IAudioCaptureClient),
                                         (void**)&cap))) break;
            if (!cable.id.empty() && SUCCEEDED(en->GetDevice(cable.id.c_str(), &dCab)))
                openFeed(dCab, acCab, feedCab, false);
            const std::wstring monId = monitorRenderId(spkId);
            const bool quickMon = g.monitor.load() || g.clipHear.load();
            if (!monId.empty() && SUCCEEDED(en->GetDevice(monId.c_str(), &dSpk)) && openFeed(dSpk, acSpk, feedSpk, quickMon)) {
                monOpenId = monId;
                monOpenQuick = quickMon;
            }
            ok = true;
        } while (false);

        bool retryLater = !ok;
        if (!ok) {
            {
                std::lock_guard<std::mutex> lk(g.devMx);
                wcscpy_s(g.status, L"Can't open this microphone. Pick another one.");
            }
            g.running = false;
        } else {
            acMic->Start();
            if (acCab) acCab->Start();
            if (acSpk) acSpk->Start();
            g.cableState = feedCab.live() ? 1 : (cable.id.empty() ? 2 : 3);
            {
                std::lock_guard<std::mutex> lk(g.devMx);
                g.status[0] = 0;
            }
            g.running = true;
            g.reopenMic = false;
            gApp.resync();
            if (lastBlockAt && GetTickCount64() - lastBlockAt > ROOMS_KEEP_MS) forgetRooms();

            ULONGLONG nextMonTry = 0;
            int monFails = 0;
            std::wstring monId;
            int monKey = -1;
            UINT64 micNextPos = 0;
            int micPosRun = 0;
            while (!g.quit.load() && !g.restart.load()) {
                int key = 0;
                if (g.monitor.load()) key |= 1;
                if (g.clipHear.load()) key |= 2;
                if (gApp.chosen() && appToMonitor()) key |= 4;
                if (takeHeld.load()) key |= 8;
                if (key != monKey) {
                    monKey = key;
                    monId = key ? monitorRenderId(spkId) : std::wstring();
                    monFails = 0;
                    nextMonTry = 0;
                    if (feedSpk.live() && !monId.empty() && (monId != monOpenId || ((key & 3) != 0) != monOpenQuick)) {
                        feedSpk.release();
                        if (acSpk) { acSpk->Stop(); acSpk->Release(); acSpk = nullptr; }
                        if (dSpk) { dSpk->Release(); dSpk = nullptr; }
                    }
                }
                const bool wantMon = !monId.empty();
                const bool quickMon = (key & 3) != 0;
                if (wantMon && !feedSpk.live()) {
                    const ULONGLONG now = GetTickCount64();
                    if (now >= nextMonTry) {
                        if (dSpk) { dSpk->Release(); dSpk = nullptr; }
                        if (en) en->GetDevice(monId.c_str(), &dSpk);
                        if (dSpk && openFeed(dSpk, acSpk, feedSpk, quickMon) && acSpk) {
                            acSpk->Start();
                            monOpenId = monId;
                            monOpenQuick = quickMon;
                            monFails = 0;
                        } else {
                            nextMonTry = now + (2000ULL << std::min(monFails, 4));
                            ++monFails;
                        }
                    }
                }
                if (!feedCab.live() && !cable.id.empty() && gCableRetry.exchange(false)) {
                    if (!dCab) en->GetDevice(cable.id.c_str(), &dCab);
                    if (dCab && openFeed(dCab, acCab, feedCab, false) && acCab) acCab->Start();
                    g.cableState = feedCab.live() ? 1 : 3;
                }
                if (!wantMon && feedSpk.live()) {
                    feedSpk.release();
                    if (acSpk) { acSpk->Stop(); acSpk->Release(); acSpk = nullptr; }
                    if (dSpk) { dSpk->Release(); dSpk = nullptr; }
                }
                if (!micSwap.busy && g.reopenMic.exchange(false)) micSwap.begin(dMic);
                if (micSwap.ready()) {
                    IAudioClient* next = micSwap.collect();
                    IAudioCaptureClient* nextCap = nullptr;
                    if (next && SUCCEEDED(next->GetService(__uuidof(IAudioCaptureClient), (void**)&nextCap)) && SUCCEEDED(next->Start())) {
                        cap->Release();
                        acMic->Stop();
                        acMic->Release();
                        acMic = next;
                        cap = nextCap;
                        next->GetBufferSize(&bfMic);
                        micNextPos = 0;
                        micPosRun = 0;
                    } else {
                        if (nextCap) nextCap->Release();
                        if (next) next->Release();
                    }
                }
                curAmount = g.amount.load();
                curPreset = g.preset.load();
                UINT32 pk = 0;
                if (FAILED(cap->GetNextPacketSize(&pk))) break;
                if (pk == 0) { WaitForSingleObject(gCapEvt, 20); continue; }
                BYTE* data = nullptr;
                UINT32 got = 0;
                DWORD flags = 0;
                UINT64 devPos = 0;
                if (FAILED(cap->GetBuffer(&data, &got, &flags, &devPos, nullptr))) break;
                const bool packetSilent = (flags & AUDCLNT_BUFFERFLAGS_SILENT) != 0;
                UINT32 lost = 0;
                if (devPos == micNextPos) micPosRun = std::min(micPosRun + 1, MIC_POS_TRUST);
                else if (micPosRun >= MIC_POS_TRUST && bfMic < MOST_LOST_FRAMES && devPos > micNextPos && devPos - micNextPos <= MOST_LOST_FRAMES) lost = (UINT32)(devPos - micNextPos);
                else micPosRun = 0;
                micNextPos = devPos + got;
                if (packetSilent) micIn.assign((size_t)got * 2, 0.0f);
                else micIn.assign((const float*)data, (const float*)data + (size_t)got * 2);
                cap->ReleaseBuffer(got);
                bool feedLost = false;
                for (int pass = lost ? 0 : 1; pass < 2; ++pass) {
                    const UINT32 frames = pass == 0 ? lost : got;
                    const bool silent = pass == 0 || packetSilent;
                    const float* f = micIn.data();
                    mono.resize(frames); wl.resize(frames); wr.resize(frames);
                    inter.resize(frames * 2);
                    const float micGain = micGainFor(g.micLevel.load());
                    if (eqSeen != g.eqVersion.load()) {
                        eqSeen = g.eqVersion.load();
                        int b[eqb::EQ_BANDS];
                        for (int k = 0; k < eqb::EQ_BANDS; ++k) b[k] = g.eqDb[k].load();
                        for (OutChain* c : chains) c->eq.set(b, g.eqLevel.load());
                    }

                    const double boostDb = eqb::BassBoost::boostFor(micGain > 0.0f ? 20.0 * log10((double)micGain) : 0.0);
                    for (eqb::BassBoost* b : {&voiceBass, &progBass, &appBass}) b->set(boostDb);
                    for (UINT32 i = 0; i < frames; ++i) {
                        float v = silent ? 0.0f : 0.5f * (f[2 * i] + f[2 * i + 1]);
                        if (!(v > -4.0f && v < 4.0f)) v = 0.0f;
                        mono[i] = v * micGain;
                    }
                    const bool vocal = vocalOn();
                    if (vocal) {
                        if (vocalSeen != g.isoVersion.load()) {
                            vocalSeen = g.isoVersion.load();
                            presetGate.configure(gateShape());
                        }
                        if (!vocalWas) { presetGate.reset(); vocalWas = true; }
                        presetGate.process(mono.data(), (int)frames);
                    } else vocalWas = false;
                    if (g.isolate.load() && !vocal) {
                        if (!isoWas) { isolator.reset(); isoWas = true; isoGain = micGain; }
                        if (isoSeen != g.isoVersion.load()) {
                            isoSeen = g.isoVersion.load();
                            isolator.configure(g.isoMode.load(), g.isoLevel.load());
                        }
                        if (micGain > 0.0f && micGain != isoGain) {
                            if (isoGain > 0.0f) isolator.shiftLevel(20.0 * log10((double)micGain / isoGain));
                            isoGain = micGain;
                        }
                        if (pass == 1 && inputTap.poll()) isolator.inputHint();
                        isolator.process(mono.data(), (int)frames);
                    } else isoWas = false;
                    if (toneSeen != g.toneVersion.load()) {
                        toneSeen = g.toneVersion.load();
                        voiceTone.set(toneGains());
                    }
                    if (micGain > 0.0f && micGain != toneGain) {
                        if (toneGain > 0.0f) voiceTone.shiftLevel(20.0 * log10((double)micGain / toneGain));
                        toneGain = micGain;
                    }
                    voiceTone.process(mono.data(), (int)frames);

                    const bool voiceChange = (g.changeOn.load() & ON_VOICE) != 0;
                    voicePace.process(mono.data(), (int)frames, voiceChange ? g.voiceSpeed.load() : 100, g.voiceKeepPitch.load());
                    g.voiceLagMs = (int)(voicePace.lagSeconds() * 1000.0 + 0.5);
                    const int mi = g.voiceMask.load();
                    const bool masked = mi >= 0 && mi < voice::NUM_MASKS;
                    shaper.set(g.pitchHz.load(), g.voiceSize.load(), g.voiceRatio.load() / 1000.0);
                    shaper.anchor(masked ? voice::MASKS[mi].anchor : voice::Anchor{});
                    const bool shapeWanted = voiceChange && !shaper.neutral();
                    if (shapeWanted) shaperIdle = 0;
                    else if (shaperWas) shaperIdle += frames;
                    if (shapeWanted || (voiceChange && shaperWas && shaperIdle < (int64_t)FS)) {
                        if (!shaperWas) { shaper.reset(); shaperWas = true; }
                        shaper.process(mono.data(), (int)frames);
                        g.voiceHz = (int)(shaper.voiceHz() + 0.5);
                        g.voiceTargetHz = (int)(shaper.targetHz() + 0.5);
                    } else if (shaperWas) {
                        shaperWas = false;
                        g.voiceHz = 0;
                    }
                    if (voiceChange && masked && voice::MASKS[mi].fx != voice::FX_NONE) {
                        voicefx.configure(voice::MASKS[mi].fx, voice::MASKS[mi].a, voice::MASKS[mi].b);
                        voicefx.process(mono.data(), (int)frames);
                    } else voicefx.configure(voice::FX_NONE, 0.0, 0.0);
                    glitchPad.set(g.glitchStyle.load());
                    if (pass == 1) {
                        const bool keyDown = chordDown(g.glitchKey.load());
                        if (!keyDown) g.glitchArmed = true;
                        g.glitchKeyHeld = keyDown && g.glitchArmed.load();
                    }
                    glitchPad.hold(voiceChange && (g.glitchHold.load() || g.glitchKeyHeld.load()));
                    glitchPad.process(mono.data(), nullptr, (int)frames);

                    micTone.on = g.cardMic.load() && !g.rawMic.load();
                    for (UINT32 i = 0; i < frames; ++i) mono[i] = micTone.process(mono[i]);

                    drive.set(g.micLevel.load());
                    const float gDrive = drive.gain();
                    for (UINT32 i = 0; i < frames; ++i) mono[i] = voiceBass.run(mono[i] * gDrive);

                    const int amt = curAmount;
                    const int echoTo = g.echoOn.load();
                    const bool fxAll = !g.enabled.load() || amt <= 0;
                    const bool effectOff = fxAll || !(echoTo & ON_VOICE);
                    const bool appFxOff = fxAll || !(echoTo & ON_PROGRAM);
                    const bool clipFxOff = fxAll || !(echoTo & ON_CLIP);

                    rv.setPreset(curPreset);
                    rv.setAmount(effectOff ? 0.0f : (float)amt);
                    wl.resize(frames); wr.resize(frames);
                    rv.processWet(mono.data(), nullptr, wl.data(), wr.data(), (int)frames, (float)FS);

                    const bool wide = g.wide.load();
                    const bool eqOn = g.eqOn.load();
                    const mix::CmssParams cmssOn{ g.cmss.load(), g.cmssMode.load(), g.cmssWidth.load() / 100.0f, g.cmssDepth.load() / 100.0f };
                    heard.resize(frames * 2);
                    for (UINT32 i = 0; i < frames; ++i) {
                        const float dry = mono[i];
                        float ml = wl[i], mr = wr[i];
                        if (wide) widen(ml, mr, WIDE_ECHO);
                        float l = drive.process(dry + ml);
                        float r = drive.process(dry + mr);
                        float cl = drive.process(dry + wl[i]);
                        float cr = drive.process(dry + wr[i]);
                        voiceMon.process(l, r, eqOn, cmssOn);
                        heard[2 * i] = l;
                        heard[2 * i + 1] = r;
                        voiceCab.process(cl, cr, eqOn, cmssOn);
                        inter[2 * i] = cl;
                        inter[2 * i + 1] = cr;
                    }
                    gRec.feed(inter.data(), frames);
                    gBackVoice.feed(inter.data(), frames);

                    interMon.resize(frames * 2);
                    const bool songOn = gClip.active() || gBoard.active(), appOn = gApp.active();
                    const int64_t roomTail = (int64_t)(pub.decayTime() * FS) + FS / 2;
                    if (songOn) songTail = roomTail; else if (songTail > 0) songTail -= frames;
                    if (appOn) appTail = roomTail; else if (appTail > 0) appTail -= frames;
                    const bool songLive = songOn || songTail > 0, appLive = appOn || appTail > 0;

                    if (songLive || appLive) {
                        if (!songBusWas) {
                            clipChanger.reset();
                            appChanger.reset();
                            clipChanging = appChanging = false;
                            songBusWas = true;
                        }
                        cL.resize(frames); cR.resize(frames); aL.resize(frames); aR.resize(frames);
                        if (gClip.cuts.load() != clipCutSeen) { clipCutSeen = gClip.cuts.load(); clipPace.reset(); }
                        if (gClip.isPaused()) {
                            std::fill(cL.begin(), cL.end(), 0.0f);
                            std::fill(cR.begin(), cR.end(), 0.0f);
                        } else {
                            clipPace.render(cL.data(), cR.data(), (int)frames, clipRate(), g.clipKeepPitch.load(),
                                            [](float* l, float* r, int n) { gClip.pull(l, r, n); });
                        }
                        gBoard.mix(cL.data(), cR.data(), (int)frames);
                        if (appOn) gApp.pull(aL.data(), aR.data(), (int)frames);
                        else { std::fill(aL.begin(), aL.end(), 0.0f); std::fill(aR.begin(), aR.end(), 0.0f); }
                        const float cg = micGainFor(g.clipGain.load());
                        const int appV = g.appGain.load();
                        const bool appMuted = g.appMute.load();
                        const bool hearApp = appToMonitor(), appOnly = g.appOnly.load(), appDryMon = hearApp && gApp.parking.load() && !appMuted;
                        const float agCab = micGainFor(appV);

                        const bool toMic = g.clipToMic.load(), clipHear = g.clipHear.load(), self = g.monitor.load();
                        const bool clipMine = !toMic && clipHear, appMine = appOnly && hearApp;
                        const bool roomToCable = (toMic && songLive) || (appLive && !appOnly);
                        const bool roomToMon = (clipHear && toMic && songLive) || (hearApp && appLive && !appOnly && !appMuted);

                        for (UINT32 i = 0; i < frames; ++i) {
                            cL[i] *= cg; cR[i] *= cg;
                            aL[i] *= agCab; aR[i] *= agCab;
                            progBass.process(cL[i], cR[i]);
                            appBass.process(aL[i], aR[i]);
                        }
                        {
                            const int to = g.changeOn.load();
                            songMask(clipChanger, clipChanging, (to & ON_CLIP) != 0 && songLive, cL, cR, (int)frames);
                            songMask(appChanger, appChanging, (to & ON_PROGRAM) != 0 && appLive, aL, aR, (int)frames);
                        }
                        pub.run(curPreset, amt, cL.data(), cR.data(), toMic, !clipFxOff,
                                aL.data(), aR.data(), !appOnly && !appFxOff, frames);
                        if ((clipMine && songOn) || (appMine && appOn)) priv.tail = roomTail;
                        else if (priv.tail > 0) priv.tail -= frames;
                        const bool privLive = ((clipMine && songOn) || (appMine && appOn)) || priv.tail > 0;
                        if (privLive)
                            priv.run(curPreset, amt, cL.data(), cR.data(), clipMine, !clipFxOff,
                                     aL.data(), aR.data(), appMine && !appFxOff, frames);
                        for (UINT32 i = 0; i < frames; ++i) {
                            const float songL = cL[i], songR = cR[i];
                            const float appL = aL[i], appR = aR[i];
                            float wetL = roomToMon ? pub.wl[i] : 0.0f, wetR = roomToMon ? pub.wr[i] : 0.0f;
                            if (privLive) { wetL += priv.wl[i]; wetR += priv.wr[i]; }
                            if (wide) widen(wetL, wetR, WIDE_ECHO);
                            float l = (clipHear ? songL : 0.0f) + (appDryMon ? appL : 0.0f) + wetL;
                            float r = (clipHear ? songR : 0.0f) + (appDryMon ? appR : 0.0f) + wetR;
                            progMon.process(l, r, eqOn, cmssOn);
                            float ml = (self ? heard[2 * i] : 0.0f) + l;
                            float mr = (self ? heard[2 * i + 1] : 0.0f) + r;
                            sumMon.process(ml, mr);
                            interMon[2 * i] = ml; interMon[2 * i + 1] = mr;
                            float pl = (toMic ? songL : 0.0f) + (appOnly ? 0.0f : appL) + (roomToCable ? pub.wl[i] : 0.0f);
                            float pr = (toMic ? songR : 0.0f) + (appOnly ? 0.0f : appR) + (roomToCable ? pub.wr[i] : 0.0f);
                            progCab.process(pl, pr, eqOn, cmssOn);
                            float xl = inter[2 * i] + pl, xr = inter[2 * i + 1] + pr;
                            sumCab.process(xl, xr);
                            inter[2 * i] = xl; inter[2 * i + 1] = xr;
                        }
                    } else {
                        songBusWas = false;
                        const bool self = g.monitor.load();
                        for (UINT32 i = 0; i < frames * 2; ++i) interMon[i] = self ? heard[i] : 0.f;
                    }
                    if (gTakeMic.active() || gTakeEar.active()) {
                        takeL.assign(frames, 0.0f); takeR.assign(frames, 0.0f);
                        hearL.assign(frames, 0.0f); hearR.assign(frames, 0.0f);
                        gTakeMic.mix(takeL.data(), takeR.data(), (int)frames);
                        gTakeEar.mix(hearL.data(), hearR.data(), (int)frames);
                        for (UINT32 i = 0; i < frames; ++i) {
                            float xl = inter[2 * i] + takeL[i], xr = inter[2 * i + 1] + takeR[i];
                            float ml = interMon[2 * i] + takeL[i] + hearL[i], mr = interMon[2 * i + 1] + takeR[i] + hearR[i];
                            sumCab.process(xl, xr);
                            sumMon.process(ml, mr);
                            inter[2 * i] = xl; inter[2 * i + 1] = xr;
                            interMon[2 * i] = ml; interMon[2 * i + 1] = mr;
                        }
                    }
                    {
                        float loud = 0.0f;
                        for (UINT32 i = 0; i < frames * 2; ++i) { const float a = fabsf(inter[i]); if (a > loud) loud = a; }
                        const int q = (int)(loud * 10000.0f);
                        int seen = g.meterPeak.load();
                        while (q > seen && !g.meterPeak.compare_exchange_weak(seen, q)) {}
                    }
                    if (feedCab.live() && !feedCab.push(inter.data(), frames)) { feedLost = true; break; }
                    if (feedSpk.live() && !feedSpk.push(interMon.data(), frames)) {
                        feedSpk.release();
                        if (acSpk) { acSpk->Stop(); acSpk->Release(); acSpk = nullptr; }
                        if (dSpk) { dSpk->Release(); dSpk = nullptr; }
                        nextMonTry = GetTickCount64() + 2000;
                    }
                    lastBlockAt = GetTickCount64();
                }
                if (feedLost) break;
            }
            if (!g.quit.load() && !g.restart.load()) retryLater = true;
            g.running = false;
            g.cableState = 0;
        }
        micSwap.abandon();
        if (cap) cap->Release();
        feedSpk.release();
        feedCab.release();
        if (acMic) { acMic->Stop(); acMic->Release(); }
        if (acSpk) { acSpk->Stop(); acSpk->Release(); }
        if (acCab) { acCab->Stop(); acCab->Release(); }
        if (dMic) dMic->Release();
        if (dSpk) dSpk->Release();
        if (dCab) dCab->Release();
        if (retryLater) {
            for (int i = 0; i < 10 && !g.quit.load(); ++i) Sleep(100);
            g.restart = true;
        }
    }
    if (en) en->Release();
    if (mmTask) AvRevertMmThreadCharacteristics(mmTask);
    if (SUCCEEDED(com)) CoUninitialize();
}

#include "engine_api.h"
static void tipsColour() {}
static bool eqOnPreset() { return false; }
static void maskLabel(int, wchar_t*) {}
static std::thread* gAudioTh = nullptr; static std::thread* gCableTh = nullptr;

static std::vector<Dev> speakerDevices() {
    std::vector<Dev> out;
    for (auto& d : listDevices(eRender))
        if (!nameContains(d.name, L"CABLE") && !cableAdapter(d)) out.push_back(d);
    return out;
}
static std::vector<Dev> realMics() {
    std::vector<Dev> all = listDevices(eCapture), mics;
    for (const Dev& d : all) if (!cableCapture(d)) mics.push_back(d);
    return mics;
}
static int clampInt(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }
static std::atomic<int>& toneBase(int k) { return k == 0 ? g.toneBaseLo : (k == 1 ? g.toneBaseMid : g.toneBaseHi); }
static std::atomic<int>& toneKnob(int k) { return k == 0 ? g.toneLo : (k == 1 ? g.toneMid : g.toneHi); }
static int toneTilt(int pad) { return (pad * TONE_RANGE + (pad >= 0 ? TONE_PAD_RANGE / 2 : -TONE_PAD_RANGE / 2)) / TONE_PAD_RANGE; }
static int toneOf(int base, int tilt) { return clampInt(base + tilt, -TONE_RANGE, TONE_RANGE); }
static int toneRoom(int fromBase) { return clampInt(fromBase * TONE_PAD_RANGE / TONE_RANGE, 0, TONE_PAD_RANGE); }

struct KeyDef { const char* name; std::atomic<int>* i; std::atomic<bool>* b; };
#define KI(n) {#n, &g.n, nullptr}
#define KB(n) {#n, nullptr, &g.n}
static const KeyDef kKeys[] = {
    KI(micLevel), KI(amount), KI(preset), KB(enabled), KI(echoOn), KI(changeOn), KB(monitor), KB(rawMic), KB(cardMic), KB(cmss), KI(cmssMode), KI(cmssWidth), KI(cmssDepth), KB(wide),
    {"eq0", &g.eqDb[0], nullptr}, {"eq1", &g.eqDb[1], nullptr}, {"eq2", &g.eqDb[2], nullptr}, {"eq3", &g.eqDb[3], nullptr},
    {"eq4", &g.eqDb[4], nullptr}, {"eq5", &g.eqDb[5], nullptr}, {"eq6", &g.eqDb[6], nullptr}, KI(eqLevel), KB(eqOn), KI(eqPreset),
    KB(isolate), KI(isoMode), KI(isoLevel), KI(gateLevel), KI(gateLo), KI(gateMid), KI(gateHi), KI(gateOut),
    KB(toneOn), KI(toneLo), KI(toneMid), KI(toneHi), KI(tonePadX), KI(tonePadY), KI(toneBaseLo), KI(toneBaseMid), KI(toneBaseHi),
    KI(clipGain), KB(clipHear), KB(clipToMic), KB(appHear), KB(appOnly), KB(appMute), KI(appGain), KI(clipSpeed), KB(clipKeepPitch), KB(clipFollow),
    KI(pitchHz), KI(voiceSize), KI(voiceHz), KI(voiceMacro), KI(voiceRatio), KI(voiceMask), KI(voiceTargetHz), KI(voiceSpeed), KB(voiceKeepPitch),
    KI(voiceLagMs), KB(glitchHold), KB(glitchKeyHeld), KB(glitchArmed), KI(glitchKey), KI(glitchStyle), KI(meterPeak),
};
static const KeyDef* findKey(const char* k) { for (const KeyDef& d : kKeys) if (!strcmp(d.name, k)) return &d; return nullptr; }

static std::wstring appWanted;
static std::wstring appEndNote() {
    switch (gApp.state.load()) {
    case clip::ProgramTap::GONE:
        return gApp.name + L" closed";
    case clip::ProgramTap::TOO_OLD:
        return L"Needs Windows 10 (2004) or newer";
    case clip::ProgramTap::REFUSED: {
        wchar_t t[96];
        swprintf_s(t, 96, L"Windows blocked this app's sound (0x%08lx)", (unsigned long)gApp.refusal.load());
        return t;
    }
    }
    return L"";
}
static void healPrograms() {
    static std::atomic<bool> busy{false};
    const clip::ParkMemory memory = gApp.hijack.memory;
    if (memory.programs().empty() || busy.exchange(true)) return;
    std::vector<std::wstring> skip;
    if (gApp.chosen()) {
        std::vector<DWORD> tree;
        clip::collectTree(gApp.pid, tree);
        for (DWORD p : tree) { const std::wstring exe = clip::imagePath(p); if (!exe.empty()) skip.push_back(exe); }
    }
    std::thread([memory, skip] {
        const HRESULT com = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        clip::healParked(memory, skip);
        if (SUCCEEDED(com)) CoUninitialize();
        busy = false;
    }).detach();
}
static void fillProgramList() {
    const DWORD keep = gApp.chosen() ? gApp.pid : 0;
    uiApps = clip::listPrograms();
    if (keep && std::none_of(uiApps.begin(), uiApps.end(), [keep](const clip::Program& p) { return p.pid == keep; }))
        uiApps.insert(uiApps.begin(), clip::Program{keep, gApp.name});
}
static bool appFollowWanted() {
    if (appWanted.empty() || gApp.chosen()) return false;
    for (const clip::Program& p : clip::listPrograms())
        if (_wcsicmp(p.name.c_str(), appWanted.c_str()) == 0) { appNote.clear(); appPick(p); fillProgramList(); return true; }
    return false;
}

namespace orb {
void Start() {
    if (gAudioTh) return;
    g.quit = false; g.restart = true;
    timeBeginPeriod(1);
    gAudioTh = new std::thread(audioThread);
    gCableTh = new std::thread(provisionCable);
}
void Stop() {
    if (!gAudioTh) return;
    g.quit = true; gApp.stop(); gClip.stop(); gHearTap.stop();
    gAudioTh->join(); delete gAudioTh; gAudioTh = nullptr;
    if (gCableTh) { gCableTh->join(); delete gCableTh; gCableTh = nullptr; }
    timeEndPeriod(1);
}
bool Running() { return gAudioTh != nullptr; }
void Restart() { g.restart = true; }
void SetEnabled(bool on) { g.enabled = on; }
void SetPreset(int i) { g.preset = i; }
void SetAmount(int a) { g.amount = a; }
void SetMonitor(bool on) { g.monitor = on; }
void SetCmss(bool on) { g.cmss = on; }
void SetCmssMode(int mode) { g.cmssMode = std::clamp(mode, 0, 1); }
void SetCmssWidth(int pct) { g.cmssWidth = std::clamp(pct, -100, 100); }
void SetCmssDepth(int pct) { g.cmssDepth = std::clamp(pct, 0, 100); }
void SetWide(bool on) { g.wide = on; }
void SetEq(const int* db, int level, bool on) { for (int i = 0; i < 7; i++) g.eqDb[i] = db[i]; g.eqLevel = level; g.eqOn = on; ++g.eqVersion; }
void SetMicLevel(int v) { g.micLevel = v; }
void SetIsolate(bool on, int mode, int level) { g.isolate = on; g.isoMode = mode; g.isoLevel = level; ++g.isoVersion; }
void SetVoice(int pitchHz, int size, int ratio, int mask, int speed, bool keepPitch) {
    g.pitchHz = pitchHz; g.voiceSize = size; g.voiceRatio = ratio; g.voiceMask = mask; g.voiceSpeed = speed; g.voiceKeepPitch = keepPitch;
}
void SetDevices(const wchar_t* micId, const wchar_t* spkId) {
    { std::lock_guard<std::mutex> lk(g.devMx); g.micId = micId ? micId : L""; g.spkId = spkId ? spkId : L""; }
    g.restart = true;
}
int Meter() { return g.meterPeak.load(); }
void Status(wchar_t* out, int n) { std::lock_guard<std::mutex> lk(g.devMx); wcsncpy_s(out, n, g.cable, _TRUNCATE); }
bool StatusWarn() { return g.statusWarn.load(); }
int CableStage() { return g.cableStage.load(); }
bool NeedCable() { return g.needCable.load(); }
void InstallCable(HWND owner) { startCableDownload(owner); }
void RunCableSetup(HWND owner) { runCableSetup(owner); }
void DefaultDevices(std::wstring& mic, std::wstring& spk) { CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED); mic = defaultDeviceId(eCapture); spk = defaultDeviceId(eRender); }

int GetI(const char* key) { const KeyDef* d = findKey(key); return !d ? 0 : d->i ? d->i->load() : d->b->load() ? 1 : 0; }
void SetI(const char* key, int v) {
    const KeyDef* d = findKey(key); if (!d) return;
    if (d->i) *d->i = v; else *d->b = v != 0;
    if (!strncmp(key, "eq", 2)) ++g.eqVersion;
    else if (!strncmp(key, "gate", 4) || !strncmp(key, "iso", 3)) ++g.isoVersion;
    else if (!strncmp(key, "tone", 4)) ++g.toneVersion;
}
static std::vector<Device> conv(const std::vector<Dev>& v) { std::vector<Device> o; for (auto& d : v) o.push_back({ d.name, d.id }); return o; }
static void ensureCom() { CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED); }
std::vector<Device> Mics() { ensureCom(); return conv(realMics()); }
std::vector<Device> Speakers() { ensureCom(); return conv(speakerDevices()); }
void CurrentDevices(std::wstring& m, std::wstring& s) { std::lock_guard<std::mutex> lk(g.devMx); m = g.micId; s = g.spkId; }

int RoomCount() { return NUM_CARD_PRESETS; }
std::wstring RoomName(int i) { std::wstring w; for (const char* p = CARD_PRESETS[i].name; *p; ++p) w += (wchar_t)(unsigned char)*p; return w; }
int MaskCount() { return voice::NUM_MASKS; }
int XfiMaskCount() { return voice::NUM_XFI_MASKS; }
int OwnMaskCount() { return voice::NUM_OWN_MASKS; }
std::wstring MaskName(int i) { return voice::MASKS[i].name; }
int OwnMask(int k) { return voice::OWN_ORDER[k]; }
void PickMask(int i) {
    if (i < 0 || i >= voice::NUM_MASKS) return;
    g.voiceMask = i; g.voiceRatio = voice::MASKS[i].ratio; g.voiceSize = voice::MASKS[i].size; g.pitchHz = 0; g.voiceMacro = 0;
}
void ToneFlat();
void ResetVoice() {
    PickMask(voice::MASK_NEUTRAL); g.voiceSpeed = 100; g.voiceKeepPitch = true; g.glitchHold = false; g.toneOn = false; ToneFlat();
}
void ApplyVoiceMacro(int v) {
    v = v < -100 ? -100 : (v > 100 ? 100 : v);
    if (v > -5 && v < 5) v = 0;
    g.voiceMacro = v; g.voiceRatio = 1000; g.pitchHz = -v / 2;
    int sz = 100 + v / 4; g.voiceSize = sz < 60 ? 60 : (sz > 150 ? 150 : sz);
}
int GlitchStyleCount() { return voice::NUM_GLITCH_STYLES; }
std::wstring GlitchStyleName(int i) { return voice::GLITCH_LOOKS[i].name; }
int EqPresetCount() { return eqb::NUM_EQ_PRESETS; }
std::wstring EqPresetName(int i) { return eqb::EQ_PRESETS[i].name; }
void EqApplyPreset(int i) {
    if (i < 0 || i >= eqb::NUM_EQ_PRESETS) return;
    for (int k = 0; k < eqb::EQ_BANDS; ++k) g.eqDb[k] = eqb::EQ_PRESETS[i].band[k];
    g.eqPreset = i; g.eqVersion.fetch_add(1);
}
int IsoModeCount() { return ISO_MODES; }
std::wstring IsoModeName(int slot) { return isoModeName(ISO_ORDER[slot]); }
int IsoModeOfSlot(int slot) { return ISO_ORDER[slot]; }
void SetTonePad(int x, int y) {
    const int lo = g.toneBaseLo.load(), mid = g.toneBaseMid.load(), hi = g.toneBaseHi.load();
    x = clampInt(x, -std::max(toneRoom(TONE_RANGE - lo), toneRoom(hi + TONE_RANGE)), std::max(toneRoom(TONE_RANGE - hi), toneRoom(lo + TONE_RANGE)));
    y = clampInt(y, -toneRoom(mid + TONE_RANGE), toneRoom(TONE_RANGE - mid));
    g.tonePadX = x; g.tonePadY = y;
    g.toneLo = toneOf(lo, -toneTilt(x)); g.toneHi = toneOf(hi, toneTilt(x)); g.toneMid = toneOf(mid, toneTilt(y));
    ++g.toneVersion;
}
void ToneFlat() { for (int k = 0; k < 3; ++k) { toneBase(k) = 0; toneKnob(k) = 0; } g.tonePadX = 0; g.tonePadY = 0; ++g.toneVersion; }

std::vector<Program> Programs() { CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED); fillProgramList(); std::vector<Program> o; for (auto& p : uiApps) o.push_back({ p.name, (unsigned)p.pid }); return o; }
int ProgramSel() {
    if (!gApp.chosen()) return 0;
    for (size_t i = 0; i < uiApps.size(); ++i) if (uiApps[i].pid == gApp.pid) return (int)i + 1;
    return 0;
}
void PickProgram(int sel) {
    appNote.clear();
    if (sel <= 0) { gApp.stop(); appWanted.clear(); }
    else if (sel - 1 < (int)uiApps.size()) { appPick(uiApps[sel - 1]); appWanted = uiApps[sel - 1].name; }
}
std::wstring AppNote() {
    std::wstring note = appNote.empty() ? std::wstring(L"Pick an app to play through your mic") : appNote;
    if (gApp.active()) {
        const bool heard = gApp.heard();
        note = !heard ? L"Waiting for " + gApp.name + L" to play sound" : g.appOnly.load() ? L"Only you hear " + gApp.name : gApp.name + L" is on your mic";
    } else if (gApp.chosen()) note = L"Opening " + gApp.name + L"...";
    return note;
}
bool AppActive() { return gApp.active(); }
void AppRepark() { appRepark(); }
void AppTick() {
    const ULONGLONG now = GetTickCount64();
    if (gApp.finished()) { appNote = appEndNote(); gApp.stop(); appWanted.clear(); }
    static ULONGLONG wantAt = 0;
    if (!appWanted.empty() && !gApp.chosen() && now - wantAt >= 2000) { wantAt = now; appFollowWanted(); }
    static ULONGLONG healAt = 0;
    if (now - healAt >= 10000) { healAt = now; healPrograms(); }
}

void SetPitchSemitones(int st) {
    const int base = voice::MASKS[std::clamp(g.voiceMask.load(), 0, voice::NUM_MASKS - 1)].ratio;
    g.voiceRatio = std::clamp((int)std::lround(base * std::pow(2.0, st / 12.0)), 400, 2500);
}
std::wstring AppWanted() { return appWanted; }
void WantProgram(const std::wstring& name) {
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    appWanted = name;
    if (name.empty()) { gApp.stop(); return; }
    if (gApp.chosen() && _wcsicmp(gApp.name.c_str(), name.c_str()) == 0) return;
    gApp.stop();
    if (!appFollowWanted()) appNote = L"Waiting for " + name + L" to open";
}
}
