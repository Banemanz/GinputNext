#pragma once
#include "Config.h"
#include "ControllerCore.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

class CPad;
class CPlayerPed;

namespace gin {

class ModernControlsHook {
public:
    bool Install(const ControllerCore* core, const Config* config);
    void Restore();
    void AfterPadUpdate(const UnifiedState& state, const Config& config, bool controllerAllowed = true);

    bool IsInstalled() const { return !sites_.empty(); }
    std::size_t SiteCount() const { return sites_.size(); }

public:
    enum class Action : std::uint8_t {
        Accelerate,
        Brake,
        HandBrake,
        Target,
        CycleWeaponLeft,
        CycleWeaponRight,
        Weapon,
        WeaponJustDown,
#if defined(GTASA)
        MeleeAttack,
        MeleeAttackJustDown,
#endif
    };

    struct CallSiteTarget {
        Action action;
        std::uintptr_t preferredAddress;
        std::uintptr_t expectedTarget;
        const char* name;
    };

private:
    struct Site {
        Action action;
        std::uintptr_t preferredAddress = 0;
        std::array<unsigned char, 5> original{};
        const char* name = nullptr;
    };

    static ModernControlsHook* active_;

    static const CallSiteTarget* CallSiteTargets(std::size_t& count);
    bool PatchKnownCallSite(const CallSiteTarget& target);
    bool ModernActive() const;
    bool PlayerInVehicle() const;

    bool AimHeld() const;
    bool FireHeld() const;
    bool FireJustDown() const;
    bool HandBrakeHeld() const;
    bool MeleeHeld() const;
    bool MeleeJustDown() const;
    bool CycleWeaponLeftJustDown() const;
    bool CycleWeaponRightJustDown() const;

    static short ToPadTrigger(float value);
    static short ToPadButton(bool value);
    static unsigned char ToByteButton(bool value);

    static short __fastcall AccelerateBridge(CPad* pad, void* unusedEdx);
    static short __fastcall BrakeBridge(CPad* pad, void* unusedEdx);
    static short __fastcall HandBrakeBridge(CPad* pad, void* unusedEdx);
    static bool __fastcall TargetBridge(CPad* pad, void* unusedEdx);
    static bool __fastcall CycleWeaponLeftBridge(CPad* pad, void* unusedEdx);
    static bool __fastcall CycleWeaponRightBridge(CPad* pad, void* unusedEdx);
#if defined(GTA3) || defined(GTAVC)
    static short __fastcall WeaponBridge(CPad* pad, void* unusedEdx);
    static bool __fastcall WeaponJustDownBridge(CPad* pad, void* unusedEdx);
#endif
#if defined(GTASA)
    static int __fastcall WeaponBridge(CPad* pad, void* unusedEdx, CPlayerPed* player);
    static unsigned char __fastcall WeaponJustDownBridge(CPad* pad, void* unusedEdx, CPlayerPed* player);
    static unsigned char __fastcall MeleeAttackBridge(CPad* pad, void* unusedEdx, bool includeSecondary);
    static unsigned char __fastcall MeleeAttackJustDownBridge(CPad* pad, void* unusedEdx, bool includeSecondary);
#endif

    const ControllerCore* core_ = nullptr;
    const Config* config_ = nullptr;
    std::vector<Site> sites_;
    UnifiedState current_{};
    UnifiedState previous_{};
    bool haveFrame_ = false;
    std::uint32_t lastFrame_ = 0;
    bool controllerAllowed_ = true;
};

} // namespace gin
