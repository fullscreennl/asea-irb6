
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

set_speed(2000, 13000*20, 200000*20) -- slope min max
base_rotation = 0
head_rotation = -7500
move_multiplier = 2.25 
base_positions = {0, 10000, -20000, 30000}
head_rotations = {0, -15000}
base_positions_index = 1
head_rotation_index = 1
zoom = -8000
while true
    do
    move_to(base_rotation,zoom,0,-1500,-7500)
    dir = scan()
    if dir ~= nil
    then
        move_multiplier = mult(dir[3])
        if dir[2] < 200  and dir[1] ~= '0' then
            zoom = 7000
            goto continue
        end
        print("- - - - ")
        print(dir[1])
        print(dir[2])
        set_speed(600, 23000*20, 300000*20)
        if (dir[1] == 'l') then
            zoom = 7000
            base_rotation = base_rotation - dir[2] * move_multiplier  
        elseif (dir[1] == 'r') then
            zoom = 7000
            base_rotation = base_rotation + dir[2] * move_multiplier
        else
            set_speed(2000, 13000*10, 200000*10)
            zoom = -8000
            move_to(base_rotation,zoom,0,-1500,head_rotation)
            set_speed(2000, 13000*20, 200000*20)
            
            base_positions_index = base_positions_index + 1
            if base_positions_index > 4 then    
               base_positions_index = 1 
            end
            base_rotation = base_positions[base_positions_index]

            head_rotation_index = head_rotation_index + 1
            if head_rotation_index > 2 then    
               head_rotation_index = 1 
            end
            head_rotation = head_rotations[head_rotation_index]
            
        end
    end
    ::continue::
end

move_to(0,0,0,0,0)

--[[
set_speed(2000, 20000*20, 300000*20) -- slope min max
move_to(0,-8000,0,-1500,-7500)

--set_speed(3000, 13000*3, 300000*3) -- slope min max
move_to(0,-8000,0,-1500,1000)
move_to(0,-8000,0,-1500,-7500)
move_to(0,-8000,0,-1500,-16000)

--set_speed(2000, 20000*5, 300000*5) -- slope min max
move_to(20000,-8000,0,-1500,-7500)

--set_speed(3000, 13000*3, 300000*3) -- slope min max
move_to(20000,-8000,0,-1500,1000)
move_to(20000,-8000,0,-1500,-7500)
move_to(20000,-8000,0,-1500,-16000)

--set_speed(2000, 20000*5, 300000*5) -- slope min max
move_to(35000,-8000,0,-1500,-7500)

--set_speed(3000, 13000*3, 300000*3) -- slope min max
move_to(35000,-8000,0,-1500,1000)
move_to(35000,-8000,0,-1500,-7500)
move_to(35000,-8000,0,-1500,-16000)

--set_speed(2000, 20000*5, 300000*5) -- slope min max
move_to(-20000,-8000,0,-1500,-7500)

--set_speed(3000, 13000*3, 300000*3) -- slope min max
move_to(-20000,-8000,0,-1500,1000)
move_to(-20000,-8000,0,-1500,-7500)
move_to(-20000,-8000,0,-1500,-16000)

--set_speed(2000, 20000*5, 300000*5) -- slope min max
move_to(-35000,-8000,0,-1500,-7500)

--set_speed(3000, 13000*3, 300000*3) -- slope min max
move_to(-35000,-8000,0,-1500,1000)
move_to(-35000,-8000,0,-1500,-7500)
move_to(-35000,-8000,0,-1500,-16000)

--home
--set_speed(2000, 20000*5, 300000*5) -- slope min max
move_to(0,0,0,0,0)

-- rect = get_face_rect()
-- print(rect)
]]--
