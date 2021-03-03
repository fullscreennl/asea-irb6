#include <stdio.h>
#include <math.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdlib.h>

#include <wiringPi.h>
#include <boilerplate/ancillaries.h>
#include <alchemy/task.h>
#include <alchemy/timer.h>

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

// green screw terminal on the PI = BCM column
// defined gpio in software = wPi column

#define MIN(a,b) (((a)<(b))?(a):(b))

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

#define FACE 24                // io19             
#define FACE_TRACK_1 15                   
#define FACE_TRACK_2 16             
#define FACE_TRACK_3 27             

#define SYNC_OVERSHOOT_STEPS 2000

#define SYNC_DELAY 4000000
#define PULSE_WIDTH 2000

#define CW HIGH
#define CCW LOW

#define MOVE_MAX_DELAY 300000
#define MOVE_MIN_DELAY 50000
#define SPEED_DELTA (MOVE_MAX_DELAY - MOVE_MIN_DELAY) 
#define START_SLOPE 45000
#define INCREMENT (SPEED_DELTA / START_SLOPE)

#define NO_FACE 0
#define X_X 1 // centered in both dirs
#define X_U 2 // horizontally centered move up
#define X_D 3 // horizontally centered move down
#define L_X 4 // vertically centered move left 
#define L_U 5 // move left move up
#define L_D 6 // move left move down
#define R_X 7 // vertically centered move right
#define R_U 8 // move right move up
#define R_D 9 // move right move down



int uart0_filestream = -1;
const char * runReadLoop();
int looping = 1;

RT_TASK sync_task;

