class Pad:
    def __init__(self):
        self.old = 0
        self.new = 0
        self.tempjoy = 0

    def updatepads(self):
        self.old = self.new
        self.new = self.tempjoy
        self.tempjoy = 0

p = Pad()

p.tempjoy = 255
p.updatepads()
assert p.new == 255 and p.old == 0
assert p.new and not p.old

p.tempjoy = 0
p.updatepads()
assert p.new == 0 and p.old == 255
assert (not p.new) and p.old

print("pre-UpdatePads edge semantics: OK")
