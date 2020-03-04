#include <stdio.h>
#include <math.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <fcntl.h> //Used for UART
#include <termios.h>	

#include <wiringPi.h>
#include <boilerplate/ancillaries.h>
#include <alchemy/task.h>
#include <alchemy/timer.h>

#include "lua-5.2.3/lua.h"
#include "lua-5.2.3/lauxlib.h"
#include "lua-5.2.3/lualib.h"

/**

 +-----+-----+---------+------+---+---Pi 3B+-+---+------+---------+-----+-----+
 | BCM | wPi |   Name  | Mode | V | Physical | V | Mode | Name    | wPi | BCM |
 +-----+-----+---------+------+---+----++----+---+------+---------+-----+-----+
 |     |     |    3.3v |      |   |  1 || 2  |   |      | 5v      |     |     |
 |   2 |   8 |   SDA.1 |  OUT | 0 |  3 || 4  |   |      | 5v      |     |     |
 |   3 |   9 |   SCL.1 |  OUT | 0 |  5 || 6  |   |      | 0v      |     |     |
 |   4 |   7 | GPIO. 7 |  OUT | 0 |  7 || 8  | 0 | IN   | TxD     | 15  | 14  |
 |     |     |      0v |      |   |  9 || 10 | 1 | IN   | RxD     | 16  | 15  |
 |  17 |   0 | GPIO. 0 |  OUT | 1 | 11 || 12 | 0 | IN   | GPIO. 1 | 1   | 18  |
 |  27 |   2 | GPIO. 2 |  OUT | 0 | 13 || 14 |   |      | 0v      |     |     |
 |  22 |   3 | GPIO. 3 |  OUT | 0 | 15 || 16 | 0 | IN   | GPIO. 4 | 4   | 23  |
 |     |     |    3.3v |      |   | 17 || 18 | 0 | IN   | GPIO. 5 | 5   | 24  |
 |  10 |  12 |    MOSI |  OUT | 0 | 19 || 20 |   |      | 0v      |     |     |
 |   9 |  13 |    MISO |  OUT | 0 | 21 || 22 | 0 | IN   | GPIO. 6 | 6   | 25  |
 |  11 |  14 |    SCLK |   IN | 0 | 23 || 24 | 1 | IN   | CE0     | 10  | 8   |
 |     |     |      0v |      |   | 25 || 26 | 1 | IN   | CE1     | 11  | 7   |
 |   0 |  30 |   SDA.0 |   IN | 1 | 27 || 28 | 1 | IN   | SCL.0   | 31  | 1   |
 |   5 |  21 | GPIO.21 |   IN | 1 | 29 || 30 |   |      | 0v      |     |     |
 |   6 |  22 | GPIO.22 |   IN | 1 | 31 || 32 | 0 | IN   | GPIO.26 | 26  | 12  |
 |  13 |  23 | GPIO.23 |   IN | 0 | 33 || 34 |   |      | 0v      |     |     |
 |  19 |  24 | GPIO.24 |   IN | 0 | 35 || 36 | 0 | IN   | GPIO.27 | 27  | 16  |
 |  26 |  25 | GPIO.25 |   IN | 0 | 37 || 38 | 0 | IN   | GPIO.28 | 28  | 20  |
 |     |     |      0v |      |   | 39 || 40 | 0 | IN   | GPIO.29 | 29  | 21  |
 +-----+-----+---------+------+---+----++----+---+------+---------+-----+-----+
 | BCM | wPi |   Name  | Mode | V | Physical | V | Mode | Name    | wPi | BCM |
 +-----+-----+---------+------+---+---Pi 3B+-+---+------+---------+-----+-----+

*/
#define AXIS1_MOTOR_PULSE 0    // io17
#define AXIS1_MOTOR_DIR 1      // io18

#define AXIS2_MOTOR_PULSE 5    // io24
#define AXIS2_MOTOR_DIR 6      // io25

#define AXIS3_MOTOR_PULSE 21   // io5 
#define AXIS3_MOTOR_DIR 22     // io6

