#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include "SDL2Dyn.h"
#include "SDLSignatureAudit.h"
#include "Log.h"
#include <array>
#include <cstring>
#include <type_traits>

static_assert(std::is_same_v<
    decltype(::SDL_JoystickIsHaptic(static_cast<SDL_Joystick*>(nullptr))),
    int>,
    "SDL_JoystickIsHaptic signature drift: expected int for SDL2 2.32.10");


namespace gin::sdl2dyn {
namespace {

HMODULE g_sdl = nullptr;
bool g_ownsModule = false;
std::string g_loadedPath;

#define DECL_SDL(name) static decltype(&::name) p_##name = nullptr
DECL_SDL(SDL_SetMainReady);
DECL_SDL(SDL_SetHint);
DECL_SDL(SDL_InitSubSystem);
DECL_SDL(SDL_QuitSubSystem);
DECL_SDL(SDL_GetError);
DECL_SDL(SDL_GetVersion);

DECL_SDL(SDL_NumJoysticks);
DECL_SDL(SDL_IsGameController);
DECL_SDL(SDL_GameControllerOpen);
DECL_SDL(SDL_GameControllerClose);
DECL_SDL(SDL_GameControllerGetJoystick);
DECL_SDL(SDL_GameControllerName);
DECL_SDL(SDL_GameControllerGetType);
DECL_SDL(SDL_GameControllerGetAttached);
DECL_SDL(SDL_GameControllerGetAxis);
DECL_SDL(SDL_GameControllerGetButton);
DECL_SDL(SDL_GameControllerAddMapping);
DECL_SDL(SDL_GameControllerRumble);
DECL_SDL(SDL_GameControllerHasSensor);
DECL_SDL(SDL_GameControllerSetSensorEnabled);
DECL_SDL(SDL_GameControllerGetSensorData);
DECL_SDL(SDL_GameControllerUpdate);

DECL_SDL(SDL_JoystickOpen);
DECL_SDL(SDL_JoystickClose);
DECL_SDL(SDL_JoystickName);
DECL_SDL(SDL_JoystickInstanceID);
DECL_SDL(SDL_JoystickGetGUID);
DECL_SDL(SDL_JoystickGetGUIDString);
DECL_SDL(SDL_JoystickGetAttached);
DECL_SDL(SDL_JoystickNumAxes);
DECL_SDL(SDL_JoystickNumButtons);
DECL_SDL(SDL_JoystickNumHats);
DECL_SDL(SDL_JoystickGetAxis);
DECL_SDL(SDL_JoystickGetButton);
DECL_SDL(SDL_JoystickGetHat);
DECL_SDL(SDL_JoystickGetVendor);
DECL_SDL(SDL_JoystickGetProduct);
DECL_SDL(SDL_JoystickIsHaptic);
DECL_SDL(SDL_JoystickUpdate);

DECL_SDL(SDL_HapticOpenFromJoystick);
DECL_SDL(SDL_HapticClose);
DECL_SDL(SDL_HapticRumbleInit);
DECL_SDL(SDL_HapticRumblePlay);
DECL_SDL(SDL_HapticRumbleStop);

DECL_SDL(SDL_PumpEvents);
#undef DECL_SDL

template <class T>
bool Resolve(T& out, const char* name) {
    out = reinterpret_cast<T>(GetProcAddress(g_sdl, name));
    if (!out) {
        Log("SDL2 export missing: %s", name);
        return false;
    }
    return true;
}

bool ResolveRequired(void*& out, const char* name) {
    out = reinterpret_cast<void*>(GetProcAddress(g_sdl, name));
    if (!out) {
        Log("SDL2 REQUIRED export missing: %s", name);
        return false;
    }
    return true;
}

template <class T>
bool ResolveRequiredTyped(T& out, const char* name) {
    void* p = nullptr;
    if (!ResolveRequired(p, name)) return false;
    out = reinterpret_cast<T>(p);
    return true;
}

template <class T>
void ResolveOptional(T& out, const char* name) {
    out = reinterpret_cast<T>(GetProcAddress(g_sdl, name));
    if (!out) {
        Log("SDL2 optional export unavailable: %s", name);
    }
}

bool ResolveAll() {
#define LOAD_REQ(name) if (!ResolveRequiredTyped(p_##name, #name)) return false
#define LOAD_OPT(name) ResolveOptional(p_##name, #name)

    // Base SDL/controller API required for the plugin to function.
    LOAD_REQ(SDL_SetMainReady);
    LOAD_REQ(SDL_SetHint);
    LOAD_REQ(SDL_InitSubSystem);
    LOAD_REQ(SDL_QuitSubSystem);
    LOAD_REQ(SDL_GetError);
    LOAD_REQ(SDL_GetVersion);

    LOAD_REQ(SDL_NumJoysticks);
    LOAD_REQ(SDL_IsGameController);
    LOAD_REQ(SDL_GameControllerOpen);
    LOAD_REQ(SDL_GameControllerClose);
    LOAD_REQ(SDL_GameControllerGetJoystick);
    LOAD_REQ(SDL_GameControllerName);
    LOAD_REQ(SDL_GameControllerGetAttached);
    LOAD_REQ(SDL_GameControllerGetAxis);
    LOAD_REQ(SDL_GameControllerGetButton);
    LOAD_REQ(SDL_GameControllerAddMapping);
    LOAD_REQ(SDL_GameControllerUpdate);

    LOAD_REQ(SDL_JoystickOpen);
    LOAD_REQ(SDL_JoystickClose);
    LOAD_REQ(SDL_JoystickName);
    LOAD_REQ(SDL_JoystickInstanceID);
    LOAD_REQ(SDL_JoystickGetGUID);
    LOAD_REQ(SDL_JoystickGetGUIDString);
    LOAD_REQ(SDL_JoystickGetAttached);
    LOAD_REQ(SDL_JoystickNumAxes);
    LOAD_REQ(SDL_JoystickNumButtons);
    LOAD_REQ(SDL_JoystickNumHats);
    LOAD_REQ(SDL_JoystickGetAxis);
    LOAD_REQ(SDL_JoystickGetButton);
    LOAD_REQ(SDL_JoystickGetHat);
    LOAD_REQ(SDL_JoystickUpdate);
    LOAD_REQ(SDL_JoystickIsHaptic);

    LOAD_REQ(SDL_HapticOpenFromJoystick);
    LOAD_REQ(SDL_HapticClose);
    LOAD_REQ(SDL_HapticRumbleInit);
    LOAD_REQ(SDL_HapticRumblePlay);
    LOAD_REQ(SDL_HapticRumbleStop);

    LOAD_REQ(SDL_PumpEvents);

    // These are conveniences added in later SDL2 releases. They are NOT
    // allowed to prevent controller input from working with an older but
    // otherwise usable SDL2 runtime.
    LOAD_OPT(SDL_GameControllerGetType);
    LOAD_OPT(SDL_GameControllerRumble);
    LOAD_OPT(SDL_GameControllerHasSensor);
    LOAD_OPT(SDL_GameControllerSetSensorEnabled);
    LOAD_OPT(SDL_GameControllerGetSensorData);
    LOAD_OPT(SDL_JoystickGetVendor);
    LOAD_OPT(SDL_JoystickGetProduct);

#undef LOAD_REQ
#undef LOAD_OPT
    return true;
}

bool TryLoadPath(const std::string& path) {
    if (!FileExists(path)) return false;
    HMODULE mod = LoadLibraryExA(path.c_str(), nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
    if (!mod) {
        Log("Failed to LoadLibrary SDL2 at \"%s\" (Win32=%lu)", path.c_str(), GetLastError());
        return false;
    }

    g_sdl = mod;
    g_ownsModule = true;

    char resolved[MAX_PATH]{};
    const DWORD n = GetModuleFileNameA(mod, resolved, MAX_PATH);
    g_loadedPath = n ? std::string(resolved, n) : path;

    if (!ResolveAll()) {
        FreeLibrary(g_sdl);
        g_sdl = nullptr;
        g_ownsModule = false;
        g_loadedPath.clear();
        return false;
    }
    return true;
}

} // namespace

bool Load(const std::string& moduleDir, const std::string& gameDir) {
    if (g_sdl) return true;

    // Use a private filename first so GInputNext can coexist with other mods
    // that happen to load a different SDL2.dll into the same GTA process.
    const std::array<std::string, 4> candidates = {
        JoinPath(moduleDir, "GInputNext.SDL2.dll"),
        JoinPath(moduleDir, "SDL2.dll"),
        JoinPath(gameDir, "GInputNext.SDL2.dll"),
        JoinPath(gameDir, "SDL2.dll")
    };

    for (const auto& path : candidates) {
        if (TryLoadPath(path)) {
            SDL_version v{};
            p_SDL_GetVersion(&v);
            Log("Loaded SDL2 dynamically from \"%s\" version=%u.%u.%u",
                g_loadedPath.c_str(), v.major, v.minor, v.patch);
            return true;
        }
    }

    Log("Could not locate GInputNext.SDL2.dll/SDL2.dll beside the ASI or game EXE.");
    return false;
}

void Unload() {
    if (!g_sdl) return;
    if (g_ownsModule) FreeLibrary(g_sdl);
    g_sdl = nullptr;
    g_ownsModule = false;
    g_loadedPath.clear();
}

bool IsLoaded() { return g_sdl != nullptr; }
const char* LoadedPath() { return g_loadedPath.c_str(); }
void GetVersion(SDL_version* outVersion) {
    if (!outVersion) return;
    *outVersion = {};
    if (p_SDL_GetVersion) p_SDL_GetVersion(outVersion);
}
bool HasGameControllerGetType() { return p_SDL_GameControllerGetType != nullptr; }

void SetMainReady() { p_SDL_SetMainReady(); }
SDL_bool SetHint(const char* name, const char* value) { return p_SDL_SetHint(name, value); }
int InitSubSystem(Uint32 flags) { return p_SDL_InitSubSystem(flags); }
void QuitSubSystem(Uint32 flags) { p_SDL_QuitSubSystem(flags); }
const char* GetError() { return p_SDL_GetError ? p_SDL_GetError() : "SDL2 not loaded"; }

int NumJoysticks() { return p_SDL_NumJoysticks(); }
SDL_bool IsGameController(int i) { return p_SDL_IsGameController(i); }
SDL_GameController* GameControllerOpen(int i) { return p_SDL_GameControllerOpen(i); }
void GameControllerClose(SDL_GameController* c) { p_SDL_GameControllerClose(c); }
SDL_Joystick* GameControllerGetJoystick(SDL_GameController* c) { return p_SDL_GameControllerGetJoystick(c); }
const char* GameControllerName(SDL_GameController* c) { return p_SDL_GameControllerName(c); }
SDL_GameControllerType GameControllerGetType(SDL_GameController* c) {
    return p_SDL_GameControllerGetType ? p_SDL_GameControllerGetType(c) : SDL_CONTROLLER_TYPE_UNKNOWN;
}
SDL_bool GameControllerGetAttached(SDL_GameController* c) { return p_SDL_GameControllerGetAttached(c); }
Sint16 GameControllerGetAxis(SDL_GameController* c, SDL_GameControllerAxis a) { return p_SDL_GameControllerGetAxis(c, a); }
Uint8 GameControllerGetButton(SDL_GameController* c, SDL_GameControllerButton b) { return p_SDL_GameControllerGetButton(c, b); }
int GameControllerAddMapping(const char* m) { return p_SDL_GameControllerAddMapping(m); }
int GameControllerRumble(SDL_GameController* c, Uint16 l, Uint16 h, Uint32 ms) {
    return p_SDL_GameControllerRumble ? p_SDL_GameControllerRumble(c, l, h, ms) : -1;
}
SDL_bool GameControllerHasSensor(SDL_GameController* c, SDL_SensorType t) {
    return p_SDL_GameControllerHasSensor ? p_SDL_GameControllerHasSensor(c, t) : SDL_FALSE;
}
int GameControllerSetSensorEnabled(SDL_GameController* c, SDL_SensorType t, SDL_bool e) {
    return p_SDL_GameControllerSetSensorEnabled ? p_SDL_GameControllerSetSensorEnabled(c, t, e) : -1;
}
int GameControllerGetSensorData(SDL_GameController* c, SDL_SensorType t, float* d, int n) {
    return p_SDL_GameControllerGetSensorData ? p_SDL_GameControllerGetSensorData(c, t, d, n) : -1;
}
void GameControllerUpdate() { p_SDL_GameControllerUpdate(); }

SDL_Joystick* JoystickOpen(int i) { return p_SDL_JoystickOpen(i); }
void JoystickClose(SDL_Joystick* j) { p_SDL_JoystickClose(j); }
const char* JoystickName(SDL_Joystick* j) { return p_SDL_JoystickName(j); }
SDL_JoystickID JoystickInstanceID(SDL_Joystick* j) { return p_SDL_JoystickInstanceID(j); }
SDL_JoystickGUID JoystickGetGUID(SDL_Joystick* j) { return p_SDL_JoystickGetGUID(j); }
void JoystickGetGUIDString(SDL_JoystickGUID g, char* out, int len) { p_SDL_JoystickGetGUIDString(g, out, len); }
SDL_bool JoystickGetAttached(SDL_Joystick* j) { return p_SDL_JoystickGetAttached(j); }
int JoystickNumAxes(SDL_Joystick* j) { return p_SDL_JoystickNumAxes(j); }
int JoystickNumButtons(SDL_Joystick* j) { return p_SDL_JoystickNumButtons(j); }
int JoystickNumHats(SDL_Joystick* j) { return p_SDL_JoystickNumHats(j); }
Sint16 JoystickGetAxis(SDL_Joystick* j, int a) { return p_SDL_JoystickGetAxis(j, a); }
Uint8 JoystickGetButton(SDL_Joystick* j, int b) { return p_SDL_JoystickGetButton(j, b); }
Uint8 JoystickGetHat(SDL_Joystick* j, int h) { return p_SDL_JoystickGetHat(j, h); }
Uint16 JoystickGetVendor(SDL_Joystick* j) { return p_SDL_JoystickGetVendor ? p_SDL_JoystickGetVendor(j) : 0; }
Uint16 JoystickGetProduct(SDL_Joystick* j) { return p_SDL_JoystickGetProduct ? p_SDL_JoystickGetProduct(j) : 0; }
int JoystickIsHaptic(SDL_Joystick* j) { return p_SDL_JoystickIsHaptic(j); }
void JoystickUpdate() { p_SDL_JoystickUpdate(); }

SDL_Haptic* HapticOpenFromJoystick(SDL_Joystick* j) { return p_SDL_HapticOpenFromJoystick(j); }
void HapticClose(SDL_Haptic* h) { p_SDL_HapticClose(h); }
int HapticRumbleInit(SDL_Haptic* h) { return p_SDL_HapticRumbleInit(h); }
int HapticRumblePlay(SDL_Haptic* h, float s, Uint32 l) { return p_SDL_HapticRumblePlay(h, s, l); }
int HapticRumbleStop(SDL_Haptic* h) { return p_SDL_HapticRumbleStop(h); }

void PumpEvents() { p_SDL_PumpEvents(); }

} // namespace gin::sdl2dyn
