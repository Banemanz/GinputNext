#include "GameplayBridge.h"
#include "Log.h"
#include "plugin.h"
#include "CPad.h"
#include "CPlayerPed.h"
#include "CTimer.h"
#include "common.h"
#if defined(GTA3) || defined(GTAVC)
#include "ClassicFirstPersonAim.h"
#endif

#if defined(GTASA)
#include "CPlayerData.h"
#elif defined(GTAVC)
#include "CCamera.h"
#endif

namespace gin {
namespace {

static CPlayerPed* GetActivePlayerPed() {
#if defined(GTASA)
    return FindPlayerPed(-1);
#elif defined(GTAVC) || defined(GTA3)
    return FindPlayerPed();
#else
    return nullptr;
#endif
}

#if defined(GTAVC)
static bool IsControllerTargetHeld(const CPad& pad, const UnifiedState& state) {
    // Same target-button convention used by GTAAdapter: modes 0/1/2 use R1,
    // mode 3 uses L1. Keep the camera override controller-only so native
    // keyboard/mouse aiming remains untouched.
    return pad.Mode == 3 ? state.lb : state.rb;
}
#endif

} // namespace

#if defined(GTAVC)
void GameplayBridge::SetClassicMouseAimOverride(bool enabled) {
    if (enabled) {
        if (!classicMouseAimOverrideActive_) {
            savedMouse3rdPerson_ = CCamera::m_bUseMouse3rdPerson;
            classicMouseAimOverrideActive_ = true;
        }

        // VC's stock ProcessPlayerWeapon() immediately clears a lock-on while this
        // PC mouse-camera flag is true. Holding controller Target therefore
        // enters VC's stock non-mouse/console lock-on path.
        CCamera::m_bUseMouse3rdPerson = false;
        return;
    }

    if (classicMouseAimOverrideActive_) {
        CCamera::m_bUseMouse3rdPerson = savedMouse3rdPerson_;
        classicMouseAimOverrideActive_ = false;
    }
}
#endif

void GameplayBridge::Reset() {
#if defined(GTAVC)
    SetClassicMouseAimOverride(false);
#endif
    lastFrame_ = 0xFFFFFFFFu;
    previousTarget_ = false;
    acquiredTarget_ = false;
    nextRetryFrame_ = 0;
}

void GameplayBridge::AfterPadUpdate(
    const UnifiedState& state,
    const Config& config) {

    if (!state.connected) {
#if defined(GTAVC)
        SetClassicMouseAimOverride(false);
#endif
        previousTarget_ = false;
        acquiredTarget_ = false;
        return;
    }

    CPad* pad = CPad::GetPad(0);
    if (!pad) {
#if defined(GTAVC)
        SetClassicMouseAimOverride(false);
#endif
        return;
    }

    CPlayerPed* player = GetActivePlayerPed();

#if defined(GTAVC)
    // VC's persistent compatibility override is only for classic lock-on
    // weapons. Its first-person weapon family owns its camera completely.
    const bool controllerTargetHeld = IsControllerTargetHeld(*pad, state);
    const bool classicFirstPersonAim =
        controllerTargetHeld && IsClassicFirstPersonAimWeapon(player);
    SetClassicMouseAimOverride(
        config.autoAim && controllerTargetHeld && !classicFirstPersonAim);
#endif

    const unsigned int frame = CTimer::m_FrameCounter;
    if (frame == lastFrame_) {
        return;
    }
    lastFrame_ = frame;

    const bool targeting = pad->GetTarget();
    const bool targetPressed = targeting && !previousTarget_;
    previousTarget_ = targeting;

    if (!config.autoAim) {
        acquiredTarget_ = false;
        return;
    }

    if (!player) return;

#if defined(GTA3) || defined(GTAVC)
    // These games' first-person weapon cameras use a separate controller aim
    // pipeline. Never inject lock-on acquisition into that path. GTAAdapter
    // also remaps the physical right stick to the retail left-stick aim channel
    // while Target is held.
    if (IsClassicFirstPersonAimWeapon(player)) {
        acquiredTarget_ = false;
        nextRetryFrame_ = frame;
        return;
    }
#endif

#if defined(GTASA)
    // SA exposes a free-aim flag on the player-data structure. Old GInput's
    // FreeAim=0 keeps the pad in lock-on mode; AutoAim=1 mirrors that intent.
    // Only force the flag while the physical target control is actually held.
    if (targeting && player->m_pPlayerData) {
        player->m_pPlayerData->m_bFreeAiming = false;
    }
#endif

    if (!targeting) {
        acquiredTarget_ = false;
        nextRetryFrame_ = frame;
        return;
    }

    // Acquire immediately on the target-button edge. If the weapon/camera task
    // was not quite ready on that exact frame, retry at a low cadence while
    // held until the stock game reports a successful lock.
    if (targetPressed || (!acquiredTarget_ && frame >= nextRetryFrame_)) {
        acquiredTarget_ = player->FindWeaponLockOnTarget();
        nextRetryFrame_ = frame + 6;

        if (config.debugInput) {
            Log("AutoAim: frame=%u targeting=1 pressed=%d acquired=%d retryAt=%u",
                frame,
                targetPressed ? 1 : 0,
                acquiredTarget_ ? 1 : 0,
                nextRetryFrame_);
        }
    }
}

} // namespace gin
