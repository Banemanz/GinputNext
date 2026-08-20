#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace gin {

class InputUpdateHook {
public:
    using BridgeFn = void(__cdecl*)();

    bool Install(BridgeFn bridge);
    void Restore();
    void CallOriginal() const;
    bool IsInstalled() const { return !sites_.empty(); }
    std::size_t SiteCount() const { return sites_.size(); }

private:
    struct Site {
        std::uintptr_t preferredAddress = 0;
        std::array<unsigned char, 5> original{};
    };

    static std::uintptr_t UpdatePadsPreferredAddress();
    std::vector<Site> sites_;
};

} // namespace gin
