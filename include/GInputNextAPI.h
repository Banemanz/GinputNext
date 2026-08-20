#pragma once
#include <cstdint>

#if defined(_WIN32)
#define GIN_EXPORT extern "C" __declspec(dllexport)
#define GIN_CALL __cdecl
#else
#define GIN_EXPORT extern "C"
#define GIN_CALL
#endif

enum GIN_ControllerFamily : std::int32_t {
    GIN_FAMILY_NONE = 0,
    GIN_FAMILY_XBOX = 1,
    GIN_FAMILY_PLAYSTATION = 2,
    GIN_FAMILY_NINTENDO = 3,
    GIN_FAMILY_GENERIC = 4
};

enum GIN_ButtonBits : std::uint32_t {
    GIN_BTN_A       = 1u << 0,
    GIN_BTN_B       = 1u << 1,
    GIN_BTN_X       = 1u << 2,
    GIN_BTN_Y       = 1u << 3,
    GIN_BTN_LB      = 1u << 4,
    GIN_BTN_RB      = 1u << 5,
    GIN_BTN_BACK    = 1u << 6,
    GIN_BTN_START   = 1u << 7,
    GIN_BTN_L3      = 1u << 8,
    GIN_BTN_R3      = 1u << 9,
    GIN_BTN_DPAD_UP = 1u << 10,
    GIN_BTN_DPAD_DOWN = 1u << 11,
    GIN_BTN_DPAD_LEFT = 1u << 12,
    GIN_BTN_DPAD_RIGHT = 1u << 13,
    GIN_BTN_GUIDE   = 1u << 14,
    GIN_BTN_MISC1   = 1u << 15
};

#pragma pack(push, 1)
struct GIN_State {
    float leftX;
    float leftY;
    float rightX;
    float rightY;
    float leftTrigger;
    float rightTrigger;
    float gyroX;
    float gyroY;
    float gyroZ;
    std::uint32_t buttons;
    std::int32_t family;
    std::int32_t connected;
};
#pragma pack(pop)

GIN_EXPORT std::int32_t GIN_CALL GIN_GetAPIVersion();
GIN_EXPORT std::int32_t GIN_CALL GIN_IsConnected();
GIN_EXPORT std::int32_t GIN_CALL GIN_GetControllerFamily();
GIN_EXPORT std::int32_t GIN_CALL GIN_GetState(GIN_State* outState);
GIN_EXPORT std::int32_t GIN_CALL GIN_Rumble(float lowFrequency, float highFrequency, std::uint32_t milliseconds);
