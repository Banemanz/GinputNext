#pragma once
#include "Config.h"
#include "../include/GInputNextAPI.h"
#include <SDL.h>
#include <cstdint>
#include <string>

namespace gin {

struct UnifiedState {
    float leftX = 0.0f;
    float leftY = 0.0f;
    float rightX = 0.0f;
    float rightY = 0.0f;
    float leftTrigger = 0.0f;
    float rightTrigger = 0.0f;
    float gyroX = 0.0f;
    float gyroY = 0.0f;
    float gyroZ = 0.0f;

    bool a = false;
    bool b = false;
    bool x = false;
    bool y = false;
    bool lb = false;
    bool rb = false;
    bool back = false;
    bool start = false;
    bool l3 = false;
    bool r3 = false;
    bool dpadUp = false;
    bool dpadDown = false;
    bool dpadLeft = false;
    bool dpadRight = false;
    bool guide = false;
    bool misc1 = false;

    GIN_ControllerFamily family = GIN_FAMILY_NONE;
    bool connected = false;
};

class ControllerCore {
public:
    bool Init(const Config& config, const std::string& moduleDir, const std::string& gameDir);
    void Shutdown();
    void Tick();

    const UnifiedState& State() const { return state_; }
    bool IsConnected() const { return state_.connected; }
    GIN_ControllerFamily Family() const { return state_.family; }

    bool Rumble(float low, float high, std::uint32_t ms);
    const char* DeviceName() const { return deviceName_.c_str(); }

private:
    void InstallBuiltInMappings();
    bool TryInstallGC201Mapping(int joystickIndex);
    void ScanAndOpen();
    bool OpenGameController(int joystickIndex);
    bool OpenGenericJoystick(int joystickIndex);
    void CloseDevice();
    void PollGameController();
    void PollGenericJoystick();
    void RefreshFamily();
    void ApplyDeadzones();
    static void RadialDeadzone(float& x, float& y, float inner, float outer, float sensitivity);
    static float TriggerFromController(Sint16 value);
    static float TriggerFromGeneric(Sint16 value, bool centered);
    static float Axis(Sint16 value);
    int LoadMappingFile(const std::string& path);

    bool initialized_ = false;
    Config config_{};
    std::string moduleDir_;
    std::string gameDir_;
    std::string deviceName_;

    SDL_GameController* controller_ = nullptr;
    SDL_Joystick* joystick_ = nullptr;
    SDL_Haptic* haptic_ = nullptr;
    SDL_JoystickID instanceId_ = -1;
    bool usingGeneric_ = false;
    int scanCountdown_ = 0;
    bool gyroActive_ = false;
    UnifiedState state_{};
};

std::uint32_t ButtonMask(const UnifiedState& s);

} // namespace gin
