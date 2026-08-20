#include "GameplayBridge.h"
#include "Log.h"
#include "plugin.h"
#include "CPad.h"
#include "CPlayerPed.h"
#include "CTimer.h"
#include "CCamera.h"
#include "common.h"

#if defined(GTASA)
#include "CPlayerData.h"
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

} // namespace

void GameplayBridge::Reset() {
#if defined(GTA3) || defined(GTAVC)
    if (legacyMouseAimOverride_) {
        CCamera::m_bUseMouse3rdPerson = savedMouse3rdPerson_;
    }
#endif

    lastFrame_ = 0xFFFFFFFFu;
    previousTarget_ = false;
    acquiredTarget_ = false;
    nextRetryFrame_ = 0;
    legacyMouseAimOverride_ = false;
    savedMouse3rdPerson_ = true;
}

void GameplayBridge::AfterPadUpdate(
    const UnifiedState& state,
    const Config& config) {

    if (!state.connected) {
#if defined(GTA3) || defined(GTAVC)
        if (legacyMouseAimOverride_) {
            CCamera::m_bUseMouse3rdPerson = savedMouse3rdPerson_;
            legacyMouseAimOverride_ = false;
        }
#endif
        previousTarget_ = false;
        acquiredTarget_ = false;
        return;
    }

    CPad* pad = CPad::GetPad(0);
    if (!pad) return;

    const unsigned int frame = CTimer::m_FrameCounter;
    if (frame == lastFrame_) {
        return;
    }
    lastFrame_ = frame;

    const bool targeting = pad->GetTarget();
    const bool targetPressed = targeting && !previousTarget_;
    previousTarget_ = targeting;

#if defined(GTA3) || defined(GTAVC)
    // III/VC's stock player-weapon code only enters the controller lock-on
    // branch when the PC mouse third-person camera mode is disabled. SDL can
    // correctly deliver Target while the game still believes mouse-look owns
    // aiming, which makes a direct lock-on attempt appear to do nothing.
    //
    // Temporarily hand aiming ownership to the stock controller branch while
    // Target is held, then restore the user's previous mouse-camera mode.
    if (config.autoAim && targeting) {
        if (!legacyMouseAimOverride_) {
            savedMouse3rdPerson_ = CCamera::m_bUseMouse3rdPerson;
            legacyMouseAimOverride_ = true;

            if (config.debugInput) {
                Log("LegacyAutoAim: controller target took camera aim ownership; savedMouse3rdPerson=%d",
                    savedMouse3rdPerson_ ? 1 : 0);
            }
        }
        CCamera::m_bUseMouse3rdPerson = false;
    } else if (legacyMouseAimOverride_) {
        CCamera::m_bUseMouse3rdPerson = savedMouse3rdPerson_;
        legacyMouseAimOverride_ = false;

        if (config.debugInput) {
            Log("LegacyAutoAim: restored mouse third-person camera mode=%d",
                savedMouse3rdPerson_ ? 1 : 0);
        }
    }
#endif

    if (!config.autoAim) {
        acquiredTarget_ = false;
        return;
    }

    CPlayerPed* player = GetActivePlayerPed();
    if (!player) return;

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
