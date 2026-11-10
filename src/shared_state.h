

#pragma once
#include <windows.h>
#include <cstdint>

namespace shared {

struct State {
    volatile LONG seq;
    int32_t eqSet, eqOn;
    double eqBand[7];
    double eqLevel;
    int32_t mixSet, mixMuted;
    int32_t masterPct, volumePct;
};

inline State* Open() {
    static State* p = nullptr;
    if (p) return p;
    HANDLE m = CreateFileMappingW(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, sizeof(State), L"Local\\CreativeEAXCloneState");
    if (!m) return nullptr;
    p = (State*)MapViewOfFile(m, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(State));
    return p;
}
inline void Bump(State* s) { if (s) InterlockedIncrement(&s->seq); }

}
