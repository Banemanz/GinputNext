# GInputNext v15 camera / aiming behavior

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


## GTA III / Vice City PC camera compatibility

The original PC weapon code drops a lock-on while its mouse third-person camera
flag is active. The two games need slightly different compatibility handling:

- **Vice City:** retains the v12 behavior: while SDL controller Target is held,
  the mouse-camera flag is saved, forced false, and restored exactly on release.
- **GTA III:** the flag is forced false only inside the stock
  `CPlayerPed::ProcessPlayerWeapon()` call. It is restored before the rest of
  GTA III's camera system runs. If the user was in the normal PC mouse camera,
  only the queued `MODE_SYPHON` handoff is cancelled; the acquired target and
  stock target/weapon logic remain intact. First-person weapon modes are not
  cancelled, and classic/non-mouse camera users retain stock Syphon behavior.

Neither path changes San Andreas or native keyboard/mouse aiming.


### GTA III M16 first-person camera

The GTA III M16 is excluded from GInputNext's lock-on compatibility path.
When the current weapon is `WEAPONTYPE_M16`, GInputNext does not call
`FindWeaponLockOnTarget()` and does not suppress `m_bUseMouse3rdPerson` around
`ProcessPlayerWeapon()`. GTA III therefore owns the complete
`MODE_M16_1ST_PERSON` enter/hold/exit sequence exactly as stock.

Default packaged Y settings:

```text
GTA III / Vice City: InvertCameraY=0, InvertAimY=0
San Andreas:         InvertCameraY=1, InvertAimY=1
```


## v15 classic first-person right-stick routing

GTA III and Vice City retail controller first-person weapon cameras consume
`SniperModeLookLeftRight/UpDown`, which are sourced from the game left stick.
For these weapons only, GInputNext routes the physical right stick into that
retail aim channel and clears the game's right-stick look-around channel.
This prevents the two camera-input paths from competing while preserving
left-stick aiming as a fallback. GTA SA is not part of this compatibility path.
