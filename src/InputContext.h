#pragma once
#include "CPad.h"
#include "CMenuManager.h"
#include "common.h"
namespace gin {
inline CVehicle* ControlledVehicle() {
#if defined(GTASA)
    return FindPlayerVehicle(-1, true); // Include RC mission vehicles.
#else
    return FindPlayerVehicle();
#endif
}
inline bool FrontendActive() { return FrontEndMenuManager.m_bMenuActive; }
inline bool PlayerControlsAvailable(const CPad* pad) {
    return pad && pad == CPad::GetPad(0) && !FrontendActive() && !pad->DisablePlayerControls;
}
}
