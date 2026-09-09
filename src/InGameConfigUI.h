#pragma once
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include "Config.h"
#include "ControllerCore.h"
#include <array>
#include <string>

namespace gin {

class InGameConfigUI {
public:
    void Init(const std::string& configPath);
    void Shutdown();
    void Tick(const UnifiedState& state, Config& config, const char* inputOwner = "Controller");

    bool IsOpen() const { return open_; }
    void SetModernAvailable(bool available) { modernAvailable_ = available; }

private:
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    bool EnsureClassRegistered();
    bool EnsureWindow();
    HWND FindGameWindow();
    void SyncOverlayToGameWindow();
    void PumpMessages();
    void SetOpen(bool open);

    bool KeyPressed(int vk);
    bool TogglePressed(const UnifiedState& state, const Config& config);
    bool ControllerPressed(bool current, bool& last);
    void HandleMenuInput(const UnifiedState& state, Config& config);

    void MoveSelection(int delta);
    void ActivateSelection(Config& config);
    void SaveConfig(Config& config);
    void Draw(HDC hdc);

    std::string configPath_;
    HWND hwnd_ = nullptr;
    HWND gameHwnd_ = nullptr;
    Config* liveConfig_ = nullptr;
    UnifiedState liveState_{};
    std::string liveInputOwner_ = "Controller";
    bool modernAvailable_ = false;
    bool registered_ = false;
    bool open_ = false;
    int selected_ = 1;
    std::string status_ = "F8 / Back+Start+Y opens menu.";

    std::array<bool, 256> keyDown_{};
    bool lastControllerChord_ = false;
    bool lastDpadUp_ = false;
    bool lastDpadDown_ = false;
    bool lastDpadLeft_ = false;
    bool lastDpadRight_ = false;
    bool lastA_ = false;
    bool lastB_ = false;
};

} // namespace gin