#define AXIS4_MOTOR_PULSE 4    // io23
#define AXIS4_MOTOR_DIR 3      // io22

#define AXIS5_MOTOR_PULSE 26   // io12
#define AXIS5_MOTOR_DIR 28     // io20

#define LIMIT1 7               // io4
#define LIMIT2 2               // io27
#define LIMIT3 29              // io21
#define LIMIT4 23              // io13
#define LIMIT5 25              // io26

#define SYNC_OVERSHOOT_STEPS 2000

#define SYNC_DELAY 4000000
#define PULSE_WIDTH 2000

#define CW HIGH
#define CCW LOW

#define MOVE_MAX_DELAY 300000
#define MOVE_MIN_DELAY 22000
#define SPEED_DELTA (MOVE_MAX_DELAY - MOVE_MIN_DELAY) 
#define START_SLOPE 45000
#define INCREMENT (SPEED_DELTA / START_SLOPE)

int uart0_filestream = -1;
const char * runReadLoop();
int looping = 1;

RT_TASK sync_task;

typedef struct AseaBotState{
    int steps_a1;
    int steps_a2;
    int steps_a3;
    int steps_a4;
    int steps_a5;
    float delay;
}BotState;

BotState *BOT; // global state of the robot

typedef struct BotSpeedState{
    float max_delay;
    float min_delay;
    float slope;
    float increment;
    float decrement;
    float speed_delta;
    float half_slope;
    float f;
    float a;
}SpeedState;

SpeedState *SPEED; // global speed state of the robot


void catch_signal(int sig){
}

int _digitalRead(int input){
    int hs = 0;
    int ls = 0;
    while(1){
        int state = digitalRead(input); 
        if(state == HIGH){
            hs ++;
        }else{
            ls ++;
        }
        if(hs > 100){
            // printf("h: %i l: %i\n", hs, ls);
            return HIGH;
        }
        if(ls > 100){
            // printf("h: %i l: %i\n", hs, ls);
            return LOW;
        }
    }
}

int setupSerial(){
	//OPEN THE UART
	//The flags (defined in fcntl.h):
	//	Access modes (use 1 of these):
	//		O_RDONLY - Open for reading only.
	//		O_RDWR - Open for reading and writing.
	//		O_WRONLY - Open for writing only.
	//
	//	O_NDELAY / O_NONBLOCK (same function) - Enables nonblocking mode. When set read requests on the file can return immediately with a failure status
	//											if there is no input immediately available (instead of blocking). Likewise, write requests can also return
	//											immediately with a failure status if the output can't be written immediately.
	//
	//	O_NOCTTY - When set and path identifies a terminal device, open() shall not cause the terminal device to become the controlling terminal for the process.
	uart0_filestream = open("/dev/ttyS0", O_RDWR | O_NOCTTY | O_NDELAY);		//Open in non blocking read/write mode
	if (uart0_filestream == -1)
	{
		//ERROR - CAN'T OPEN SERIAL PORT
		printf("Error - Unable to open UART.  Ensure it is not in use by another application\n");
	}	

	//CONFIGURE THE UART
	//The flags (defined in /usr/include/termios.h - see http://pubs.opengroup.org/onlinepubs/007908799/xsh/termios.h.html):
	//	Baud rate:- B1200, B2400, B4800, B9600, B19200, B38400, B57600, B115200, B230400, B460800,
    //  B500000, B576000, B921600, B1000000, B1152000, B1500000, B2000000, B2500000, B3000000, B3500000, B4000000
	//	CSIZE:- CS5, CS6, CS7, CS8
	//	CLOCAL - Ignore modem status lines
	//	CREAD - Enable receiver
	//	IGNPAR = Ignore characters with parity errors
	//	ICRNL - Map CR to NL on input (Use for ASCII comms where you want to auto correct end of line characters - don't use for bianry comms!)
	//	PARENB - Parity enable
	//	PARODD - Odd parity (else even)
	struct termios options;
	tcgetattr(uart0_filestream, &options);
	options.c_cflag = B9600 | CS8 | CLOCAL | CREAD;		//<Set baud rate
	options.c_iflag = IGNPAR;
	options.c_oflag = 0;
	options.c_lflag = 0;
	tcflush(uart0_filestream, TCIFLUSH);
	tcsetattr(uart0_filestream, TCSANOW, &options);
    //runReadLoop();
}

