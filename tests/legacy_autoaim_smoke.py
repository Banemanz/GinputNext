class LegacyAimBridge:
    def __init__(self, mouse_mode=True):
        self.mouse_mode = mouse_mode
        self.saved = True
        self.active = False

    def update(self, autoaim, targeting):
        if autoaim and targeting:
            if not self.active:
                self.saved = self.mouse_mode
                self.active = True
            self.mouse_mode = False
        elif self.active:
            self.mouse_mode = self.saved
            self.active = False


# Mouse third-person aiming is temporarily disabled while controller Target is held.
b = LegacyAimBridge(True)
b.update(True, True)
assert b.mouse_mode is False and b.active is True
b.update(True, True)
assert b.mouse_mode is False
b.update(True, False)
assert b.mouse_mode is True and b.active is False

# Preserve an already-disabled mouse camera mode after release.
b = LegacyAimBridge(False)
b.update(True, True)
assert b.mouse_mode is False
b.update(False, True)
assert b.mouse_mode is False and b.active is False

print("legacy III/VC autoaim camera ownership smoke: OK")
