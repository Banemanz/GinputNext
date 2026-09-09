#include "GTAAdapter.h"
#include "InputContext.h"
#include "ControlActions.h"
#include "plugin.h"
#include "CPad.h"
#include "common.h"
#if defined(GTA3) || defined(GTAVC)
#include "CPlayerPed.h"
#include "ClassicFirstPersonAim.h"
#endif
#include <algorithm>
#include <cmath>
#include <cstring>
#include <cstdint>

namespace gin {
namespace {

static short AxisToPad(float v) {
    const float clamped = std::clamp(v, -1.0f, 1.0f);
    return static_cast<short>(std::lround(clamped * 128.0f));
}

static short TriggerToPad(float v) {
    return static_cast<short>(std::lround(std::clamp(v, 0.0f, 1.0f) * 255.0f));
}

static short Press(bool down) {
    return down ? 255 : 0;
}

static bool modeOwned = false;
static short savedMode = 0;
static void RestoreModernMode(CPad* pad) {
    if (modeOwned && pad) pad->Mode = savedMode;
    modeOwned = false;
}
static void ApplyModernMode(CPad* pad, bool enabled) {
    if (!enabled) { RestoreModernMode(pad); return; }
    if (!modeOwned) { savedMode = pad->Mode; modeOwned = true; }
    pad->Mode = 0; // Native horn/jump/sprint queries use this logical layout.
}
static bool IsModernProfile(const Config& config) {
    return config.controlProfile == ControlProfile::Modern;
}

static bool PlayerInVehicle() {
#if defined(GTASA)
    return ControlledVehicle() != nullptr;
#elif defined(GTA3) || defined(GTAVC)
    return ControlledVehicle() != nullptr;
#else
    return false;
#endif
}

static bool IsControllerTargetHeld(const CPad& pad, const UnifiedState& s, const Config& config) {
    if (IsModernProfile(config)) {
        // Modern on-foot aim is LT/L2.  Do not include LB/RB here: LB is
        // weapon-wheel/drive-by intent in IV/V and RB is cover/handbrake.
        return s.leftTrigger >= 0.30f;
    }

    // Classic III/VC/SA GetTarget semantics use R1/RightShoulder1 for pad
    // modes 0/1/2 and L1/LeftShoulder1 for mode 3.
    return pad.Mode == 3 ? s.lb : s.rb;
}

#if defined(GTASA)
namespace sa {

// GTA SA 1.0 US pad globals recovered from the SA IDB dump. Plugin-SDK's
// public CPad class exposes the pad layout but not these static controller
// preference bytes.
constexpr std::uintptr_t kPadInvertLook4Pad = 0x00B73402;
constexpr std::uintptr_t kPadSniperAimWithRightStick = 0x008CD782;

static bool& PadBool(std::uintptr_t address) {
    return *reinterpret_cast<bool*>(address);
}

static bool policyOwned = false, savedSniper = false, savedInvert = false;
static void RestoreNativeAimPolicy() {
    if (!policyOwned) return;
    PadBool(kPadSniperAimWithRightStick) = savedSniper;
    PadBool(kPadInvertLook4Pad) = savedInvert;
    policyOwned = false;
}
static void ApplyNativeAimPolicy(bool userInvertVertical) {
    if (!policyOwned) {
        savedSniper = PadBool(kPadSniperAimWithRightStick);
        savedInvert = PadBool(kPadInvertLook4Pad);
        policyOwned = true;
    }
    // SA's sniper/RPG/weapon-look code chooses the active stick first and then
    // applies bInvertLook4Pad. Pre-inverting only PCTempJoyState.RightStickY
    // makes right-stick aiming disagree with the retail left-stick fallback.
    //
    // Runtime testing showed the native byte's sign is opposite of the
    // user-facing GInputNext convention after SDL-normalized stick staging:
    //   GInputNext Invert*=0 -> native bInvertLook4Pad=1
    //   GInputNext Invert*=1 -> native bInvertLook4Pad=0
    // This preserves coherent left-stick fallback and right-stick aim without
    // resurrecting the one-stick-only preflip bandaid.
    PadBool(kPadSniperAimWithRightStick) = true;
    PadBool(kPadInvertLook4Pad) = !userInvertVertical;
}

} // namespace sa
#endif

} // namespace

void GTAAdapter::ClearStagedGamepad() {
#if defined(GTASA)
    sa::RestoreNativeAimPolicy();
#endif
    CPad* pad = CPad::GetPad(0);
    if (!pad) return;
    RestoreModernMode(pad);
    std::memset(&pad->PCTempJoyState, 0, sizeof(pad->PCTempJoyState));
}

void GTAAdapter::StageBeforePadUpdate(const UnifiedState& s, const Config& config, bool controllerAllowed) {
    CPad* pad = CPad::GetPad(0);
    if (!pad) return;

    // This is intentionally PCTempJoyState, NOT NewState.
    //
    // GTA's own CPad::UpdatePads later performs the normal state transition:
    //   OldState <- previous NewState
    //   NewState <- reconcile(keyboard, joystick, mouse)
    //
    // Staging here therefore preserves all of the game's normal "pressed",
    // "just pressed", "just released", pause-menu and script semantics.
    auto& d = pad->PCTempJoyState;
    std::memset(&d, 0, sizeof(d));

    if (!s.connected || !controllerAllowed) {
        RestoreModernMode(pad);
#if defined(GTASA)
        sa::RestoreNativeAimPolicy();
#endif
        return;
    }

    const bool modernProfile = IsModernProfile(config) && !FrontendActive();
    const bool modernVehicle = modernProfile && PlayerInVehicle();
    const bool modernOnFoot = modernProfile && !modernVehicle;
    ApplyModernMode(pad, modernProfile);

    d.LeftStickX = AxisToPad(s.leftX);
    d.LeftStickY = AxisToPad(s.leftY);

    float rightY = s.rightY;
    const bool targeting = !PlayerInVehicle() && !FrontendActive() && IsControllerTargetHeld(*pad, s, config);

    // Camera and weapon-aim inversion are independent choices.
    // Exactly one policy is selected for a frame, so enabling both does NOT
    // double-invert while aiming.
    const bool invertVertical =
        targeting ? config.invertAimY : config.invertCameraY;

#if defined(GTASA)
    // Keep the staged SA stick axes in the game's native convention.  Vertical
    // direction belongs in SA's own pad preference byte so every SA aim path
    // (right stick, left-stick fallback, sniper/RPG, and weapon aim) agrees.
    // ApplyNativeAimPolicy handles the native byte's opposite sign convention.
    if (FrontendActive()) sa::RestoreNativeAimPolicy();
    else sa::ApplyNativeAimPolicy(invertVertical);
    d.RightStickX = AxisToPad(s.rightX);
    d.RightStickY = AxisToPad(rightY);
#elif defined(GTA3) || defined(GTAVC)
    if (invertVertical) {
        rightY = -rightY;
    }

    CPlayerPed* player = FindPlayerPed();
    const bool classicFirstPersonAim =
        targeting && IsClassicFirstPersonAimWeapon(player);

    if (classicFirstPersonAim) {
        // III/VC first-person weapon cameras do NOT use the normal right-stick
        // look channel for controller aim. Retail SniperModeLook* reads the
        // LEFT stick. Feeding both channels makes the old generic right-stick
        // look path compete with the weapon camera and visibly kick/glitch it.
        //
        // Modernize only this context: physical right stick -> retail aim
        // channel, and suppress the generic right-stick channel completely.
        // Keep retail left-stick aiming as a fallback when the right stick is
        // centered, which also preserves the games' original behavior.
        const bool useRightStick =
            s.rightX != 0.0f || s.rightY != 0.0f;

        d.LeftStickX = AxisToPad(useRightStick ? s.rightX : s.leftX);
        d.LeftStickY = AxisToPad(useRightStick ? rightY : s.leftY);
        d.RightStickX = 0;
        d.RightStickY = 0;
    } else {
        d.RightStickX = AxisToPad(s.rightX);
        d.RightStickY = AxisToPad(rightY);
    }
#else
    d.RightStickX = AxisToPad(s.rightX);
    d.RightStickY = AxisToPad(rightY);
#endif

    // Native action values keep camera, physics, audio and script readers coherent.
    // Raw physical RT/LT never occupy the classic side-camera trigger slots.
    d.ButtonCross = modernVehicle && !ModernVehicleIsBicycle() ? TriggerToPad(s.rightTrigger) : Press(s.a);
    d.ButtonSquare = modernVehicle ? TriggerToPad(s.leftTrigger) : Press(s.x);
    d.ButtonTriangle = Press(s.y);
    d.ButtonCircle = Press(modernVehicle ? s.lb : (modernProfile ? ModernAttackHeld(s) : s.b));
    // SA primary/secondary vehicle weapon selectors are distinct: Circle / L1.
    d.LeftShoulder1 = Press(modernVehicle ? s.b : (modernOnFoot ? false : s.lb));
    d.RightShoulder1 = modernOnFoot ? Press(s.leftTrigger >= 0.30f) :
        Press(modernVehicle ? (s.rb || (!ModernVehicleIsBicycle() && s.a)) : s.rb);
    // On foot these are weapon-cycle / shift-target action slots, not raw triggers.
    d.LeftShoulder2 = modernOnFoot ? Press(s.lb) : (modernProfile ? 0 : TriggerToPad(s.leftTrigger));
    d.RightShoulder2 = modernOnFoot ? Press(s.rb) : (modernProfile ? 0 : TriggerToPad(s.rightTrigger));
    if (modernVehicle) {
        auto* vehicle = ControlledVehicle();
#if defined(GTA3)
        const bool driveByVehicle = vehicle && vehicle->m_nVehicleClass == 0;
#else
        const int appearance = vehicle ? vehicle->GetVehicleAppearance() : 0;
        const bool driveByVehicle = appearance == VEHICLE_APPEARANCE_AUTOMOBILE ||
            appearance == VEHICLE_APPEARANCE_BIKE || appearance == VEHICLE_APPEARANCE_BOAT;
#endif
        if (driveByVehicle && !ModernVehicleIsAircraft() && s.lb) {
            d.LeftShoulder2 = Press(s.rightX < -0.5f);
            d.RightShoulder2 = Press(s.rightX > 0.5f);
        }
        if (ModernVehicleIsAircraft()) {
            d.LeftShoulder2 = Press(s.lb);
            d.RightShoulder2 = Press(s.rb);
            d.RightShoulder1 = Press(s.x);
            d.ButtonCircle = Press(s.a);
        }
    }

    d.DPadUp    = Press(s.dpadUp);
    d.DPadDown  = Press(s.dpadDown);
    d.DPadLeft  = Press(s.dpadLeft);
    d.DPadRight = Press(s.dpadRight);

    // Critical: SDL START/Options is the game's logical Start field (Pause).
    // Because this is staged before UpdatePads, OldState/NewState edge
    // detection works normally instead of losing the press one frame later.
    d.Start  = Press(s.start);
    d.Select = Press(s.back);

    d.ShockButtonL = Press(s.l3);
    d.ShockButtonR = Press(s.r3);

#if defined(GTASA)
    // Leave SA-only helper fields (chat/walk/radio/vehicle mouse-look) untouched.
#elif defined(GTAVC)
    // Leave VC's extra controller helper fields untouched.
#elif defined(GTA3)
    // Leave GTA III's m_bChatIndicated helper field untouched.
#endif
}

void GTAAdapter::MirrorGameRumble(ControllerCore& core, const Config& config) {
    if (!config.rumbleEnabled || !config.gameRumbleEnabled || !core.IsConnected()) return;

    CPad* pad = CPad::GetPad(0);
    if (!pad) return;

    if (pad->ShakeDur > 0 && pad->ShakeFreq > 0) {
        const float s = std::clamp(
            static_cast<float>(static_cast<unsigned char>(pad->ShakeFreq)) / 255.0f,
            0.0f, 1.0f);
        core.Rumble(s, s, 60);
    }
}

} // namespace gin