const char * runReadLoop(char * buf) {
	tcflush(uart0_filestream,TCIFLUSH);
	looping = 1;
	int incomplete = 1;
	int incomingChars  = 0;
    char rect[19] = ""; 
    int collecting = 0;
	while(looping || incomplete){
		//----- CHECK FOR ANY RX BYTES -----
		if (uart0_filestream != -1)
		{
			// Read up to 255 characters from the port if they are there
			char rx_buffer[256];
			int rx_length = read(uart0_filestream, (void*)rx_buffer, 255);		//Filestream, buffer to store in, number of bytes to read (max)
			if (rx_length < 0)
			{
				//An error occured (will occur if there are no bytes)
			}
			else if (rx_length == 0)
			{
				//No data waiting
			}
			else
			{
				rx_buffer[rx_length] = '\0';
                if(strstr(rx_buffer, "*") != NULL) {
				    //printf("end : %s\n", rx_buffer);
                    collecting = 0;
                }
                if(collecting){
				    incomingChars += rx_length;
                    strcat(rect, rx_buffer);
                }
                if(strstr(rx_buffer, "#") != NULL) {
				    //printf("start : %s\n", rx_buffer);
                    collecting = 1;
                }
				//printf("%i bytes read : %s\n", rx_length, rx_buffer);
				looping = 0;
				if (incomingChars >=19) {
				    incomplete = 0;
                    strncpy(buf, rect, 19);
                    return buf;
				}
			}
		}else{
            looping = 0;
			incomplete = 0;
        }
	}
    printf("----------------\n");
}	

void setUp(){
    
    SPEED = (SpeedState*) malloc(sizeof(SpeedState));
    SPEED->max_delay = MOVE_MAX_DELAY;
    SPEED->min_delay = MOVE_MIN_DELAY;
    SPEED->slope = START_SLOPE;
    SPEED->speed_delta = SPEED_DELTA;
    SPEED->increment = 0;
    SPEED->decrement = 0;
    SPEED->half_slope = SPEED->slope / 2.0;
    SPEED->f = SPEED->speed_delta / SPEED->slope;
    SPEED->a = SPEED->f / SPEED->half_slope;

    BOT = (BotState *) malloc(sizeof(BotState));
    BOT->steps_a1 = 0;
    BOT->steps_a2 = 0;
    BOT->steps_a3 = 0;
    BOT->steps_a4 = 0;
    BOT->steps_a5 = 0;
    BOT->delay = SPEED->max_delay;

    int dir_axis1 = HIGH; // CW
    int dir_axis2 = HIGH; // CW
    int dir_axis3 = HIGH; // CW
    int dir_axis4 = HIGH; // CW
    int dir_axis5 = HIGH; // CW

    //motors
    pinMode(AXIS1_MOTOR_PULSE, OUTPUT);
    pinMode(AXIS1_MOTOR_DIR, OUTPUT);
    pinMode(AXIS2_MOTOR_PULSE, OUTPUT);
    pinMode(AXIS2_MOTOR_DIR, OUTPUT);
    pinMode(AXIS3_MOTOR_PULSE, OUTPUT);
    pinMode(AXIS3_MOTOR_DIR, OUTPUT);
    pinMode(AXIS4_MOTOR_PULSE, OUTPUT);
    pinMode(AXIS4_MOTOR_DIR, OUTPUT);  
    pinMode(AXIS5_MOTOR_PULSE, OUTPUT);
    pinMode(AXIS5_MOTOR_DIR, OUTPUT);  

    //sync switches
    pinMode(LIMIT1, INPUT);
    pinMode(LIMIT2, INPUT);
    pinMode(LIMIT3, INPUT);
    pinMode(LIMIT4, INPUT);
    pinMode(LIMIT5, INPUT);

    //pull up
    pullUpDnControl(LIMIT1, PUD_UP);
    pullUpDnControl(LIMIT2, PUD_UP);
    pullUpDnControl(LIMIT3, PUD_UP);
    pullUpDnControl(LIMIT4, PUD_UP);
    pullUpDnControl(LIMIT5, PUD_UP);

    //read state of switches
    int limit_1_state = _digitalRead(LIMIT1);
    int limit_2_state = _digitalRead(LIMIT2);
    int limit_3_state = _digitalRead(LIMIT3);
    int limit_4_state = _digitalRead(LIMIT4);
    int limit_5_state = _digitalRead(LIMIT5);
 
    //get direction to move to
    if (limit_1_state == HIGH) {
        dir_axis1 = CW;
    } else {
        dir_axis1 = CCW;
    }
    if (limit_2_state == HIGH) {
        dir_axis2 = CW;
    } else {
        dir_axis2 = CCW;
    }
    if (limit_3_state == HIGH) {
        dir_axis3 = CW;
    } else {
        dir_axis3 = CCW;
    }
    if (limit_4_state == HIGH) {
        dir_axis4 = CCW;
    } else {
        dir_axis4 = CW;
    }
    if (limit_5_state == HIGH) {
        dir_axis5 = CW;
    } else {
        dir_axis5 = CCW;
    }

    digitalWrite(AXIS1_MOTOR_DIR, dir_axis1);
    digitalWrite(AXIS2_MOTOR_DIR, dir_axis2);
    digitalWrite(AXIS3_MOTOR_DIR, dir_axis3);
    digitalWrite(AXIS4_MOTOR_DIR, dir_axis4);
    digitalWrite(AXIS5_MOTOR_DIR, dir_axis5);
	setupSerial();
}

