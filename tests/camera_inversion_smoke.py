def y_policy(targeting, invert_camera, invert_aim, raw_y):
    invert = invert_aim if targeting else invert_camera
    return -raw_y if invert else raw_y

# Both enabled: same corrected direction, no double inversion.
assert y_policy(False, True, True, 0.75) == -0.75
assert y_policy(True,  True, True, 0.75) == -0.75

# Independent preferences.
assert y_policy(False, False, True, 0.75) == 0.75
assert y_policy(True,  False, True, 0.75) == -0.75
assert y_policy(False, True, False, -0.4) == 0.4
assert y_policy(True,  True, False, -0.4) == -0.4

print("camera/aim inversion policy smoke: OK")


from pathlib import Path
ROOT = Path(__file__).resolve().parents[1]
adapter = (ROOT / "src" / "GTAAdapter.cpp").read_text()
config_h = (ROOT / "src" / "Config.h").read_text()
sa_ini = (ROOT / "config" / "GInputNext.ini").read_text()

# San Andreas must use native pad inversion instead of pre-inverting only RightStickY.
assert "kPadInvertLook4Pad = 0x00B73402" in adapter
assert "kPadSniperAimWithRightStick = 0x008CD782" in adapter
assert "PadBool(kPadInvertLook4Pad) = !userInvertVertical;" in adapter
assert "PadBool(kPadSniperAimWithRightStick) = true;" in adapter
assert "bool invertCameraY = false;" in config_h
assert "bool invertAimY = false;" in config_h
assert "InvertCameraY=0" in sa_ini
assert "InvertAimY=0" in sa_ini


# SA native bInvertLook4Pad has the opposite sign from GInputNext's normalized user setting.
def sa_native_byte(user_invert):
    return not user_invert

assert sa_native_byte(False) is True
assert sa_native_byte(True) is False

print("SA native pad inversion policy audit: OK")
