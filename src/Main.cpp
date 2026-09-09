#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include "plugin.h"
#include "Config.h"
#include "ControllerCore.h"
#include "GTAAdapter.h"
#include "GameplayBridge.h"
#include "InGameConfigUI.h"
#if defined(GTA3)
#include "GTA3WeaponAimHook.h"
#endif
#include "InputUpdateHook.h"
#include "InputArbitration.h"
#include "Log.h"
#include "ModernControlsHook.h"
#include "NativeDInputBlocker.h"
#include "PauseBridge.h"
#include "../include/GInputNextAPI.h"
#include <string>

namespace gin {
namespace {

Config g_config;
ControllerCore g_core;
NativeDInputBlocker g_nativeDInput;
InputUpdateHook g_inputHook;
InputArbitration g_inputArbitration;
PauseBridge g_pauseBridge;
GameplayBridge g_gameplayBridge;
InGameConfigUI g_inGameConfigUI;
ModernControlsHook g_modernControlsHook;
#if defined(GTA3)
GTA3WeaponAimHook g_gta3WeaponAimHook;
#endif

bool g_ready = false;
bool g_initAttempted = false;
bool g_shutdown = false;
std::string g_gameDir;
std::string g_moduleDir;
std::string g_configPath;

const char* GameTag() {
#if defined(GTA3)
    return "GTA III";
#elif defined(GTAVC)
    return "GTA Vice City";
#elif defined(GTASA)
    return "GTA San Andreas";
#else
    return "Unknown GTA";
#endif
}

std::string FindConfigPath() {
    const auto local = JoinPath(g_moduleDir, "GInputNext.ini");
    if (FileExists(local)) return local;

    const auto root = JoinPath(g_gameDir, "GInputNext.ini");
    if (FileExists(root)) return root;

    // Prefer module-local location for the diagnostic even when absent.
    return local;
}

void __cdecl UpdatePadsBridge() {
    // The hook replaces CALL sites, never CPad::UpdatePads itself.
    // If runtime setup somehow failed, preserve the original game call.
    if (!g_ready || g_shutdown) {
        g_inputHook.CallOriginal();
        return;
    }

    // Keep GTA's legacy DirectInput gamepad devices out of the way. The mouse
    // pointer is a completely separate RsGlobal.ps->diMouse slot.
    g_nativeDInput.Maintain();

    // Poll SDL immediately BEFORE the game reconciles its controller state.
    g_core.ApplyLiveConfig(g_config);
    g_core.Tick();

    if (g_config.debugInput) {
        static std::uint32_t lastMask = 0;
        const std::uint32_t mask = ButtonMask(g_core.State());
        if (mask != lastMask) {
            Log("input edge: mask=0x%08lX changed=0x%08lX start=%d back=%d",
                static_cast<unsigned long>(mask),
                static_cast<unsigned long>(mask ^ lastMask),
                g_core.State().start ? 1 : 0,
                g_core.State().back ? 1 : 0);
            lastMask = mask;
        }
    }

    const bool inputOwnerAllowed = g_inputArbitration.Tick(g_core.State(), g_config);

    g_inGameConfigUI.SetModernAvailable(g_modernControlsHook.IsInstalled());
    const bool wasOverlayOpen = g_inGameConfigUI.IsOpen();
    g_inGameConfigUI.Tick(g_core.State(), g_config, g_inputArbitration.OwnerName());
    if (!g_modernControlsHook.IsInstalled()) g_config.controlProfile = ControlProfile::Classic;
    static bool overlayReleasePending = false;
    bool menuKeyHeld = false;
    for (int vk : {VK_ESCAPE,VK_BACK,VK_RETURN,VK_SPACE,VK_UP,VK_DOWN,VK_LEFT,VK_RIGHT})
        menuKeyHeld |= (GetAsyncKeyState(vk) & 0x8000) != 0;
    if (wasOverlayOpen || g_inGameConfigUI.IsOpen()) overlayReleasePending = true;
    else if (!menuKeyHeld && ButtonMask(g_core.State()) == 0 &&
             g_core.State().leftTrigger < 0.30f && g_core.State().rightTrigger < 0.30f)
        overlayReleasePending = false;
    const bool controllerAllowed = inputOwnerAllowed && !overlayReleasePending;
    UnifiedState effectiveState = controllerAllowed ? g_core.State() : UnifiedState{};
#if defined(GTA3)
    g_gta3WeaponAimHook.SetControllerAllowed(controllerAllowed);
#endif

    GTAAdapter::StageBeforePadUpdate(g_core.State(), g_config, controllerAllowed);

    // Execute the untouched original GTA input update. This creates proper
    // OldState/NewState transitions for normal gameplay controls.
    g_inputHook.CallOriginal();

    if (overlayReleasePending) {
        if (auto* pad = CPad::GetPad(0)) {
            pad->NewState = {}; pad->OldState = {};
            pad->PCTempKeyState = {}; pad->PCTempMouseState = {};
        }
        CPad::NewKeyState = {}; CPad::OldKeyState = {};
    }
    // Start/Pause is special on the classic PC ports. Preserve logical Start
    // for scripts/mods and optionally synthesize the native PC Escape edge so
    // the retail frontend actually opens/closes the pause menu.
    g_pauseBridge.AfterPadUpdate(
        effectiveState,
        g_config.startActsAsEscape,
        g_config.debugInput);

    g_modernControlsHook.AfterPadUpdate(g_core.State(), g_config, controllerAllowed);
    g_gameplayBridge.AfterPadUpdate(g_core.State(), g_config, controllerAllowed);

    GTAAdapter::MirrorGameRumble(g_core, g_config);
}

void OnInit() {
    if (g_ready || g_shutdown) return;
    g_initAttempted = true;

    g_pauseBridge.Reset();
    g_gameplayBridge.Reset();
    g_inputArbitration.Reset();

    g_gameDir = GameDirectory();
    g_moduleDir = ModuleDirectory();

    // Keep logs/config/dependency beside the ASI. This is what makes a
    // modloader\GInputNext\ or scripts\GInputNext install self-contained.
    LogOpen(JoinPath(g_moduleDir, "GInputNext.log"));

    Log("GInputNext v25 starting for %s", GameTag());
    Log("Plugin-SDK reports: %s", plugin::GetGameVersionName());
    Log("GameDir   = \"%s\"", g_gameDir.c_str());
    Log("ModuleDir = \"%s\"", g_moduleDir.c_str());

    g_configPath = FindConfigPath();
    const auto& ini = g_configPath;
    if (!g_config.Load(ini)) {
        Log("Config not found/readable; using built-in defaults. Expected: \"%s\"", ini.c_str());
    } else {
        Log("Loaded config: \"%s\"", ini.c_str());
    }

    g_inGameConfigUI.Init(g_configPath);

    if (!g_config.enabled) {
        Log("Disabled by config.");
        g_ready = true;
        return;
    }

#if defined(GTASA)
    const bool auditedVersion = plugin::GetGameVersion() == GAME_10US_COMPACT ||
        plugin::GetGameVersion() == GAME_10US_HOODLUM;
#else
    const bool auditedVersion = plugin::GetGameVersion() == GAME_10EN;
#endif
    if (!plugin::IsSupportedGameVersion() || !auditedVersion) {
        Log("Unsupported game executable for this build; plugin remains inert.");
        g_ready = true;
        return;
    }

    // Suppress GTA's native DInput controller, but not its DInput mouse.
    g_nativeDInput.Enable(g_config.suppressNativeGamepad);

    if (!g_core.Init(g_config, g_moduleDir, g_gameDir)) {
        Log("Controller backend initialization failed; restoring native DirectInput pad.");
        g_nativeDInput.Restore();
        g_ready = true;
        return;
    }

    if (!g_inputHook.Install(&UpdatePadsBridge)) {
        Log("CRITICAL: could not install pre-UpdatePads call-site bridge.");
        Log("Restoring native DirectInput because late NewState injection is intentionally not used.");
        g_core.Shutdown();
        g_nativeDInput.Restore();
        g_ready = true;
        return;
    }

    if (!g_modernControlsHook.Install(&g_core, &g_config)) {
        g_config.controlProfile = ControlProfile::Classic;
        Log("WARNING: modern hooks unavailable; Classic selected.");
    }

#if defined(GTA3)
    if (!g_gta3WeaponAimHook.Install(&g_core, &g_config)) {
        Log("WARNING: GTA III scoped weapon-aim hook could not be installed; controller input remains active but GTA III AutoAim compatibility may be unavailable.");
    }
#endif

    g_ready = true;
    Log("GInputNext ready: nativeDInputSuppression=%d updatePadCallSites=%u",
        g_nativeDInput.IsEnabled() ? 1 : 0,
        static_cast<unsigned>(g_inputHook.SiteCount()));
}

void ShutdownRuntime() {
    if (g_shutdown) return;
    g_shutdown = true;

    if (!g_initAttempted) {
        return;
    }

    const bool hadInputHook = g_inputHook.IsInstalled();

    // Restore executable call sites before unloading any code they target.
    g_modernControlsHook.Restore();
#if defined(GTA3)
    g_gta3WeaponAimHook.Restore();
#endif
    g_inputHook.Restore();

    // Only clear the staging buffer if this runtime actually owned the pad
    // pipeline. A disabled/failed plugin must not mutate GTA input on unload.
    if (hadInputHook) {
        GTAAdapter::ClearStagedGamepad();
        g_pauseBridge.Reset();
        g_gameplayBridge.Reset();
    }

    g_inGameConfigUI.Shutdown();

    g_core.Shutdown();

    // Give GTA ownership of its original DirectInput COM pointers again.
    g_nativeDInput.Restore();

    Log("GInputNext shutdown.");
    LogClose();
    g_ready = false;
}

void OnShutdownRw() {
    ShutdownRuntime();
}

class Plugin {
public:
    Plugin() {
        plugin::Events::initRwEvent.Add(OnInit);
        plugin::Events::shutdownRwEvent.Add(OnShutdownRw);
    }

