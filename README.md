# GInputNext v9


## v4 compile fix

v3 had one bad SDL wrapper declaration:

```cpp
SDL_bool JoystickIsHaptic(...);
```

SDL2 2.32.10 actually declares `SDL_JoystickIsHaptic()` as returning `int`
because negative values are used for errors. MSVC therefore correctly rejected
the wrapper's implicit `int -> SDL_bool` conversion with C2440.

v4 changes the wrapper to `int` and adds compile-time signature audits so this
specific class of SDL wrapper mismatch is caught immediately in every GTA
target.


## v9: Windows path quoting fix

v8 exposed a Windows batch/PowerShell path bug during dependency preparation:

```text
Test-Path : Illegal characters in path.
FETCH_SDL2.ps1 ... Test-Path $header
```

The public BAT passed the project root as:

```bat
-Root "%~dp0"
```

`%~dp0` always ends in a backslash. Passing a quoted argument ending in a
backslash through cmd.exe -> powershell.exe can leave a malformed/trailing
quote in the value seen by the script, producing paths with illegal `"` chars.

v9 removes that argument entirely from the public dependency BAT files. The
PowerShell fetch scripts derive the root from their own file location, which
is deterministic.

They also defensively:
- trim stray quote characters from any explicitly supplied `-Root`;
- canonicalize it with `System.IO.Path.GetFullPath`;
- print the canonical dependency/header/DLL paths before validation.

So `BUILD_ALL.bat` no longer passes a trailing-backslash root through the
cmd/PowerShell boundary.

## v8: build-system repair

v7 had two PowerShell control-flow bugs that could make every BAT file appear
to "just close after downloading dependencies" without ever invoking MSBuild:

1. `FETCH_SDL2.ps1` used `exit 0` when SDL2 was already present. When that
   script was invoked *inside* `BUILD_PROJECT.ps1`, `exit` terminated the whole
   PowerShell host instead of returning to the build script.
2. `BUILD_PROJECT.ps1` checked `$LASTEXITCODE` immediately after invoking other
   PowerShell scripts. `$LASTEXITCODE` is for native executables and can be
   stale/null after a PowerShell script call.

v8 changes the fetch script to `return`, removes `$LASTEXITCODE` checks around
PowerShell-script invocations, and uses `Start-Process -Wait -PassThru` for a
reliable MSBuild exit code.

