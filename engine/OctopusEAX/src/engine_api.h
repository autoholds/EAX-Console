
#pragma once
#include <windows.h>
#include <string>
#include <vector>
namespace orb {

void Start(); void Stop(); bool Running(); void Restart();

void SetEnabled(bool on); void SetPreset(int i); void SetAmount(int a); void SetMonitor(bool on);
void SetCmss(bool on); void SetCmssMode(int mode); void SetCmssWidth(int pct); void SetCmssDepth(int pct); void SetWide(bool on);
void SetEq(const int* db7, int level, bool on);
void SetMicLevel(int v);
void SetIsolate(bool on, int mode, int level);
void SetVoice(int pitchHz, int size, int ratio, int mask, int speed, bool keepPitch);
void SetDevices(const wchar_t* micId, const wchar_t* spkId);

int Meter(); void Status(wchar_t* out, int n); bool StatusWarn(); int CableStage(); bool NeedCable();
void InstallCable(HWND owner); void RunCableSetup(HWND owner);
void DefaultDevices(std::wstring& mic, std::wstring& spk);

int  GetI(const char* key);
void SetI(const char* key, int v);

struct Device { std::wstring name, id; };
std::vector<Device> Mics();
std::vector<Device> Speakers();
void CurrentDevices(std::wstring& micId, std::wstring& spkId);

int RoomCount(); std::wstring RoomName(int i);
int MaskCount(); int XfiMaskCount(); int OwnMaskCount(); std::wstring MaskName(int i); int OwnMask(int k);
void PickMask(int i);
void ApplyVoiceMacro(int v);
int GlitchStyleCount(); std::wstring GlitchStyleName(int i);
int EqPresetCount(); std::wstring EqPresetName(int i); void EqApplyPreset(int i);
int IsoModeCount(); std::wstring IsoModeName(int slot); int IsoModeOfSlot(int slot);
void SetTonePad(int x, int y); void ToneFlat(); void ResetVoice();

struct Program { std::wstring name; unsigned pid; };
std::vector<Program> Programs();
int  ProgramSel();
void PickProgram(int sel);
std::wstring AppNote();
bool AppActive();
void AppRepark();
void AppTick();

void SetPitchSemitones(int st);
std::wstring AppWanted();
void WantProgram(const std::wstring& name);
}
