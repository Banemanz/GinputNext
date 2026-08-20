from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def route(left_x, left_y, right_x, right_y, first_person):
    if not first_person:
        return (left_x, left_y, right_x, right_y)
    use_right = right_x != 0.0 or right_y != 0.0
    return (
        right_x if use_right else left_x,
        right_y if use_right else left_y,
        0.0,
        0.0,
    )


# Normal gameplay stays modern dual-stick: left moves, right looks.
assert route(.5, -.25, .75, .1, False) == (.5, -.25, .75, .1)

# III/VC first-person weapon aim: physical right stick feeds retail LEFT-stick aim.
assert route(.5, -.25, .75, .1, True) == (.75, .1, 0.0, 0.0)

# Preserve original left-stick first-person aiming when right stick is centered.
assert route(.5, -.25, 0.0, 0.0, True) == (.5, -.25, 0.0, 0.0)

adapter = (ROOT / "src" / "GTAAdapter.cpp").read_text()
assert "classicFirstPersonAim" in adapter
assert "d.LeftStickX = AxisToPad(useRightStick ? s.rightX : s.leftX);" in adapter
assert "d.LeftStickY = AxisToPad(useRightStick ? rightY : s.leftY);" in adapter
assert "d.RightStickX = 0;" in adapter
assert "d.RightStickY = 0;" in adapter
assert "#if defined(GTA3) || defined(GTAVC)" in adapter

# SA must retain the ordinary right-stick staging branch.
assert "#else\n    d.RightStickX = AxisToPad(s.rightX);" in adapter

print("III/VC first-person physical-right -> retail-left aim routing smoke: OK")