void sync_bot(){
    int homed = 0;
    int nearly_homed = 0;

    int axis_1_overshoot_dir = LOW;
    int axis_2_overshoot_dir = LOW;
    int axis_3_overshoot_dir = LOW;
    int axis_4_overshoot_dir = HIGH;
    int axis_5_overshoot_dir = LOW;

    int axis_1_homed = 0;
    int axis_2_homed = 0;
    int axis_3_homed = 0;
    int axis_4_homed = 0;
    int axis_5_homed = 0;

    int nearly_homed_count = 0;
    
    //read state of switches
    int limit_1_state = _digitalRead(LIMIT1);
    int limit_2_state = _digitalRead(LIMIT2);
    int limit_3_state = _digitalRead(LIMIT3);
    int limit_4_state = _digitalRead(LIMIT4);
    int limit_5_state = _digitalRead(LIMIT5);

    while (!nearly_homed){
        rt_task_wait_period(NULL);
        if (digitalRead(LIMIT1) == limit_1_state){
            digitalWrite(AXIS1_MOTOR_PULSE, HIGH);
            rt_task_sleep(PULSE_WIDTH);
            digitalWrite(AXIS1_MOTOR_PULSE, LOW);
            axis_1_homed = 0;
        } else{
            axis_1_homed = 1;
        }
        if (digitalRead(LIMIT2) == limit_2_state){
            digitalWrite(AXIS2_MOTOR_PULSE, HIGH);
            rt_task_sleep(PULSE_WIDTH);
            digitalWrite(AXIS2_MOTOR_PULSE, LOW);
            axis_2_homed = 0;
        } else {
            axis_2_homed = 1;
        }
        if (digitalRead(LIMIT3) == limit_3_state){
            digitalWrite(AXIS3_MOTOR_PULSE, HIGH);
            rt_task_sleep(PULSE_WIDTH);
            digitalWrite(AXIS3_MOTOR_PULSE, LOW);
            axis_3_homed = 0;
        } else {
            axis_3_homed = 1;
        }
        if (digitalRead(LIMIT4) == limit_4_state){
            digitalWrite(AXIS4_MOTOR_PULSE, HIGH);
            rt_task_sleep(PULSE_WIDTH);
            digitalWrite(AXIS4_MOTOR_PULSE, LOW);
            axis_4_homed = 0;
        } else {
            axis_4_homed = 1;
        }
        if (digitalRead(LIMIT5) == limit_5_state){
            digitalWrite(AXIS5_MOTOR_PULSE, HIGH);
            rt_task_sleep(PULSE_WIDTH);
            digitalWrite(AXIS5_MOTOR_PULSE, LOW);
            axis_5_homed = 0;
        } else {
            axis_5_homed = 1;
        }
        if (axis_1_homed && axis_2_homed && axis_3_homed && axis_4_homed && axis_5_homed) {
            nearly_homed_count ++;
            if(nearly_homed_count > 100){
                // printf(" %i %i %i %i %i \n", axis_1_homed, axis_2_homed, axis_3_homed, axis_4_homed, axis_5_homed);
                // printf("nearly homed!\n");
                nearly_homed = 1;
            }
        }
        rt_task_set_periodic(&sync_task, TM_NOW, SYNC_DELAY);
    }

    int stepcounter = 0;
    digitalWrite(AXIS1_MOTOR_DIR, axis_1_overshoot_dir);
    digitalWrite(AXIS2_MOTOR_DIR, axis_2_overshoot_dir);
    digitalWrite(AXIS3_MOTOR_DIR, axis_3_overshoot_dir);
    digitalWrite(AXIS4_MOTOR_DIR, axis_4_overshoot_dir);
    digitalWrite(AXIS5_MOTOR_DIR, axis_5_overshoot_dir);

    while (stepcounter < SYNC_OVERSHOOT_STEPS){
        rt_task_wait_period(NULL);

        digitalWrite(AXIS1_MOTOR_PULSE, HIGH);
        digitalWrite(AXIS2_MOTOR_PULSE, HIGH);
        digitalWrite(AXIS3_MOTOR_PULSE, HIGH);
        digitalWrite(AXIS4_MOTOR_PULSE, HIGH);
        digitalWrite(AXIS5_MOTOR_PULSE, HIGH);

        rt_task_sleep(PULSE_WIDTH);

        digitalWrite(AXIS1_MOTOR_PULSE, LOW);
        digitalWrite(AXIS2_MOTOR_PULSE, LOW);
        digitalWrite(AXIS3_MOTOR_PULSE, LOW);
        digitalWrite(AXIS4_MOTOR_PULSE, LOW);
        digitalWrite(AXIS5_MOTOR_PULSE, LOW);

        stepcounter ++;
        rt_task_set_periodic(&sync_task, TM_NOW, SYNC_DELAY);
    }
  
    //read state of switches
    limit_1_state = _digitalRead(LIMIT1);
    limit_2_state = _digitalRead(LIMIT2);
    limit_3_state = _digitalRead(LIMIT3);
    limit_4_state = _digitalRead(LIMIT4);
    limit_5_state = _digitalRead(LIMIT5);
    
    digitalWrite(AXIS1_MOTOR_DIR, axis_1_overshoot_dir?0:1);
    digitalWrite(AXIS2_MOTOR_DIR, axis_2_overshoot_dir?0:1);
    digitalWrite(AXIS3_MOTOR_DIR, axis_3_overshoot_dir?0:1);
    digitalWrite(AXIS4_MOTOR_DIR, axis_4_overshoot_dir?0:1);
    digitalWrite(AXIS5_MOTOR_DIR, axis_5_overshoot_dir?0:1);
   
    int really_homed_count = 0;

    while (!homed){
        rt_task_wait_period(NULL);
        if (digitalRead(LIMIT1) == limit_1_state){
            digitalWrite(AXIS1_MOTOR_PULSE, HIGH);
            rt_task_sleep(PULSE_WIDTH);
            digitalWrite(AXIS1_MOTOR_PULSE, LOW);
            axis_1_homed = 0;
        } else{
            axis_1_homed = 1;
        }
        if (digitalRead(LIMIT2) == limit_2_state){
            digitalWrite(AXIS2_MOTOR_PULSE, HIGH);
            rt_task_sleep(PULSE_WIDTH);
            digitalWrite(AXIS2_MOTOR_PULSE, LOW);
            axis_2_homed = 0;
        } else {
            axis_2_homed = 1;
        }
        if (digitalRead(LIMIT3) == limit_3_state){
            digitalWrite(AXIS3_MOTOR_PULSE, HIGH);
            rt_task_sleep(PULSE_WIDTH);
            digitalWrite(AXIS3_MOTOR_PULSE, LOW);
            axis_3_homed = 0;
        } else {
            axis_3_homed = 1;
        }
        if (digitalRead(LIMIT4) == limit_4_state){
            digitalWrite(AXIS4_MOTOR_PULSE, HIGH);
            rt_task_sleep(PULSE_WIDTH);
            digitalWrite(AXIS4_MOTOR_PULSE, LOW);
            axis_4_homed = 0;
        } else {
            axis_4_homed = 1;
        }
        if (digitalRead(LIMIT5) == limit_5_state){
            digitalWrite(AXIS5_MOTOR_PULSE, HIGH);
            rt_task_sleep(PULSE_WIDTH);
            digitalWrite(AXIS5_MOTOR_PULSE, LOW);
            axis_5_homed = 0;
        } else {
            axis_5_homed = 1;
        }
        if (axis_1_homed && axis_2_homed && axis_3_homed && axis_4_homed && axis_5_homed) {
            really_homed_count ++;
            if(really_homed_count > 100){
                // printf(" %i %i %i %i %i \n", axis_1_homed, axis_2_homed, axis_3_homed, axis_4_homed, axis_5_homed);
                printf("ASEA ROBOT AT YOUR SERVICE!\n");
                homed = 1;
            }
        }
        rt_task_set_periodic(&sync_task, TM_NOW, SYNC_DELAY);
    }
    
    //read state of switches
    _digitalRead(LIMIT1);
    _digitalRead(LIMIT2);
    _digitalRead(LIMIT3);
    _digitalRead(LIMIT4);
    _digitalRead(LIMIT5);

}

