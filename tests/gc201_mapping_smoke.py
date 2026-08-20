raw = [0] * 13

def norm(raw):
    return {
        'square': raw[0], 'cross': raw[1], 'circle': raw[2], 'triangle': raw[3],
        'l1': raw[4], 'r1': raw[5], 'l2': raw[6], 'r2': raw[7],
        'select': raw[8], 'start': raw[9], 'l3': raw[10], 'r3': raw[11],
        'guide': raw[12],
    }

raw[9] = 1
n = norm(raw)
assert n['start'] == 1 and n['r3'] == 0 and n['r2'] == 0
raw = [0] * 13
raw[11] = 1
n = norm(raw)
assert n['r3'] == 1 and n['start'] == 0
raw = [0] * 13
raw[1] = 1
n = norm(raw)
assert n['cross'] == 1 and n['square'] == 0
print('GC201 mapping smoke: OK')
