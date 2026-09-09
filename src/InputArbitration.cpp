#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include "InputArbitration.h"
#include "CPad.h"
#include <algorithm>
#include <cmath>
namespace gin {
void InputArbitration::Reset() {
    owner_=Owner::Controller; controllerAnchor_={}; keys_.fill(false);
}
const char* InputArbitration::OwnerName() const {
    return owner_==Owner::Controller ? "Controller" : "Keyboard/mouse";
}
bool InputArbitration::DetectControllerActivity(const UnifiedState& s,const Config& config) {
    if (!s.connected) { controllerAnchor_={}; return false; }
    bool active=false;
    // Accumulating analog movement avoids missing slow movements while an
    // unchanged held control cannot steal ownership back from keyboard/mouse.
    auto axis=[&](float value,float& anchor,float threshold) {
        if (std::fabs(value)<threshold) { anchor=value; return; }
        if (std::fabs(anchor)<threshold || std::fabs(value-anchor)>=0.10f) {
            active=true; anchor=value;
        }
    };
    const float stick=std::clamp(config.controllerWakeStickThreshold,0.05f,0.95f);
    const float trigger=std::clamp(config.controllerWakeTriggerThreshold,0.05f,0.95f);
    axis(s.leftX,controllerAnchor_.leftX,stick); axis(s.leftY,controllerAnchor_.leftY,stick);
    axis(s.rightX,controllerAnchor_.rightX,stick); axis(s.rightY,controllerAnchor_.rightY,stick);
    axis(s.leftTrigger,controllerAnchor_.leftTrigger,trigger);
    axis(s.rightTrigger,controllerAnchor_.rightTrigger,trigger);
    auto button=[&](bool value,bool& old) { active |= value && !old; old=value; };
    button(s.a,controllerAnchor_.a); button(s.b,controllerAnchor_.b);
    button(s.x,controllerAnchor_.x); button(s.y,controllerAnchor_.y);
    button(s.lb,controllerAnchor_.lb); button(s.rb,controllerAnchor_.rb);
    button(s.back,controllerAnchor_.back); button(s.start,controllerAnchor_.start);
    button(s.l3,controllerAnchor_.l3); button(s.r3,controllerAnchor_.r3);
    button(s.dpadUp,controllerAnchor_.dpadUp); button(s.dpadDown,controllerAnchor_.dpadDown);
    button(s.dpadLeft,controllerAnchor_.dpadLeft); button(s.dpadRight,controllerAnchor_.dpadRight);
    button(s.guide,controllerAnchor_.guide); button(s.misc1,controllerAnchor_.misc1);
    return active;
}
bool InputArbitration::DetectKeyboardMouseActivity() {
    bool active=false;
    for (int vk=1;vk<255;++vk) {
        if (vk>=0xC3 && vk<=0xDA) continue; // VK_GAMEPAD_* is not keyboard input.
        const bool down=(GetAsyncKeyState(vk)&0x8000)!=0;
        active |= down && !keys_[vk]; keys_[vk]=down;
    }
    // Last completed native relative sample; screen coordinates are unreliable
    // because the game recenters/locks the cursor.
    const auto& mouse=CPad::NewMouseControllerState;
    active |= std::fabs(mouse.x)+std::fabs(mouse.y)>=1.0f || mouse.wheelUp || mouse.wheelDown;
    return active;
}
bool InputArbitration::Tick(const UnifiedState& controller,const Config& config) {
    DWORD pid=0; GetWindowThreadProcessId(GetForegroundWindow(),&pid);
    const bool focused=pid==GetCurrentProcessId();
    const bool padActivity=DetectControllerActivity(controller,config);
    const bool pcActivity=focused && DetectKeyboardMouseActivity();
    if (!focused && !config.backgroundInput) { owner_=Owner::KeyboardMouse; return false; }
    if (!config.autoSwitchKeyboardMouse) { owner_=Owner::Controller; return controller.connected; }
    if (!controller.connected || pcActivity) owner_=Owner::KeyboardMouse;
    else if (padActivity) owner_=Owner::Controller;
    // Never restore controller ownership merely because a timeout elapsed.
    return owner_==Owner::Controller;
}
}
