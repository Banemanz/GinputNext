#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include "Log.h"
#include <cstdarg>
#include <cstdio>
#include <fstream>
#include <mutex>

namespace gin {
namespace {
std::ofstream g_log;
std::mutex g_logMutex;

std::string DirectoryFromModule(HMODULE module) {
    char path[MAX_PATH]{};
    const DWORD n = GetModuleFileNameA(module, path, MAX_PATH);
    if (!n || n >= MAX_PATH) return ".";
    std::string s(path, n);
    const auto pos = s.find_last_of("\\/");
    return pos == std::string::npos ? std::string(".") : s.substr(0, pos);
}
}

std::string GameDirectory() {
    return DirectoryFromModule(nullptr);
}

std::string ModuleDirectory() {
    HMODULE self = nullptr;
    if (!GetModuleHandleExA(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
            GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCSTR>(&ModuleDirectory),
            &self)) {
        return GameDirectory();
    }
    return DirectoryFromModule(self);
}

std::string JoinPath(const std::string& a, const std::string& b) {
    if (a.empty()) return b;
    const char last = a.back();
    if (last == '\\' || last == '/') return a + b;
    return a + "\\" + b;
}

bool FileExists(const std::string& path) {
    const DWORD a = GetFileAttributesA(path.c_str());
    return a != INVALID_FILE_ATTRIBUTES && (a & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

void LogOpen(const std::string& path) {
    std::lock_guard<std::mutex> lock(g_logMutex);
    if (g_log.is_open()) g_log.close();
    g_log.open(path, std::ios::out | std::ios::trunc);
}

void LogClose() {
    std::lock_guard<std::mutex> lock(g_logMutex);
    if (g_log.is_open()) {
        g_log.flush();
        g_log.close();
    }
}

void Log(const char* fmt, ...) {
    char buf[2048]{};
    va_list ap;
    va_start(ap, fmt);
    vsnprintf_s(buf, sizeof(buf), _TRUNCATE, fmt, ap);
    va_end(ap);

    std::lock_guard<std::mutex> lock(g_logMutex);
    if (g_log.is_open()) {
        g_log << buf << "\n";
        g_log.flush();
    }
}

} // namespace gin