int pstate = 0;
int speed(int total_steps, int steps_left){
    int state = 1; // 1 = easing in, 2 = max speed, 3 = easing out
    if (total_steps - steps_left < SPEED->slope){
        state = 1;
    }else if(steps_left < SPEED->slope){
        state = 3;
    }else{
        state = 2;
    }

    int step = total_steps - steps_left;

    if (state == 1){
        if(step < SPEED->half_slope){
            SPEED->increment += SPEED->a;
        }else{
            SPEED->increment -= SPEED->a;
        }
        BOT->delay -= SPEED->increment;
        if(BOT->delay < SPEED->min_delay){
           BOT->delay = SPEED->min_delay;
        }    
    }else if(state == 3){
        if(step < total_steps-SPEED->half_slope){
            SPEED->decrement += SPEED->a;
        }else{
            SPEED->decrement -= SPEED->a;
        }
        BOT->delay += SPEED->decrement;
    }
    //if(step%100 == 0){
        //printf("delay %f state %i step %i \n", BOT->delay, state, step);
    //}
    pstate = state;
}

int max(int a, int b, int c, int d, int e){
    int l = a;
    if(b > l){l = b;}
    if(c > l){l = c;}
    if(d > l){l = d;}
    if(e > l){l = e;}
    return l;
}

