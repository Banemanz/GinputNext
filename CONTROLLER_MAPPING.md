# GInputNext v6 controller mapping

## Why the GC201 was wildly wrong

Observed runtime signature:

```text
GC201 Controller1.00
axes=4 buttons=13 hats=1
```

The old fallback treated raw b7 as Start and b9 as R3. On the common
PlayStation-position DirectInput layout, those are R2 and Start respectively.
That exactly creates the reported symptom where physical Start behaves like
R3/look-behind and another shoulder/trigger can act like Pause.

The old fallback also requested trigger axes 4/5 from a device exposing only
four axes. Its invalid-axis value was 0, and the centered-trigger conversion
turned 0 into about 50% pressed. That could create permanent phantom trigger
input.

## v6 GC201 auto-map

For a device name containing GC201 with 4 axes, at least 12 buttons and a hat,
v6 adds this SDL GameController mapping before opening the device:

```text
b0  Square
b1  Cross
b2  Circle
b3  Triangle
b4  L1
b5  R1
b6  L2
b7  R2
b8  Select
b9  Start
b10 L3
b11 R3
b12 Guide/Home
hat0 D-pad
a0/a1 left stick
a2/a3 right stick
```

SDL then exposes it through the normalized controller path.

## GTA logical mapping

GInputNext feeds GTA's PlayStation-named logical fields rather than hardcoding
specific gameplay actions:

```text
A -> Cross
B -> Circle
X -> Square
Y -> Triangle
Start -> Start/Pause
Back -> Select
```

GTA itself then decides what those logical buttons do on foot, in vehicles,
in menus and in scripts.
