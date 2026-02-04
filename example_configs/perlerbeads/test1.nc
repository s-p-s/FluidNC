(--- TCS3472 JSON output demo ---)
(Useful if you want to parse results on a serial logger.)

G90
G21

;Positions
#<pickup>=[360.0-136.0]
#<scanner>=[360.0-90.0]
#<dump_0>=0.0
#<dump_1>=36.0
#<dump_2>=72.0
#<dump_3>=108.0
#<dump_4>=144.0
#<dump_5>=180.0

#<shake>=3.0
#<shake_repeats>=10
#<feedrate>=40000

O100 REPEAT [5]
    ;Move to Pickup position and shake
    G1 X#<pickup> F#<feedrate>
    G91
    O110 REPEAT [#<shake_repeats>]
        G1 X[#<shake>] F#<feedrate>
        G1 X[-#<shake>] F#<feedrate>
    O110 ENDREPEAT
    G90
    
    ;Move to scanner
    G1 X#<scanner> F#<feedrate>
    G4 P0.2

    ;Restet position
    G1 X360.0 F#<feedrate>
    G92 X0

    ;Goto dump positions
    G1 X#<dump_0> F#<feedrate>
    G4 P0.2

    ;Stir the pot
    G91
    G1 X[4*360.0] F80000
    G90
    G92 X[#<_x>MOD 360.0]
O100 ENDREPEAT

G0 X0

M30
