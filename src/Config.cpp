#include "Config.h"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <string>
#include <unordered_map>

namespace gin {
namespace {

static std::string Trim(std::string s) {
    auto notSpace = [](unsigned char c) { return !std::isspace(c); };
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), notSpace));
    s.erase(std::find_if(s.rbegin(), s.rend(), notSpace).base(), s.end());
    return s;
}

static std::string Lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

static bool ParseBool(const std::string& s, bool fallback) {
    const auto v = Lower(Trim(s));
    if (v == "1" || v == "true" || v == "yes" || v == "on") return true;
    if (v == "0" || v == "false" || v == "no" || v == "off") return false;
    return fallback;
}

static int ParseInt(const std::string& s, int fallback) {
    try { return std::stoi(Trim(s)); } catch (...) { return fallback; }
}

static float ParseFloat(const std::string& s, float fallback) {
    try { return std::stof(Trim(s)); } catch (...) { return fallback; }
}

using Ini = std::unordered_map<std::string, std::string>;

static Ini ReadIni(const std::string& path) {
    Ini out;
    std::ifstream f(path);
    if (!f) return out;

    std::string section;
    std::string line;
    while (std::getline(f, line)) {
        line = Trim(line);
        if (line.empty() || line[0] == ';' || line[0] == '#') continue;
        if (line.front() == '[' && line.back() == ']') {
            section = Lower(Trim(line.substr(1, line.size() - 2)));
            continue;
        }
        const auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        auto key = Lower(Trim(line.substr(0, eq)));
        auto value = Trim(line.substr(eq + 1));
        out[section + "." + key] = value;
    }
    return out;
}

static std::string Get(const Ini& ini, const char* section, const char* key, const char* fallback = "") {
    const std::string k = Lower(section) + "." + Lower(key);
    auto it = ini.find(k);
    return it == ini.end() ? std::string(fallback) : it->second;
}

} // namespace

