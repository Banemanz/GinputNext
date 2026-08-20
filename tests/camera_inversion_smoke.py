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