    ~Plugin() {
        // Idempotent fallback for normal FreeLibrary/Mod Loader teardown.
        ShutdownRuntime();
    }
} g_plugin;

} // namespace
} // namespace gin

GIN_EXPORT std::int32_t GIN_CALL GIN_GetAPIVersion() {
    return 4;
}

GIN_EXPORT std::int32_t GIN_CALL GIN_IsConnected() {
    return gin::g_core.IsConnected() ? 1 : 0;
}

GIN_EXPORT std::int32_t GIN_CALL GIN_GetControllerFamily() {
    return static_cast<std::int32_t>(gin::g_core.Family());
}

GIN_EXPORT std::int32_t GIN_CALL GIN_GetState(GIN_State* outState) {
    if (!outState) return 0;

    const auto& s = gin::g_core.State();
    outState->leftX = s.leftX;
    outState->leftY = s.leftY;
    outState->rightX = s.rightX;
    outState->rightY = s.rightY;
    outState->leftTrigger = s.leftTrigger;
    outState->rightTrigger = s.rightTrigger;
    outState->gyroX = s.gyroX;
    outState->gyroY = s.gyroY;
    outState->gyroZ = s.gyroZ;
    outState->buttons = gin::ButtonMask(s);
    outState->family = static_cast<std::int32_t>(s.family);
    outState->connected = s.connected ? 1 : 0;
    return 1;
}

GIN_EXPORT std::int32_t GIN_CALL GIN_Rumble(
    float lowFrequency,
    float highFrequency,
    std::uint32_t milliseconds) {

    return gin::g_core.Rumble(lowFrequency, highFrequency, milliseconds) ? 1 : 0;
}
