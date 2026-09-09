#pragma once
#include "../include/GInputNextAPI.h"
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

}
