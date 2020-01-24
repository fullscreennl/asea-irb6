import matplotlib.pyplot as plt

# settings
max_delay = 200
min_delay = 100
slope = 10000
steps = 30000

# init
speed_delta = max_delay - min_delay
delay = max_delay
half_slope = slope / 2.0
increment = 0
decrement = 0
f = speed_delta / slope
a = (f*2 / half_slope)

data = []
for step in range(steps):
    if step < slope: # easing in
        if step < half_slope:
            increment += a
        else:
            increment -= a
        delay -= increment
    if step > steps-slope: # easing out
        if step < steps-half_slope:
            decrement += a
        else:
            decrement -= a
        delay += decrement
    data.append(delay)

plt.plot(data)
plt.ylabel('ASEA irb6 delay')
plt.show()