bool Config::Load(const std::string& path) {
    const Ini ini = ReadIni(path);
    if (ini.empty()) return false;

    enabled = ParseBool(Get(ini, "Core", "Enabled"), enabled);
    controllerIndex = std::max(0, ParseInt(Get(ini, "Core", "ControllerIndex"), controllerIndex));
    allowGenericDirectInput = ParseBool(Get(ini, "Core", "AllowGenericDirectInput"), allowGenericDirectInput);
    suppressNativeGamepad = ParseBool(Get(ini, "Core", "SuppressNativeGamepad"), suppressNativeGamepad);
    startActsAsEscape = ParseBool(Get(ini, "Core", "StartActsAsEscape"), startActsAsEscape);
    backgroundInput = ParseBool(Get(ini, "Core", "BackgroundInput"), backgroundInput);
    hotplugScanFrames = std::clamp(ParseInt(Get(ini, "Core", "HotplugScanFrames"), hotplugScanFrames), 1, 600);
    debugInput = ParseBool(Get(ini, "Core", "DebugInput"), debugInput);

    leftInnerDeadzone = std::clamp(ParseFloat(Get(ini, "Sticks", "LeftInnerDeadzone"), leftInnerDeadzone), 0.0f, 0.95f);
    rightInnerDeadzone = std::clamp(ParseFloat(Get(ini, "Sticks", "RightInnerDeadzone"), rightInnerDeadzone), 0.0f, 0.95f);
    outerDeadzone = std::clamp(ParseFloat(Get(ini, "Sticks", "OuterDeadzone"), outerDeadzone), 0.0f, 0.25f);
    leftSensitivity = std::clamp(ParseFloat(Get(ini, "Sticks", "LeftSensitivity"), leftSensitivity), 0.1f, 3.0f);
    rightSensitivity = std::clamp(ParseFloat(Get(ini, "Sticks", "RightSensitivity"), rightSensitivity), 0.1f, 3.0f);
    // Legacy v10 alias first, then the clearer v11 camera-specific key.
    invertCameraY = ParseBool(Get(ini, "Sticks", "InvertRightY"), invertCameraY);
    invertCameraY = ParseBool(Get(ini, "Sticks", "InvertCameraY"), invertCameraY);
    invertAimY = ParseBool(Get(ini, "Sticks", "InvertAimY"), invertAimY);

    autoAim = ParseBool(Get(ini, "Gameplay", "AutoAim"), autoAim);

    gyroEnabled = ParseBool(Get(ini, "Gyro", "Enabled"), gyroEnabled);
    gyroSensitivity = std::clamp(ParseFloat(Get(ini, "Gyro", "Sensitivity"), gyroSensitivity), 0.0f, 5.0f);
    invertGyroX = ParseBool(Get(ini, "Gyro", "InvertX"), invertGyroX);
    invertGyroY = ParseBool(Get(ini, "Gyro", "InvertY"), invertGyroY);

    rumbleEnabled = ParseBool(Get(ini, "Rumble", "Enabled"), rumbleEnabled);
    gameRumbleEnabled = ParseBool(Get(ini, "Rumble", "MirrorGameRumble"), gameRumbleEnabled);
    rumbleStrength = std::clamp(ParseFloat(Get(ini, "Rumble", "Strength"), rumbleStrength), 0.0f, 1.0f);

    generic.leftX = ParseInt(Get(ini, "GenericDirectInput", "LeftX"), generic.leftX);
    generic.leftY = ParseInt(Get(ini, "GenericDirectInput", "LeftY"), generic.leftY);
    generic.rightX = ParseInt(Get(ini, "GenericDirectInput", "RightX"), generic.rightX);
    generic.rightY = ParseInt(Get(ini, "GenericDirectInput", "RightY"), generic.rightY);
    generic.leftTriggerAxis = ParseInt(Get(ini, "GenericDirectInput", "LeftTriggerAxis"), generic.leftTriggerAxis);
    generic.rightTriggerAxis = ParseInt(Get(ini, "GenericDirectInput", "RightTriggerAxis"), generic.rightTriggerAxis);
    generic.leftTriggerButton = ParseInt(Get(ini, "GenericDirectInput", "LeftTriggerButton"), generic.leftTriggerButton);
    generic.rightTriggerButton = ParseInt(Get(ini, "GenericDirectInput", "RightTriggerButton"), generic.rightTriggerButton);

    generic.a = ParseInt(Get(ini, "GenericDirectInput", "A"), generic.a);
    generic.b = ParseInt(Get(ini, "GenericDirectInput", "B"), generic.b);
    generic.x = ParseInt(Get(ini, "GenericDirectInput", "X"), generic.x);
    generic.y = ParseInt(Get(ini, "GenericDirectInput", "Y"), generic.y);
    generic.lb = ParseInt(Get(ini, "GenericDirectInput", "LB"), generic.lb);
    generic.rb = ParseInt(Get(ini, "GenericDirectInput", "RB"), generic.rb);
    generic.back = ParseInt(Get(ini, "GenericDirectInput", "Back"), generic.back);
    generic.start = ParseInt(Get(ini, "GenericDirectInput", "Start"), generic.start);
    generic.l3 = ParseInt(Get(ini, "GenericDirectInput", "L3"), generic.l3);
    generic.r3 = ParseInt(Get(ini, "GenericDirectInput", "R3"), generic.r3);
    generic.guide = ParseInt(Get(ini, "GenericDirectInput", "Guide"), generic.guide);
    generic.misc1 = ParseInt(Get(ini, "GenericDirectInput", "Misc1"), generic.misc1);
    generic.hat = ParseInt(Get(ini, "GenericDirectInput", "Hat"), generic.hat);
    generic.centeredTriggerAxes = ParseBool(Get(ini, "GenericDirectInput", "CenteredTriggerAxes"), generic.centeredTriggerAxes);

    return true;
}

} // namespace gin
