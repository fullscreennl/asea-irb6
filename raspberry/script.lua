-- speed defaults
-- ----------------------------
-- define MOVE_MAX_DELAY 300000
-- define MOVE_MIN_DELAY 22000
-- define START_SLOPE 45000

-- home()

set_speed(5000, 1000, 300000) -- slope min max
--[[
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


move_to(0,0,0,-10000,0)
move_to(0,0,0,140000,-140000)
move_to(0,0,0,140000,-140000+35000)
move_to(0,0,0,140000,-140000)
move_to(0,0,0,0,0)

-- start me up
scanpos_upper_arm = -40000

move_to(0,0,0,-10000,0)
move_to(0,scanpos_upper_arm,0,-10000,-35000)

--move_to(120000, 40000, 30000,0, -30000)
--move_to(120000, 40000, 30000, 0, 0)
--move_to(-120000, -30000, -20000, 0, -30000)
--move_to(-120000, -30000, -20000, 0, -50000)

move_to(0,scanpos_upper_arm,0,-10000,-80000)
move_to(0,scanpos_upper_arm,0,-10000,10000)
--move_to(0,scanpos_upper_arm,0,-10000,-80000)
--move_to(0,scanpos_upper_arm,0,-10000,10000)
--move_to(0,scanpos_upper_arm,0,-10000,-35000)
]]--

scanpos_upper_arm = -40000

move_to(120000,scanpos_upper_arm,0,-10000,-35000)
move_to(120000,-scanpos_upper_arm,0,-10000,-35000)
move_to(120000,-scanpos_upper_arm,0,-10000,-80000)
move_to(120000,-scanpos_upper_arm,30000,-10000,-10000)

move_to(0,0,0,0,0)