void move_bot(int numsteps1, int numsteps2, int numsteps3, int numsteps4, int numsteps5){
    int done = 0;
    int longest = max(numsteps1, numsteps2, numsteps3, numsteps4, numsteps5);
    if(longest < (SPEED->slope*2)){
        SPEED->slope = longest / 2.0; 
        SPEED->min_delay = SPEED->max_delay - SPEED->slope * (SPEED->speed_delta / longest);
        SPEED->speed_delta = SPEED->max_delay - SPEED->min_delay;
        SPEED->half_slope = SPEED->slope / 2.0;
        SPEED->f = SPEED->speed_delta / SPEED->slope;
        SPEED->a = SPEED->f*2.0 / SPEED->half_slope;
    }
    // printf("slope %i \n", slope);
    int c1 = 0, c2 = 0, c3 = 0, c4 = 0, c5 = 0;
    int counter = 0;
    double f1 = (float)longest/numsteps1;
    double f2 = (float)longest/numsteps2;
    double f3 = (float)longest/numsteps3;
    double f4 = (float)longest/numsteps4;
    double f5 = (float)longest/numsteps5;
    while(counter < longest){
        rt_task_wait_period(NULL);
        speed(longest, longest - counter);
        if(fmod(counter, f1) < 1.0  && c1 < numsteps1){
            digitalWrite(AXIS1_MOTOR_PULSE, HIGH);
            c1 ++;
        } 
        if(fmod(counter, f2) < 1.0  && c2 < numsteps2){
            digitalWrite(AXIS2_MOTOR_PULSE, HIGH);
            c2 ++;
        } 
        if(fmod(counter, f3) < 1.0  && c3 < numsteps3){
            digitalWrite(AXIS3_MOTOR_PULSE, HIGH);
            c3 ++;
        } 
        if(fmod(counter, f4) < 1.0  && c4 < numsteps4){
            digitalWrite(AXIS4_MOTOR_PULSE, HIGH);
            c4 ++;
        } 
        if(fmod(counter, f5) < 1.0  && c5 < numsteps5){
            digitalWrite(AXIS5_MOTOR_PULSE, HIGH);
            c5 ++;
        } 
        rt_task_sleep(2000);
        digitalWrite(AXIS1_MOTOR_PULSE, LOW);
        digitalWrite(AXIS2_MOTOR_PULSE, LOW);
        digitalWrite(AXIS3_MOTOR_PULSE, LOW);
        digitalWrite(AXIS4_MOTOR_PULSE, LOW);
        digitalWrite(AXIS5_MOTOR_PULSE, LOW);
        rt_task_set_periodic(&sync_task, TM_NOW, BOT->delay);
        counter ++;
    }
    // printf("loop done steps taken %i %i %i %i %i\n", c1, c2, c3, c4, c5);
}

