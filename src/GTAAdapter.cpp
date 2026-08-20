#include "GTAAdapter.h"
#include "plugin.h"
#include "CPad.h"
#if defined(GTA3) || defined(GTAVC)
#include "CPlayerPed.h"
#include "ClassicFirstPersonAim.h"
#include "common.h"
#endif
#include <algorithm>
#include <cmath>
#include <cstring>

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

static bool IsControllerTargetHeld(const CPad& pad, const UnifiedState& s) {
    // Classic III/VC/SA GetTarget semantics use R1/RightShoulder1 for pad
    // modes 0/1/2 and L1/LeftShoulder1 for mode 3.
    return pad.Mode == 3 ? s.lb : s.rb;
}

} // namespace

void GTAAdapter::ClearStagedGamepad() {
    CPad* pad = CPad::GetPad(0);
    if (!pad) return;
    std::memset(&pad->PCTempJoyState, 0, sizeof(pad->PCTempJoyState));
}

void GTAAdapter::StageBeforePadUpdate(const UnifiedState& s, const Config& config) {
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

    if (!s.connected) return;

    d.LeftStickX = AxisToPad(s.leftX);
    d.LeftStickY = AxisToPad(s.leftY);

    float rightY = s.rightY;
    const bool targeting = IsControllerTargetHeld(*pad, s);

    // Camera and weapon-aim inversion are independent choices.
    // Exactly one policy is selected for a frame, so enabling both does NOT
    // double-invert while aiming.
    const bool invertVertical =
        targeting ? config.invertAimY : config.invertCameraY;

    if (invertVertical) {
        rightY = -rightY;
    }

#if defined(GTA3) || defined(GTAVC)
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

    // SDL logical layout -> GTA's PlayStation-named logical controller state:
    // A=Cross, B=Circle, X=Square, Y=Triangle.
    d.ButtonCross    = Press(s.a);
    d.ButtonCircle   = Press(s.b);
    d.ButtonSquare   = Press(s.x);
    d.ButtonTriangle = Press(s.y);

    d.LeftShoulder1  = Press(s.lb);
    d.LeftShoulder2  = TriggerToPad(s.leftTrigger);
    d.RightShoulder1 = Press(s.rb);
    d.RightShoulder2 = TriggerToPad(s.rightTrigger);

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
