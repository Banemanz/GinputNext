#pragma once
#include <string>

namespace gin {

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
    bool invertCameraY = true;
#if defined(GTASA)
    bool invertAimY = true;
#else
    bool invertAimY = false;
#endif

    bool autoAim = true;

    bool gyroEnabled = false;
    float gyroSensitivity = 0.35f;
    bool invertGyroX = false;
    bool invertGyroY = false;

    bool rumbleEnabled = true;
    bool gameRumbleEnabled = true;
    float rumbleStrength = 1.0f;

    GenericMap generic;

    bool Load(const std::string& path);
};

} // namespace gin