int move_to(int steps1, int steps2, int steps3, int steps4, int steps5){
    // printf("move to %i %i %i %i %i\n", steps1, steps2, steps3, steps4, steps5);
    int steps_to_move_a1 = abs(steps1 - BOT->steps_a1);
    int steps_to_move_a2 = abs(steps2 - BOT->steps_a2);
    int steps_to_move_a3 = abs(steps3 - BOT->steps_a3);
    int steps_to_move_a4 = abs(steps4 - BOT->steps_a4);
    int steps_to_move_a5 = abs(steps5 - BOT->steps_a5);
    if(steps1 < BOT->steps_a1){
        digitalWrite(AXIS1_MOTOR_DIR, CCW);
    }else{
        digitalWrite(AXIS1_MOTOR_DIR, CW);
    }
    if(steps2 < BOT->steps_a2){
        digitalWrite(AXIS2_MOTOR_DIR, CCW);
    }else{
        digitalWrite(AXIS2_MOTOR_DIR, CW);
    }
    if(steps3 < BOT->steps_a3){
        digitalWrite(AXIS3_MOTOR_DIR, CCW);
    }else{
        digitalWrite(AXIS3_MOTOR_DIR, CW);
    }
    if(steps4 < BOT->steps_a4){
        digitalWrite(AXIS4_MOTOR_DIR, CCW);
    }else{
        digitalWrite(AXIS4_MOTOR_DIR, CW);
    }
    if(steps5 < BOT->steps_a5){
        digitalWrite(AXIS5_MOTOR_DIR, CCW);
    }else{
        digitalWrite(AXIS5_MOTOR_DIR, CW);
    }
    BOT->steps_a1 = steps1;
    BOT->steps_a2 = steps2;
    BOT->steps_a3 = steps3;
    BOT->steps_a4 = steps4;
    BOT->steps_a5 = steps5;
    move_bot(steps_to_move_a1, steps_to_move_a2, steps_to_move_a3, steps_to_move_a4, steps_to_move_a5);
}

