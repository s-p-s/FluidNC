(--- TCS3472 read demo ---)
(Goes to a fixed point, takes a reading, prints values, stores params.)

G90
G21

(1) Move to sensor position (edit as needed)
G0 X0 Y0
G4 P0.2

(2) Fresh reading; write named params with prefix "tcs"
$TCS=READ id=0 fresh=yes p=tcs

(3) Repeat last reading without waiting
$TCS=LAST id=0 p=tcs

(4) Example: stop if it is too dark / no bead present
O10 IF [#<TCS_C> LT 50]
  M0
O10 ENDIF

M2
