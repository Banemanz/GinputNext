#pragma once
enum {VEHICLE_APPEARANCE_AUTOMOBILE=1,VEHICLE_APPEARANCE_BIKE=2,VEHICLE_APPEARANCE_HELI=3,
 VEHICLE_APPEARANCE_BOAT=4,VEHICLE_APPEARANCE_PLANE=5};
struct CVehicle{unsigned m_nVehicleClass{},m_nVehicleSubClass{},m_nModelIndex{};
 int appearance=1;int GetVehicleAppearance(){return appearance;}};
inline CVehicle testVehicle;inline CVehicle* testCurrentVehicle=nullptr;
