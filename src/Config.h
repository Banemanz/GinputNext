#pragma once
#include <string>

namespace gin {

enum class ControlProfile {
    Classic,
    Modern
};

struct GenericMap {
    int leftX = 0;
    int leftY = 1;
    int rightX = 2;
    int rightY = 3;

    int leftTriggerAxis = -1;
    int rightTriggerAxis = -1;
    int leftTriggerButton = 6;
    int rightTriggerButton = 7;

    // Common legacy PlayStation-position DirectInput ordering:
    // b0=Square, b1=Cross, b2=Circle, b3=Triangle.
    int a = 1;
    int b = 2;
    int x = 0;
    int y = 3;
    int lb = 4;
    int rb = 5;
    int back = 8;
    int start = 9;
    int l3 = 10;
    int r3 = 11;
    int guide = 12;
    int misc1 = -1;
    int hat = 0;
    bool centeredTriggerAxes = true;
};

struct Config {
    bool enabled = true;
    int controllerIndex = 0;
    bool allowGenericDirectInput = true;
    bool suppressNativeGamepad = true;
    bool startActsAsEscape = true;
    bool backgroundInput = false;
    int hotplugScanFrames = 30;
    bool debugInput = false;

    float leftInnerDeadzone = 0.15f;
    float rightInnerDeadzone = 0.12f;
    float outerDeadzone = 0.02f;
    float leftSensitivity = 1.0f;
    float rightSensitivity = 1.0f;

    // User-facing inversion defaults are off for all games.
    // San Andreas vertical direction is applied through the game's native
    // CPad::bInvertLook4Pad path, whose sign is opposite of this normalized
    // user-facing setting after SDL stick staging.
    bool invertCameraY = false;
    bool invertAimY = false;

    bool autoAim = true;

    // When the player uses keyboard or mouse, temporarily stand down the
    // controller staging and modern action hooks so native PC controls do not
    // fight with a connected-but-idle controller. Any meaningful controller
    // movement/button immediately hands ownership back to GInputNext.
    bool autoSwitchKeyboardMouse = true;
    int keyboardMouseCooldownFrames = 90; // Legacy INI round-trip only; ownership now waits for fresh input.
    float controllerWakeStickThreshold = 0.20f;
    float controllerWakeTriggerThreshold = 0.30f;

    // Classic preserves each game's PS2-era logical controls. Modern is
    // implemented with game-specific action-method hooks so shared CPad fields
    // are not globally reshuffled across frontend/scripts/weapon/vehicle paths.
    ControlProfile controlProfile = ControlProfile::Classic;

    bool inGameConfigEnabled = true;
    bool inGameConfigControllerChord = true;
    int inGameConfigHotkeyVK = 0x77; // VK_F8

    bool gyroEnabled = false;
    float gyroSensitivity = 0.35f;
    bool invertGyroX = false;
    bool invertGyroY = false;

    bool rumbleEnabled = true;
    bool gameRumbleEnabled = true;
    float rumbleStrength = 1.0f;

    GenericMap generic;

    bool Load(const std::string& path);
    bool Save(const std::string& path) const;
};

} // namespace gin
