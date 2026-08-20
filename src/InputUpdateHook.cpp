#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include "plugin.h"
#include "Patch.h"
#include "InputUpdateHook.h"
#include "Log.h"
#include <cstring>

namespace gin {

std::uintptr_t InputUpdateHook::UpdatePadsPreferredAddress() {
#if defined(GTASA)
    return 0x541DD0;
#elif defined(GTAVC)
    return 0x4AB6C0; // 1.0 EN target
#elif defined(GTA3)
    return 0x492720; // 1.0 EN target
#else
    return 0;
#endif
}

bool InputUpdateHook::Install(BridgeFn bridge) {
    if (!sites_.empty()) return true;
    if (!bridge) return false;

    auto* module = reinterpret_cast<unsigned char*>(GetModuleHandleA(nullptr));
    if (!module) return false;

    auto* dos = reinterpret_cast<IMAGE_DOS_HEADER*>(module);
    if (dos->e_magic != IMAGE_DOS_SIGNATURE) {
        Log("Input hook: invalid DOS header.");
        return false;
    }

    auto* nt = reinterpret_cast<IMAGE_NT_HEADERS32*>(module + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE) {
        Log("Input hook: invalid PE header.");
        return false;
    }

    const std::uintptr_t targetPreferred = UpdatePadsPreferredAddress();
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

            const std::uintptr_t preferredCall =
                preferredImageBase + (actualCall - actualImageBase);

            Site site{};
            site.preferredAddress = preferredCall;
            plugin::patch::GetRaw(preferredCall, site.original.data(), site.original.size(), true);
            sites_.push_back(site);
        }
        break;
    }

    if (!foundText) {
        Log("Input hook: executable .text section not found.");
        return false;
    }

    if (sites_.empty()) {
        Log("Input hook: no CALL references to CPad::UpdatePads target=%p were found.",
            reinterpret_cast<void*>(targetActual));
        return false;
    }

    // A call-site redirect is substantially safer here than replacing the
    // CPad::UpdatePads function entry. Every discovered call still executes
    // the original function after our pre-input staging.
    for (const auto& site : sites_) {
        plugin::patch::RedirectCall(
            site.preferredAddress,
            reinterpret_cast<void*>(bridge),
            true);
        Log("Input hook: redirected CALL at preferred 0x%08lX -> bridge",
            static_cast<unsigned long>(site.preferredAddress));
    }

    Log("Input hook installed: %u call site(s), UpdatePads target preferred=0x%08lX",
        static_cast<unsigned>(sites_.size()),
        static_cast<unsigned long>(targetPreferred));
    return true;
}

void InputUpdateHook::Restore() {
    for (const auto& site : sites_) {
        plugin::patch::SetRaw(
            site.preferredAddress,
            const_cast<unsigned char*>(site.original.data()),
            site.original.size(),
            true);
    }
    if (!sites_.empty()) {
        Log("Input hook restored: %u call site(s).", static_cast<unsigned>(sites_.size()));
    }
    sites_.clear();
}

void InputUpdateHook::CallOriginal() const {
    const std::uintptr_t target = plugin::GetGlobalAddress(UpdatePadsPreferredAddress());
    auto fn = reinterpret_cast<void(__cdecl*)()>(target);
    fn();
}

} // namespace gin
