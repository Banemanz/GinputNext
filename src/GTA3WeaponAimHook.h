#pragma once

#if defined(GTA3)

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

class CPlayerPed;
class CPad;

namespace gin {

struct Config;
class ControllerCore;

class GTA3WeaponAimHook {
public:
    bool Install(const ControllerCore* core, const Config* config);
    void Restore();

    bool IsInstalled() const { return !sites_.empty(); }
    std::size_t SiteCount() const { return sites_.size(); }

private:
    struct Site {
        std::uintptr_t preferredAddress = 0;
        std::array<unsigned char, 5> original{};
    };

    static void __fastcall Bridge(CPlayerPed* player, void* unusedEdx, CPad* pad);
    void Invoke(CPlayerPed* player, CPad* pad);
    void CallOriginal(CPlayerPed* player, CPad* pad) const;
    bool ShouldUseControllerLockOn(const CPad& pad) const;
    static std::uintptr_t ProcessPlayerWeaponPreferredAddress();

    const ControllerCore* core_ = nullptr;
    const Config* config_ = nullptr;
    std::vector<Site> sites_;

    static GTA3WeaponAimHook* active_;
};

} // namespace gin

#endif
