
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
    rect = get_face_rect()
    print(rect)
    if (rect ~= "0000,0000,0000,0000") 
    then
        elems = split(rect, ",")
        local x = tonumber(elems[1])
        local y = tonumber(elems[2])
        local w = tonumber(elems[3])
        local h = tonumber(elems[4])
        if (x == nil or y == nil or w == nil or h == nil)
        then
			sleep(0.01)
			scan()
        else
            center_x = x + w/2.0
            center_y = y + h/2.0
            screen_center_x = 1920/2
            screen_center_y = 1080/2
            if (center_x < screen_center_x)
            then
                return {'r', screen_center_x - center_x}
            else
                return {'l', center_x - screen_center_x}
            end
        end
    else
        print('no face')
        return {'0', 0}
    end
end

set_speed(2000, 13000*20, 200000*20) -- slope min max
base_rotation = 0
move_multiplier = 3
while true
    do
    move_to(base_rotation,-8000,0,-1500,-7500)
    dir = scan()
    if dir ~= nil
    then
        print("- - - - ")
        print(dir[1])
        print(dir[2])
        set_speed(600, 13000*20, 300000*20)
        if (dir[1] == 'l') then
            base_rotation = base_rotation - dir[2] * move_multiplier  
        elseif (dir[1] == 'r') then
            base_rotation = base_rotation + dir[2] * move_multiplier
        else
            set_speed(2000, 13000*20, 200000*20)
            base_rotation = 20000
        end
    end

    move_to(base_rotation,-8000,0,-1500,-7500)
    dir = scan()
    if dir ~= nil
    then
        print("- - - - ")
        print(dir[1])
        print(dir[2])
        set_speed(600, 13000*20, 300000*20)
        if (dir[1] == 'l') then
            base_rotation = base_rotation - dir[2] * move_multiplier
        elseif (dir[1] == 'r') then
           base_rotation = base_rotation + dir[2] * move_multiplier
        else
            set_speed(2000, 13000*20, 200000*20)
            base_rotation = 0
        end
    end
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
