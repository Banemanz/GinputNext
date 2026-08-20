#pragma once
#include <string>

namespace gin {
std::string GameDirectory();
std::string ModuleDirectory();
std::string JoinPath(const std::string& a, const std::string& b);
bool FileExists(const std::string& path);
void LogOpen(const std::string& path);
void LogClose();
void Log(const char* fmt, ...);
} // namespace gin
