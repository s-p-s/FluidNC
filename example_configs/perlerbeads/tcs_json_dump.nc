(--- TCS3472 JSON output demo ---)
(Useful if you want to parse results on a serial logger.)

G90
G21

G0 X0 Y0
G4 P0.2

$TCS=READ id=0 fresh=yes p=tcs json=yes

M2
