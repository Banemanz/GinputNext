# GInputNext v3 compatibility audit

## Exact targets

Plugin-SDK:
`62fd0ef66f704cf7e649607b57cc6e8097ed6e58`

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
