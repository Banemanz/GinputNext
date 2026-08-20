#pragma once
#include "ControllerCore.h"

namespace gin {

class GTAAdapter {
public:
    static void StageBeforePadUpdate(const UnifiedState& s, const Config& config);
    static void ClearStagedGamepad();
    static void MirrorGameRumble(ControllerCore& core, const Config& config);
};

} // namespace gin
