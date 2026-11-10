<p align="center">
  <img src="assets/Official/Logo_EAX_Console.png" alt="EAX Console" width="320"/>
</p>

<h1 align="center">EAX Console</h1>

<p align="center">
  Creative SB Live! 24-bit External EAX Console rebuilt for Windows 10/11.
</p>

<p align="center">
  <img src="https://img.shields.io/badge/platform-Windows%2010%2F11-blue?style=flat-square"/>
  <img src="https://img.shields.io/badge/language-C%2B%2B17-brightgreen?style=flat-square"/>
  <img src="https://img.shields.io/badge/compiler-MinGW--w64-orange?style=flat-square"/>
</p>

---

## What is this

The SB Live! 24-bit External EAX Console rebuilt from scratch in C++17 using Win32 and GDI+. Every pixel, tab, and control matches the original — running on a modern WASAPI audio engine instead of Creative's drivers.

The result is three standalone executables that work on any modern Windows PC without installing Creative drivers:

| Executable | Description |
|------------|-------------|
| `EAX.exe` | The main EAX Console — all 5 tabs, voice effects, CMSS, karaoke, cleanup, time scale |
| `CTEQ.exe` | Graphic Equalizer — 10-band EQ with Creative skin |
| `SurMixer.exe` | Surround Mixer — multi-channel volume control |

---

## Features

### EAX Console (`EAX.exe`)

**Effects tab**
- 15 Creative EAX reverb rooms (Bathroom, Stone Room, Auditorium, … Haunted Cavern)
- Adjustable Effects Amount from 0 to 100
- Echo routing: your voice, sound clips, program audio — any combination

**CMSS tab**
- CMSS 3D: ITD head delay + low-pass cross-talk (classic Creative 3D upmix)
- Stereo Surround: M-S decomposition with adjustable width (−100 % to +100 %)
- Smooth mode and width transitions — no clicks or zipper noise

**Karaoke tab**
- 42 voice masks (18 X-Fi + 24 Orbital)
- Pitch shift (Hz), size (%), speed (%), voice changer
- Glitch / hold-to-repeat with 4 styles
- Vocal tone pad (deeper/sharper, more/less middle)

**Clean Up tab**
- Noise isolation with 5 modes: Vocal gate, Gate, Clean, Studio, Broadcast
- Echo suppression: voice, clips, program
- Mic level, raw mic mode, card mic mode

**Time Scale tab**
- Time-stretch the audio without changing pitch

**Audio Configuration window**
- Device management: pick microphone and speakers independently, enable/disable each
- Named setups (Save as / Load / Delete)
- Mic level, noise isolation strength, echo assignment
- Voice fine-tune: pitch, size, speed, all-effects-off Default button
- Program on the mic: route any app's sound into your microphone
- System tray: closing parks EAX in the tray; audio keeps running
- VB-Audio Virtual Cable installer

---

## Requirements

- Windows 10 or 11, 64-bit
- A microphone
- VB-Audio Virtual Cable (free — EAX.exe offers to download and set it up)
- Creative SB Live! 24-bit External drivers installed **or** use the `publish\` build (which bundles the theme files)

---

## Running the publish build

Download a release ZIP, extract, and run `EAX.exe`. Everything is self-contained — no drivers, no DLL dependencies, no installer.

Keep the folders (`assets`, `assets_original`, `eq_assets`, `mix_assets`, `creative`) next to the executables.

Settings and saved setups are stored in `%APPDATA%\CreativeEAXClone`.

---

## Building from source

Requires MSYS2 mingw64 (`g++`, `windres`) at `C:\msys64\mingw64\bin`.

```bat
set GXX=C:\msys64\mingw64\bin\g++.exe
set RC=C:\msys64\mingw64\bin\windres.exe

%RC% -I res res\eax_res.rc -O coff -o res\eax_res.o
%GXX% -O2 -std=c++17 -c engine\OctopusEAX\src\engine_core.cpp -o res\engine_core.o -Iengine\OctopusEAX\shim -DNOMINMAX -D_CRT_SECURE_NO_WARNINGS -w
%GXX% -O2 -std=c++17 -municode -mwindows -static -static-libgcc -static-libstdc++ ^
    src\eax_console.cpp res\eax_res.o res\engine_core.o -o EAX.exe ^
    -lwinmm -lcomdlg32 -lcomctl32 -lgdiplus -lgdi32 -luser32 -lole32 ^
    -lmsimg32 -ladvapi32 -luuid -lavrt -lmf -lmfplat -lmfreadwrite -lmfuuid ^
    -lurlmon -lwininet -luxtheme -ldwmapi -lshlwapi -lshell32 -lcrypt32 -lwintrust -loleaut32
```

---

## Project structure

```
EAX UI/
├── src/
│   ├── eax_console.cpp         main window + all 5 tabs
│   ├── config_window.h         Audio Configuration dialog
│   ├── settings_store.h        settings.ini persistence + named setups
│   ├── shared_state.h          shared UI state
│   ├── media_bar.h             media player bar
│   ├── eq_console.cpp          CTEQ Graphic Equalizer
│   ├── mixer_console.cpp       Surround Mixer
│   └── portable_paths.h        resolves Creative theme file paths
├── engine/OctopusEAX/
│   ├── src/
│   │   ├── engine_core.cpp     audio engine entry point
│   │   ├── engine_api.h        public API used by the UI
│   │   ├── mixing/             CMSS 3D, Stereo Surround, EQ chain, soft clip
│   │   ├── reverb/             EAX reverb rooms
│   │   ├── voice/              masks, pitch, speed, glitch
│   │   ├── isolation/          noise isolation
│   │   ├── equalizer/          graphic EQ
│   │   └── playback/           sound clips, program tap, recorder
│   └── shim/                   WASAPI compatibility header
├── assets/                     UI sprites and icons
├── assets_original/            fallback assets
├── eq_assets/                  CTEQ assets
├── mix_assets/                 Surround Mixer assets
└── res/                        .rc and .ico files
```

---

## How it works

The EAX Console UI reads the original Creative sprite sheets (`assets/`) and reconstructs every control — title bar, tab strip, sliders, checkboxes, combo boxes — using GDI+ owner-draw. No MFC, no Creative DLLs, no COM.

The audio side uses **Orbital**, a pure C++ WASAPI engine that runs the reverb rooms, voice masks, noise isolation, equalizer and all mixing entirely in software.

VB-Audio Virtual Cable is the bridge: EAX Console writes your processed voice to a virtual cable, and Discord or your game reads from that same cable as its microphone input.

---

## Thanks

**[@TWODEMONBROTHERS](https://github.com/TWODEMONBROTHERS/)** — built the full reverb system and the equalizer .
**[@ALMASONYH](https://github.com/ALMASONYH/)** — the Orbital engine this runs on.
