#pragma once

#if defined(GTA3) || defined(GTAVC)
#include "CPlayerPed.h"
#endif

namespace gin {

#if defined(GTA3) || defined(GTAVC)
inline bool IsClassicFirstPersonAimWeapon(CPlayerPed* player) {
    if (!player || !player->GetWeapon()) return false;

    const auto weapon = player->GetWeapon()->m_eWeaponType;

#if defined(GTA3)
    // GTA III controller first-person weapon cameras read SniperModeLook*,
    // which is the game's LEFT-stick channel. These weapons must not enter
    // the controller lock-on compatibility path.
    return weapon == WEAPONTYPE_M16 ||
           weapon == WEAPONTYPE_SNIPERRIFLE ||
           weapon == WEAPONTYPE_ROCKETLAUNCHER;
#elif defined(GTAVC)
    // Vice City routes these weapons through MODE_M16_1STPERSON,
    // MODE_SNIPER/MODE_CAMERA, or MODE_ROCKETLAUNCHER.
    return weapon == WEAPONTYPE_M4 ||
           weapon == WEAPONTYPE_RUGER ||
           weapon == WEAPONTYPE_SNIPERRIFLE ||
           weapon == WEAPONTYPE_LASERSCOPE ||
           weapon == WEAPONTYPE_ROCKETLAUNCHER ||
           weapon == WEAPONTYPE_M60 ||
           weapon == WEAPONTYPE_CAMERA;
#endif
}
#endif

} // namespace gin
