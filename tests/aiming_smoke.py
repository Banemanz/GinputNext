def target_held(mode, rb, lb):
    return lb if mode == 3 else rb

def aim_y(mode, rb, lb, y, invert):
    return -y if invert and target_held(mode, rb, lb) else y

assert aim_y(0, True, False, 0.75, True) == -0.75
assert aim_y(0, False, False, 0.75, True) == 0.75
assert aim_y(3, False, True, -0.4, True) == 0.4
assert aim_y(3, True, False, -0.4, True) == -0.4
print('aim-only inversion smoke: OK')

class AutoAim:
    def __init__(self):
        self.prev = False
        self.acquired = False
        self.retry = 0
    def frame(self, frame, target, acquire_result):
        pressed = target and not self.prev
        self.prev = target
        if not target:
            self.acquired = False
            self.retry = frame
            return False
        called = pressed or (not self.acquired and frame >= self.retry)
        if called:
            self.acquired = acquire_result
            self.retry = frame + 6
        return called

a = AutoAim()
assert a.frame(100, True, False) is True
assert a.frame(101, True, False) is False
assert a.frame(106, True, True) is True
assert a.frame(107, True, True) is False
assert a.frame(108, False, False) is False
assert a.frame(109, True, True) is True
print('autoaim retry smoke: OK')
