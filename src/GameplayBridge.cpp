#include "GameplayBridge.h"
#include "Log.h"
#include "plugin.h"
#include "CPad.h"
#include "CPlayerPed.h"
#include "CTimer.h"
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
    lastFrame_ = 0xFFFFFFFFu;
    previousTarget_ = false;
    acquiredTarget_ = false;
    nextRetryFrame_ = 0;
}

void GameplayBridge::AfterPadUpdate(
    const UnifiedState& state,
    const Config& config) {

    if (!state.connected) {
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
