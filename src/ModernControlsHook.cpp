#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include "plugin.h"
#include "Patch.h"
#include "CPad.h"
#include "CPlayerPed.h"
#include "CVehicle.h"
#include "common.h"
#include "ModernControlsHook.h"
#include "Log.h"
#include "InputContext.h"
#include "ControlActions.h"
#include "CTimer.h"
#include <cstring>
#include <algorithm>
#include <cmath>

namespace gin {
namespace {

constexpr float kTriggerDigitalThreshold = 0.30f;

#if defined(GTA3)
// IDB-audited player-control call sites only. v21 scanned every CALL to broad
// CPad methods and therefore altered camera/audio/script/animation users too.
// These sites are restricted to vehicle input and on-foot weapon control paths.
constexpr ModernControlsHook::CallSiteTarget kCallSiteTargets[] = {
    { ModernControlsHook::Action::WeaponJustDown, 0x004F1D49, 0x00493700, "PlayerControlSniper::WeaponJustDown" },
    { ModernControlsHook::Action::Weapon,         0x004F1E49, 0x004936C0, "PlayerControlM16::GetWeapon" },
    { ModernControlsHook::Action::Weapon,         0x004F203A, 0x004936C0, "ProcessPlayerWeapon::GetWeapon#1" },
    { ModernControlsHook::Action::WeaponJustDown, 0x004F2086, 0x00493700, "ProcessPlayerWeapon::WeaponJustDown#1" },
    { ModernControlsHook::Action::Weapon,         0x004F20AB, 0x004936C0, "ProcessPlayerWeapon::GetWeapon#2" },
    { ModernControlsHook::Action::WeaponJustDown, 0x004F20DB, 0x00493700, "ProcessPlayerWeapon::WeaponJustDown#2" },
    { ModernControlsHook::Action::Target,         0x004F1D2D, 0x00493970, "PlayerControlSniper::GetTarget" },
    { ModernControlsHook::Action::Target,         0x004F1E2D, 0x00493970, "PlayerControlM16::GetTarget" },
    { ModernControlsHook::Action::Target,         0x004F2157, 0x00493970, "ProcessPlayerWeapon::GetTarget" },
    { ModernControlsHook::Action::CycleWeaponRight, 0x004F2327, 0x00493940, "ProcessWeaponSwitch::CycleWeaponRight" },
    { ModernControlsHook::Action::CycleWeaponLeft,  0x004F23E2, 0x00493910, "ProcessWeaponSwitch::CycleWeaponLeft" },

    { ModernControlsHook::Action::HandBrake,      0x0053B6CA, 0x00493560, "Automobile::GetHandBrake" },
    { ModernControlsHook::Action::Brake,          0x0053B94E, 0x004935A0, "Automobile::GetBrake" },
    { ModernControlsHook::Action::Accelerate,     0x0053B962, 0x00493780, "Automobile::GetAccelerate" },
    { ModernControlsHook::Action::Brake,          0x0053EC9D, 0x004935A0, "Boat::GetBrake" },
    { ModernControlsHook::Action::Accelerate,     0x0053ED63, 0x00493780, "Boat::GetAccelerate" },
    { ModernControlsHook::Action::Brake,          0x00553355, 0x004935A0, "FlyingControl::GetBrake#1" },
    { ModernControlsHook::Action::Accelerate,     0x00553367, 0x00493780, "FlyingControl::GetAccelerate#1" },
    { ModernControlsHook::Action::Brake,          0x0055432E, 0x004935A0, "FlyingControl::GetBrake#2" },
    { ModernControlsHook::Action::Accelerate,     0x00554358, 0x00493780, "FlyingControl::GetAccelerate#2" },
    // Additional player/camera readers verified against the supplied IDBs.
    { ModernControlsHook::Action::WeaponJustDown, 0x0045D60C, 0x00493700, "FightCamera::WeaponJustDown" },
    { ModernControlsHook::Action::Weapon, 0x0045E98A, 0x004936C0, "FollowPedCamera::Weapon" },
    { ModernControlsHook::Action::Target, 0x00469927, 0x00493970, "CamControl::Target" },
};
#elif defined(GTAVC)
constexpr ModernControlsHook::CallSiteTarget kCallSiteTargets[] = {
    { ModernControlsHook::Action::Weapon,         0x00534B02, 0x004AA830, "ProcessPlayerWeapon::GetWeapon#1" },
    { ModernControlsHook::Action::WeaponJustDown, 0x00534BBF, 0x004AA7B0, "ProcessPlayerWeapon::WeaponJustDown#1" },
    { ModernControlsHook::Action::WeaponJustDown, 0x00534C3D, 0x004AA7B0, "ProcessPlayerWeapon::WeaponJustDown#2" },
    { ModernControlsHook::Action::Weapon,         0x00534C6B, 0x004AA830, "ProcessPlayerWeapon::GetWeapon#2" },
    { ModernControlsHook::Action::WeaponJustDown, 0x00534C9E, 0x004AA7B0, "ProcessPlayerWeapon::WeaponJustDown#3" },
    { ModernControlsHook::Action::Target,         0x00534CDF, 0x004AA4D0, "ProcessPlayerWeapon::GetTarget" },
    { ModernControlsHook::Action::Target,         0x0053536A, 0x004AA4D0, "PlayerControlM16::GetTarget" },
    { ModernControlsHook::Action::Weapon,         0x0053538F, 0x004AA830, "PlayerControlM16::GetWeapon" },
    { ModernControlsHook::Action::Target,         0x0053560A, 0x004AA4D0, "PlayerControlSniper::GetTarget" },
    { ModernControlsHook::Action::WeaponJustDown, 0x00535658, 0x004AA7B0, "PlayerControlSniper::WeaponJustDown" },
    { ModernControlsHook::Action::Weapon,         0x00535754, 0x004AA830, "PlayerControlSniper::GetWeapon" },
    { ModernControlsHook::Action::CycleWeaponRight, 0x005345F8, 0x004AA530, "ProcessWeaponSwitch::CycleWeaponRight" },
    { ModernControlsHook::Action::CycleWeaponLeft,  0x005346B2, 0x004AA560, "ProcessWeaponSwitch::CycleWeaponLeft" },

    { ModernControlsHook::Action::HandBrake,      0x00588E6C, 0x004AA9B0, "Automobile::GetHandBrake" },
    { ModernControlsHook::Action::Brake,          0x005890ED, 0x004AA960, "Automobile::GetBrake#1" },
    { ModernControlsHook::Action::Accelerate,     0x00589100, 0x004AA760, "Automobile::GetAccelerate#1" },
    { ModernControlsHook::Action::Accelerate,     0x0058918A, 0x004AA760, "Automobile::GetAccelerate#2" },
    { ModernControlsHook::Action::Brake,          0x005891B6, 0x004AA960, "Automobile::GetBrake#2" },
    { ModernControlsHook::Action::Accelerate,     0x005891DE, 0x004AA760, "Automobile::GetAccelerate#3" },
    { ModernControlsHook::Action::Brake,          0x0058920B, 0x004AA960, "Automobile::GetBrake#3" },
    { ModernControlsHook::Action::Brake,          0x005A4BED, 0x004AA960, "Boat::GetBrake" },
    { ModernControlsHook::Action::Accelerate,     0x005A4CAF, 0x004AA760, "Boat::GetAccelerate" },
    { ModernControlsHook::Action::Brake,          0x005B5DA6, 0x004AA960, "FlyingControl::GetBrake#1" },
    { ModernControlsHook::Action::Accelerate,     0x005B5DB8, 0x004AA760, "FlyingControl::GetAccelerate#1" },
    { ModernControlsHook::Action::Brake,          0x005B6A3F, 0x004AA960, "FlyingControl::GetBrake#2" },
    { ModernControlsHook::Action::Accelerate,     0x005B6A51, 0x004AA760, "FlyingControl::GetAccelerate#2" },
    // Additional player/camera readers verified against the supplied IDBs.
    { ModernControlsHook::Action::WeaponJustDown, 0x0047F2AA, 0x004AA7B0, "FightCamera::WeaponJustDown" },
    { ModernControlsHook::Action::Weapon, 0x0047D4F3, 0x004AA830, "FollowPedCamera::Weapon" },
    { ModernControlsHook::Action::Target, 0x00472155, 0x004AA4D0, "CamControl::Target" },
    { ModernControlsHook::Action::Target, 0x0047358C, 0x004AA4D0, "CamControl::Target" },
    { ModernControlsHook::Action::HandBrake, 0x0060A5A9, 0x004AA9B0, "Bike::HandBrake" },
    { ModernControlsHook::Action::Brake, 0x0060A77D, 0x004AA960, "Bike::Brake" },
    { ModernControlsHook::Action::Accelerate, 0x0060A790, 0x004AA760, "Bike::Accelerate" },
    { ModernControlsHook::Action::Accelerate, 0x0060A7F7, 0x004AA760, "Bike::Accelerate" },
    { ModernControlsHook::Action::Brake, 0x0060A823, 0x004AA960, "Bike::Brake" },
    { ModernControlsHook::Action::Accelerate, 0x0060A84B, 0x004AA760, "Bike::Accelerate" },
    { ModernControlsHook::Action::Brake, 0x0060A878, 0x004AA960, "Bike::Brake" },
};
#elif defined(GTASA)
constexpr ModernControlsHook::CallSiteTarget kCallSiteTargets[] = {
    // On-foot weapon/aim task sites. Do not patch pickups, scripts, camera-only
    // queries, AI reaction tasks, or cutscene/animation helpers.
    { ModernControlsHook::Action::WeaponJustDown,      0x00685A52, 0x00540250, "OnFootWeapon::WeaponJustDown#1" },
    { ModernControlsHook::Action::Target,              0x00685ABA, 0x00540670, "OnFootWeapon::GetTarget#1" },
    { ModernControlsHook::Action::Target,              0x00685BAC, 0x00540670, "OnFootWeapon::GetTarget#2" },
    { ModernControlsHook::Action::Target,              0x00685C03, 0x00540670, "OnFootWeapon::GetTarget#3" },
    { ModernControlsHook::Action::MeleeAttackJustDown, 0x00685C26, 0x00540390, "OnFootWeapon::MeleeAttackJustDown#1" },
    { ModernControlsHook::Action::Target,              0x00685CD4, 0x00540670, "OnFootWeapon::GetTarget#4" },
    { ModernControlsHook::Action::Target,              0x00685E1E, 0x00540670, "OnFootWeapon::GetTarget#5" },
    { ModernControlsHook::Action::MeleeAttackJustDown, 0x00685E35, 0x00540390, "OnFootWeapon::MeleeAttackJustDown#2" },
    { ModernControlsHook::Action::MeleeAttack,         0x00685F12, 0x00540340, "OnFootWeapon::GetMeleeAttack" },
    { ModernControlsHook::Action::WeaponJustDown,      0x006861AF, 0x00540250, "OnFootWeapon::WeaponJustDown#2" },
    { ModernControlsHook::Action::Weapon,              0x0068628E, 0x00540180, "OnFootWeapon::GetWeapon#1" },
    { ModernControlsHook::Action::Target,              0x0068630A, 0x00540670, "OnFootWeapon::GetTarget#6" },
    { ModernControlsHook::Action::Target,              0x0068644F, 0x00540670, "OnFootWeapon::GetTarget#7" },
    { ModernControlsHook::Action::WeaponJustDown,      0x006864CE, 0x00540250, "OnFootWeapon::WeaponJustDown#3" },
    { ModernControlsHook::Action::WeaponJustDown,      0x006865D2, 0x00540250, "OnFootWeapon::WeaponJustDown#4" },
    { ModernControlsHook::Action::Target,              0x00686899, 0x00540670, "OnFootWeapon::GetTarget#8" },
    { ModernControlsHook::Action::Target,              0x00686EBE, 0x00540670, "OnFootWeapon::GetTarget#9" },
    { ModernControlsHook::Action::Weapon,              0x00686FC2, 0x00540180, "OnFootWeapon::GetWeapon#2" },
    { ModernControlsHook::Action::Target,              0x006876E7, 0x00540670, "PlayerControlFighter::GetTarget#1" },
    { ModernControlsHook::Action::Target,              0x006877C8, 0x00540670, "PlayerControlFighter::GetTarget#2" },
    { ModernControlsHook::Action::Target,              0x00688047, 0x00540670, "PlayerControlDucked::GetTarget" },
    { ModernControlsHook::Action::CycleWeaponRight,    0x0060D8BF, 0x00540640, "ProcessWeaponSwitch::CycleWeaponRight" },
    { ModernControlsHook::Action::CycleWeaponLeft,     0x0060DA7E, 0x00540610, "ProcessWeaponSwitch::CycleWeaponLeft" },

    // Vehicle control sites only. Audio, AI, enter-car, camera modifier, pickup,
    // script and plane-weapon status call sites are intentionally not patched.
    { ModernControlsHook::Action::HandBrake,  0x006AD77F, 0x00540040, "Automobile::GetHandBrake" },
    { ModernControlsHook::Action::Accelerate, 0x006AD992, 0x005403F0, "Automobile::GetAccelerate#1" },
    { ModernControlsHook::Action::Brake,      0x006AD9A5, 0x00540080, "Automobile::GetBrake#1" },
    { ModernControlsHook::Action::Accelerate, 0x006ADAD7, 0x005403F0, "Automobile::GetAccelerate#2" },
    { ModernControlsHook::Action::Brake,      0x006ADB03, 0x00540080, "Automobile::GetBrake#2" },
    { ModernControlsHook::Action::Accelerate, 0x006ADB2B, 0x005403F0, "Automobile::GetAccelerate#3" },
    { ModernControlsHook::Action::Brake,      0x006ADB52, 0x00540080, "Automobile::GetBrake#3" },
    { ModernControlsHook::Action::HandBrake,  0x006BE368, 0x00540040, "Bike::GetHandBrake" },
    { ModernControlsHook::Action::Accelerate, 0x006BE6E4, 0x005403F0, "Bike::GetAccelerate#1" },
    { ModernControlsHook::Action::Brake,      0x006BE6F7, 0x00540080, "Bike::GetBrake#1" },
    { ModernControlsHook::Action::Accelerate, 0x006BE748, 0x005403F0, "Bike::GetAccelerate#2" },
    { ModernControlsHook::Action::Brake,      0x006BE774, 0x00540080, "Bike::GetBrake#2" },
    { ModernControlsHook::Action::Accelerate, 0x006BE7A5, 0x005403F0, "Bike::GetAccelerate#3" },
    { ModernControlsHook::Action::Brake,      0x006BE7CC, 0x00540080, "Bike::GetBrake#3" },
    { ModernControlsHook::Action::Accelerate, 0x006C484B, 0x005403F0, "Heli::GetAccelerate" },
    { ModernControlsHook::Action::Brake,      0x006C4855, 0x00540080, "Heli::GetBrake" },
    { ModernControlsHook::Action::HandBrake,  0x006CB2B6, 0x00540040, "Plane::GetHandBrake" },
    { ModernControlsHook::Action::Accelerate, 0x006CB2EF, 0x005403F0, "Plane::GetAccelerate#1" },
    { ModernControlsHook::Action::Brake,      0x006CB302, 0x00540080, "Plane::GetBrake#1" },
    { ModernControlsHook::Action::Brake,      0x006CB61E, 0x00540080, "Plane::GetBrake#2" },
    { ModernControlsHook::Action::Brake,      0x006CB644, 0x00540080, "Plane::GetBrake#3" },
    { ModernControlsHook::Action::Brake,      0x006CB66F, 0x00540080, "Plane::GetBrake#4" },
    { ModernControlsHook::Action::Brake,      0x006CB6C5, 0x00540080, "Plane::GetBrake#5" },
    { ModernControlsHook::Action::Accelerate, 0x006CB6DB, 0x005403F0, "Plane::GetAccelerate#2" },
    { ModernControlsHook::Action::HandBrake,  0x006DC491, 0x00540040, "VehicleBoatControl::GetHandBrake#1" },
    { ModernControlsHook::Action::HandBrake,  0x006DCCA5, 0x00540040, "VehicleBoatControl::GetHandBrake#2" },
    { ModernControlsHook::Action::Brake,      0x006F0A38, 0x00540080, "Boat::GetBrake" },
    { ModernControlsHook::Action::Accelerate, 0x006F0AC9, 0x005403F0, "Boat::GetAccelerate" },
    { ModernControlsHook::Action::Accelerate, 0x006F87D5, 0x005403F0, "Train::GetAccelerate#1" },
    { ModernControlsHook::Action::Brake,      0x006F87DF, 0x00540080, "Train::GetBrake#1" },
    { ModernControlsHook::Action::Brake,      0x006F880F, 0x00540080, "Train::GetBrake#2" },
    { ModernControlsHook::Action::Accelerate, 0x006F8827, 0x005403F0, "Train::GetAccelerate#2" },
    { ModernControlsHook::Action::Brake,      0x006F8842, 0x00540080, "Train::GetBrake#3" },
    { ModernControlsHook::Action::Accelerate, 0x006F885C, 0x005403F0, "Train::GetAccelerate#3" },
    // Additional player/camera readers verified against the supplied IDBs.
    { ModernControlsHook::Action::Weapon, 0x005224DB, 0x00540180, "AimWeaponCamera::Weapon" },
    { ModernControlsHook::Action::Weapon, 0x00522625, 0x00540180, "AimWeaponCamera::Weapon" },
};
#else
constexpr ModernControlsHook::CallSiteTarget kCallSiteTargets[] = {};
#endif

} // namespace

ModernControlsHook* ModernControlsHook::active_ = nullptr;

const ModernControlsHook::CallSiteTarget* ModernControlsHook::CallSiteTargets(std::size_t& count) {
    count = sizeof(kCallSiteTargets) / sizeof(kCallSiteTargets[0]);
    return count ? kCallSiteTargets : nullptr;
}

bool ModernControlsHook::Install(const ControllerCore* core, const Config* config) {
    if (!sites_.empty()) return true;
    if (!core || !config || active_) return false;

    core_ = core;
    config_ = config;

    std::size_t count = 0;
    const CallSiteTarget* targets = CallSiteTargets(count);
    if (!targets || count == 0) {
        Log("ModernControls: no audited call-site targets for this game build.");
        core_ = nullptr;
        config_ = nullptr;
        return false;
    }

    unsigned patched = 0;
    for (std::size_t i = 0; i < count; ++i) {
        patched += PatchKnownCallSite(targets[i]) ? 1u : 0u;
    }

    if (patched != count) {
        Restore();
        Log("ModernControls: incomplete installation rolled back; modern profile remains unavailable.");
        core_ = nullptr;
        config_ = nullptr;
        sites_.clear();
        return false;
    }

    active_ = this;
    Log("ModernControls installed: %u/%u audited player-control call site(s).",
        patched, static_cast<unsigned>(count));
    return true;
}

bool ModernControlsHook::PatchKnownCallSite(const CallSiteTarget& target) {
    void* bridge = nullptr;
    switch (target.action) {
    case Action::Accelerate: bridge = reinterpret_cast<void*>(&ModernControlsHook::AccelerateBridge); break;
    case Action::Brake: bridge = reinterpret_cast<void*>(&ModernControlsHook::BrakeBridge); break;
    case Action::HandBrake: bridge = reinterpret_cast<void*>(&ModernControlsHook::HandBrakeBridge); break;
    case Action::Target: bridge = reinterpret_cast<void*>(&ModernControlsHook::TargetBridge); break;
    case Action::CycleWeaponLeft: bridge = reinterpret_cast<void*>(&ModernControlsHook::CycleWeaponLeftBridge); break;
    case Action::CycleWeaponRight: bridge = reinterpret_cast<void*>(&ModernControlsHook::CycleWeaponRightBridge); break;
#if defined(GTA3) || defined(GTAVC)
    case Action::Weapon: bridge = reinterpret_cast<void*>(&ModernControlsHook::WeaponBridge); break;
    case Action::WeaponJustDown: bridge = reinterpret_cast<void*>(&ModernControlsHook::WeaponJustDownBridge); break;
#endif
#if defined(GTASA)
    case Action::Weapon: bridge = reinterpret_cast<void*>(&ModernControlsHook::WeaponBridge); break;
    case Action::WeaponJustDown: bridge = reinterpret_cast<void*>(&ModernControlsHook::WeaponJustDownBridge); break;
    case Action::MeleeAttack: bridge = reinterpret_cast<void*>(&ModernControlsHook::MeleeAttackBridge); break;
    case Action::MeleeAttackJustDown: bridge = reinterpret_cast<void*>(&ModernControlsHook::MeleeAttackJustDownBridge); break;
#endif
    default: break;
    }

    if (!bridge || !target.preferredAddress) return false;

    Site site{};
    site.action = target.action;
    site.preferredAddress = target.preferredAddress;
    site.name = target.name;
    plugin::patch::GetRaw(site.preferredAddress, site.original.data(), site.original.size(), true);
    std::int32_t displacement = 0;
    std::memcpy(&displacement, site.original.data() + 1, sizeof(displacement));
    const auto actual = plugin::GetGlobalAddress(site.preferredAddress);
    if (site.original[0] != 0xE8 || actual + 5 + displacement !=
        plugin::GetGlobalAddress(target.expectedTarget)) {
        Log("ModernControls: rejected mismatched/modified site %s at 0x%08lX",
            target.name, static_cast<unsigned long>(site.preferredAddress));
        return false;
    }
    plugin::patch::RedirectCall(site.preferredAddress, bridge, true);
    sites_.push_back(site);

    Log("ModernControls: patched %s at 0x%08lX",
        target.name ? target.name : "call site",
        static_cast<unsigned long>(target.preferredAddress));
    return true;
}

void ModernControlsHook::Restore() {
    for (const auto& site : sites_) {
        plugin::patch::SetRaw(
            site.preferredAddress,
            const_cast<unsigned char*>(site.original.data()),
            site.original.size(),
            true);
    }

    if (!sites_.empty()) {
        Log("ModernControls restored: %u call site(s).", static_cast<unsigned>(sites_.size()));
    }

    sites_.clear();
    core_ = nullptr;
    config_ = nullptr;
    haveFrame_ = false;
    current_ = {};
    previous_ = {};
    controllerAllowed_ = true;
    if (active_ == this) active_ = nullptr;
}

void ModernControlsHook::AfterPadUpdate(const UnifiedState& state, const Config& config, bool controllerAllowed) {
    config_ = &config;
    controllerAllowed_ = controllerAllowed;
    const auto frame = static_cast<std::uint32_t>(CTimer::m_FrameCounter);
    if (haveFrame_ && lastFrame_ == frame) return;
    lastFrame_ = frame;
    if (!haveFrame_) {
        previous_ = {};
        current_ = state;
        haveFrame_ = true;
        config_ = &config;
        controllerAllowed_ = controllerAllowed;
        return;
    }

    previous_ = current_;
    current_ = state;
    config_ = &config;
    controllerAllowed_ = controllerAllowed;
}

bool ModernControlsHook::ModernActive() const {
    return core_ && config_ && controllerAllowed_ &&
        config_->controlProfile == ControlProfile::Modern && current_.connected && !FrontendActive();
}

bool ModernControlsHook::PlayerInVehicle() const {
#if defined(GTASA)
    return ControlledVehicle() != nullptr;
#elif defined(GTA3) || defined(GTAVC)
    return ControlledVehicle() != nullptr;
#else
    return false;
#endif
}

bool ModernControlsHook::AimHeld() const {
    // Modern means triggers own aim/fire. Do not alias LB/RB here; doing so made
    // old shoulder actions leak back into the modern profile.
    return current_.leftTrigger >= kTriggerDigitalThreshold;
}

bool ModernControlsHook::FireHeld() const { return ModernAttackHeld(current_); }
bool ModernControlsHook::FireJustDown() const {
    return ModernAttackHeld(current_) && !ModernAttackHeld(previous_);
}

bool ModernControlsHook::HandBrakeHeld() const {
    // IV used A/Cross as handbrake while V standardized RB/R1.  In modern mode
    // raw Cross is suppressed in vehicles, so accepting both here no longer
    // duplicates the old accelerate action.
    return ModernVehicleIsAircraft() ? current_.x :
        (current_.rb || (!ModernVehicleIsBicycle() && current_.a));
}

bool ModernControlsHook::MeleeHeld() const {
    // GTA V maps light melee / reload / contextual combat to B/Circle while RT
    // remains weapon fire / heavy attack.  SA exposes a separate melee query,
    // so use B there instead of making RT punch every time the player is unarmed.
    return current_.b;
}

bool ModernControlsHook::MeleeJustDown() const {
    return current_.b && !previous_.b;
}

bool ModernControlsHook::CycleWeaponLeftJustDown() const {
    return (current_.lb || current_.dpadLeft) && !(previous_.lb || previous_.dpadLeft);
}

bool ModernControlsHook::CycleWeaponRightJustDown() const {
    return (current_.rb || current_.dpadRight) && !(previous_.rb || previous_.dpadRight);
}

short ModernControlsHook::ToPadTrigger(float value) {
    return static_cast<short>(std::lround(std::clamp(value, 0.0f, 1.0f) * 255.0f));
}

short ModernControlsHook::ToPadButton(bool value) {
    return value ? 255 : 0;
}

unsigned char ModernControlsHook::ToByteButton(bool value) {
    return value ? 1 : 0;
}

short __fastcall ModernControlsHook::AccelerateBridge(CPad* pad, void*) {
    if (active_ && active_->ModernActive() && pad == CPad::GetPad(0) && active_->PlayerInVehicle()) {
        if (!PlayerControlsAvailable(pad)) return 0;
        if (ModernVehicleIsBicycle()) return ToPadButton(active_->current_.a);
        return ToPadTrigger(active_->current_.rightTrigger);
    }
    return pad ? pad->GetAccelerate() : 0;
}

short __fastcall ModernControlsHook::BrakeBridge(CPad* pad, void*) {
    if (active_ && active_->ModernActive() && pad == CPad::GetPad(0) && active_->PlayerInVehicle()) {
        if (!PlayerControlsAvailable(pad)) return 0;
        return ToPadTrigger(active_->current_.leftTrigger);
    }
    return pad ? pad->GetBrake() : 0;
}

short __fastcall ModernControlsHook::HandBrakeBridge(CPad* pad, void*) {
    if (active_ && active_->ModernActive() && pad == CPad::GetPad(0) && active_->PlayerInVehicle()) {
        if (!PlayerControlsAvailable(pad)) return 0;
        return ToPadButton(active_->HandBrakeHeld());
    }
    return pad ? pad->GetHandBrake() : 0;
}

bool __fastcall ModernControlsHook::TargetBridge(CPad* pad, void*) {
    if (active_ && active_->ModernActive() && pad == CPad::GetPad(0) && !active_->PlayerInVehicle()) {
        if (!PlayerControlsAvailable(pad)) return false;
        return active_->AimHeld();
    }
    return pad ? pad->GetTarget() : false;
}

bool __fastcall ModernControlsHook::CycleWeaponLeftBridge(CPad* pad, void*) {
    if (active_ && active_->ModernActive() && pad == CPad::GetPad(0) && !active_->PlayerInVehicle()) {
        if (!PlayerControlsAvailable(pad)) return false;
#if defined(GTASA)
        if (pad->bDisablePlayerCycleWeapon) return false;
#endif
        return active_->CycleWeaponLeftJustDown();
    }
    return pad ? pad->CycleWeaponLeftJustDown() : false;
}

bool __fastcall ModernControlsHook::CycleWeaponRightBridge(CPad* pad, void*) {
    if (active_ && active_->ModernActive() && pad == CPad::GetPad(0) && !active_->PlayerInVehicle()) {
        if (!PlayerControlsAvailable(pad)) return false;
#if defined(GTASA)
        if (pad->bDisablePlayerCycleWeapon) return false;
#endif
        return active_->CycleWeaponRightJustDown();
    }
    return pad ? pad->CycleWeaponRightJustDown() : false;
}

#if defined(GTA3) || defined(GTAVC)
short __fastcall ModernControlsHook::WeaponBridge(CPad* pad, void*) {
    if (active_ && active_->ModernActive() && pad == CPad::GetPad(0) && !active_->PlayerInVehicle()) {
        if (!PlayerControlsAvailable(pad)) return 0;
        return ToPadButton(active_->FireHeld());
    }
#if defined(GTA3)
    // The 2025-10-25 SDK declares bool, but 4936C0 returns signed pad pressure.
    return pad ? plugin::CallMethodAndReturn<short, 0x4936C0, CPad*>(pad) : 0;
#else
    return pad ? pad->GetWeapon() : 0;
#endif
}

bool __fastcall ModernControlsHook::WeaponJustDownBridge(CPad* pad, void*) {
    if (active_ && active_->ModernActive() && pad == CPad::GetPad(0) && !active_->PlayerInVehicle()) {
        if (!PlayerControlsAvailable(pad)) return 0;
        return active_->FireJustDown();
    }
    return pad ? pad->WeaponJustDown() : 0;
}
#endif

#if defined(GTASA)
int __fastcall ModernControlsHook::WeaponBridge(CPad* pad, void*, CPlayerPed* player) {
    if (active_ && active_->ModernActive() && pad == CPad::GetPad(0) && !active_->PlayerInVehicle()) {
        if (!PlayerControlsAvailable(pad)) return 0;
        if (pad->bDisablePlayerFireWeapon) return 0;
        return ToPadButton(active_->FireHeld());
    }
    return pad
        ? plugin::CallMethodAndReturn<int, 0x540180, CPad*, CPlayerPed*>(pad, player)
        : 0;
}

unsigned char __fastcall ModernControlsHook::WeaponJustDownBridge(CPad* pad, void*, CPlayerPed* player) {
    if (active_ && active_->ModernActive() && pad == CPad::GetPad(0) && !active_->PlayerInVehicle()) {
        if (!PlayerControlsAvailable(pad) || pad->bDisablePlayerFireWeapon) return 0;
        return ToByteButton(active_->FireJustDown());
    }
    return pad
        ? plugin::CallMethodAndReturn<unsigned char, 0x540250, CPad*, CPlayerPed*>(pad, player)
        : 0;
}

unsigned char __fastcall ModernControlsHook::MeleeAttackBridge(CPad* pad, void*, bool includeSecondary) {
    if (active_ && active_->ModernActive() && pad == CPad::GetPad(0) && !active_->PlayerInVehicle()) {
        if (!PlayerControlsAvailable(pad)) return 0;
        if (active_->MeleeHeld()) return 1;
        if (!includeSecondary) return 0;
        if (active_->current_.a) return 2;
        if (active_->current_.x) return 3;
        return active_->current_.y ? 4 : 0;
    }
    return pad
        ? plugin::CallMethodAndReturn<unsigned char, 0x540340, CPad*, bool>(pad, includeSecondary)
        : 0;
}

unsigned char __fastcall ModernControlsHook::MeleeAttackJustDownBridge(CPad* pad, void*, bool includeSecondary) {
    if (active_ && active_->ModernActive() && pad == CPad::GetPad(0) && !active_->PlayerInVehicle()) {
        if (!PlayerControlsAvailable(pad)) return 0;
        if (active_->MeleeJustDown()) return 1;
        if (!includeSecondary) return 0;
        if (active_->current_.a && !active_->previous_.a) return 2;
        if (active_->current_.x) return 3; // Native held-X defence.
        return active_->current_.y && !active_->previous_.y ? 4 : 0;
    }
    return pad
        ? plugin::CallMethodAndReturn<unsigned char, 0x540390, CPad*, bool>(pad, includeSecondary)
        : 0;
}
#endif

} // namespace gin
