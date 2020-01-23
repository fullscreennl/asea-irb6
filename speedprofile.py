import matplotlib.pyplot as plt

# settings
max_speed = 200
min_speed = 100
slope = 1000
steps = 8000

# init
speed_delta = max_speed - min_speed
speed = min_speed
half_slope = slope / 2.0
increment = 0
decrement = 0
f = speed_delta / slope

data = []
accel = []
for step in range(steps):
    prev_speed = speed
    if step < slope: # easing in
        if step < half_slope:
            increment += (f*2 / half_slope)
        else:
            increment -= (f*2 / half_slope)
        speed += increment
    if step > steps-slope: # easing out
        if step < steps-half_slope:
            decrement += (f*2 / half_slope)
        else:
            decrement -= (f*2 / half_slope)
        speed -= decrement
    acc = speed - prev_speed
    data.append(speed)
    accel.append(acc*100)


plt.plot(data)
plt.plot(accel)
plt.ylabel('ASEA irb6 speed')
plt.show()