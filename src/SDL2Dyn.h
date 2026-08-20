#pragma once
#include <SDL.h>
#include <string>

namespace gin::sdl2dyn {

bool Load(const std::string& moduleDir, const std::string& gameDir);
void Unload();
bool IsLoaded();
const char* LoadedPath();
void GetVersion(SDL_version* outVersion);
bool HasGameControllerGetType();

void SetMainReady();
SDL_bool SetHint(const char* name, const char* value);
int InitSubSystem(Uint32 flags);
void QuitSubSystem(Uint32 flags);
const char* GetError();

int NumJoysticks();
SDL_bool IsGameController(int joystickIndex);
SDL_GameController* GameControllerOpen(int joystickIndex);
void GameControllerClose(SDL_GameController* controller);
SDL_Joystick* GameControllerGetJoystick(SDL_GameController* controller);
const char* GameControllerName(SDL_GameController* controller);
SDL_GameControllerType GameControllerGetType(SDL_GameController* controller);
SDL_bool GameControllerGetAttached(SDL_GameController* controller);
Sint16 GameControllerGetAxis(SDL_GameController* controller, SDL_GameControllerAxis axis);
Uint8 GameControllerGetButton(SDL_GameController* controller, SDL_GameControllerButton button);
int GameControllerAddMapping(const char* mapping);
int GameControllerRumble(SDL_GameController* controller, Uint16 low, Uint16 high, Uint32 ms);
SDL_bool GameControllerHasSensor(SDL_GameController* controller, SDL_SensorType type);
int GameControllerSetSensorEnabled(SDL_GameController* controller, SDL_SensorType type, SDL_bool enabled);
int GameControllerGetSensorData(SDL_GameController* controller, SDL_SensorType type, float* data, int numValues);
void GameControllerUpdate();

SDL_Joystick* JoystickOpen(int deviceIndex);
void JoystickClose(SDL_Joystick* joystick);
const char* JoystickName(SDL_Joystick* joystick);
SDL_JoystickID JoystickInstanceID(SDL_Joystick* joystick);
SDL_JoystickGUID JoystickGetGUID(SDL_Joystick* joystick);
void JoystickGetGUIDString(SDL_JoystickGUID guid, char* pszGUID, int cbGUID);
SDL_bool JoystickGetAttached(SDL_Joystick* joystick);
int JoystickNumAxes(SDL_Joystick* joystick);
int JoystickNumButtons(SDL_Joystick* joystick);
int JoystickNumHats(SDL_Joystick* joystick);
Sint16 JoystickGetAxis(SDL_Joystick* joystick, int axis);
Uint8 JoystickGetButton(SDL_Joystick* joystick, int button);
Uint8 JoystickGetHat(SDL_Joystick* joystick, int hat);
Uint16 JoystickGetVendor(SDL_Joystick* joystick);
Uint16 JoystickGetProduct(SDL_Joystick* joystick);
int JoystickIsHaptic(SDL_Joystick* joystick);
void JoystickUpdate();

SDL_Haptic* HapticOpenFromJoystick(SDL_Joystick* joystick);
void HapticClose(SDL_Haptic* haptic);
int HapticRumbleInit(SDL_Haptic* haptic);
int HapticRumblePlay(SDL_Haptic* haptic, float strength, Uint32 length);
int HapticRumbleStop(SDL_Haptic* haptic);

void PumpEvents();

} // namespace gin::sdl2dyn
