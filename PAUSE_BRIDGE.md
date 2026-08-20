# GInputNext v7 Start/Pause bridge

## Why every button worked except Pause

SDL2 was correctly reporting the controller's logical `START` button after the
v6 mapping repair. The remaining problem is in the classic GTA PC frontend,
not the controller mapping.

The PC ports were built around keyboard Escape + bindable DirectInput controls.
Reverse-engineered re3/reVC code documents console-style Start handling behind
a separate `REGISTER_START_BUTTON` feature. Retail-style PC pause behavior is
Escape-centric.

That matches an independent GTA SA controller guide: ordinary DirectInput can
bind Start as a button but Start does not pause; mapping Start to Escape makes
it behave as expected.

## v7 behavior

After the original `CPad::UpdatePads()` finishes, GInputNext merges the SDL
Start state into two places:

1. GTA logical controller Start (`OldState.Start` / `NewState.Start`) so scripts
   and mods can see a real Start edge.
2. GTA keyboard Escape (`OldKeyState.esc` / `NewKeyState.esc`) so the retail PC
   frontend performs its native pause/resume behavior.

No Windows keyboard event is injected and no menu function is called directly.
The game still owns the pause frontend.

The merge preserves a real physical keyboard Escape press.

## Multiple UpdatePads calls

SA has several call sites into `CPad::UpdatePads`. v7 keys the synthetic
Old/New Start edge to `CTimer::m_FrameCounter`, so multiple pad updates in one
rendered frame see the same edge instead of consuming it on the first call.

## Config

```ini
[Core]
StartActsAsEscape=1
```

Disable only if another input/frontend mod already implements its own Start to
pause bridge.
