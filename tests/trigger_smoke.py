def trig(value, valid, centered):
    if not valid:
        return 0.0
    if centered:
        return max(0.0, min(1.0, (value + 32768.0) / 65535.0))
    return max(0.0, min(1.0, max(0, value) / 32767.0))

assert trig(0, False, True) == 0.0
assert 0.49 < trig(0, True, True) < 0.51
print('trigger invalid-axis smoke: OK')
