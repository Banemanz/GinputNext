class Bridge:
    def __init__(self):
        self.last_frame = None
        self.prev = False
        self.old = False
        self.new = False

    def update(self, frame, start):
        if frame != self.last_frame:
            self.last_frame = frame
            self.old = self.prev
            self.new = start
            self.prev = start
        return self.old, self.new

b = Bridge()
# first call this frame: Start edge
assert b.update(100, True) == (False, True)
# second UpdatePads call in SAME frame must preserve same edge
assert b.update(100, True) == (False, True)
# next frame while held
assert b.update(101, True) == (True, True)
# release edge
assert b.update(102, False) == (True, False)
# next press
assert b.update(103, True) == (False, True)
print('pause bridge frame-edge smoke: OK')
