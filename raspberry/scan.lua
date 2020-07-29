
-- speed defaults
-- ----------------------------
-- define MOVE_MAX_DELAY 300000
-- define MOVE_MIN_DELAY 22000
-- define START_SLOPE 45000

--home()
function sleep(n)
  os.execute("sleep " .. tonumber(n))
end


function split (inputstr, sep)
	if sep == nil then
			sep = "%s"
	end
	local t={}
	for str in string.gmatch(inputstr, "([^"..sep.."]+)") do
			table.insert(t, str)
	end
	return t
end

--[[
function move_to(a,b,c,d,e)
end

function set_speed(a,b,c)
end

nullframe = false

function get_face_rect()
    if nullframe == true
        then
            return "0000,0000,0000,0000"
        end
    return "12a34,1234,3344,444"
end
]]--


function scan()
    return {'0', 0, 0}
end


function mult(w)
    if w > 0 then
       return -0.0055 * w + 5.0
    end
    return 3
end

set_speed(4000, 13000*20, 200000*20) -- slope min max
base_rotation = 0
head_rotation = -7500
move_multiplier = 2.25 
base_positions = {0, 10000, -20000, 30000}
head_rotations = {0, -15000}
base_positions_index = 1
head_rotation_index = 1
zoom = -8000
move_to(base_rotation,zoom,0,-1500,-7500)
while true
    do
    set_speed(4000, 13000*20, 200000*20) -- slope min max
    move_to(-20000,zoom,0,-1500,-7500)
    set_speed(4000, 13000*20, 200000*20) -- slope min max
    move_to(30000,zoom,0,-1500,-7500)
    set_speed(4000, 13000*20, 200000*20) -- slope min max
    move_to(32000,zoom,0,-1500,-7500)
end

