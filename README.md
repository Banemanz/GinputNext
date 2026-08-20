# GInputNext

Modern controller support for the classic PC versions of **Grand Theft Auto III**, **Grand Theft Auto: Vice City**, and **Grand Theft Auto: San Andreas**.

GInputNext is a shared x86 Plugin-SDK controller backend built around **SDL2**, intended as a modern successor in spirit to older XInput-focused controller mods. The goal is simple: let classic RenderWare-era GTA games properly use Xbox, PlayStation, Nintendo, and generic DirectInput-style controllers through one common input layer.

> **Status:** Work in progress / experimental. GTA San Andreas 1.0 US is the primary tested target. GTA III and Vice City builds are included but still need broader runtime testing.

---

## Features

- Shared controller backend for **GTA III / Vice City / San Andreas**
- **Xbox 360 / Xbox One / Xbox Series** controller support
- **DualShock 3 / DualShock 4 / DualSense** support through SDL2 where supported by Windows
- **Nintendo Switch Pro / Joy-Con** family support through SDL2 mappings
- Generic **DirectInput / SDL_Joystick** fallback for older or unusual USB controllers
- Automatic controller hotplug / reconnect
- Analog sticks with configurable deadzones and sensitivity
- Analog or digital trigger support
- Rumble / haptic support where available
- Optional gyro bridge on controllers exposed by SDL2 with motion sensors
- Shared controller mapping database support
- Built-in compatibility mapping for problematic legacy-style pads such as the tested **GC201 Controller1.00**
- Native GTA keyboard + mouse input remains enabled
- Native GTA DirectInput gamepad polling is suppressed to avoid double-input conflicts
- Proper `CPad` integration so gameplay, menus, and scripts see controller state through the game's normal input structures
- Start / Options can be bridged to the classic PC Escape pause path
- Designed to work from the **game root**, **scripts folder**, or **Mod Loader subfolder**
- Private SDL runtime loading through `GInputNext.SDL2.dll` to avoid DLL search-path and mod conflicts
- Small exported C API for other ASI plugins to query controller state / family and request rumble

---

## Supported Games

| Game | Target executable | Plugin-SDK library | Status |
|---|---|---:|---|
| GTA III | 1.0 EN | `plugin_iii.lib` | Builds; runtime testing needed |
| GTA Vice City | 1.0 EN | `plugin_vc.lib` | Builds; runtime testing needed |
| GTA San Andreas | 1.0 US | `plugin.lib` | Primary tested target |

Target Plugin-SDK commit:

```text
62fd0ef66f704cf7e649607b57cc6e8097ed6e58
```

---

## Controller Architecture

GInputNext does not replace GTA's entire input system.

Instead, it replaces only the **gamepad hardware backend** while preserving the game's existing keyboard, mouse, scripts, frontend, and gameplay logic.

```text
Controller
    ↓
SDL2 GameController / SDL_Joystick
    ↓
GInputNext normalized controller state
    ↓
CPad::PCTempJoyState
    ↓
original GTA CPad::UpdatePads()
    ↓
OldState / NewState
    ↓
frontend / gameplay / SCM / CLEO / mods
```

At the same time:

```text
Keyboard ───────► native GTA input
Mouse ──────────► native GTA input
Native DInput pad polling ──► disabled
SDL2 controller input ───────► GInputNext
```

This prevents the same physical controller from being read twice with two different button mappings.

---

## Default Logical Mapping

GInputNext normalizes controllers to GTA's PlayStation-style logical button layout.

| SDL logical input | GTA logical input | PlayStation equivalent |
|---|---|---|
| A / South | `ButtonCross` | Cross |
| B / East | `ButtonCircle` | Circle |
| X / West | `ButtonSquare` | Square |
| Y / North | `ButtonTriangle` | Triangle |
| LB | `LeftShoulder1` | L1 |
| LT | `LeftShoulder2` | L2 |
| RB | `RightShoulder1` | R1 |
| RT | `RightShoulder2` | R2 |
| Back | `Select` | Select / Create |
| Start | `Start` | Start / Options |
| Left stick click | `ShockButtonL` | L3 |
| Right stick click | `ShockButtonR` | R3 |
| D-pad | D-pad | D-pad |
| Left stick | Left stick | Left stick |
| Right stick | Right stick | Right stick |

The game itself still decides what those logical controls do on foot, in vehicles, in menus, and in scripts.

---

## Start / Pause Handling

Classic PC GTA does not always treat the logical controller `Start` field as the retail pause key.

By default GInputNext keeps a real logical Start state for scripts/mods **and** bridges Start / Options to GTA's native PC Escape pause path:

```ini
[Core]
StartActsAsEscape=1
```

This can be disabled if another frontend/input mod already implements its own Start-to-pause behavior.

---

## Installation

### Mod Loader

Recommended layout:

```text
GTA San Andreas/
└── modloader/
    └── GInputNext/
        ├── GInputNext.asi
        ├── GInputNext.SDL2.dll
        ├── GInputNext.ini
        └── GInputNext.gamecontrollerdb.txt
```

Equivalent layouts may be used for GTA III and Vice City when their ASI / Mod Loader setup supports loading plugins from subfolders.

### Scripts folder

```text
Game Folder/
└── scripts/
    ├── GInputNext.asi
    ├── GInputNext.SDL2.dll
    ├── GInputNext.ini
    └── GInputNext.gamecontrollerdb.txt
```

### Game root

A traditional root install also works:

```text
Game Folder/
├── GInputNext.asi
├── GInputNext.SDL2.dll
├── GInputNext.ini
└── GInputNext.gamecontrollerdb.txt
```

An ASI loader is required.

---

## Configuration

Example:

