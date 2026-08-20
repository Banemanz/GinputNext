# GInputNext

Modern controller support for the classic PC versions of **Grand Theft Auto III**, **Grand Theft Auto: Vice City**, and **Grand Theft Auto: San Andreas**.

GInputNext is a shared x86 Plugin-SDK controller backend built around **SDL2**. It is intended as a modern successor in spirit to older XInput-focused controller mods: Xbox, PlayStation, Nintendo, and legacy DirectInput-style pads go through one normalized input layer while GTA keeps its normal keyboard, mouse, frontend, gameplay, and script logic.

> **Status:** Work in progress. GTA San Andreas 1.0 US is the most heavily tested target. GTA III and Vice City share the same backend and are supported, with additional runtime testing still welcome.

## Features

- Shared controller backend for **GTA III / Vice City / San Andreas**
- Xbox 360 / Xbox One / Xbox Series controllers
- DualShock 3 / DualShock 4 / DualSense through SDL2 where supported by Windows
- Nintendo Switch Pro / Joy-Con family mappings
- Generic DirectInput / SDL_Joystick fallback for older or unusual USB controllers
- SDL controller mapping database support
- Built-in compatibility mapping for legacy-style pads such as `GC201 Controller1.00`
- Hotplug / reconnect
- Analog sticks with configurable deadzones and sensitivity
- Analog or digital triggers
- Rumble / haptics where available
- Optional gyro bridge
- Native GTA keyboard and mouse remain enabled
- Native GTA DirectInput gamepad polling is suppressed to prevent double input
- Controller state enters GTA through `CPad::PCTempJoyState` before the original `CPad::UpdatePads()` reconciliation
- Start / Options compatibility bridge to the classic PC Escape pause path
- Console-style auto aim / lock-on option
- Separate normal-camera and weapon-aim vertical inversion
- Game root, scripts-folder, and Mod Loader-friendly installation
- Private `GInputNext.SDL2.dll` loading from the ASI directory
- Small exported C API for other ASI plugins

## Supported Games

| Game | Target executable | Plugin-SDK library | Status |
|---|---|---|---|
| GTA III | 1.0 EN | `plugin_iii.lib` | Supported; runtime testing ongoing |
| GTA Vice City | 1.0 EN | `plugin_vc.lib` | Supported; runtime testing ongoing |
| GTA San Andreas | 1.0 US | `plugin.lib` | Primary tested target |

Target Plugin-SDK commit:

```text
62fd0ef66f704cf7e649607b57cc6e8097ed6e58
```

All three games are classic **32-bit x86** targets.

## Controller Architecture

GInputNext does not replace GTA's entire input system.

```text
Controller
    ↓
SDL2 GameController / SDL_Joystick
    ↓
GInputNext normalized state
    ↓
CPad::PCTempJoyState
    ↓
original GTA CPad::UpdatePads()
    ↓
OldState / NewState
    ↓
frontend / gameplay / SCM / CLEO / mods
```

At the hardware level:

```text
Keyboard ───────────────────────► native GTA
Mouse ──────────────────────────► native GTA
GTA native DirectInput gamepad ─► suppressed
SDL2 controller ────────────────► GInputNext
```

This avoids the same physical controller being read once by SDL2 and again by GTA's old DirectInput backend with a conflicting button map.

## Default Logical Mapping

GInputNext normalizes controllers to GTA's PlayStation-style logical controller state.

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
| Left-stick click | `ShockButtonL` | L3 |
| Right-stick click | `ShockButtonR` | R3 |
| D-pad | D-pad | D-pad |
| Left stick | Left stick | Left stick |
| Right stick | Right stick | Right stick |

GTA itself still decides what those logical controls do on foot, in vehicles, in menus, and in scripts.

## Start / Pause Handling

The classic PC ports do not reliably treat the logical controller `Start` field as the retail pause key. By default GInputNext keeps a real logical Start state for scripts/mods **and** mirrors its edge into GTA's native PC Escape path:

```ini
[Core]
StartActsAsEscape=1
```

No Windows keyboard event is injected and the frontend is not called directly; GTA still owns pause/resume behavior.

## Aiming Options

```ini
[Sticks]
InvertCameraY=1
InvertAimY=

[Gameplay]
AutoAim=1
```

`InvertCameraY` controls normal third-person / vehicle / free-look vertical direction.

`InvertAimY` controls vertical right-stick direction while the logical Target button is held. Leave it blank to use the built-in game-specific default:

| Game | `InvertAimY` default |
|---|---:|
| GTA III | `0` |
| Vice City | `0` |
| San Andreas | `1` |

Set `InvertAimY=0` or `InvertAimY=1` explicitly to override that default.

`AutoAim=1` uses each game's own `CPlayerPed::FindWeaponLockOnTarget()` target selection rather than implementing a custom scanner, so weapon range, target visibility, target priority, and target choice remain stock GTA.

