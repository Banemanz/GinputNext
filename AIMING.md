# GInputNext v11 camera / aiming behavior

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
InvertAimY=1
```

This controls right-stick vertical direction while GTA's logical Target button
is held.

The two settings are alternatives, not cumulative:

```text
Target held? no  -> InvertCameraY
Target held? yes -> InvertAimY
```

So `InvertCameraY=1` and `InvertAimY=1` gives the same corrected vertical
direction in both normal camera and weapon aiming without double-inverting.

`InvertRightY` from v10 is accepted as a legacy alias for `InvertCameraY`, but
new configs should use the clearer v11 key.

## Auto aim / lock-on

```ini
[Gameplay]
AutoAim=1
```

GInputNext asks the game's own `CPlayerPed::FindWeaponLockOnTarget()` to
acquire a target. It does not implement a custom target scanner, so weapon
range, target visibility, target priority and target choice remain stock GTA.

On San Andreas, AutoAim also keeps `m_bFreeAiming` false while Target is held.
