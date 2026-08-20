#include "PauseBridge.h"
#include "Log.h"
#include "plugin.h"
#include "CPad.h"
#include "CTimer.h"
#include <algorithm>

namespace gin {
namespace {

static short Press(bool down) {
    return down ? 255 : 0;
}

static void MergeDigital(short& dst, bool down) {
    if (down && dst < 255) dst = 255;
}

} // namespace

void PauseBridge::Reset() {
    lastFrame_ = 0xFFFFFFFFu;
    previousStart_ = false;
    frameOldStart_ = false;
    frameNewStart_ = false;
}

void PauseBridge::AfterPadUpdate(
    const UnifiedState& state,
    bool startActsAsEscape,
    bool debugInput) {

    CPad* pad = CPad::GetPad(0);
    if (!pad) return;

    // UpdatePads can be called from several retail code paths in one rendered
    // frame. Use GTA's frame counter so all calls in the same frame see the
    // same synthetic Old/New edge instead of consuming the Start press on the
    // first call and erasing it on the second.
    const unsigned int frame = CTimer::m_FrameCounter;
    if (frame != lastFrame_) {
        lastFrame_ = frame;
        frameOldStart_ = previousStart_;
        frameNewStart_ = state.connected && state.start;
        previousStart_ = frameNewStart_;

        if (debugInput && frameNewStart_ != frameOldStart_) {
            Log("Start edge bridge: frame=%u old=%d new=%d escape=%d",
                frame,
                frameOldStart_ ? 1 : 0,
                frameNewStart_ ? 1 : 0,
                startActsAsEscape ? 1 : 0);
        }
    }

    // Preserve a genuine logical Start state for scripts/mods. Retail PC pad
    // code was designed around bindable DirectInput controls and does not
    // consistently preserve console-style Start through the whole pipeline.
    MergeDigital(pad->OldState.Start, frameOldStart_);
    MergeDigital(pad->NewState.Start, frameNewStart_);
    MergeDigital(pad->PCTempJoyState.Start, frameNewStart_);

    if (!startActsAsEscape) return;

    // The classic PC frontends are Escape-centric. re3/reVC documents this
    // distinction explicitly: Start-button frontend support is a separate
    // REGISTER_START_BUTTON feature, while retail-style PC pause paths use
    // Escape. Merge the physical Start edge into the keyboard Escape state so
    // Start/Options behaves exactly like the game's native pause key without
    // stealing a real keyboard Esc press.
    MergeDigital(CPad::OldKeyState.esc, frameOldStart_);
    MergeDigital(CPad::NewKeyState.esc, frameNewStart_);

    // Also expose the semantic Start bit through the temporary keyboard pad
    // state for code paths that reconcile controller-state structures rather
    // than querying CKeyboardState directly.
    MergeDigital(pad->PCTempKeyState.Start, frameNewStart_);
}

} // namespace gin
