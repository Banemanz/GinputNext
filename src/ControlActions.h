#pragma once
#include "UnifiedState.h"
#include "InputContext.h"
#include "CPlayerPed.h"
#include "CVehicle.h"
#if defined(GTA3)
#include "eVehicleModel.h"
#endif
namespace gin {
inline bool ModernVehicleIsAircraft() {
    auto* vehicle = ControlledVehicle();
    if (!vehicle) return false;
#if defined(GTA3)
    return vehicle->m_nModelIndex == MODEL_DODO;
#else
    const auto appearance = vehicle->GetVehicleAppearance();
    return appearance == VEHICLE_APPEARANCE_HELI || appearance == VEHICLE_APPEARANCE_PLANE;
#endif
}
inline bool ModernVehicleIsBicycle() {
#if defined(GTASA)
    auto* vehicle = ControlledVehicle();
    // CBmx constructor 0x6BF85E writes 10 at CVehicle+0x594 in the supplied IDB.
    return vehicle && vehicle->m_nVehicleSubClass == 10;
#else
    return false;
#endif
}
inline bool EquippedWeaponIsMelee() {
    auto* player = FindPlayerPed();
    if (!player || !player->GetWeapon()) return false;
    const auto type = player->GetWeapon()->m_eWeaponType;
#if defined(GTA3)
    return type == WEAPONTYPE_UNARMED || type == WEAPONTYPE_BASEBALLBAT;
#elif defined(GTAVC)
    return type <= WEAPONTYPE_CHAINSAW;
#else
    return type <= WEAPONTYPE_CANE;
#endif
}
inline bool ModernAttackHeld(const UnifiedState& state) {
    return EquippedWeaponIsMelee() ? state.b : state.rightTrigger >= 0.30f;
}
}