```ini
[Core]
Enabled=1
ControllerIndex=0
AllowGenericDirectInput=1
SuppressNativeGamepad=1
StartActsAsEscape=1
BackgroundInput=0
HotplugScanFrames=30
DebugInput=0

[Sticks]
LeftInnerDeadzone=0.15
RightInnerDeadzone=0.12
OuterDeadzone=0.02
LeftSensitivity=1.00
RightSensitivity=1.00
InvertRightY=0

[Gyro]
Enabled=0
Sensitivity=0.35
InvertX=0
InvertY=0

[Rumble]
Enabled=1
MirrorGameRumble=1
Strength=1.00
```

For unknown legacy DirectInput controllers, a raw fallback map is also available in the `[GenericDirectInput]` section.

---

## GC201 / Legacy PlayStation-Style DirectInput Mapping

The tested `GC201 Controller1.00` exposes a classic 4-axis / 13-button / 1-hat layout.

GInputNext contains an automatic profile for that device family:

```text
b0  = Square
b1  = Cross
b2  = Circle
b3  = Triangle
b4  = L1
b5  = R1
b6  = L2
b7  = R2
b8  = Select
b9  = Start
b10 = L3
b11 = R3
b12 = Guide / Home
hat0 = D-pad
a0/a1 = Left stick
a2/a3 = Right stick
```

This mapping is installed before the device is opened as an SDL GameController.

---

## Building

### Requirements

- Visual Studio 2022 / v143
- Win32 / x86 C++ toolchain
- Plugin-SDK at commit:

```text
62fd0ef66f704cf7e649607b57cc6e8097ed6e58
```

Set:

```bat
PLUGIN_SDK_DIR=C:\path\to\plugin-sdk
```

The following Plugin-SDK Release libraries must already exist:

```text
output\lib\plugin_iii.lib
output\lib\plugin_vc.lib
output\lib\plugin.lib
```

### Build everything

```bat
BUILD_ALL.bat
```

### Build one game

```bat
BUILD_GTA3.bat
BUILD_GTAVC.bat
BUILD_GTASA.bat
```

### Prepare dependencies only

```bat
PREPARE_DEPS.bat
```

The build scripts automatically download and validate:

- **SDL2 2.32.10 Visual C++ development package**
- the **32-bit x86** `SDL2.dll`
- a pinned `SDL_GameControllerDB`

The build rejects the wrong SDL architecture and verifies required exports before compiling.

---

## Build Outputs

```text
dist/
├── GTA3/
│   └── GInputNext/
│       ├── GInputNext.asi
│       ├── GInputNext.SDL2.dll
│       ├── GInputNext.ini
│       └── GInputNext.gamecontrollerdb.txt
├── GTAVC/
│   └── GInputNext/
│       └── ...
└── GTASA/
    └── GInputNext/
        └── ...
```

Build transcripts are written to:

```text
build_logs/
```

The public BAT files pause on failure when launched directly so compiler or dependency errors do not disappear immediately.

---

## Exported API

`include/GInputNextAPI.h` exposes a small C ABI for other plugins:

```cpp
GIN_GetAPIVersion();
GIN_IsConnected();
GIN_GetControllerFamily();
GIN_GetState(...);
GIN_Rumble(...);
```

This can be used by other ASI mods for things such as:

- dynamic Xbox / PlayStation / Nintendo button prompts
- controller-aware UI
- gyro-aware gameplay mods
- custom rumble effects

---

## Runtime Logging

`GInputNext.log` is written beside the ASI.

Useful information includes:

- detected game/version
- game directory
- ASI module directory
- SDL2 runtime version
- controller name
- VID / PID
- GameController vs generic joystick mode
- detected controller family
- raw joystick signature
- installed mapping profile
- DirectInput suppression status
- pad-update bridge installation
- optional normalized input-edge diagnostics

Enable extra input logging with:

```ini
[Core]
DebugInput=1
```

---

## Known Limitations

- GTA III and Vice City currently need more real-world controller/runtime testing.
- Original DualShock 3 Bluetooth behavior on Windows still depends heavily on the installed driver / Bluetooth stack.
- Unknown generic DirectInput devices may need a custom raw button/axis profile if they are not present in the SDL controller database.
- Controller glyph replacement is not currently included; the exported controller-family API exists so a separate prompt module can implement it cleanly.
- Gyro is experimental and disabled by default.
- This project targets the classic **32-bit RenderWare PC releases**, not Definitive Edition.

---

## Design Goals

GInputNext intentionally avoids turning into a giant input rewrite.

The project tries to keep its integration surface small:

- SDL2 owns controller hardware support
- GTA keeps keyboard/mouse
- GTA keeps frontend/gameplay/script behavior
- Plugin-SDK provides the game bridge
- controller mappings stay data-driven where possible
- game-specific code stays thin

The result should be one controller backend that can be maintained across all three classic 3D-era GTA PC games.

---

## Third-Party Projects

GInputNext uses or integrates with:

- **SDL2** — controller, joystick, HID, sensor and haptic backend
- **SDL_GameControllerDB** — community controller mappings
- **Plugin-SDK** — GTA III / Vice City / San Andreas SDK bridge

See `THIRD_PARTY_NOTICES.md` for licensing and pinned dependency details.

---

## License

GInputNext source is released under the **MIT License** unless otherwise noted.

Third-party dependencies retain their respective licenses.

---

## Contributing / Testing

Controller testing is especially useful for:

- GTA III 1.0 EN
- Vice City 1.0 EN
- San Andreas 1.0 US
- DualShock 3
- DualShock 4
- DualSense
- Xbox controllers
- Switch Pro controllers
- unusual generic DirectInput USB pads

When reporting a controller issue, include `GInputNext.log` and, if relevant, enable:

```ini
DebugInput=1
```

and describe the physical button pressed versus the action observed in-game.
