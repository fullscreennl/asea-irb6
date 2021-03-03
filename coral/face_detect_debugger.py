from periphery import GPIO
gpio_line3 = GPIO(73, "out")
gpio_line1 = GPIO(138, "out")
gpio_line2 = GPIO(140, "out")
gpio_line0 = GPIO(6, "out") 
gpio_line0.write(False)
gpio_line1.write(False)
gpio_line2.write(False)
gpio_line3.write(False)
current = 0

gpios = [gpio_line0,gpio_line1,gpio_line2,gpio_line3]

class Constants:
    NO_FACE = 0
    X_X = 1
    X_U = 2
    X_D = 3
    L_X = 4
    L_U = 5
    L_D = 6
    R_X = 7
    R_U = 8
    R_D = 9       

def dec2bin(n):
    return bin(n).replace("0b", "").zfill(4)  

while True:
    t = input('stroke key for face detection simulation, current_value: ' + str(current))
    t = int(t)
    current = t
    binString = dec2bin(t)
    for count, a in enumerate(binString):
        gpios[count].write(bool(int(a)))
