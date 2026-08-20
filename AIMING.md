# GInputNext camera / aiming behavior

## General camera vertical direction

```ini
[Sticks]
InvertCameraY=1
```

This controls the right-stick vertical direction during normal third-person
camera, vehicle camera, and other non-targeting look modes.

## Weapon aiming vertical direction

```ini
[Sticks]
InvertAimY=
```

This controls right-stick vertical direction while GTA's logical Target button
is held.

The built-in defaults intentionally differ because the three PC ports do not
agree on controller aiming direction:

```text
GTA III        InvertAimY=0
Vice City      InvertAimY=0
San Andreas    InvertAimY=1
```

Leave `InvertAimY` blank to use that game-specific default, or explicitly set
`0` / `1` to override it.

Camera and aim inversion are alternatives, not cumulative:

```text
Target held? no  -> InvertCameraY
Target held? yes -> InvertAimY
```

`InvertRightY` from older builds is accepted as a legacy alias for
`InvertCameraY`, but new configs should use the clearer key.

## Auto aim / lock-on

```ini
[Gameplay]
AutoAim=1
```

GInputNext asks the game's own `CPlayerPed::FindWeaponLockOnTarget()` to
acquire a target. It does not implement a custom target scanner, so weapon
range, target visibility, target priority and target choice remain stock GTA.

### San Andreas

While controller Target is held, AutoAim also keeps the player's
`m_bFreeAiming` flag false so the game remains on its normal lock-on path.

### GTA III / Vice City

The original III/VC PC weapon code has an extra PC-specific gate: controller
lock-on only runs while `CCamera::m_bUseMouse3rdPerson` is false. A controller
can therefore report Target correctly while the game still thinks the mouse
third-person camera owns aiming, causing lock-on to appear broken.

When `AutoAim=1` and controller Target is held, GInputNext now:

1. saves the current `CCamera::m_bUseMouse3rdPerson` value;
2. temporarily sets it to `false` so the stock controller lock-on branch runs;
3. keeps using the game's own `FindWeaponLockOnTarget()` logic;
4. restores the previous mouse-camera mode when Target is released, AutoAim is
   disabled, the controller disconnects, or the plugin shuts down.

This preserves the user's normal mouse-camera setting outside controller
lock-on instead of globally disabling mouse third-person aiming.
