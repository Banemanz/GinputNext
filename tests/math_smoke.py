import math

def radial(x, y, inner, outer, sensitivity):
    mag = math.sqrt(x*x+y*y)
    if mag <= inner or mag <= 1e-5:
        return (0.0, 0.0)
    usable = max(inner+1e-4, 1.0-outer)
    scaled = min(max((mag-inner)/(usable-inner), 0.0), 1.0)*sensitivity
    inv = 1.0/mag
    return (max(-1,min(1,x*inv*scaled)), max(-1,min(1,y*inv*scaled)))

assert radial(0.05,0.05,0.15,0.02,1.0)==(0.0,0.0)
x,y = radial(1.0,0.0,0.15,0.02,1.0)
assert 0.99 <= x <= 1.0 and abs(y) < 1e-6
x,y = radial(0.5,0.5,0.15,0.02,1.0)
assert x > 0 and y > 0 and math.sqrt(x*x+y*y) <= 1.01
print("deadzone smoke: OK")
