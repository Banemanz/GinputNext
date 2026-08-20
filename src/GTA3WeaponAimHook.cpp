#if defined(GTA3)

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include "plugin.h"
#include "Patch.h"
#include "CCamera.h"
#include "CCam.h"
#include "CPad.h"
#include "CPlayerPed.h"
#include "Config.h"
#include "ClassicFirstPersonAim.h"
#include "ControllerCore.h"
#include "GTA3WeaponAimHook.h"
#include "Log.h"
#include <cstring>

namespace gin {

GTA3WeaponAimHook* GTA3WeaponAimHook::active_ = nullptr;

std::uintptr_t GTA3WeaponAimHook::ProcessPlayerWeaponPreferredAddress() {
    // Plugin-SDK 2025-10-27, GTA III 1.0 EN.
    return 0x4F1EF0;
}

bool GTA3WeaponAimHook::Install(const ControllerCore* core, const Config* config) {
    if (!sites_.empty()) return true;
    if (!core || !config || active_) return false;

    auto* module = reinterpret_cast<unsigned char*>(GetModuleHandleA(nullptr));
    if (!module) return false;

    auto* dos = reinterpret_cast<IMAGE_DOS_HEADER*>(module);
    if (dos->e_magic != IMAGE_DOS_SIGNATURE) {
        Log("GTA3 aim hook: invalid DOS header.");
        return false;
    }

    auto* nt = reinterpret_cast<IMAGE_NT_HEADERS32*>(module + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE) {
        Log("GTA3 aim hook: invalid PE header.");
        return false;
    }

    const std::uintptr_t targetPreferred = ProcessPlayerWeaponPreferredAddress();
    const std::uintptr_t targetActual = plugin::GetGlobalAddress(targetPreferred);
    const std::uintptr_t preferredImageBase = static_cast<std::uintptr_t>(nt->OptionalHeader.ImageBase);
    const std::uintptr_t actualImageBase = reinterpret_cast<std::uintptr_t>(module);

    IMAGE_SECTION_HEADER* section = IMAGE_FIRST_SECTION(nt);
    bool foundText = false;

    for (unsigned i = 0; i < nt->FileHeader.NumberOfSections; ++i, ++section) {
        char name[9]{};
        std::memcpy(name, section->Name, 8);
        if (std::strcmp(name, ".text") != 0) continue;
        foundText = true;

        unsigned char* begin = module + section->VirtualAddress;
        const std::size_t size = static_cast<std::size_t>(
            section->Misc.VirtualSize ? section->Misc.VirtualSize : section->SizeOfRawData);

        for (std::size_t off = 0; off + 5 <= size; ++off) {
            unsigned char* p = begin + off;
            if (*p != 0xE8) continue;

            std::int32_t rel = 0;
            std::memcpy(&rel, p + 1, sizeof(rel));
            const std::uintptr_t actualCall = reinterpret_cast<std::uintptr_t>(p);
            const std::uintptr_t dest = actualCall + 5 + static_cast<std::intptr_t>(rel);
            if (dest != targetActual) continue;

            Site site{};
            site.preferredAddress = preferredImageBase + (actualCall - actualImageBase);
            plugin::patch::GetRaw(
                site.preferredAddress,
                site.original.data(),
                site.original.size(),
                true);
            sites_.push_back(site);
        }
        break;
    }

    if (!foundText) {
        Log("GTA3 aim hook: executable .text section not found.");
        return false;
    }

    if (sites_.empty()) {
        Log("GTA3 aim hook: no CALL references to CPlayerPed::ProcessPlayerWeapon target=%p.",
            reinterpret_cast<void*>(targetActual));
        return false;
    }

    core_ = core;
    config_ = config;
    active_ = this;

    for (const auto& site : sites_) {
        plugin::patch::RedirectCall(
            site.preferredAddress,
            reinterpret_cast<void*>(&GTA3WeaponAimHook::Bridge),
            true);
        Log("GTA3 aim hook: redirected ProcessPlayerWeapon CALL at preferred 0x%08lX",
            static_cast<unsigned long>(site.preferredAddress));
    }

    Log("GTA3 aim hook installed: %u call site(s), target preferred=0x%08lX",
        static_cast<unsigned>(sites_.size()),
        static_cast<unsigned long>(targetPreferred));
    return true;
}

void GTA3WeaponAimHook::Restore() {
    for (const auto& site : sites_) {
        plugin::patch::SetRaw(
            site.preferredAddress,
            const_cast<unsigned char*>(site.original.data()),
            site.original.size(),
            true);
    }

    if (!sites_.empty()) {
        Log("GTA3 aim hook restored: %u call site(s).", static_cast<unsigned>(sites_.size()));
    }

    sites_.clear();
    core_ = nullptr;
    config_ = nullptr;
    if (active_ == this) active_ = nullptr;
}

bool GTA3WeaponAimHook::ShouldUseControllerLockOn(const CPad& pad) const {
    if (!core_ || !config_ || !config_->autoAim || !core_->IsConnected()) return false;
    const UnifiedState& state = core_->State();
    return pad.Mode == 3 ? state.lb : state.rb;
}

void GTA3WeaponAimHook::CallOriginal(CPlayerPed* player, CPad* pad) const {
    if (!player) return;
    player->ProcessPlayerWeapon(pad);
}

void __fastcall GTA3WeaponAimHook::Bridge(CPlayerPed* player, void*, CPad* pad) {
    if (!active_) {
        if (player) player->ProcessPlayerWeapon(pad);
        return;
    }
    active_->Invoke(player, pad);
}

void GTA3WeaponAimHook::Invoke(CPlayerPed* player, CPad* pad) {
    if (!player || !pad) {
        CallOriginal(player, pad);
        return;
    }

    // GTA III's M16, sniper rifle and rocket launcher own dedicated stock
    // first-person camera paths. They are not console-style lock-on weapons,
    // so let ProcessPlayerWeapon see the exact retail PC camera state and do
    // not run any lock-on compatibility shim. GTAAdapter handles modern
    // right-stick aiming by feeding the retail SniperModeLook* channel.
    if (IsClassicFirstPersonAimWeapon(player)) {
        CallOriginal(player, pad);
        return;
    }

    const bool controllerLockOn = ShouldUseControllerLockOn(*pad);
    if (!controllerLockOn) {
        CallOriginal(player, pad);
        return;
    }

    // GTA III's PC ProcessPlayerWeapon drops a valid lock-on whenever the
    // global mouse-third-person flag is true. v12 held this flag false for the
    // entire Target press, which also changed unrelated camera processing and
    // produced the visible camera sway. Scope the compatibility lie to the one
    // function that needs it instead.
    const bool savedMouse3rdPerson = CCamera::m_bUseMouse3rdPerson;
    CCamera::m_bUseMouse3rdPerson = false;

    CallOriginal(player, pad);

    CCamera::m_bUseMouse3rdPerson = savedMouse3rdPerson;

    // When the user was already in GTA III's normal PC mouse camera, retain
    // the lock-on target but do not let ProcessPlayerWeapon queue the console
    // Syphon camera. The target marker, target validation, weapon logic and
    // aiming remain stock; only the unwanted camera-mode handoff is cancelled.
    // Classic/non-mouse camera users still get the original Syphon behavior.
    if (savedMouse3rdPerson &&
        player->m_pPointGunAt &&
        TheCamera.m_PlayerWeaponMode.Mode == MODE_SYPHON) {
        TheCamera.ClearPlayerWeaponMode();
    }
}

} // namespace gin

#endif
