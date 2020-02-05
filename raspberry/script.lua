-- speed defaults
-- ----------------------------
-- define MOVE_MAX_DELAY 300000
-- define MOVE_MIN_DELAY 22000
-- define START_SLOPE 45000

--home()

set_speed(3000, 13000*3, 300000*3) -- slope min max
move_to(0,0,0,28000,-28000)
--rect = get_face_rect()
-- print(rect)
move_to(0,0,0,0,0)
move_to(0,0,0,2000,0)
move_to(0,0,0,0,0)
move_to(0,0,0,2000,0)


--set_speed(50000, 5000, 300000) -- slope min max
--move_to(0,0,0,140000,-140000)
--move_to(0,0,0,0,0)

--[[
set_speed(10000, 40000, 300000) -- slope min max
move_to(0,0,0,140000,-140000)
move_to(0,0,0,0,0)

set_speed(10000, 20000, 300000) -- slope min max
move_to(0,0,0,140000,-140000)
move_to(0,0,0,0,0)

set_speed(10000, 10000, 300000) -- slope min max
move_to(0,0,0,140000,-140000)
move_to(0,0,0,0,0)

set_speed(10000, 5000, 300000) -- slope min max
move_to(0,0,0,140000,-140000)
move_to(0,0,0,0,0)




move_to(0,0,0,-10000,0)
move_to(0,0,0,140000,-140000)
move_to(0,0,0,140000,-140000+35000)
move_to(0,0,0,140000,-140000)
move_to(0,0,0,0,0)


move_to(0,0,0,-10000,0)
move_to(0,0,0,140000,-140000)
move_to(0,0,0,140000,-140000+35000)
move_to(0,0,0,140000,-140000)
move_to(0,0,0,0,0)

-- start me up
scanpos_upper_arm = -40000

move_to(0,0,0,-10000,0)
move_to(0,scanpos_upper_arm,0,-10000,-35000)

move_to(120000, 40000, 30000,0, -30000)
move_to(120000, 40000, 30000, 0, 0)
move_to(-120000, -30000, -20000, 0, -30000)
move_to(-120000, -30000, -20000, 0, -50000)

move_to(0,scanpos_upper_arm,0,-10000,-80000)
move_to(0,scanpos_upper_arm,0,-10000,10000)
move_to(0,scanpos_upper_arm,0,-10000,-80000)
move_to(0,scanpos_upper_arm,0,-10000,10000)
move_to(0,scanpos_upper_arm,0,-10000,-35000)

set_speed(20000, 80000, 300000) -- slope min max
]]--
set_speed(2000, 20000*5, 300000*5) -- slope min max
move_to(24000,-8000,0,-2000,-5000)
move_to(24000,8000,0,-2000,-5000)
move_to(24000,-8000,0,-2000,-5000)
move_to(0,0,0,0,0)
