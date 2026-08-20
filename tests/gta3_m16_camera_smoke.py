from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

hook = (ROOT / "src" / "GTA3WeaponAimHook.cpp").read_text()
bridge = (ROOT / "src" / "GameplayBridge.cpp").read_text()
policy = (ROOT / "src" / "ClassicFirstPersonAim.h").read_text()

# The whole III first-person family bypass must happen before the lock-on
# compatibility flag is altered.
bypass_pos = hook.index("if (IsClassicFirstPersonAimWeapon(player))")
flag_pos = hook.index("CCamera::m_bUseMouse3rdPerson = false;")
assert bypass_pos < flag_pos

# Shared AutoAim must not pre-acquire targets for III/VC first-person weapons.
assert "if (IsClassicFirstPersonAimWeapon(player))" in bridge
assert "acquiredTarget_ = false;" in bridge
assert "nextRetryFrame_ = frame;" in bridge

# GTA III first-person family is complete.
for token in ("WEAPONTYPE_M16", "WEAPONTYPE_SNIPERRIFLE", "WEAPONTYPE_ROCKETLAUNCHER"):
    assert token in policy

# No broad special-camera cancellation was added. Only the existing Syphon
# cancellation remains, so first-person modes are left to retail code.
assert "TheCamera.m_PlayerWeaponMode.Mode == MODE_SYPHON" in hook
assert "MODE_M16_1ST_PERSON" not in hook
assert "MODE_SNIPER" not in hook
assert "MODE_ROCKET" not in hook

print("GTA III first-person family stock-camera isolation smoke: OK")
