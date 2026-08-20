#pragma once

namespace gin {

class NativeDInputBlocker {
public:
    void Enable(bool enabled);
    void Maintain();
    void Restore();

    bool IsEnabled() const { return enabled_; }
    bool IsSuppressing() const { return suppressing_; }

private:
    void SuppressSlot(void*& slot, void*& saved, const char* name);

    bool enabled_ = false;
    bool suppressing_ = false;
    void* savedDevice1_ = nullptr;
    void* savedDevice2_ = nullptr;
};

} // namespace gin
