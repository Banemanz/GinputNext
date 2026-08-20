#define WIN32_LEAN_AND_MEAN
#define DIRECTINPUT_VERSION 0x0800
#include <Windows.h>
#include <dinput.h>

#include "NativeDInputBlocker.h"
#include "Log.h"
#include "RenderWare.h"

extern RsGlobalType& RsGlobal;

namespace gin {

void NativeDInputBlocker::Enable(bool enabled) {
    enabled_ = enabled;
    if (!enabled_) {
        Log("Native DirectInput gamepad suppression disabled by config.");
        return;
    }

    Maintain();
}

void NativeDInputBlocker::SuppressSlot(void*& slot, void*& saved, const char* name) {
    if (!slot) {
        return;
    }

    // If the game recreated a DirectInput pad while we were active, remember
    // the newest pointer so shutdown can hand ownership back to the game.
    saved = slot;

    auto* device = reinterpret_cast<IDirectInputDevice8A*>(slot);
    const HRESULT hr = device->Unacquire();

    Log("Suppressing native %s: ptr=%p Unacquire=0x%08lX",
        name, slot, static_cast<unsigned long>(hr));

    // This is the important part: GTA's native ProcessPad path checks these
    // pointers before polling DirectInput. Mouse DirectInput is a separate
    // RsGlobal.ps->diMouse pointer and is deliberately left untouched.
    slot = nullptr;
    suppressing_ = true;
}

void NativeDInputBlocker::Maintain() {
    if (!enabled_) {
        return;
    }

    if (!RsGlobal.ps) {
        return;
    }

    SuppressSlot(RsGlobal.ps->diDevice1, savedDevice1_, "DirectInput pad 1");
    SuppressSlot(RsGlobal.ps->diDevice2, savedDevice2_, "DirectInput pad 2");
}

void NativeDInputBlocker::Restore() {
    if (!enabled_) {
        return;
    }

    if (RsGlobal.ps) {
        if (!RsGlobal.ps->diDevice1 && savedDevice1_) {
            RsGlobal.ps->diDevice1 = savedDevice1_;
            Log("Restored native DirectInput pad 1 pointer for game shutdown: %p", savedDevice1_);
        }

        if (!RsGlobal.ps->diDevice2 && savedDevice2_) {
            RsGlobal.ps->diDevice2 = savedDevice2_;
            Log("Restored native DirectInput pad 2 pointer for game shutdown: %p", savedDevice2_);
        }
    }

    savedDevice1_ = nullptr;
    savedDevice2_ = nullptr;
    suppressing_ = false;
}

} // namespace gin
