#include "Config.h"
#include <algorithm>
#include <cctype>
#include <cmath>
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


static std::string BoolText(bool v) {
    return v ? "1" : "0";
}

static ControlProfile ParseProfile(const std::string& s, ControlProfile fallback) {
    const auto v = Lower(Trim(s));
    if (v == "modern" || v == "gta4" || v == "gta5" || v == "iv" || v == "v" || v == "1" || v == "true" || v == "on") {
        return ControlProfile::Modern;
    }
    if (v == "classic" || v == "ps2" || v == "default" || v == "0" || v == "false" || v == "off") {
        return ControlProfile::Classic;
    }
    return fallback;
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
    try { const float v = std::stof(Trim(s)); return std::isfinite(v) ? v : fallback; } catch (...) { return fallback; }
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

    autoSwitchKeyboardMouse = ParseBool(Get(ini, "Input", "AutoSwitchKeyboardMouse"), autoSwitchKeyboardMouse);
    keyboardMouseCooldownFrames = std::clamp(ParseInt(Get(ini, "Input", "KeyboardMouseCooldownFrames"), keyboardMouseCooldownFrames), 0, 600);
    controllerWakeStickThreshold = std::clamp(ParseFloat(Get(ini, "Input", "ControllerWakeStickThreshold"), controllerWakeStickThreshold), 0.05f, 0.95f);
    controllerWakeTriggerThreshold = std::clamp(ParseFloat(Get(ini, "Input", "ControllerWakeTriggerThreshold"), controllerWakeTriggerThreshold), 0.05f, 0.95f);

    // Legacy boolean alias first, then Profile so a human-readable Profile key
    // wins when both are present in generated or hand-edited INIs.
    controlProfile = ParseBool(Get(ini, "Controls", "Modern"), controlProfile == ControlProfile::Modern)
        ? ControlProfile::Modern
        : ControlProfile::Classic;
    controlProfile = ParseProfile(Get(ini, "Controls", "Profile"), controlProfile);
    // Modern is now implemented through action-method hooks, not raw CPad field remaps.

    inGameConfigEnabled = ParseBool(Get(ini, "InGameConfig", "Enabled"), inGameConfigEnabled);
    inGameConfigControllerChord = ParseBool(Get(ini, "InGameConfig", "ControllerChord"), inGameConfigControllerChord);
    inGameConfigHotkeyVK = std::clamp(ParseInt(Get(ini, "InGameConfig", "HotkeyVK"), inGameConfigHotkeyVK), 0, 255);

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

bool Config::Save(const std::string& path) const {
    std::ofstream f(path, std::ios::trunc);
    if (!f) return false;

    f << "; GInputNext.ini - generated by GInputNext trainer-style in-game config\n";
    f << "; Classic = original GTA/PS2-era mapping. Modern = GTA IV/V-style action hooks.\n\n";

    f << "[Core]\n";
    f << "Enabled=" << BoolText(enabled) << "\n";
    f << "ControllerIndex=" << controllerIndex << "\n";
    f << "AllowGenericDirectInput=" << BoolText(allowGenericDirectInput) << "\n";
    f << "SuppressNativeGamepad=" << BoolText(suppressNativeGamepad) << "\n";
    f << "StartActsAsEscape=" << BoolText(startActsAsEscape) << "\n";
    f << "BackgroundInput=" << BoolText(backgroundInput) << "\n";
    f << "HotplugScanFrames=" << hotplugScanFrames << "\n";
    f << "DebugInput=" << BoolText(debugInput) << "\n\n";

    f << "[Sticks]\n";
    f << "LeftInnerDeadzone=" << leftInnerDeadzone << "\n";
    f << "RightInnerDeadzone=" << rightInnerDeadzone << "\n";
    f << "OuterDeadzone=" << outerDeadzone << "\n";
    f << "LeftSensitivity=" << leftSensitivity << "\n";
    f << "RightSensitivity=" << rightSensitivity << "\n";
    f << "InvertCameraY=" << BoolText(invertCameraY) << "\n";
    f << "InvertAimY=" << BoolText(invertAimY) << "\n\n";

    f << "[Gameplay]\n";
    f << "AutoAim=" << BoolText(autoAim) << "\n\n";

    f << "[Input]\n";
    f << "AutoSwitchKeyboardMouse=" << BoolText(autoSwitchKeyboardMouse) << "\n";
    f << "KeyboardMouseCooldownFrames=" << keyboardMouseCooldownFrames << "\n";
    f << "ControllerWakeStickThreshold=" << controllerWakeStickThreshold << "\n";
    f << "ControllerWakeTriggerThreshold=" << controllerWakeTriggerThreshold << "\n\n";

    f << "[Controls]\n";
    f << "Profile=" << (controlProfile == ControlProfile::Modern ? "Modern" : "Classic") << "\n";
    f << "Modern=" << BoolText(controlProfile == ControlProfile::Modern) << "\n\n";

    f << "[InGameConfig]\n";
    f << "Enabled=" << BoolText(inGameConfigEnabled) << "\n";
    f << "HotkeyVK=" << inGameConfigHotkeyVK << "\n";
    f << "ControllerChord=" << BoolText(inGameConfigControllerChord) << "\n\n";

    f << "[Gyro]\n";
    f << "Enabled=" << BoolText(gyroEnabled) << "\n";
    f << "Sensitivity=" << gyroSensitivity << "\n";
    f << "InvertX=" << BoolText(invertGyroX) << "\n";
    f << "InvertY=" << BoolText(invertGyroY) << "\n\n";

    f << "[Rumble]\n";
    f << "Enabled=" << BoolText(rumbleEnabled) << "\n";
    f << "MirrorGameRumble=" << BoolText(gameRumbleEnabled) << "\n";
    f << "Strength=" << rumbleStrength << "\n\n";

    f << "[GenericDirectInput]\n";
    f << "LeftX=" << generic.leftX << "\n";
    f << "LeftY=" << generic.leftY << "\n";
    f << "RightX=" << generic.rightX << "\n";
    f << "RightY=" << generic.rightY << "\n";
    f << "LeftTriggerAxis=" << generic.leftTriggerAxis << "\n";
    f << "RightTriggerAxis=" << generic.rightTriggerAxis << "\n";
    f << "LeftTriggerButton=" << generic.leftTriggerButton << "\n";
    f << "RightTriggerButton=" << generic.rightTriggerButton << "\n";
    f << "A=" << generic.a << "\n";
    f << "B=" << generic.b << "\n";
    f << "X=" << generic.x << "\n";
    f << "Y=" << generic.y << "\n";
    f << "LB=" << generic.lb << "\n";
    f << "RB=" << generic.rb << "\n";
    f << "Back=" << generic.back << "\n";
    f << "Start=" << generic.start << "\n";
    f << "L3=" << generic.l3 << "\n";
    f << "R3=" << generic.r3 << "\n";
    f << "Guide=" << generic.guide << "\n";
    f << "Misc1=" << generic.misc1 << "\n";
    f << "Hat=" << generic.hat << "\n";
    f << "CenteredTriggerAxes=" << BoolText(generic.centeredTriggerAxes) << "\n";

    f.flush();
    return f.good();
}

} // namespace gin
