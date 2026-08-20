# GInputNext compatibility audit

## Exact targets

Plugin-SDK 2025-10-27 snapshot:
`624a6a49265fd7a6fc63bda1611013ceabeacb8a`

Classic x86 targets:

- GTA III 1.0 EN
- Vice City 1.0 EN
- San Andreas 1.0 US

VS2022 v143, Win32, C++latest, MBCS, static project CRT.

## Input timing fix

SA's reversed `CGame::Process()` begins by calling `CPad::UpdatePads()`.
`CPad::UpdatePads()` owns the normal transition/reconciliation that creates
usable `OldState` and `NewState`.

v2 injected from later Plugin-SDK events, after the game's main pad update.

v3 instead stages into `CPad::PCTempJoyState` immediately before the real
UpdatePads call, then executes the original function.

This preserves normal pressed/released edge semantics for:

- Start/Pause
- frontend
- gameplay
- SCM/CLEO/script pad checks
- `JustDown`/`JustUp` style helpers

## UpdatePads targets

- SA: `0x541DD0`
- VC 1.0 EN: `0x4AB6C0`
- III 1.0 EN: `0x492720`

The EXE `.text` section is scanned for `CALL rel32` references that resolve to
that target. Only those call sites are redirected with
`plugin::patch::RedirectCall`.

The UpdatePads function entry is not replaced.

## Cross-game CPad compatibility

The pinned III and VC headers expose:

- NewState
- OldState
- PCTempKeyState
- PCTempJoyState
- PCTempMouseState

SA uses the same staging/reconciliation layout.

GInputNext only writes the common controller portion and leaves game-specific
extra fields untouched.

## Native DirectInput suppression

The pinned RenderWare platform skeletons for III / VC / SA contain separate:

- `diMouse`
- `diDevice1`
- `diDevice2`

Only `diDevice1` and `diDevice2` are suppressed.

SA's reversed `CPad::ProcessPad` explicitly checks these native joystick
pointers and returns when the selected device is null.

## Mod Loader / scripts

v3 has no linker dependency on `SDL2.dll`.

The ASI explicitly loads `GInputNext.SDL2.dll` from its own module directory
using `LoadLibraryEx`, then resolves the required API with `GetProcAddress`.

The INI, optional controller DB and log are module-relative first as well.

## Shutdown

Shutdown is idempotent and restores:

- patched UpdatePads call sites
- original native DirectInput controller pointers

then closes SDL devices/subsystems.

A C++ global destructor is included as a normal FreeLibrary/Mod Loader teardown
fallback in addition to the Plugin-SDK RW shutdown event.

## GTA III v13 weapon-camera scope

GTA III 1.0 EN `CPlayerPed::ProcessPlayerWeapon` target: `0x4F1EF0`.

v13 scans the EXE `.text` section for direct `CALL rel32` references to that
target and redirects only those call sites. The function entry is untouched.
The shim temporarily suppresses `CCamera::m_bUseMouse3rdPerson` only during the
original weapon-processing call, restores it immediately, and cancels only a
queued `MODE_SYPHON` camera handoff when the saved camera was the normal PC
mouse camera. VC and SA do not include this source file.

## GTA III v14 M16 isolation

GTA III's M16 is a dedicated first-person weapon camera path, not a normal
lock-on path. v14 explicitly bypasses both GInputNext auto-aim acquisition and
the GTA III scoped mouse-camera compatibility shim while the current weapon is
`WEAPONTYPE_M16`. The retail `ProcessPlayerWeapon()` implementation therefore
controls `MODE_M16_1ST_PERSON` without a competing Syphon/lock target.

Vice City and San Andreas source/project behavior is unchanged by this fix.


## v15 III/VC first-person controller path

- GTA III first-person family: M16, sniper rifle, rocket launcher.
- Vice City first-person family: M4, Ruger, sniper rifle, laser scope, rocket
  launcher, M60, camera.
- These families bypass GInputNext lock-on injection/compatibility camera
  overrides.
- While controller Target is held, physical right stick is routed to the
  stock `SniperModeLook*` left-stick channel and the game right-stick fields
  are zeroed.
- Normal lock-on weapons remain unchanged; GTA SA remains unchanged.
