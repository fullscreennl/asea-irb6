onder = 0
boven = -8000

set_speed(20000, 20000*10, 300000*10) -- slope min max
move_to(0, boven, onder, 1800, -7500)

onder = 1000
set_speed(20000, 20000*10, 300000*10) -- slope min max
move_to(0, boven, onder, 1800, -7500)

set_speed(20000, 20000*10, 300000*10) -- slope min max
move_to(0,0,0,0,0)

