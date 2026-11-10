

#pragma once
#include <windows.h>
#include <string>

namespace Creative {
inline std::wstring ExeFolder() { wchar_t p[MAX_PATH]; GetModuleFileNameW(nullptr, p, MAX_PATH); std::wstring s(p); return s.substr(0, s.find_last_of(L"\\/")); }
inline bool Exists(const std::wstring& p) { return GetFileAttributesW(p.c_str()) != INVALID_FILE_ATTRIBUTES; }
inline std::wstring Root() {
    const std::wstring bundled = ExeFolder() + L"\\creative\\USB SBAudigy2 NX\\";
    return Exists(bundled) ? bundled : L"C:\\Program Files (x86)\\Creative\\USB SBAudigy2 NX\\";
}
inline std::wstring Dir(const wchar_t* sub) { return Root() + sub; }
inline std::wstring ThemeDir() {
    const std::wstring bundled = ExeFolder() + L"\\creative\\Theme";
    return Exists(bundled) ? bundled : L"C:\\Program Files (x86)\\Creative\\Theme";
}
}
