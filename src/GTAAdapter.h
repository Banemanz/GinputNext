#pragma once
#include "ControllerCore.h"

namespace gin {

class GTAAdapter {
public:
    static void StageBeforePadUpdate(const UnifiedState& s);
    static void ClearStagedGamepad();
    static void MirrorGameRumble(ControllerCore& core, const Config& config);
};

} // namespace gin
