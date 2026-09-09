#pragma once
#include "Config.h"
#include "UnifiedState.h"
#include <array>
namespace gin {
class InputArbitration {
public:
    enum class Owner { Controller, KeyboardMouse };
    void Reset();
    bool Tick(const UnifiedState&, const Config&);
    bool ControllerAllowed() const { return owner_ == Owner::Controller; }
    Owner CurrentOwner() const { return owner_; }
    const char* OwnerName() const;
private:
    bool DetectControllerActivity(const UnifiedState&, const Config&);
    bool DetectKeyboardMouseActivity();
    Owner owner_ = Owner::Controller;
    UnifiedState controllerAnchor_{};
    std::array<bool,256> keys_{};
};
}