All public BAT files now pause when launched directly and write durable logs
under `build_logs\`. `BUILD_ALL.bat` prepares dependencies once, then builds
GTA III -> VC -> SA in sequence and verifies each ASI exists before continuing.

## v7: Start/Pause and BUILD_ALL repair

v6 normalized the controller correctly, but classic PC GTA has a separate
frontend problem: logical controller Start is not reliably the retail PC pause
key. v7 now preserves Start for scripts/mods **and** merges its edge into GTA's
native keyboard Escape state after the original pad update. This makes
Start/Options use the game's own PC pause/resume path.

`StartActsAsEscape=1` is enabled by default.

`BUILD_ALL.bat` was also rewritten to call the same per-game build scripts that
already work individually, in a strict GTA III -> VC -> SA sequence, and it
verifies each produced ASI before continuing.

See `PAUSE_BRIDGE.md`.

## v6: controller mapping repair

The first successful GC201 runtime test exposed the real mapping failure:

```text
GC201 Controller1.00 axes=4 buttons=13 hats=1
```

v6 now:

- downloads and packages a pinned SDL_GameControllerDB;
- gives that DB first chance to normalize a device;
- auto-recognizes the GC201 4-axis / 12-13-button DirectInput signature;
- installs a PlayStation-position mapping before opening it;
- moves Select/Start/L3/R3 to raw buttons 8/9/10/11;
- treats L2/R2 as digital buttons 6/7 for the legacy fallback;
- fixes invalid trigger axes so missing axes produce 0%, never phantom 50%;
- changes generic face-button fallback to b0 Square, b1 Cross, b2 Circle,
  b3 Triangle.

See `CONTROLLER_MAPPING.md`.

## v5: SDL runtime/package fix

A runtime log showed:

```text
SDL2 export missing: SDL_GameControllerGetType
```

That means the DLL being loaded was not the expected modern SDL2 runtime.
`SDL_GameControllerGetType` is part of SDL2's public controller API in the
2.32.10 headers.

v5 removes the guesswork:

- `BUILD_*.bat` downloads the official `SDL2-devel-2.32.10-VC.zip` asset from
  the SDL GitHub release.
- it uses **only** `lib\x86\SDL2.dll`.
- the downloader verifies the PE machine is `0x014C` (32-bit x86).
- the build uses Visual Studio `dumpbin` to verify x86 and important exports.
- every produced game bundle is checked for:
  - `GInputNext.asi`
  - `GInputNext.SDL2.dll`
  - `GInputNext.ini`
- `GInputNext.SDL2.dll` is the official x86 SDL2 runtime copied under a private
  name beside the ASI.

You should no longer manually supply a random `SDL2.dll`.

You can force-refresh SDL independently with:

```bat
PREPARE_SDL2_X86.bat
```

### Older SDL fallback

The loader is also more tolerant now. Newer convenience exports such as
`SDL_GameControllerGetType`, controller sensor APIs, VID/PID helpers and the
dedicated GameController rumble function are optional at runtime.

If an older SDL2 is accidentally present but still has the basic controller
API, GInputNext can continue with generic/fallback controller-family detection
instead of refusing to load merely because `GetType` is absent.

The official build path still requires and packages SDL2 2.32.10 x86.

A shared modern-controller backend for the classic PC versions of:

- Grand Theft Auto III 1.0 EN
- Grand Theft Auto: Vice City 1.0 EN
- Grand Theft Auto: San Andreas 1.0 US

Target Plugin-SDK commit:

`62fd0ef66f704cf7e649607b57cc6e8097ed6e58`

SDL backend: **SDL2 2.32.10**

## Main v3 fix: use GTA's real input pipeline

v1/v2 proved the SDL controller backend and native DirectInput suppression, but
the SDL state was being merged into `CPad::NewState` too late in the frame.

That breaks edge-triggered controls even when held controls seem to work.

GTA's input pipeline is effectively:

```text
CPad::UpdatePads
    OldState <- previous NewState
    build keyboard state
    build joystick state
    build mouse state
    reconcile into NewState
        ↓
menus / scripts / gameplay
```

v3 now does:

```text
SDL2 poll
    ↓
CPad::PCTempJoyState
    ↓
original GTA CPad::UpdatePads()
    ↓
normal OldState/NewState transition
    ↓
pause menu + SCM/CLEO + gameplay
```

That is specifically intended to fix:

- Start/Options not behaving as Pause
- `JustDown` / `JustUp` behavior
- menu navigation
- script-visible controller presses
- controls that appeared to work only while held

## Safe call-site bridge

v3 does **not** replace the `CPad::UpdatePads` function entry.

At runtime it scans the main EXE `.text` section for normal x86 `CALL`
instructions whose target is the correct game function:

```text
SA 1.0 US : 0x541DD0
VC 1.0 EN : 0x4AB6C0
III 1.0 EN: 0x492720
```

Only those call sites are redirected to a wrapper that:

1. keeps GTA's old DirectInput gamepad disabled,
2. polls SDL2,
3. stages SDL state into `PCTempJoyState`,
4. calls the untouched original `CPad::UpdatePads`,
5. mirrors GTA rumble.

The original call bytes are saved and restored on shutdown.

## Native DirectInput suppression

Classic III / VC / SA expose separate platform pointers:

```text
RsGlobal.ps->diMouse
RsGlobal.ps->diDevice1
RsGlobal.ps->diDevice2
```

GInputNext suppresses only:

```text
diDevice1
diDevice2
```

and leaves:

```text
diMouse
```

alone.

So the intended ownership is:

```text
keyboard -> native GTA
mouse    -> native GTA
controller -> SDL2 -> GInputNext -> GTA logical pad state
```

instead of the same physical controller being read once by SDL2 and again by
GTA's old DirectInput code with a conflicting map.

## Logical mapping

```text
SDL A      -> ButtonCross     -> Cross
SDL B      -> ButtonCircle    -> Circle
SDL X      -> ButtonSquare    -> Square
SDL Y      -> ButtonTriangle  -> Triangle

