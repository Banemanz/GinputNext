#pragma once
#include "ControllerCore.h"

namespace gin {

class PauseBridge {
public:
    void Reset();
    void AfterPadUpdate(const UnifiedState& state, bool startActsAsEscape, bool debugInput);

private:
    unsigned int lastFrame_ = 0xFFFFFFFFu;
    bool previousStart_ = false;
    bool frameOldStart_ = false;
    bool frameNewStart_ = false;
};

} // namespace gin
