#include <array>
#include <vector>
#include <string>
#include <map>
#include <cassert>
#include <iostream>
#include <fstream>
#include <cmath>
#include <sys/mman.h>
#include "Windows.h"
#include "CPad.h"
#include "CTimer.h"
#include "CMenuManager.h"
#include "common.h"
#include "Patch.h"
#include "eVehicleModel.h"
#define private public
#include "ModernControlsHook.h"
#undef private
#include "GTAAdapter.h"
#include "InputArbitration.h"
namespace gin { void Log(const char*,...) {} bool ControllerCore::Rumble(float,float,std::uint32_t){return false;} }
using namespace gin;
int main(){
#if defined(GTASA)
 for(auto a:{0x8CD000u,0xB73000u})assert(mmap(reinterpret_cast<void*>(a),4096,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED_NOREPLACE,-1,0)!=MAP_FAILED);
#endif
 Config c;c.controlProfile=ControlProfile::Modern;ControllerCore core;ModernControlsHook h;
 std::size_t count=0;auto* sites=h.CallSiteTargets(count);assert(count>20);
 for(std::size_t i=0;i<count;++i){auto&b=plugin::patch::code[sites[i].preferredAddress];b[0]=0xE8;
 auto rel=static_cast<std::int32_t>(sites[i].expectedTarget-sites[i].preferredAddress-5);std::memcpy(b.data()+1,&rel,4);}
 auto original=plugin::patch::code;plugin::patch::code[sites[count-1].preferredAddress][0]=0x90;
 assert(!h.Install(&core,&c));assert(!h.IsInstalled());
 for(std::size_t i=0;i<count-1;++i)assert(plugin::patch::code[sites[i].preferredAddress]==original[sites[i].preferredAddress]);
 plugin::patch::code=original;assert(h.Install(&core,&c));
 UnifiedState s;s.connected=true;auto*p=CPad::GetPad(0);
 auto sample=[&](bool allowed=true){++CTimer::m_FrameCounter;h.AfterPadUpdate(s,c,allowed);};
 auto neutral=[&]{s={};s.connected=true;};
 auto stage=[&]{GTAAdapter::StageBeforePadUpdate(s,c,true);p->OldState=p->NewState;p->NewState=p->PCTempJoyState;};
 sample();s.rightTrigger=1;sample();assert(h.FireHeld()&&h.FireJustDown());
 h.AfterPadUpdate(s,c,true);assert(h.FireJustDown());
#if defined(GTASA)
 assert(h.WeaponJustDownBridge(p,nullptr,&testPlayer)==1);
#else
 assert(h.WeaponJustDownBridge(p,nullptr)==true);
#endif
 sample();assert(!h.FireJustDown());
 neutral();s.lb=true;sample();assert(h.CycleWeaponLeftJustDown());s.dpadLeft=true;sample();assert(!h.CycleWeaponLeftJustDown());
 testPlayer.weapon.m_eWeaponType=0;neutral();s.rightTrigger=1;sample();assert(!h.FireHeld());s.b=true;sample();assert(h.FireJustDown());
 testPlayer.weapon.m_eWeaponType=99;neutral();s.leftTrigger=1;s.rightTrigger=1;sample();assert(h.TargetBridge(p,nullptr));
 p->DisablePlayerControls=1;assert(!h.TargetBridge(p,nullptr));assert(!h.CycleWeaponLeftBridge(p,nullptr));
#if defined(GTASA)
 assert(!h.WeaponBridge(p,nullptr,&testPlayer));
#else
 assert(!h.WeaponBridge(p,nullptr));
#endif
 p->DisablePlayerControls=0;
#if defined(GTASA)
 p->bDisablePlayerFireWeapon=true;assert(!h.WeaponBridge(p,nullptr,&testPlayer));p->bDisablePlayerFireWeapon=false;
 p->bDisablePlayerCycleWeapon=true;assert(!h.CycleWeaponLeftBridge(p,nullptr));p->bDisablePlayerCycleWeapon=false;
 for(int i=0;i<4;++i){neutral();sample();if(i==0)s.b=true;if(i==1)s.a=true;if(i==2)s.x=true;if(i==3)s.y=true;sample();
 assert(h.MeleeAttackBridge(p,nullptr,true)==i+1);assert(h.MeleeAttackJustDownBridge(p,nullptr,true)==i+1);
 assert(h.MeleeAttackBridge(p,nullptr,false)==(i==0?1:0));sample();assert(h.MeleeAttackJustDownBridge(p,nullptr,true)==(i==2?3:0));}
#endif
 p->NewState.RightShoulder1=255;sample(false);assert(h.TargetBridge(p,nullptr));
 #if defined(GTA3)
 assert(h.WeaponBridge(p,nullptr)==77); // Explicit native pressure ABI, not SDK bool.
#endif
 testPads[1].NewState.RightShoulder1=255;sample();assert(h.TargetBridge(&testPads[1],nullptr));
 p->Mode=3;neutral();s.rightTrigger=1;stage();assert(p->Mode==0&&p->NewState.ButtonCircle==255&&!p->NewState.RightShoulder2);
 s.rightTrigger=0;s.leftTrigger=.1f;stage();assert(!p->GetTarget());s.leftTrigger=1;s.lb=true;stage();assert(p->GetTarget()&&p->NewState.LeftShoulder2==255);
 testCurrentVehicle=&testVehicle;neutral();s.rightTrigger=1;stage();sample();assert(p->GetAccelerate()==255&&!p->NewState.RightShoulder2&&h.AccelerateBridge(p,nullptr)==255);
 p->DisablePlayerControls=1;assert(!h.AccelerateBridge(p,nullptr));p->DisablePlayerControls=0;
 s.rightTrigger=0;s.a=true;stage();assert(!p->GetAccelerate()&&p->GetHandBrake()==255);
 neutral();s.lb=true;stage();assert(p->NewState.ButtonCircle==255&&!p->NewState.LeftShoulder1);
 neutral();s.b=true;stage();assert(!p->NewState.ButtonCircle&&p->NewState.LeftShoulder1==255);
 neutral();s.lb=true;s.rightX=1;stage();assert(p->NewState.RightShoulder2==255);
#if defined(GTA3)
 testVehicle.m_nModelIndex=MODEL_DODO;
#else
 testVehicle.appearance=VEHICLE_APPEARANCE_HELI;
#endif
 neutral();s.lb=s.rb=s.a=true;stage();sample();assert(p->NewState.LeftShoulder2&&p->NewState.RightShoulder2&&p->NewState.ButtonCircle&&!p->GetHandBrake()&&!h.HandBrakeBridge(p,nullptr));
#if defined(GTASA)
 testVehicle.appearance=2;testVehicle.m_nVehicleSubClass=10;neutral();s.rightTrigger=1;stage();sample();assert(!p->GetAccelerate()&&!h.AccelerateBridge(p,nullptr));
 s.rightTrigger=0;s.a=true;stage();sample();assert(p->GetAccelerate()==255&&h.AccelerateBridge(p,nullptr)==255);assert(!p->GetHandBrake()&&!h.HandBrakeBridge(p,nullptr));
#endif
 FrontEndMenuManager.m_bMenuActive=true;neutral();s.b=s.x=s.rb=true;stage();assert(p->Mode==3&&p->NewState.ButtonCircle==255&&p->NewState.ButtonSquare==255&&p->GetTarget());
 FrontEndMenuManager.m_bMenuActive=false;testCurrentVehicle=nullptr;stage();GTAAdapter::StageBeforePadUpdate(s,c,false);assert(p->Mode==3&&!p->PCTempJoyState.ButtonCircle);
#if defined(GTASA)
 assert(!*reinterpret_cast<bool*>(0x8CD782)&&!*reinterpret_cast<bool*>(0xB73402));
#endif
 h.Restore();assert(plugin::patch::code==original);
 InputArbitration owner;owner.Reset();neutral();s.a=true;assert(owner.Tick(s,c));testKeys['J']=true;assert(!owner.Tick(s,c));testKeys['J']=false;
 for(int i=0;i<200;++i)assert(!owner.Tick(s,c));s.a=false;owner.Tick(s,c);s.b=true;assert(owner.Tick(s,c));
 CPad::NewMouseControllerState.x=2;assert(!owner.Tick(s,c));CPad::NewMouseControllerState.x=0;
 s.b=false;owner.Tick(s,c);s.leftX=.25f;assert(owner.Tick(s,c));testKeys['W']=true;assert(!owner.Tick(s,c));testKeys['W']=false;assert(!owner.Tick(s,c));s.leftX=.3f;assert(!owner.Tick(s,c));s.leftX=.4f;assert(owner.Tick(s,c));
 testPid=2;assert(!owner.Tick(s,c));testPid=1;s={};assert(!owner.Tick(s,c));
 {std::ofstream f("config-test.ini");f<<"[Sticks]\nLeftSensitivity=nan\n[Controls]\nModern=1\nProfile=Classic\n";}
 Config a;assert(a.Load("config-test.ini")&&std::isfinite(a.leftSensitivity)&&a.controlProfile==ControlProfile::Classic);
 a.controlProfile=ControlProfile::Modern;assert(a.Save("config-test.ini"));Config b;assert(b.Load("config-test.ini")&&b.controlProfile==ControlProfile::Modern);assert(!a.Save("/dev/full"));std::remove("config-test.ini");
 std::cout<<"PASS: "<<count<<" hooks; rollback, edges, combat, locks, staging, vehicle contexts, frontend, ownership, config"<<std::endl;
}
