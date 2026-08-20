#pragma once
#include "Config.h"
#include "ControllerCore.h"

namespace gin {

class GameplayBridge {
public:
    void Reset();
    void AfterPadUpdate(const UnifiedState& state, const Config& config);

private:
    unsigned int lastFrame_ = 0xFFFFFFFFu;
    bool previousTarget_ = false;
    bool acquiredTarget_ = false;
    unsigned int nextRetryFrame_ = 0;
};

} // namespace gin
