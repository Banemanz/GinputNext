# GInputNext aiming notes

## User-facing vertical inversion

Defaults are now non-inverted across all supported games:

```ini
[Sticks]
InvertCameraY=0
InvertAimY=0
```

`InvertCameraY` controls normal third-person / vehicle / free-look vertical direction.

`InvertAimY` controls vertical stick direction while the logical Target button is held.
Exactly one policy is selected for a frame, so enabling both does not double-invert
while aiming.

`InvertRightY` from older builds is still accepted as a legacy alias for
`InvertCameraY`, but new configs should use the clearer names above.

## San Andreas native pad inversion

San Andreas is deliberately different from the III/VC workaround. SA already has
native pad globals for first-person weapon look:

- `CPad::bSniperAimWithRightStick` at `0x008CD782`
- `CPad::bInvertLook4Pad` at `0x00B73402`

The SA IDB dump shows `SniperModeLookLeftRight` / `SniperModeLookUpDown` choosing
between left-stick and right-stick fields via `bSniperAimWithRightStick`. The
vertical path then applies `bInvertLook4Pad` after the active stick has been
selected. `AimWeaponUpDown` and `LookAroundUpDown` also read the same native
invert byte.

Because of that, GInputNext must not pre-invert only `RightStickY` for SA. Doing
so makes right-stick sniper/RPG aim disagree with the game's left-stick fallback
aim. v18/v19 keeps the staged `PCTempJoyState` axes in the normal SA controller
convention and drives the native `bInvertLook4Pad` byte instead, writing the opposite of the user-facing invert flag because SA's byte sign is opposite after SDL staging.

GInputNext also keeps `bSniperAimWithRightStick=1`, matching SA's stock data
default and ensuring sniper/RPG aim primarily follows the physical right stick
while movement remains on the left stick.

## III / VC classic first-person weapon routing

GTA III and Vice City still have a separate classic PC-era weapon-camera quirk.
Their stock first-person weapon cameras read the left stick for sniper/M16/RPG
aim, while the normal right-stick look channel can compete with that camera.

For those games only, while Target is held with classic first-person weapons,
GInputNext routes the physical right stick into the retail left-stick aim channel
and clears the generic right-stick channel. San Andreas does not use this III/VC
left-stick reroute because it has the native right-stick sniper/RPG selector
described above.
