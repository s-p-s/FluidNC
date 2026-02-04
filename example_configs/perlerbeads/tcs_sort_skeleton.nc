(--- Perler bead sorter skeleton (hostless) ---)
(This is a template: fill in coordinates and classification rules.)

G90
G21

(Positions - edit these)
#<SENSOR_X>=0
#<SENSOR_Y>=0
#<BIN_RED_X>=50
#<BIN_RED_Y>=0
#<BIN_OTHER_X>=50
#<BIN_OTHER_Y>=50

(Loop counter)
#<I>=0

O100 WHILE [#<I> LT 10]
  (Move bead under sensor)
  G0 X#<SENSOR_X> Y#<SENSOR_Y>
  G4 P0.1

  (Read sensor)
  $TCS=READ id=0 fresh=yes p=tcs

  (Very simple classification example)
  O110 IF [#<TCS_C> LT 50]
    (No bead / too dark)
    M0
  O110 ENDIF

  ("Red-ish" if normalized red is high)
  O120 IF [#<TCS_RN> GT 0.45]
    G0 X#<BIN_RED_X> Y#<BIN_RED_Y>
  O120 ELSE
    G0 X#<BIN_OTHER_X> Y#<BIN_OTHER_Y>
  O120 ENDIF

  (Increment loop counter)
  #<I>=[#<I>+1]
O100 ENDWHILE

M2
