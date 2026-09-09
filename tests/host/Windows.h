#pragma once
#include <array>
using DWORD=unsigned long; using HWND=void*;
inline std::array<bool,256> testKeys{}; inline DWORD testPid=1;
inline short GetAsyncKeyState(int k){return testKeys[k]?static_cast<short>(0x8000):0;}
inline HWND GetForegroundWindow(){return nullptr;}
inline DWORD GetWindowThreadProcessId(HWND,DWORD*p){*p=testPid;return 1;}
inline DWORD GetCurrentProcessId(){return 1;}