typedef struct AseaBotState{
    int target_a1;
    int target_a2;
    int target_a3;
    int target_a4;
    int target_a5;
    int steps_a1;
    int steps_a2;
    int steps_a3;
    int steps_a4;
    int steps_a5;
    int dir_a1;
    int dir_a2;
    int dir_a3;
    int dir_a4;
    int dir_a5;
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

    //FACE DETECTION
    pinMode(FACE, INPUT);
    pullUpDnControl(FACE, PUD_UP);
    pinMode(FACE_TRACK_1, INPUT);
    pullUpDnControl(FACE_TRACK_1, PUD_DOWN);
    pinMode(FACE_TRACK_2, INPUT);
    pullUpDnControl(FACE_TRACK_2, PUD_DOWN);

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
                alarm(1);
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

void home(){
    rt_task_set_periodic(&sync_task, TM_NOW, BOT->delay);
    rt_task_create(&sync_task, "sync-task", 0, 99, 0);
    rt_task_start(&sync_task, &sync_bot, NULL);
    pause();
    rt_task_delete(&sync_task);
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
    if(step%100 == 0){
        //printf("delay %f state %i step %i \n", BOT->delay, state, step);
    }
    pstate = state;
}

int stop(int total_steps, int steps_left){
    BOT->delay += SPEED->f;
    if(BOT->delay > SPEED->max_delay){
        BOT->delay = SPEED->max_delay;
    }
    //printf("stopping d %f  \n", BOT->delay);
}

int max(int a, int b, int c, int d, int e){
    int l = a;
    if(b > l){l = b;}
    if(c > l){l = c;}
    if(d > l){l = d;}
    if(e > l){l = e;}
    return l;
}

int decode_state(){
    int io_1 = _digitalRead(FACE);
    int io_2 = _digitalRead(FACE_TRACK_1);
    int io_3 = _digitalRead(FACE_TRACK_2);
    int io_4 = _digitalRead(FACE_TRACK_3);
    int result = io_1 | (io_2 << 1) | (io_3 << 2) | (io_4 << 3);
    return result;
}


int can_move_lower_arm(int wanted_steps_lower_arm, int steps_upper_arm){
    if(wanted_steps_lower_arm < -5500){
        return 0;
    }
    if(wanted_steps_lower_arm > 10500){
        return 0;
    }

    if(steps_upper_arm > 3500){
        if(steps_upper_arm - wanted_steps_lower_arm < 9000){
            return 1;
        }else{
            return 0;
        }
    }
    
    if(steps_upper_arm < 1500){
        if(steps_upper_arm - wanted_steps_lower_arm > -9000){
            return 1;
        }else{
            return 0;
        }
    }

    return 1;
}

int can_move_upper_arm(int wanted_steps_upper_arm, int steps_lower_arm){
    if(wanted_steps_upper_arm < -8000){
        return 0;
    }
    if(wanted_steps_upper_arm > 8000){
        return 0;
    }

    if(steps_lower_arm > 1000){
        if(steps_lower_arm - wanted_steps_upper_arm < 9000){
            return 1;
        }else{
            return 0;
        }
    }
    
    if(steps_lower_arm < -1000){
        if(steps_lower_arm - wanted_steps_upper_arm > -9000){
            return 1;
        }else{
            return 0;
        }
    }

    return 1;
}

void move_bot(int numsteps1, int numsteps2, int numsteps3, int numsteps4, int numsteps5){
    int exit_easing = 0;
    int _face_state = decode_state();
    int face_loop_counter = 0;
    int current_face_track_state = LOW;
    int track_speed = 700000;
    while (_face_state > NO_FACE){
        rt_task_wait_period(NULL);
        _face_state = _digitalRead(FACE);
            
        digitalWrite(AXIS1_MOTOR_PULSE, HIGH);
        digitalWrite(AXIS3_MOTOR_PULSE, HIGH);
        digitalWrite(AXIS2_MOTOR_PULSE, HIGH);

        //int _face_track_state_1 = _digitalRead(FACE_TRACK_1);
        //int _face_track_state_2 = _digitalRead(FACE_TRACK_2);
        _face_state = decode_state();

        if(current_face_track_state != _face_state){
            track_speed = 700000;
        }
        current_face_track_state = _face_state;
        if(track_speed > 300000){
            track_speed -= 50;
        }
    
        if(face_loop_counter%100 == 0){
            // printf("Track state 1 %i\n", _face_track_state_1);
            // printf("Track state 2 %i\n", _face_track_state_2);
        }
        
        int target_axis_5 = (float)BOT->steps_a1 * -0.447368 - 7500.0;
        //printf(">> :  %i : %i\n", BOT->steps_a5, target_axis_5);

        while (target_axis_5 != BOT->steps_a5){
            //printf("steps axis 5 :  %i : %i\n", BOT->steps_a5, target_axis_5);
            digitalWrite(AXIS5_MOTOR_PULSE, HIGH);
            if(target_axis_5 > BOT->steps_a5){
                BOT->steps_a5 ++;
                digitalWrite(AXIS5_MOTOR_DIR, CW);
                BOT->dir_a5 = CW;
            }else if(target_axis_5 < BOT->steps_a5){
                BOT->steps_a5 --;
                digitalWrite(AXIS5_MOTOR_DIR, CCW);
                BOT->dir_a5 = CCW;
            }        
            rt_task_sleep(2000);
            digitalWrite(AXIS5_MOTOR_PULSE, LOW);
        }

        if(_face_state == X_X){
            rt_task_set_periodic(&sync_task, TM_NOW, track_speed); 
            track_speed = 700000;
            continue;
        }

        // move left or right
        if(_face_state == R_X || _face_state == R_U || _face_state == R_D){
            if(BOT->steps_a1 > 19000){
                rt_task_set_periodic(&sync_task, TM_NOW, track_speed);
            }else{
                BOT->steps_a1 ++;
                digitalWrite(AXIS1_MOTOR_DIR, CW);
                BOT->dir_a1 = CW;
                digitalWrite(AXIS1_MOTOR_PULSE, LOW);
            }
        }else if (_face_state == L_X || _face_state == L_U || _face_state == L_D){
            if(BOT->steps_a1 < -19000){
                rt_task_set_periodic(&sync_task, TM_NOW, track_speed);
            }else{
                BOT->steps_a1 --;
                digitalWrite(AXIS1_MOTOR_DIR, CCW);
                BOT->dir_a1 = CCW;
                digitalWrite(AXIS1_MOTOR_PULSE, LOW);
            }
        }
       
        // up and down are reversed !!
        // bitch!
        if(_face_state == X_U || _face_state == L_U || _face_state == R_U){
            printf("going up!\n");
            if (can_move_lower_arm(BOT->steps_a3 - 1, BOT->steps_a2)){
                BOT->steps_a3 --;
                digitalWrite(AXIS3_MOTOR_DIR, CCW);
                BOT->dir_a3 = CCW;
                digitalWrite(AXIS3_MOTOR_PULSE, LOW);
            }
            
            if (can_move_upper_arm(BOT->steps_a2 - 1, BOT->steps_a3)){
                BOT->steps_a2 --;
                digitalWrite(AXIS2_MOTOR_DIR, CCW);
                BOT->dir_a2 = CCW;
                digitalWrite(AXIS2_MOTOR_PULSE, LOW);
            }
        }else if (_face_state == X_D || _face_state == L_D || _face_state == R_D){
            if (can_move_lower_arm(BOT->steps_a3 + 1, BOT->steps_a2)){
                BOT->steps_a3 ++;
                digitalWrite(AXIS3_MOTOR_DIR, CW);
                BOT->dir_a3 = CW;
                digitalWrite(AXIS3_MOTOR_PULSE, LOW);
            }
           
            if (can_move_upper_arm(BOT->steps_a2 + 1, BOT->steps_a3)){
                BOT->steps_a2 ++;
                digitalWrite(AXIS2_MOTOR_DIR, CW);
                BOT->dir_a2 = CW;
                digitalWrite(AXIS2_MOTOR_PULSE, LOW);
            }
        }

        face_loop_counter ++;
        rt_task_sleep(2000);
        rt_task_set_periodic(&sync_task, TM_NOW, track_speed);
    }
    numsteps1 = abs(BOT->target_a1 - BOT->steps_a1);
    numsteps2 = abs(BOT->target_a2 - BOT->steps_a2);
    numsteps3 = abs(BOT->target_a3 - BOT->steps_a3);
    numsteps4 = abs(BOT->target_a4 - BOT->steps_a4);
    numsteps5 = abs(BOT->target_a5 - BOT->steps_a5);

    int done = 0;
    int longest = max(numsteps1, numsteps2, numsteps3, numsteps4, numsteps5);
    //printf("longest %i \n",longest);
	if(longest < (SPEED->slope*2)){
        SPEED->slope = longest / 2.0; 
        SPEED->min_delay = SPEED->max_delay - SPEED->slope * (SPEED->speed_delta / longest);
        SPEED->speed_delta = SPEED->max_delay - SPEED->min_delay;
        SPEED->half_slope = SPEED->slope / 2.0;
        SPEED->f = SPEED->speed_delta / SPEED->slope;
        SPEED->a = SPEED->f*2.0 / SPEED->half_slope;
    }

    int c1 = 0, c2 = 0, c3 = 0, c4 = 0, c5 = 0;
    int counter = 0;
    double f1 = (float)longest/numsteps1;
    double f2 = (float)longest/numsteps2;
    double f3 = (float)longest/numsteps3;
    double f4 = (float)longest/numsteps4;
    double f5 = (float)longest/numsteps5;
    
    if(BOT->target_a1 < BOT->steps_a1){
        digitalWrite(AXIS1_MOTOR_DIR, CCW);
        BOT->dir_a1 = CCW;
    }else{
        digitalWrite(AXIS1_MOTOR_DIR, CW);
        BOT->dir_a1 = CW;
    }
    if(BOT->target_a5 < BOT->steps_a5){
        digitalWrite(AXIS5_MOTOR_DIR, CCW);
        BOT->dir_a5 = CCW;
    }else{
        digitalWrite(AXIS5_MOTOR_DIR, CW);
        BOT->dir_a5 = CW;
    }

    while(counter < longest){
       
        int face_state = _digitalRead(FACE);
        if (face_state == HIGH && exit_easing == 0){
            //printf("has face delay %f\n", BOT->delay);
            //printf("has face %i\n", face_state);
            int new_slope = (int)(SPEED->max_delay - BOT->delay)/SPEED->f;
            longest = counter + new_slope;
            SPEED->slope = new_slope;
            SPEED->half_slope = (int)(new_slope/2.0);
            SPEED->speed_delta = SPEED->max_delay - BOT->delay;
            SPEED->f = SPEED->speed_delta / SPEED->slope;
            SPEED->a = SPEED->f*2.0 / SPEED->half_slope;
            //printf("new slope %i\n", new_slope);
            exit_easing = 1;
        }

        rt_task_wait_period(NULL);
        if(face_state == HIGH){
            //stop(longest, longest - counter);
            speed(longest, longest - counter);
            //printf("stopping delay %f\n", BOT->delay);
        }else{
            speed(longest, longest - counter);
        }
        if(fmod(counter, f1) < 1.0  && c1 < numsteps1){
            digitalWrite(AXIS1_MOTOR_PULSE, HIGH);
            c1 ++;
            if(BOT->dir_a1 == CW){
                BOT->steps_a1 ++;
            }else{
                BOT->steps_a1 --;
            }
        } 
        if(fmod(counter, f2) < 1.0  && c2 < numsteps2){
            int step_ahead2 = -1;
            if(BOT->dir_a2 == CW){
                step_ahead2 = 1;
            }
            if (can_move_upper_arm(BOT->steps_a2 + step_ahead2, BOT->steps_a3)){
                digitalWrite(AXIS2_MOTOR_PULSE, HIGH);
                c2 ++;
                if(BOT->dir_a2 == CW){
                    BOT->steps_a2 ++;
                }else{
                    BOT->steps_a2 --;
                }
            }
        } 
        if(fmod(counter, f3) < 1.0  && c3 < numsteps3){
            int step_ahead3 = -1;
            if(BOT->dir_a3 == CW){
                step_ahead3 = 1;
            }
            if (can_move_lower_arm(BOT->steps_a3 + step_ahead3, BOT->steps_a2)){
                digitalWrite(AXIS3_MOTOR_PULSE, HIGH);
                c3 ++;
                if(BOT->dir_a3 == CW){
                    BOT->steps_a3 ++;
                }else{
                    BOT->steps_a3 --;
                }
            }
        } 
        if(fmod(counter, f4) < 1.0  && c4 < numsteps4){
            digitalWrite(AXIS4_MOTOR_PULSE, HIGH);
            c4 ++;
            if(BOT->dir_a4 == CW){
                BOT->steps_a4 ++;
            }else{
                BOT->steps_a4 --;
            }
        } 
        if(fmod(counter, f5) < 1.0  && c5 < numsteps5){
            digitalWrite(AXIS5_MOTOR_PULSE, HIGH);
            c5 ++;
            if(BOT->dir_a5 == CW){
                BOT->steps_a5 ++;
            }else{
                BOT->steps_a5 --;
            }
        } 
        rt_task_sleep(2000);
        
        digitalWrite(AXIS1_MOTOR_PULSE, LOW);
        digitalWrite(AXIS2_MOTOR_PULSE, LOW);
        digitalWrite(AXIS3_MOTOR_PULSE, LOW);
        digitalWrite(AXIS4_MOTOR_PULSE, LOW);
        digitalWrite(AXIS5_MOTOR_PULSE, LOW);

        //printf("STATE %i \n", decode_state());

        rt_task_set_periodic(&sync_task, TM_NOW, BOT->delay);
        counter ++;
    }
    // printf("loop done steps taken %i %i %i %i %i\n", c1, c2, c3, c4, c5);
}

int move_to(int steps1, int steps2, int steps3, int steps4, int steps5){
    // printf("move to %i %i %i %i %i\n", steps1, steps2, steps3, steps4, steps5);

    BOT->target_a1 = steps1;
    BOT->target_a2 = steps2;
    BOT->target_a3 = steps3;
    BOT->target_a4 = steps4;
    BOT->target_a5 = steps5;

    int steps_to_move_a1 = abs(steps1 - BOT->steps_a1);
    int steps_to_move_a2 = abs(steps2 - BOT->steps_a2);
    int steps_to_move_a3 = abs(steps3 - BOT->steps_a3);
    int steps_to_move_a4 = abs(steps4 - BOT->steps_a4);
    int steps_to_move_a5 = abs(steps5 - BOT->steps_a5);
    if(steps1 < BOT->steps_a1){
        digitalWrite(AXIS1_MOTOR_DIR, CCW);
        BOT->dir_a1 = CCW;
    }else{
        digitalWrite(AXIS1_MOTOR_DIR, CW);
        BOT->dir_a1 = CW;
    }
    if(steps2 < BOT->steps_a2){
        digitalWrite(AXIS2_MOTOR_DIR, CCW);
        BOT->dir_a2 = CCW;
    }else{
        digitalWrite(AXIS2_MOTOR_DIR, CW);
        BOT->dir_a2 = CW;
    }
    if(steps3 < BOT->steps_a3){
        digitalWrite(AXIS3_MOTOR_DIR, CCW);
        BOT->dir_a3 = CCW;
    }else{
        digitalWrite(AXIS3_MOTOR_DIR, CW);
        BOT->dir_a3 = CW;
    }
    if(steps4 < BOT->steps_a4){
        digitalWrite(AXIS4_MOTOR_DIR, CCW);
        BOT->dir_a4 = CCW;
    }else{
        digitalWrite(AXIS4_MOTOR_DIR, CW);
        BOT->dir_a4 = CW;
    }
    if(steps5 < BOT->steps_a5){
        digitalWrite(AXIS5_MOTOR_DIR, CCW);
        BOT->dir_a5 = CCW;
    }else{
        digitalWrite(AXIS5_MOTOR_DIR, CW);
        BOT->dir_a5 = CW;
    }
    move_bot(steps_to_move_a1, steps_to_move_a2, steps_to_move_a3, steps_to_move_a4, steps_to_move_a5);
}

int set_speed(float slope, float min_delay, float max_delay){
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

void program(){
    //loadLua(script);
    int zoom = -7000;
    int pivot = 1800;
    set_speed(2000, 13000*10, 200000*20);// slope min max
    move_to(0,zoom,0,-pivot,-7500);
    int slope = 3000;
    while(1){
        set_speed(slope, 13000*10, 200000*20); // slope min max
        move_to(-19000,zoom,0,-pivot,1000);

        set_speed(slope, 13000*10, 200000*20); // slope min max
        move_to(-19000,5000,0,-pivot,1000);
        
        set_speed(slope, 13000*10, 200000*20); // slope min max
        move_to(-19000,5000,9000,-pivot,1000);

        set_speed(slope, 13000*10, 200000*20); // slope min max
        move_to(-19000,5000,-4000,-pivot,1000);

        set_speed(slope, 13000*10, 200000*20); // slope min max
        move_to(19000,zoom,0,-pivot,-16000);

        set_speed(slope, 13000*10, 200000*20); // slope min max
        move_to(19000,0,0,-pivot,-16000);
    }

    alarm(1);
}

int run(){
    rt_task_set_periodic(&sync_task, TM_NOW, BOT->delay);
    rt_task_create(&sync_task, "sync-task", 0, 99, 0);
    rt_task_start(&sync_task, &program, NULL);
    pause();
    rt_task_delete(&sync_task);
}

int set_default_speed(){
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


int main(int argc, char* argv[])
{
    signal(SIGTERM, catch_signal);
    signal(SIGINT, catch_signal);
    /* Avoids memory swapping for this program */
    mlockall(MCL_CURRENT|MCL_FUTURE);
    wiringPiSetup();
    setUp();
    char *mode = "home";
    if(argc == 2){
        mode = argv[1];
    }
    char home_str[] = "home";
    if(strcmp(mode, home_str) == 0){
        home();
    }else{
        run();
    }
    return 0;
}
