from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


class VCPersistentMouseAimBridge:
    def __init__(self):
        self.active = False
        self.saved = False

    def apply(self, current_mouse_cam, enabled):
        if enabled:
            if not self.active:
                self.saved = current_mouse_cam
                self.active = True
            return False
        if self.active:
            restored = self.saved
            self.active = False
            return restored
        return current_mouse_cam


# VC classic lock-on weapons retain the v12 behavior exactly.
b = VCPersistentMouseAimBridge()
cam = True
cam = b.apply(cam, True)
assert cam is False
cam = b.apply(cam, True)
assert cam is False
cam = b.apply(cam, False)
assert cam is True

# If VC already had mouse-camera mode disabled, never force it on.
b = VCPersistentMouseAimBridge()
cam = False
cam = b.apply(cam, True)
assert cam is False
cam = b.apply(cam, False)
assert cam is False


def gta3_scoped_weapon_pass(saved_mouse_cam, controller_target, has_lock, queued_mode):
    """Model the v15 GTA III ProcessPlayerWeapon shim for normal lock-on weapons."""
    during_weapon = False if controller_target else saved_mouse_cam
    after_weapon = saved_mouse_cam
    clear_syphon = (
        controller_target
        and saved_mouse_cam
        and has_lock
        and queued_mode == "SYPHON"
    )
    return during_weapon, after_weapon, clear_syphon


# GTA III normal lock-on: only ProcessPlayerWeapon sees the compatibility flag.
during, after, clear = gta3_scoped_weapon_pass(True, True, True, "SYPHON")
assert during is False
assert after is True
assert clear is True

# GTA III classic/non-mouse camera users retain stock Syphon camera.
during, after, clear = gta3_scoped_weapon_pass(False, True, True, "SYPHON")
assert during is False
assert after is False
assert clear is False

# First-person queued modes are never cancelled by the stabilization rule.
during, after, clear = gta3_scoped_weapon_pass(True, True, True, "SNIPER")
assert during is False and after is True and clear is False

# No controller targeting => no compatibility behavior.
during, after, clear = gta3_scoped_weapon_pass(True, False, False, "NONE")
assert during is True and after is True and clear is False


def classic_first_person_weapon(game, weapon):
    iii = {"M16", "SNIPER", "RPG"}
    vc = {"M4", "RUGER", "SNIPER", "LASERSCOPE", "RPG", "M60", "CAMERA"}
    return weapon in (iii if game == "III" else vc)


# Entire III first-person family gets neither injected lock-on nor camera shim.
for weapon in ("M16", "SNIPER", "RPG"):
    assert classic_first_person_weapon("III", weapon)

# Entire VC first-person family stands down from the persistent lock-on override.
for weapon in ("M4", "RUGER", "SNIPER", "LASERSCOPE", "RPG", "M60", "CAMERA"):
    assert classic_first_person_weapon("VC", weapon)

# Normal lock-on weapons remain on the compatibility paths.
assert not classic_first_person_weapon("III", "AK47")
assert not classic_first_person_weapon("VC", "UZI")

# Static scope audit: runtime hook belongs only to GTA III project.
gta3_proj = (ROOT / "projects" / "GInputNext_GTA3.vcxproj").read_text()
vc_proj = (ROOT / "projects" / "GInputNext_GTAVC.vcxproj").read_text()
sa_proj = (ROOT / "projects" / "GInputNext_GTASA.vcxproj").read_text()
assert "GTA3WeaponAimHook.cpp" in gta3_proj
assert "GTA3WeaponAimHook.cpp" not in vc_proj
assert "GTA3WeaponAimHook.cpp" not in sa_proj

policy = (ROOT / "src" / "ClassicFirstPersonAim.h").read_text()
for token in (
    "WEAPONTYPE_M16", "WEAPONTYPE_SNIPERRIFLE", "WEAPONTYPE_ROCKETLAUNCHER",
    "WEAPONTYPE_M4", "WEAPONTYPE_RUGER", "WEAPONTYPE_LASERSCOPE",
    "WEAPONTYPE_M60", "WEAPONTYPE_CAMERA",
):
    assert token in policy

bridge = (ROOT / "src" / "GameplayBridge.cpp").read_text()
assert "!classicFirstPersonAim" in bridge
assert "if (IsClassicFirstPersonAimWeapon(player))" in bridge

hook = (ROOT / "src" / "GTA3WeaponAimHook.cpp").read_text()
assert "return 0x4F1EF0;" in hook
assert "CCamera::m_bUseMouse3rdPerson = false;" in hook
assert "TheCamera.m_PlayerWeaponMode.Mode == MODE_SYPHON" in hook
assert "TheCamera.ClearPlayerWeaponMode();" in hook
assert "if (IsClassicFirstPersonAimWeapon(player))" in hook

print("III/VC v15 first-person exclusion + classic lock-on compatibility smoke: OK")
