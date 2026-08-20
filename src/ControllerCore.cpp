#include "ControllerCore.h"
#include "SDL2Dyn.h"
#include "Log.h"
#include <algorithm>
#include <cmath>
#include <cctype>
#include <fstream>
#include <string>

namespace gin {
namespace {

namespace sdl = sdl2dyn;

static bool Button(SDL_GameController* c, SDL_GameControllerButton b) {
    return c && sdl::GameControllerGetButton(c, b) != 0;
}

static bool JoyButton(SDL_Joystick* j, int index) {
    return j && index >= 0 && index < sdl::JoystickNumButtons(j)
        && sdl::JoystickGetButton(j, index) != 0;
}

static Sint16 JoyAxis(SDL_Joystick* j, int index) {
    return (j && index >= 0 && index < sdl::JoystickNumAxes(j))
        ? sdl::JoystickGetAxis(j, index)
        : 0;
}


static std::string LowerAscii(const std::string& text) {
    std::string out = text;
    std::transform(out.begin(), out.end(), out.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return out;
}

static bool ContainsNoCase(const std::string& haystack, const char* needle) {
    const auto h = LowerAscii(haystack);
    std::string n = needle ? needle : "";
    std::transform(n.begin(), n.end(), n.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return h.find(n) != std::string::npos;
}

static float TriggerButton(SDL_Joystick* j, int index) {
    if (!j || index < 0 || index >= sdl::JoystickNumButtons(j)) return 0.0f;
    return sdl::JoystickGetButton(j, index) ? 1.0f : 0.0f;
}

static float TriggerAxis(SDL_Joystick* j, int index, bool centered) {
    if (!j || index < 0 || index >= sdl::JoystickNumAxes(j)) return 0.0f;
    const Sint16 value = sdl::JoystickGetAxis(j, index);
    if (centered) {
        return std::clamp((static_cast<float>(value) + 32768.0f) / 65535.0f, 0.0f, 1.0f);
    }
    return std::clamp(static_cast<float>(std::max<Sint16>(0, value)) / 32767.0f, 0.0f, 1.0f);
}

static GIN_ControllerFamily FamilyFromFallbackIdentity(
    Uint16 vendor,
    const std::string& deviceName) {

    // Common USB vendor IDs.
    if (vendor == 0x054C) return GIN_FAMILY_PLAYSTATION; // Sony
    if (vendor == 0x045E) return GIN_FAMILY_XBOX;        // Microsoft
    if (vendor == 0x057E) return GIN_FAMILY_NINTENDO;   // Nintendo

    const auto n = LowerAscii(deviceName);

    if (n.find("dualshock") != std::string::npos ||
        n.find("dualsense") != std::string::npos ||
        n.find("playstation") != std::string::npos ||
        n.find("ps3") != std::string::npos ||
        n.find("ps4") != std::string::npos ||
        n.find("ps5") != std::string::npos) {
        return GIN_FAMILY_PLAYSTATION;
    }

    if (n.find("xbox") != std::string::npos ||
        n.find("xinput") != std::string::npos) {
        return GIN_FAMILY_XBOX;
    }

    if (n.find("nintendo") != std::string::npos ||
        n.find("switch") != std::string::npos ||
        n.find("joy-con") != std::string::npos ||
        n.find("joycon") != std::string::npos) {
        return GIN_FAMILY_NINTENDO;
    }

    return GIN_FAMILY_GENERIC;
}

static GIN_ControllerFamily FamilyFromType(SDL_GameControllerType t) {
    switch (t) {
    case SDL_CONTROLLER_TYPE_XBOX360:
    case SDL_CONTROLLER_TYPE_XBOXONE:
        return GIN_FAMILY_XBOX;
    case SDL_CONTROLLER_TYPE_PS3:
    case SDL_CONTROLLER_TYPE_PS4:
    case SDL_CONTROLLER_TYPE_PS5:
        return GIN_FAMILY_PLAYSTATION;
    case SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_PRO:
    case SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_JOYCON_LEFT:
    case SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_JOYCON_RIGHT:
    case SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_JOYCON_PAIR:
        return GIN_FAMILY_NINTENDO;
    default:
        return GIN_FAMILY_GENERIC;
    }
}

} // namespace

int ControllerCore::LoadMappingFile(const std::string& path) {
    std::ifstream f(path);
    if (!f) return -1;

    int accepted = 0;
    std::string line;
    while (std::getline(f, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty() || line[0] == '#') continue;
        if (sdl::GameControllerAddMapping(line.c_str()) >= 0) ++accepted;
    }
    return accepted;
}

bool ControllerCore::Init(const Config& config, const std::string& moduleDir, const std::string& gameDir) {
    if (initialized_) return true;
    config_ = config;
    moduleDir_ = moduleDir;
    gameDir_ = gameDir;

    if (!sdl::Load(moduleDir_, gameDir_)) {
        Log("SDL2 dynamic backend load failed.");
        return false;
    }

    SDL_version runtimeVersion{};
    sdl::GetVersion(&runtimeVersion);
    Log("SDL2 runtime accepted: %u.%u.%u; GameControllerGetType=%s",
        runtimeVersion.major,
        runtimeVersion.minor,
        runtimeVersion.patch,
        sdl::HasGameControllerGetType() ? "yes" : "no (fallback family detection)");

    // We're a DLL/ASI and don't use SDL2main.
    sdl::SetMainReady();

    sdl::SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, config_.backgroundInput ? "1" : "0");
    sdl::SetHint("SDL_JOYSTICK_HIDAPI", "1");
    sdl::SetHint("SDL_JOYSTICK_HIDAPI_PS3", "1");
    sdl::SetHint("SDL_JOYSTICK_HIDAPI_PS4", "1");
    sdl::SetHint("SDL_JOYSTICK_HIDAPI_PS5", "1");
    sdl::SetHint("SDL_JOYSTICK_HIDAPI_XBOX", "1");
    sdl::SetHint("SDL_JOYSTICK_HIDAPI_SWITCH", "1");

    if (sdl::InitSubSystem(SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER | SDL_INIT_HAPTIC) != 0) {
        Log("SDL controller init failed: %s", sdl::GetError());
        sdl::Unload();
        return false;
    }

    if (config_.gyroEnabled && sdl::InitSubSystem(SDL_INIT_SENSOR) != 0) {
        Log("SDL sensor init failed (gyro disabled): %s", sdl::GetError());
        config_.gyroEnabled = false;
    }

    // Prefer data beside the ASI so modloader/scripts installs are self-contained.
    const auto moduleDb = JoinPath(moduleDir_, "GInputNext.gamecontrollerdb.txt");
    const auto gameDb = JoinPath(gameDir_, "GInputNext.gamecontrollerdb.txt");

    int dbCount = LoadMappingFile(moduleDb);
    if (dbCount >= 0) {
        Log("Loaded %d external controller mappings from \"%s\"", dbCount, moduleDb.c_str());
    } else if (moduleDb != gameDb) {
        dbCount = LoadMappingFile(gameDb);
        if (dbCount >= 0) {
            Log("Loaded %d external controller mappings from \"%s\"", dbCount, gameDb.c_str());
        }
    }

    InstallBuiltInMappings();

    initialized_ = true;
    scanCountdown_ = 0;
    ScanAndOpen();
    return true;
}

void ControllerCore::Shutdown() {
    if (!initialized_) {
        sdl::Unload();
        return;
    }

    CloseDevice();
    sdl::QuitSubSystem(SDL_INIT_SENSOR);
    sdl::QuitSubSystem(SDL_INIT_HAPTIC | SDL_INIT_GAMECONTROLLER | SDL_INIT_JOYSTICK);
    initialized_ = false;
    state_ = {};
    sdl::Unload();
}

void ControllerCore::CloseDevice() {
    if (haptic_) {
        sdl::HapticRumbleStop(haptic_);
        sdl::HapticClose(haptic_);
        haptic_ = nullptr;
    }
    if (controller_) {
        sdl::GameControllerClose(controller_);
        controller_ = nullptr;
    }
    if (joystick_) {
        sdl::JoystickClose(joystick_);
        joystick_ = nullptr;
    }
    if (state_.connected) {
        Log("Controller disconnected: %s", deviceName_.c_str());
    }
    instanceId_ = -1;
    usingGeneric_ = false;
    gyroActive_ = false;
    deviceName_.clear();
    state_ = {};
}

bool ControllerCore::OpenGameController(int joystickIndex) {
    SDL_GameController* c = sdl::GameControllerOpen(joystickIndex);
    if (!c) return false;

    controller_ = c;
    joystick_ = nullptr;
    usingGeneric_ = false;

    SDL_Joystick* joy = sdl::GameControllerGetJoystick(controller_);
    instanceId_ = joy ? sdl::JoystickInstanceID(joy) : -1;

    const char* name = sdl::GameControllerName(controller_);
    deviceName_ = name ? name : "SDL GameController";

    if (config_.gyroEnabled && sdl::GameControllerHasSensor(controller_, SDL_SENSOR_GYRO)) {
        gyroActive_ =
            sdl::GameControllerSetSensorEnabled(controller_, SDL_SENSOR_GYRO, SDL_TRUE) == 0;
    }

    state_.connected = true;
    RefreshFamily();

    const auto type = sdl::GameControllerGetType(controller_);
    const auto vendor = joy ? sdl::JoystickGetVendor(joy) : 0;
    const auto product = joy ? sdl::JoystickGetProduct(joy) : 0;

    Log("Controller connected: name=\"%s\" type=%d family=%d vid=%04X pid=%04X gyro=%d",
        deviceName_.c_str(), static_cast<int>(type), static_cast<int>(state_.family),
        vendor, product, gyroActive_ ? 1 : 0);
    return true;
}

bool ControllerCore::OpenGenericJoystick(int joystickIndex) {
    SDL_Joystick* j = sdl::JoystickOpen(joystickIndex);
    if (!j) return false;

    joystick_ = j;
    controller_ = nullptr;
    usingGeneric_ = true;
    instanceId_ = sdl::JoystickInstanceID(joystick_);

    const char* name = sdl::JoystickName(joystick_);
    deviceName_ = name ? name : "Generic DirectInput joystick";

    if (config_.rumbleEnabled && sdl::JoystickIsHaptic(joystick_)) {
        haptic_ = sdl::HapticOpenFromJoystick(joystick_);
        if (haptic_ && sdl::HapticRumbleInit(haptic_) != 0) {
            sdl::HapticClose(haptic_);
            haptic_ = nullptr;
        }
    }

    state_.connected = true;
    state_.family = GIN_FAMILY_GENERIC;

    const Uint16 vendor = sdl::JoystickGetVendor(joystick_);
    const Uint16 product = sdl::JoystickGetProduct(joystick_);
    Log("Generic joystick connected: name=\"%s\" vid=%04X pid=%04X axes=%d buttons=%d hats=%d haptic=%d",
        deviceName_.c_str(), vendor, product,
        sdl::JoystickNumAxes(joystick_),
        sdl::JoystickNumButtons(joystick_),
        sdl::JoystickNumHats(joystick_),
        haptic_ ? 1 : 0);
    return true;
}

bool ControllerCore::TryInstallGC201Mapping(int joystickIndex) {
    if (sdl::IsGameController(joystickIndex)) return false;

    SDL_Joystick* probe = sdl::JoystickOpen(joystickIndex);
    if (!probe) return false;

    const char* rawName = sdl::JoystickName(probe);
    const std::string name = rawName ? rawName : "";
    const int axes = sdl::JoystickNumAxes(probe);
    const int buttons = sdl::JoystickNumButtons(probe);
    const int hats = sdl::JoystickNumHats(probe);
    const Uint16 vendor = sdl::JoystickGetVendor(probe);
    const Uint16 product = sdl::JoystickGetProduct(probe);

    char guidText[64]{};
    sdl::JoystickGetGUIDString(sdl::JoystickGetGUID(probe), guidText, sizeof(guidText));

    Log("raw joystick probe: index=%d name=\"%s\" guid=%s vid=%04X pid=%04X axes=%d buttons=%d hats=%d",
        joystickIndex, name.c_str(), guidText, vendor, product, axes, buttons, hats);

    const bool gc201ClassicDInput =
        ContainsNoCase(name, "gc201") && axes == 4 && buttons >= 12 && hats >= 1;

    if (!gc201ClassicDInput) {
        sdl::JoystickClose(probe);
        return false;
    }

    std::string mapping;
    mapping.reserve(512);
    mapping += guidText;
    mapping += ",GC201 Controller1.00 (GInputNext auto),";
    mapping += "a:b1,b:b2,x:b0,y:b3,";
    mapping += "back:b8,start:b9,";
    if (buttons >= 13) mapping += "guide:b12,";
    mapping += "leftshoulder:b4,rightshoulder:b5,";
    mapping += "lefttrigger:b6,righttrigger:b7,";
    mapping += "leftstick:b10,rightstick:b11,";
    mapping += "dpup:h0.1,dpright:h0.2,dpdown:h0.4,dpleft:h0.8,";
    mapping += "leftx:a0,lefty:a1,rightx:a2,righty:a3,";
    mapping += "platform:Windows,";

    sdl::JoystickClose(probe);

    const int result = sdl::GameControllerAddMapping(mapping.c_str());
    if (result < 0) {
        Log("GC201 auto-map failed: %s", sdl::GetError());
        return false;
    }

    Log("GC201 auto-map installed: Square=b0 Cross=b1 Circle=b2 Triangle=b3 "
        "L1=b4 R1=b5 L2=b6 R2=b7 Select=b8 Start=b9 L3=b10 R3=b11 Guide=%s",
        buttons >= 13 ? "b12" : "none");
    return true;
}

void ControllerCore::InstallBuiltInMappings() {
    const int count = sdl::NumJoysticks();
    int installed = 0;
    for (int i = 0; i < count; ++i) {
        if (sdl::IsGameController(i)) continue;
        if (TryInstallGC201Mapping(i)) ++installed;
    }
    if (installed > 0) {
        Log("Installed %d built-in controller mapping(s) before device open.", installed);
    }
}

void ControllerCore::ScanAndOpen() {
    if (!initialized_ || controller_ || joystick_) return;

    const int count = sdl::NumJoysticks();
    int logicalIndex = 0;

    for (int i = 0; i < count; ++i) {
        if (!sdl::IsGameController(i)) continue;
        if (logicalIndex++ != config_.controllerIndex) continue;
        if (OpenGameController(i)) return;
    }

    if (!config_.allowGenericDirectInput) return;

    logicalIndex = 0;
    for (int i = 0; i < count; ++i) {
        if (sdl::IsGameController(i)) continue;
        if (logicalIndex++ != config_.controllerIndex) continue;
        if (OpenGenericJoystick(i)) return;
    }
}

float ControllerCore::Axis(Sint16 value) {
    if (value < 0) return std::max(-1.0f, static_cast<float>(value) / 32768.0f);
    return std::min(1.0f, static_cast<float>(value) / 32767.0f);
}

float ControllerCore::TriggerFromController(Sint16 value) {
    return std::clamp(
        static_cast<float>(std::max<Sint16>(0, value)) / 32767.0f,
        0.0f, 1.0f);
}

float ControllerCore::TriggerFromGeneric(Sint16 value, bool centered) {
    if (centered) {
        return std::clamp(
            (static_cast<float>(value) + 32768.0f) / 65535.0f,
            0.0f, 1.0f);
    }
    return TriggerFromController(value);
}

void ControllerCore::RadialDeadzone(
    float& x, float& y, float inner, float outer, float sensitivity) {

    const float mag = std::sqrt(x * x + y * y);
    if (mag <= inner || mag <= 0.00001f) {
        x = y = 0.0f;
        return;
    }

    const float usableMax = std::max(inner + 0.0001f, 1.0f - outer);
    const float scaled =
        std::clamp((mag - inner) / (usableMax - inner), 0.0f, 1.0f)
        * sensitivity;

    const float inv = 1.0f / mag;
    x = std::clamp(x * inv * scaled, -1.0f, 1.0f);
    y = std::clamp(y * inv * scaled, -1.0f, 1.0f);
}

void ControllerCore::ApplyDeadzones() {
    RadialDeadzone(
        state_.leftX, state_.leftY,
        config_.leftInnerDeadzone, config_.outerDeadzone, config_.leftSensitivity);

    RadialDeadzone(
        state_.rightX, state_.rightY,
        config_.rightInnerDeadzone, config_.outerDeadzone, config_.rightSensitivity);

    if (config_.invertRightY) state_.rightY = -state_.rightY;
}

void ControllerCore::RefreshFamily() {
    if (controller_) {
        const auto type = sdl::GameControllerGetType(controller_);
        const auto typedFamily = FamilyFromType(type);
        if (typedFamily != GIN_FAMILY_GENERIC ||
            type != SDL_CONTROLLER_TYPE_UNKNOWN) {
            state_.family = typedFamily;
            return;
        }

        SDL_Joystick* joy = sdl::GameControllerGetJoystick(controller_);
        const Uint16 vendor = joy ? sdl::JoystickGetVendor(joy) : 0;
        state_.family = FamilyFromFallbackIdentity(vendor, deviceName_);
    } else if (joystick_) {
        const Uint16 vendor = sdl::JoystickGetVendor(joystick_);
        state_.family = FamilyFromFallbackIdentity(vendor, deviceName_);
    } else {
        state_.family = GIN_FAMILY_NONE;
    }
}

void ControllerCore::PollGameController() {
    state_ = {};
    if (!controller_ || !sdl::GameControllerGetAttached(controller_)) return;

    state_.connected = true;
    RefreshFamily();

    state_.leftX  = Axis(sdl::GameControllerGetAxis(controller_, SDL_CONTROLLER_AXIS_LEFTX));
    state_.leftY  = Axis(sdl::GameControllerGetAxis(controller_, SDL_CONTROLLER_AXIS_LEFTY));
    state_.rightX = Axis(sdl::GameControllerGetAxis(controller_, SDL_CONTROLLER_AXIS_RIGHTX));
    state_.rightY = Axis(sdl::GameControllerGetAxis(controller_, SDL_CONTROLLER_AXIS_RIGHTY));
    state_.leftTrigger =
        TriggerFromController(sdl::GameControllerGetAxis(controller_, SDL_CONTROLLER_AXIS_TRIGGERLEFT));
    state_.rightTrigger =
        TriggerFromController(sdl::GameControllerGetAxis(controller_, SDL_CONTROLLER_AXIS_TRIGGERRIGHT));

    state_.a = Button(controller_, SDL_CONTROLLER_BUTTON_A);
    state_.b = Button(controller_, SDL_CONTROLLER_BUTTON_B);
    state_.x = Button(controller_, SDL_CONTROLLER_BUTTON_X);
    state_.y = Button(controller_, SDL_CONTROLLER_BUTTON_Y);
    state_.lb = Button(controller_, SDL_CONTROLLER_BUTTON_LEFTSHOULDER);
    state_.rb = Button(controller_, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER);
    state_.back = Button(controller_, SDL_CONTROLLER_BUTTON_BACK);
    state_.start = Button(controller_, SDL_CONTROLLER_BUTTON_START);
    state_.l3 = Button(controller_, SDL_CONTROLLER_BUTTON_LEFTSTICK);
    state_.r3 = Button(controller_, SDL_CONTROLLER_BUTTON_RIGHTSTICK);
    state_.dpadUp = Button(controller_, SDL_CONTROLLER_BUTTON_DPAD_UP);
    state_.dpadDown = Button(controller_, SDL_CONTROLLER_BUTTON_DPAD_DOWN);
    state_.dpadLeft = Button(controller_, SDL_CONTROLLER_BUTTON_DPAD_LEFT);
    state_.dpadRight = Button(controller_, SDL_CONTROLLER_BUTTON_DPAD_RIGHT);
    state_.guide = Button(controller_, SDL_CONTROLLER_BUTTON_GUIDE);
    state_.misc1 = Button(controller_, SDL_CONTROLLER_BUTTON_MISC1);

    if (gyroActive_) {
        float data[3]{};
        if (sdl::GameControllerGetSensorData(
                controller_, SDL_SENSOR_GYRO, data, 3) == 0) {

            state_.gyroX = data[0];
            state_.gyroY = data[1];
            state_.gyroZ = data[2];

            if (config_.gyroEnabled) {
                const float gx =
                    (config_.invertGyroX ? -data[1] : data[1]) * config_.gyroSensitivity;
                const float gy =
                    (config_.invertGyroY ? -data[0] : data[0]) * config_.gyroSensitivity;

                state_.rightX = std::clamp(state_.rightX + gx, -1.0f, 1.0f);
                state_.rightY = std::clamp(state_.rightY + gy, -1.0f, 1.0f);
            }
        }
    }

    ApplyDeadzones();
}

void ControllerCore::PollGenericJoystick() {
    state_ = {};
    if (!joystick_ || !sdl::JoystickGetAttached(joystick_)) return;

    const auto& g = config_.generic;
    state_.connected = true;
    state_.family = GIN_FAMILY_GENERIC;

    state_.leftX = Axis(JoyAxis(joystick_, g.leftX));
    state_.leftY = Axis(JoyAxis(joystick_, g.leftY));
    state_.rightX = Axis(JoyAxis(joystick_, g.rightX));
    state_.rightY = Axis(JoyAxis(joystick_, g.rightY));

    if (g.leftTriggerButton >= 0) {
        state_.leftTrigger = TriggerButton(joystick_, g.leftTriggerButton);
    } else {
        state_.leftTrigger = TriggerAxis(joystick_, g.leftTriggerAxis, g.centeredTriggerAxes);
    }

    if (g.rightTriggerButton >= 0) {
        state_.rightTrigger = TriggerButton(joystick_, g.rightTriggerButton);
    } else {
        state_.rightTrigger = TriggerAxis(joystick_, g.rightTriggerAxis, g.centeredTriggerAxes);
    }

    state_.a = JoyButton(joystick_, g.a);
    state_.b = JoyButton(joystick_, g.b);
    state_.x = JoyButton(joystick_, g.x);
    state_.y = JoyButton(joystick_, g.y);
    state_.lb = JoyButton(joystick_, g.lb);
    state_.rb = JoyButton(joystick_, g.rb);
    state_.back = JoyButton(joystick_, g.back);
    state_.start = JoyButton(joystick_, g.start);
    state_.l3 = JoyButton(joystick_, g.l3);
    state_.r3 = JoyButton(joystick_, g.r3);
    state_.guide = JoyButton(joystick_, g.guide);
    state_.misc1 = JoyButton(joystick_, g.misc1);

    if (g.hat >= 0 && g.hat < sdl::JoystickNumHats(joystick_)) {
        const Uint8 hat = sdl::JoystickGetHat(joystick_, g.hat);
        state_.dpadUp = (hat & SDL_HAT_UP) != 0;
        state_.dpadDown = (hat & SDL_HAT_DOWN) != 0;
        state_.dpadLeft = (hat & SDL_HAT_LEFT) != 0;
        state_.dpadRight = (hat & SDL_HAT_RIGHT) != 0;
    }

    ApplyDeadzones();
}

void ControllerCore::Tick() {
    if (!initialized_) return;

    sdl::PumpEvents();
    sdl::GameControllerUpdate();
    sdl::JoystickUpdate();

    if (controller_ && !sdl::GameControllerGetAttached(controller_)) {
        CloseDevice();
    } else if (joystick_ && !sdl::JoystickGetAttached(joystick_)) {
        CloseDevice();
    }

    if (!controller_ && !joystick_) {
        if (--scanCountdown_ <= 0) {
            scanCountdown_ = std::max(1, config_.hotplugScanFrames);
            ScanAndOpen();
        }
    }

    if (controller_) {
        PollGameController();
    } else if (joystick_) {
        PollGenericJoystick();
    } else {
        state_ = {};
    }
}

bool ControllerCore::Rumble(float low, float high, std::uint32_t ms) {
    if (!initialized_ || !config_.rumbleEnabled || !state_.connected) return false;

    low = std::clamp(low * config_.rumbleStrength, 0.0f, 1.0f);
    high = std::clamp(high * config_.rumbleStrength, 0.0f, 1.0f);

    if (controller_) {
        const Uint16 lo = static_cast<Uint16>(low * 65535.0f);
        const Uint16 hi = static_cast<Uint16>(high * 65535.0f);
        return sdl::GameControllerRumble(controller_, lo, hi, ms) == 0;
    }

    if (haptic_) {
        return sdl::HapticRumblePlay(haptic_, std::max(low, high), ms) == 0;
    }

    return false;
}

std::uint32_t ButtonMask(const UnifiedState& s) {
    std::uint32_t m = 0;
    if (s.a) m |= GIN_BTN_A;
    if (s.b) m |= GIN_BTN_B;
    if (s.x) m |= GIN_BTN_X;
    if (s.y) m |= GIN_BTN_Y;
    if (s.lb) m |= GIN_BTN_LB;
    if (s.rb) m |= GIN_BTN_RB;
    if (s.back) m |= GIN_BTN_BACK;
    if (s.start) m |= GIN_BTN_START;
    if (s.l3) m |= GIN_BTN_L3;
    if (s.r3) m |= GIN_BTN_R3;
    if (s.dpadUp) m |= GIN_BTN_DPAD_UP;
    if (s.dpadDown) m |= GIN_BTN_DPAD_DOWN;
    if (s.dpadLeft) m |= GIN_BTN_DPAD_LEFT;
    if (s.dpadRight) m |= GIN_BTN_DPAD_RIGHT;
    if (s.guide) m |= GIN_BTN_GUIDE;
    if (s.misc1) m |= GIN_BTN_MISC1;
    return m;
}

} // namespace gin
