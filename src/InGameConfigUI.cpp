#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "InGameConfigUI.h"
#include "Log.h"
#include <algorithm>
#include <cstdio>
#include <cstring>

namespace gin {
namespace {

constexpr const char* kClassName = "GInputNextTrainerOverlay";
constexpr int kPanelWidth = 560;
constexpr int kPanelHeight = 344;
constexpr int kPanelMargin = 22;
constexpr int kItemCount = 8;

struct WindowSearch {
    HWND skip = nullptr;
    HWND best = nullptr;
    LONG bestArea = 0;
};

BOOL CALLBACK EnumGameWindows(HWND hwnd, LPARAM param) {
    auto* search = reinterpret_cast<WindowSearch*>(param);
    if (!search || hwnd == search->skip || !IsWindowVisible(hwnd) || GetWindow(hwnd, GW_OWNER)) {
        return TRUE;
    }

    DWORD windowPid = 0;
    GetWindowThreadProcessId(hwnd, &windowPid);
    if (windowPid != GetCurrentProcessId()) {
        return TRUE;
    }

    char cls[128]{};
    GetClassNameA(hwnd, cls, sizeof(cls));
    if (std::strcmp(cls, kClassName) == 0) {
        return TRUE;
    }

    RECT rc{};
    if (!GetWindowRect(hwnd, &rc)) {
        return TRUE;
    }

    const LONG w = std::max<LONG>(0, rc.right - rc.left);
    const LONG h = std::max<LONG>(0, rc.bottom - rc.top);
    const LONG area = w * h;
    if (area > search->bestArea) {
        search->best = hwnd;
        search->bestArea = area;
    }
    return TRUE;
}

void FillSolid(HDC hdc, const RECT& rc, COLORREF color) {
    HBRUSH brush = CreateSolidBrush(color);
    FillRect(hdc, &rc, brush);
    DeleteObject(brush);
}

void DrawTextLine(HDC hdc, int x, int y, const char* text, COLORREF color) {
    SetTextColor(hdc, color);
    SetBkMode(hdc, TRANSPARENT);
    TextOutA(hdc, x, y, text, static_cast<int>(std::strlen(text)));
}

void DrawTextRight(HDC hdc, int right, int y, const char* text, COLORREF color) {
    SIZE size{};
    GetTextExtentPoint32A(hdc, text, static_cast<int>(std::strlen(text)), &size);
    DrawTextLine(hdc, right - size.cx, y, text, color);
}

const char* ControllerFamilyName(GIN_ControllerFamily family, bool connected) {
    if (!connected) return "No controller";
    switch (family) {
    case GIN_FAMILY_XBOX: return "Xbox";
    case GIN_FAMILY_PLAYSTATION: return "PlayStation";
    case GIN_FAMILY_NINTENDO: return "Nintendo";
    case GIN_FAMILY_GENERIC: return "Generic";
    default: return "Unknown";
    }
}

std::string FitText(HDC hdc, const std::string& text, int maxWidth) {
    SIZE size{};
    GetTextExtentPoint32A(hdc, text.c_str(), static_cast<int>(text.size()), &size);
    if (size.cx <= maxWidth) return text;

    std::string out = text;
    while (out.size() > 4) {
        out.resize(out.size() - 1);
        std::string candidate = out + "...";
        GetTextExtentPoint32A(hdc, candidate.c_str(), static_cast<int>(candidate.size()), &size);
        if (size.cx <= maxWidth) return candidate;
    }
    return "...";
}

const char* OnOff(bool value) {
    return value ? "ON" : "OFF";
}

} // namespace

void InGameConfigUI::Init(const std::string& configPath) {
    configPath_ = configPath;
}

void InGameConfigUI::Shutdown() {
    if (hwnd_ && IsWindow(hwnd_)) {
        DestroyWindow(hwnd_);
        hwnd_ = nullptr;
    }
    open_ = false;
}

bool InGameConfigUI::EnsureClassRegistered() {
    if (registered_) {
        return true;
    }

    WNDCLASSEXA wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = &InGameConfigUI::WindowProc;
    wc.hInstance = GetModuleHandleA(nullptr);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = nullptr;
    wc.lpszClassName = kClassName;

    registered_ = RegisterClassExA(&wc) != 0 || GetLastError() == ERROR_CLASS_ALREADY_EXISTS;
    if (!registered_) {
        Log("InGameConfig: trainer overlay RegisterClassEx failed err=%lu", static_cast<unsigned long>(GetLastError()));
    }
    return registered_;
}

bool InGameConfigUI::EnsureWindow() {
    if (hwnd_ && IsWindow(hwnd_)) {
        return true;
    }
    if (!EnsureClassRegistered()) {
        return false;
    }

    hwnd_ = CreateWindowExA(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_LAYERED | WS_EX_NOACTIVATE,
        kClassName,
        "GInputNext",
        WS_POPUP,
        80, 80, kPanelWidth, kPanelHeight,
        nullptr, nullptr, GetModuleHandleA(nullptr), this);

    if (!hwnd_) {
        Log("InGameConfig: trainer overlay CreateWindowEx failed err=%lu", static_cast<unsigned long>(GetLastError()));
        return false;
    }

    SetLayeredWindowAttributes(hwnd_, 0, 238, LWA_ALPHA);
    return true;
}

HWND InGameConfigUI::FindGameWindow() {
    WindowSearch search{};
    search.skip = hwnd_;
    EnumWindows(EnumGameWindows, reinterpret_cast<LPARAM>(&search));
    return search.best;
}

void InGameConfigUI::SyncOverlayToGameWindow() {
    if (!hwnd_ || !IsWindow(hwnd_)) {
        return;
    }

    if (!gameHwnd_ || !IsWindow(gameHwnd_)) {
        gameHwnd_ = FindGameWindow();
    }

    RECT rc{};
    if (gameHwnd_ && GetWindowRect(gameHwnd_, &rc)) {
        const int x = rc.left + kPanelMargin;
        const int y = rc.top + kPanelMargin;
        SetWindowPos(hwnd_, HWND_TOPMOST, x, y, kPanelWidth, kPanelHeight,
            SWP_NOACTIVATE | SWP_SHOWWINDOW);
    } else {
        SetWindowPos(hwnd_, HWND_TOPMOST, 80, 80, kPanelWidth, kPanelHeight,
            SWP_NOACTIVATE | SWP_SHOWWINDOW);
    }
}

void InGameConfigUI::PumpMessages() {
    if (!hwnd_) {
        return;
    }

    MSG msg{};
    while (PeekMessageA(&msg, hwnd_, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
}

bool InGameConfigUI::KeyPressed(int vk) {
    if (vk < 0 || vk >= static_cast<int>(keyDown_.size())) {
        return false;
    }

    const bool down = (GetAsyncKeyState(vk) & 0x8000) != 0;
    const bool pressed = down && !keyDown_[static_cast<size_t>(vk)];
    keyDown_[static_cast<size_t>(vk)] = down;
    return pressed;
}

bool InGameConfigUI::ControllerPressed(bool current, bool& last) {
    const bool pressed = current && !last;
    last = current;
    return pressed;
}

bool InGameConfigUI::TogglePressed(const UnifiedState& state, const Config& config) {
    const bool hotkeyPressed = KeyPressed(config.inGameConfigHotkeyVK);

    bool chord = false;
    if (config.inGameConfigControllerChord) {
        chord = state.connected && state.back && state.start && state.y;
    }
    const bool chordPressed = chord && !lastControllerChord_;
    lastControllerChord_ = chord;

    return hotkeyPressed || chordPressed;
}

void InGameConfigUI::SetOpen(bool open) {
    if (open_ == open) {
        return;
    }

    open_ = open;
    if (open_) {
        for (int vk=0;vk<256;++vk) keyDown_[vk] = (GetAsyncKeyState(vk) & 0x8000) != 0;
        lastDpadUp_=liveState_.dpadUp; lastDpadDown_=liveState_.dpadDown;
        lastDpadLeft_=liveState_.dpadLeft; lastDpadRight_=liveState_.dpadRight;
        lastA_=liveState_.a; lastB_=liveState_.b;
        EnsureWindow();
        SyncOverlayToGameWindow();
        ShowWindow(hwnd_, SW_SHOWNOACTIVATE);
        InvalidateRect(hwnd_, nullptr, FALSE);
        Log("InGameConfig: trainer overlay opened.");
    } else {
        if (hwnd_ && IsWindow(hwnd_)) {
            ShowWindow(hwnd_, SW_HIDE);
        }
        Log("InGameConfig: trainer overlay closed.");
    }
}

void InGameConfigUI::Tick(const UnifiedState& state, Config& config, const char* inputOwner) {
    liveConfig_ = &config;
    liveState_ = state;
    liveInputOwner_ = inputOwner ? inputOwner : "Controller";

    if (!config.inGameConfigEnabled) {
        SetOpen(false);
        PumpMessages();
        return;
    }

    DWORD pid = 0;
    GetWindowThreadProcessId(GetForegroundWindow(), &pid);
    if (pid != GetCurrentProcessId()) { SetOpen(false); PumpMessages(); return; }
    if (TogglePressed(state, config)) {
        SetOpen(!open_);
    }

    if (open_) {
        EnsureWindow();
        SyncOverlayToGameWindow();
        HandleMenuInput(state, config);
        InvalidateRect(hwnd_, nullptr, FALSE);
    }

    PumpMessages();
}

void InGameConfigUI::MoveSelection(int delta) {
    selected_ = (selected_ + delta) % kItemCount;
    if (selected_ < 0) {
        selected_ += kItemCount;
    }
}

void InGameConfigUI::HandleMenuInput(const UnifiedState& state, Config& config) {
    const bool up = KeyPressed(VK_UP) | ControllerPressed(state.connected && state.dpadUp, lastDpadUp_);
    const bool down = KeyPressed(VK_DOWN) | ControllerPressed(state.connected && state.dpadDown, lastDpadDown_);
    const bool left = KeyPressed(VK_LEFT) | ControllerPressed(state.connected && state.dpadLeft, lastDpadLeft_);
    const bool right = KeyPressed(VK_RIGHT) | ControllerPressed(state.connected && state.dpadRight, lastDpadRight_);
    const bool accept = KeyPressed(VK_RETURN) | KeyPressed(VK_SPACE) | ControllerPressed(state.connected && state.a, lastA_);
    const bool cancel = KeyPressed(VK_ESCAPE) | KeyPressed(VK_BACK) | ControllerPressed(state.connected && state.b, lastB_);

    if (up) {
        MoveSelection(-1);
    }
    if (down) {
        MoveSelection(1);
    }
    if (cancel) {
        SetOpen(false);
        return;
    }
    if (left | right | accept) {
        ActivateSelection(config);
    }
}

void InGameConfigUI::SaveConfig(Config& config) {
    const bool ok = config.Save(configPath_);
    status_ = ok ? "Saved to GInputNext.ini" : "Save failed; check write permissions";
    Log("InGameConfig: trainer overlay save %s.", ok ? "succeeded" : "failed");
}

void InGameConfigUI::ActivateSelection(Config& config) {
    switch (selected_) {
    case 0:
        if (!modernAvailable_) { status_ = "Modern unavailable: see GInputNext.log"; break; }
        config.controlProfile = (config.controlProfile == ControlProfile::Modern)
            ? ControlProfile::Classic
            : ControlProfile::Modern;
        status_ = (config.controlProfile == ControlProfile::Modern)
            ? "Modern profile enabled."
            : "Classic controls enabled.";
        Log("InGameConfig: control profile set to %s.",
            config.controlProfile == ControlProfile::Modern ? "Modern" : "Classic");
        break;
    case 1:
        config.autoAim = !config.autoAim;
        status_ = "AutoAim toggled.";
        break;
    case 2:
        config.startActsAsEscape = !config.startActsAsEscape;
        status_ = "Pause bridge toggled.";
        break;
    case 3:
        config.invertCameraY = !config.invertCameraY;
        status_ = "Camera Y toggled.";
        break;
    case 4:
        config.invertAimY = !config.invertAimY;
        status_ = "Aim Y toggled.";
        break;
    case 5:
        config.rumbleEnabled = !config.rumbleEnabled;
        config.gameRumbleEnabled = config.rumbleEnabled;
        status_ = "Rumble toggled.";
        break;
    case 6:
        SaveConfig(config);
        break;
    case 7:
        SetOpen(false);
        break;
    default:
        break;
    }
}

void InGameConfigUI::Draw(HDC hdc) {
    RECT rc{};
    GetClientRect(hwnd_, &rc);

    FillSolid(hdc, rc, RGB(8, 8, 8));

    RECT border = rc;
    FrameRect(hdc, &border, reinterpret_cast<HBRUSH>(GetStockObject(WHITE_BRUSH)));

    HFONT titleFont = CreateFontA(22, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE, "Arial");
    HFONT itemFont = CreateFontA(18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE, "Arial");
    HFONT oldFont = static_cast<HFONT>(SelectObject(hdc, titleFont));

    DrawTextLine(hdc, 18, 14, "GInputNext", RGB(120, 255, 120));

    char controllerLine[96]{};
    std::snprintf(controllerLine, sizeof(controllerLine), "Controller: %s",
        ControllerFamilyName(liveState_.family, liveState_.connected));
    DrawTextRight(hdc, kPanelWidth - 18, 17, controllerLine, RGB(205, 225, 255));

    char ownerLine[96]{};
    std::snprintf(ownerLine, sizeof(ownerLine), "Input: %s", liveInputOwner_.c_str());
    DrawTextRight(hdc, kPanelWidth - 18, 39, ownerLine, RGB(180, 210, 235));

    SelectObject(hdc, itemFont);

    const Config* c = liveConfig_;
    char lines[kItemCount][128]{};
    std::snprintf(lines[0], sizeof(lines[0]), "Control profile: %s",
        c && c->controlProfile == ControlProfile::Modern ? "Modern" : "Classic");
    std::snprintf(lines[1], sizeof(lines[1]), "AutoAim / lock-on assist: %s", c ? OnOff(c->autoAim) : "?");
    std::snprintf(lines[2], sizeof(lines[2]), "Start / Options opens pause: %s", c ? OnOff(c->startActsAsEscape) : "?");
    std::snprintf(lines[3], sizeof(lines[3]), "Invert camera Y: %s", c ? OnOff(c->invertCameraY) : "?");
    std::snprintf(lines[4], sizeof(lines[4]), "Invert aim Y: %s", c ? OnOff(c->invertAimY) : "?");
    std::snprintf(lines[5], sizeof(lines[5]), "Rumble: %s", c ? OnOff(c->rumbleEnabled) : "?");
    std::snprintf(lines[6], sizeof(lines[6]), "Save settings to INI");
    std::snprintf(lines[7], sizeof(lines[7]), "Close menu");

    for (int i = 0; i < kItemCount; ++i) {
        const int y = 66 + i * 24;
        if (i == selected_) {
            RECT hi{12, y - 3, kPanelWidth - 12, y + 21};
            FillSolid(hdc, hi, RGB(48, 78, 48));
            DrawTextLine(hdc, 22, y, ">", RGB(255, 235, 120));
            DrawTextLine(hdc, 44, y, lines[i], RGB(255, 255, 255));
        } else {
            DrawTextLine(hdc, 44, y, lines[i], RGB(220, 220, 220));
        }
    }

    RECT footerRule{12, kPanelHeight - 66, kPanelWidth - 12, kPanelHeight - 65};
    FillSolid(hdc, footerRule, RGB(42, 42, 42));

    const std::string status = FitText(hdc, status_, kPanelWidth - 36);
    DrawTextLine(hdc, 18, kPanelHeight - 54, status.c_str(), RGB(190, 220, 190));
    DrawTextLine(hdc, 18, kPanelHeight - 30, "F8 / B / Esc: close   D-pad: move   A / Enter: select", RGB(155, 155, 155));

    SelectObject(hdc, oldFont);
    DeleteObject(itemFont);
    DeleteObject(titleFont);
}

LRESULT CALLBACK InGameConfigUI::WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    InGameConfigUI* self = reinterpret_cast<InGameConfigUI*>(GetWindowLongPtrA(hwnd, GWLP_USERDATA));

    switch (msg) {
    case WM_NCCREATE: {
        auto* cs = reinterpret_cast<CREATESTRUCTA*>(lParam);
        self = reinterpret_cast<InGameConfigUI*>(cs->lpCreateParams);
        SetWindowLongPtrA(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        return TRUE;
    }
    case WM_ERASEBKGND:
        return 1;
    case WM_PAINT: {
        PAINTSTRUCT ps{};
        HDC hdc = BeginPaint(hwnd, &ps);
        if (self) {
            self->Draw(hdc);
        }
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_CLOSE:
        if (self) {
            self->SetOpen(false);
            return 0;
        }
        break;
    case WM_DESTROY:
        if (self && self->hwnd_ == hwnd) {
            self->hwnd_ = nullptr;
            self->open_ = false;
        }
        return 0;
    }

    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

} // namespace gin