San Andreas keeps its free-aim state on the lock-on branch while controller Target is held. GTA III and Vice City have an additional PC-specific quirk: their stock lock-on branch is gated by `CCamera::m_bUseMouse3rdPerson`. GInputNext temporarily hands aiming ownership to the stock controller branch while Target is held, then restores the previous mouse-camera setting on release.

More detail is in [`AIMING.md`](AIMING.md).

## Installation

An ASI loader is required.

### Mod Loader

Recommended layout:

```text
Game Folder/
└── modloader/
    └── GInputNext/
        ├── GInputNext.asi
        ├── GInputNext.SDL2.dll
        ├── GInputNext.ini
        └── GInputNext.gamecontrollerdb.txt
```

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

```text
Game Folder/
├── GInputNext.asi
├── GInputNext.SDL2.dll
├── GInputNext.ini
└── GInputNext.gamecontrollerdb.txt
```

The config, controller database, SDL runtime, and log are resolved relative to the ASI so nested Mod Loader installs do not require dumping everything into the game root.

## Configuration

Typical configuration:

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
InvertCameraY=1
InvertAimY=

[Gameplay]
AutoAim=1

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

Unknown legacy DirectInput devices can also use the raw `[GenericDirectInput]` fallback mapping.

## GC201 / Legacy PlayStation-Style DirectInput Mapping

The tested `GC201 Controller1.00` exposes a classic 4-axis / 13-button / 1-hat layout. GInputNext includes an automatic profile:

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

The mapping is installed before the device is opened through SDL's normalized GameController path.

## Building

### Requirements

- Visual Studio 2022 / v143
- Win32 / x86 C++ toolchain
- Plugin-SDK at commit `62fd0ef66f704cf7e649607b57cc6e8097ed6e58`

Set:

```bat
set PLUGIN_SDK_DIR=C:\path\to\plugin-sdk
```

The corresponding Plugin-SDK Release libraries must already exist:

```text
output\lib\plugin_iii.lib
output\lib\plugin_vc.lib
output\lib\plugin.lib
```

Prepare dependencies:

```bat
PREPARE_DEPS.bat
```

Build a target:

```bat
BUILD_GTA3.bat
BUILD_GTAVC.bat
BUILD_GTASA.bat
```

The scripts download and validate the official **SDL2 2.32.10 Visual C++ development package**, select the **32-bit x86** runtime, and download the pinned SDL controller mapping database.

Outputs are created under:

```text
dist\GTA3\GInputNext\
dist\GTAVC\GInputNext\
dist\GTASA\GInputNext\
```

Build transcripts are written under `build_logs\`.

## Exported API

`include/GInputNextAPI.h` exposes a small C ABI:

```cpp
GIN_GetAPIVersion();
GIN_IsConnected();
GIN_GetControllerFamily();
GIN_GetState(...);
GIN_Rumble(...);
```

Other ASI plugins can use this for controller-family-aware prompts, controller-aware UI, motion features, or custom rumble.

## Runtime Logging

`GInputNext.log` is written beside the ASI and records useful information such as:

- detected game/version
- game and ASI module directories
- SDL2 runtime version
- controller name / VID / PID
- normalized GameController vs generic joystick mode
- installed mapping profile
- DirectInput suppression status
- pad-update bridge installation
- optional input-edge / auto-aim diagnostics

Enable additional diagnostics with:

```ini
[Core]
DebugInput=1
```

## Known Limitations

- GTA III and Vice City still benefit from more controller/runtime testing than San Andreas.
- Original DualShock 3 Bluetooth behavior on Windows depends heavily on the installed driver / Bluetooth stack.
- Unknown generic DirectInput pads may require a custom raw mapping if they are not in the SDL controller database.
- Controller glyph replacement is not currently included; the exported controller-family API is intended to support a separate prompt module cleanly.
- Gyro support is experimental and disabled by default.
- Definitive Edition is not supported; this project targets the classic 32-bit RenderWare PC releases.

## Design Goals

GInputNext intentionally keeps its integration surface small:

- SDL2 owns controller hardware support
- GTA keeps keyboard and mouse
- GTA keeps its frontend, gameplay, weapon targeting, and script behavior
- Plugin-SDK provides the game bridge
- mappings stay data-driven where possible
- game-specific differences stay behind thin compatibility code

The goal is one maintainable controller backend across all three classic 3D-era GTA PC games.

## Third-Party Projects

- **SDL2** — controller, joystick, HID, sensor, and haptic backend
- **SDL_GameControllerDB** — community controller mappings
- **Plugin-SDK** — GTA III / Vice City / San Andreas SDK bridge

See `THIRD_PARTY_NOTICES.md` for dependency and license information.

## License

GInputNext source is released under the **MIT License** unless otherwise noted. Third-party dependencies retain their respective licenses.

## Testing / Bug Reports

Testing is especially useful on:

- GTA III 1.0 EN
- Vice City 1.0 EN
- San Andreas 1.0 US
- DualShock 3 / 4
- DualSense
- Xbox controllers
- Switch Pro controllers
- unusual DirectInput USB pads

For input issues, include `GInputNext.log`, enable `DebugInput=1` when useful, and describe the physical control pressed versus the observed in-game action.
