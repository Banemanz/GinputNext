#pragma once
#include <SDL.h>
#include <type_traits>

namespace gin::sdl_signature_audit {

// These are the exact return types used by the v3/v4 dynamic wrapper.
// Keep this header cheap and compile it in every target so wrapper drift is
// caught by the compiler before it becomes three identical GTA build errors.

static_assert(std::is_same_v<
    decltype(::SDL_SetHint(nullptr, nullptr)),
    SDL_bool>);

static_assert(std::is_same_v<
    decltype(::SDL_IsGameController(0)),
    SDL_bool>);

static_assert(std::is_same_v<
    decltype(::SDL_GameControllerGetAttached(static_cast<SDL_GameController*>(nullptr))),
    SDL_bool>);

static_assert(std::is_same_v<
    decltype(::SDL_GameControllerHasSensor(
        static_cast<SDL_GameController*>(nullptr),
        SDL_SENSOR_GYRO)),
    SDL_bool>);

static_assert(std::is_same_v<
    decltype(::SDL_JoystickGetAttached(static_cast<SDL_Joystick*>(nullptr))),
    SDL_bool>);

// SDL2 deliberately returns int here because negative values are errors.
// Treating it as SDL_bool loses that ABI/API contract and MSVC correctly
// rejects the implicit int -> enum conversion.
static_assert(std::is_same_v<
    decltype(::SDL_JoystickIsHaptic(static_cast<SDL_Joystick*>(nullptr))),
    int>);

static_assert(std::is_same_v<
    decltype(::SDL_GameControllerRumble(
        static_cast<SDL_GameController*>(nullptr), 0, 0, 0)),
    int>);

} // namespace gin::sdl_signature_audit
