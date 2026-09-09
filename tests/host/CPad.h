#pragma once
struct CControllerState {
 short LeftStickX,LeftStickY,RightStickX,RightStickY;
 short LeftShoulder1,LeftShoulder2,RightShoulder1,RightShoulder2;
 short DPadUp,DPadDown,DPadLeft,DPadRight,Start,Select;
 short ButtonSquare,ButtonTriangle,ButtonCross,ButtonCircle,ShockButtonL,ShockButtonR;
};
struct MouseState{float x{},y{};bool wheelUp{},wheelDown{};};
struct CPad {
 CControllerState NewState{},OldState{},PCTempJoyState{};
 short Mode{},DisablePlayerControls{},ShakeDur{},ShakeFreq{};
 bool bDisablePlayerFireWeapon{},bDisablePlayerCycleWeapon{};
 inline static MouseState NewMouseControllerState{};
 static CPad* GetPad(int);
 short GetAccelerate(){return DisablePlayerControls?0:NewState.ButtonCross;}
 short GetBrake(){return DisablePlayerControls?0:NewState.ButtonSquare;}
 short GetHandBrake(){return DisablePlayerControls?0:NewState.RightShoulder1;}
 bool GetTarget(){return !DisablePlayerControls && NewState.RightShoulder1;}
 short GetWeapon(){return DisablePlayerControls?0:NewState.ButtonCircle;}
 short WeaponJustDown(){return GetWeapon() && !OldState.ButtonCircle;}
 bool CycleWeaponLeftJustDown(){return !DisablePlayerControls && NewState.LeftShoulder2 && !OldState.LeftShoulder2;}
 bool CycleWeaponRightJustDown(){return !DisablePlayerControls && NewState.RightShoulder2 && !OldState.RightShoulder2;}
};
inline CPad testPads[2]; inline CPad* CPad::GetPad(int i){return &testPads[i];}