LB -> L1
LT -> L2
RB -> R1
RT -> R2

Back  -> Select
Start -> Start / Pause
L3    -> ShockButtonL
R3    -> ShockButtonR

D-pad -> D-pad
LS    -> left stick
RS    -> right stick
```

The important Pause fix is not merely the field assignment. `Start` now enters
the pad state **before** GTA performs `OldState/NewState` reconciliation.

## Controller coverage

SDL2's normalized GameController layer is used first for devices such as:

- Xbox 360 / One / Series
- DualShock 3
- DualShock 4
- DualSense
- Switch Pro / Joy-Con families
- mapped third-party controllers

If SDL has no standardized mapping, the plugin can fall back to raw
`SDL_Joystick` for generic/legacy controller hardware. Raw fallback axes and
buttons are configurable in `GInputNext.ini`.

## Mod Loader and scripts support

v3 no longer import-links `SDL2.dll`.

Instead it manually loads a private renamed copy:

```text
GInputNext.SDL2.dll
```

from the **ASI's own directory**, and resolves the SDL exports itself.

Config, mapping DB and the log are also looked up beside the ASI first.

This makes the plugin self-contained in Mod Loader:

```text
GTA San Andreas/
  modloader/
    GInputNext/
      GInputNext.asi
      GInputNext.SDL2.dll
      GInputNext.ini
```

or in a scripts directory:

```text
Grand Theft Auto Vice City/
  scripts/
    GInputNext.asi
    GInputNext.SDL2.dll
    GInputNext.ini
```

A root install still works too.

The private SDL filename also avoids fighting another mod that happens to ship
a different `SDL2.dll`.

## Other implemented features

- hotplug/reconnect
- radial stick deadzones
- outer deadzone
- independent stick sensitivity
- analog triggers
- optional Y inversion
- rumble
- optional gyro-to-right-stick bridge
- external `GInputNext.gamecontrollerdb.txt`
- exported controller-state/family/rumble C API
- native keyboard/mouse coexistence
- module-local config/log/dependency handling

## Build

Requirements:

1. Visual Studio 2022 / v143 with Win32 C++ tools.
2. `PLUGIN_SDK_DIR` points at Plugin-SDK commit:
   `62fd0ef66f704cf7e649607b57cc6e8097ed6e58`
3. Existing Release libraries:
   - `output\lib\plugin_iii.lib`
   - `output\lib\plugin_vc.lib`
   - `output\lib\plugin.lib`

Run:

```bat
BUILD_ALL.bat
```

or one of:

```bat
BUILD_GTA3.bat
BUILD_GTAVC.bat
BUILD_GTASA.bat
```

The build downloads the official SDL2 2.32.10 VC development package. It does
not build Plugin-SDK's whole solution.

Outputs are self-contained folders such as:

```text
dist\GTASA\GInputNext\
  GInputNext.asi
  GInputNext.SDL2.dll
  GInputNext.ini
  GInputNext.pdb
  GInputNext.map
```

Copy the entire `GInputNext` folder into `modloader`, or copy the files into a
`scripts`/root install.

## First SA test

After replacing v2:

1. Put the whole output folder at `modloader\GInputNext\`.
2. Launch SA.
3. Open `modloader\GInputNext\GInputNext.log`.
4. Confirm it reports:
   - ModuleDir inside Mod Loader
   - private SDL DLL loaded from that same folder
   - native DirectInput gamepad suppression
   - at least one redirected `CPad::UpdatePads` call site
5. Test Start/Options, face buttons, sticks, D-pad, shoulders/triggers, vehicle
   controls, frontend menus and a script/CLEO pad check.

If the call-site scan reports zero matches, send the log. v3 intentionally
fails safe and restores native DirectInput instead of falling back to the
known-bad late injection method.


## Diagnostic mode

If one physical button still has a suspicious mapping, set:

```ini
[Core]
DebugInput=1
```

The module-local `GInputNext.log` then records normalized button-mask changes,
including explicit Start/Back state, before the values enter GTA's pad update.


## Dependency preparation only

To fetch/validate the pinned x86 SDL2 runtime and controller mapping database
without building yet:

```bat
PREPARE_DEPS.bat
```