void program(char *script){
    loadLua(script);
    alarm(1);
}

int run(char *script){
    rt_task_set_periodic(&sync_task, TM_NOW, BOT->delay);
    rt_task_create(&sync_task, "sync-task", 0, 99, 0);
    rt_task_start(&sync_task, &program, script);
    pause();
    rt_task_delete(&sync_task);
}


int __home(lua_State *L){
    sync_bot();
}

int __read_face_rect(lua_State *L){
    char rect[20];
    memset(rect,0,20);
    runReadLoop(rect);
    printf("rect %s \n", rect);
    lua_pushstring(L, rect);
    return 1;
}

int __move_to(lua_State *L){
    SPEED->increment = 0;
    SPEED->decrement = 0;
    int axis_1 = lua_tonumber(L, 1);
    int axis_2 = lua_tonumber(L, 2);
    int axis_3 = lua_tonumber(L, 3);
    int axis_4 = lua_tonumber(L, 4);
    int axis_5 = lua_tonumber(L, 5);
    move_to(axis_1, axis_2, axis_3, axis_4, axis_5);
    BOT->delay = SPEED->max_delay;
    return 0;
}

int __set_speed(lua_State *L){
    float slope = lua_tonumber(L, 1);
    float min_delay = lua_tonumber(L, 2);
    float max_delay = lua_tonumber(L, 3);
    float speed_delta = max_delay - min_delay;
    SPEED->max_delay = max_delay;
    SPEED->min_delay = min_delay;
    SPEED->slope = slope;
    SPEED->speed_delta = speed_delta;
    SPEED->increment = 0.0;
    SPEED->decrement = 0.0;
    SPEED->half_slope = SPEED->slope / 2.0;
    SPEED->f = SPEED->speed_delta / SPEED->slope;
    SPEED->a = SPEED->f*2.0 / SPEED->half_slope;
    BOT->delay = max_delay;
    printf("speed settings f %f a %f slope %f speed delta %f\n",SPEED->f, SPEED->a, slope, speed_delta);
    return 0;
}

int __set_default_speed(lua_State *L){
    SPEED->max_delay = MOVE_MAX_DELAY;
    SPEED->min_delay = MOVE_MIN_DELAY;
    SPEED->slope = START_SLOPE;
    SPEED->speed_delta = SPEED_DELTA;
    SPEED->increment = 0;
    SPEED->decrement = 0;
    SPEED->half_slope = SPEED->slope / 2.0;
    SPEED->f = SPEED->speed_delta / SPEED->slope;
    SPEED->a = SPEED->f / SPEED->half_slope;
    return 0;
}

void loadLua(char *script){

    lua_State *L;

    L = luaL_newstate();
    luaL_openlibs(L); 
    lua_register(L,"move_to",__move_to);
    lua_register(L,"home",__home);
    lua_register(L,"set_speed",__set_speed);
    lua_register(L,"get_face_rect",__read_face_rect);
    lua_register(L,"set_default_speed",__set_default_speed);

    if (luaL_loadfile(L, script)){
        printf("luaL_loadfile() failed scriptname: %s\n",script);   
    }   

    if (lua_pcall(L, 0, 0, 0)){
        printf("lua_pcall() failed\n");  
    }

    lua_close(L);
}


int main(int argc, char* argv[])
{
    signal(SIGTERM, catch_signal);
    signal(SIGINT, catch_signal);
    /* Avoids memory swapping for this program */
    mlockall(MCL_CURRENT|MCL_FUTURE);
    wiringPiSetup();
    setUp();
    char *script= "default.lua";
    if(argc == 2){
        script = argv[1];
    }
    run(script);
    return 0;
}
