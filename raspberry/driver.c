#include <stdio.h>
#include <signal.h>
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

#define SYNC_OVERSHOOT_STEPS 10000

int dir_axis1 = HIGH; // CW
int dir_axis2 = HIGH; // CW
int dir_axis3 = HIGH; // CW
int dir_axis4 = HIGH; // CW
int dir_axis5 = HIGH; // CW

int limit_1_state = 0;
int limit_2_state = 0;
int limit_3_state = 0;
int limit_4_state = 0;
int limit_5_state = 0;

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

int speed_delay = 200000;
int pulse_width = 2000;

int homed = 0;
int nearly_homed = 0;

int CW = HIGH;
int CCW = LOW;

RT_TASK sync_task;


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
            printf("h: %i l: %i\n", hs, ls);
            return HIGH;
        }
        if(ls > 100){
            printf("h: %i l: %i\n", hs, ls);
            return LOW;
        }
    }
}

void setUp(){
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
  limit_1_state = _digitalRead(LIMIT1);
  limit_2_state = _digitalRead(LIMIT2);
  limit_3_state = _digitalRead(LIMIT3);
  limit_4_state = _digitalRead(LIMIT4);
  limit_5_state = _digitalRead(LIMIT5);

 
  //get direction to move to
  if (limit_1_state == HIGH) {
     dir_axis1 = CW;
     //axis_1_overshoot_dir = HIGH;
  } else {
     dir_axis1 = CCW;
  }
  if (limit_2_state == HIGH) {
     dir_axis2 = CW;
     //axis_2_overshoot_dir = LOW;
  } else {
     dir_axis2 = CCW;
  }
  if (limit_3_state == HIGH) {
     dir_axis3 = CW;
     //axis_3_overshoot_dir = LOW;
  } else {
     dir_axis3 = CCW;
  }
  if (limit_4_state == HIGH) {
     dir_axis4 = CCW;
     //axis_4_overshoot_dir = HIGH;
  } else {
     dir_axis4 = CW;
  }
  if (limit_5_state == HIGH) {
     dir_axis5 = CW;
     //axis_5_overshoot_dir = HIGH;
  } else {
     dir_axis5 = CCW;
  }

  digitalWrite(AXIS1_MOTOR_DIR, dir_axis1);
  digitalWrite(AXIS2_MOTOR_DIR, dir_axis2);
  digitalWrite(AXIS3_MOTOR_DIR, dir_axis3);
  digitalWrite(AXIS4_MOTOR_DIR, dir_axis4);
  digitalWrite(AXIS5_MOTOR_DIR, dir_axis5);

}

void sync_bot(void *arg){
    int nearly_homed_count = 0;
    while (!nearly_homed){
        rt_task_wait_period(NULL);
        if (digitalRead(LIMIT1) == limit_1_state){
            digitalWrite(AXIS1_MOTOR_PULSE, HIGH);
            rt_task_sleep(pulse_width);
            digitalWrite(AXIS1_MOTOR_PULSE, LOW);
            axis_1_homed = 0;
        } else{
            axis_1_homed = 1;
        }
        if (digitalRead(LIMIT2) == limit_2_state){
            digitalWrite(AXIS2_MOTOR_PULSE, HIGH);
            rt_task_sleep(pulse_width);
            digitalWrite(AXIS2_MOTOR_PULSE, LOW);
            axis_2_homed = 0;
        } else {
            axis_2_homed = 1;
        }
        if (digitalRead(LIMIT3) == limit_3_state){
            digitalWrite(AXIS3_MOTOR_PULSE, HIGH);
            rt_task_sleep(pulse_width);
            digitalWrite(AXIS3_MOTOR_PULSE, LOW);
            axis_3_homed = 0;
        } else {
            axis_3_homed = 1;
        }
        if (digitalRead(LIMIT4) == limit_4_state){
            digitalWrite(AXIS4_MOTOR_PULSE, HIGH);
            rt_task_sleep(pulse_width);
            digitalWrite(AXIS4_MOTOR_PULSE, LOW);
            axis_4_homed = 0;
        } else {
            axis_4_homed = 1;
        }
        if (digitalRead(LIMIT5) == limit_5_state){
            digitalWrite(AXIS5_MOTOR_PULSE, HIGH);
            rt_task_sleep(pulse_width);
            digitalWrite(AXIS5_MOTOR_PULSE, LOW);
            axis_5_homed = 0;
        } else {
            axis_5_homed = 1;
        }
        //printf(" %i %i %i %i %i \n", axis_1_homed, axis_2_homed, axis_3_homed, axis_4_homed, axis_5_homed);
        if (axis_1_homed && axis_2_homed && axis_3_homed && axis_4_homed && axis_5_homed) {
        //if (axis_4_homed && axis_5_homed) {
            nearly_homed_count ++;
            if(nearly_homed_count > 100){
                printf(" %i %i %i %i %i \n", axis_1_homed, axis_2_homed, axis_3_homed, axis_4_homed, axis_5_homed);
                printf("nearly homed!\n");
                nearly_homed = 1;
            }
        }
        rt_task_set_periodic(&sync_task, TM_NOW, speed_delay);
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

        rt_task_sleep(pulse_width);

        digitalWrite(AXIS1_MOTOR_PULSE, LOW);
        digitalWrite(AXIS2_MOTOR_PULSE, LOW);
        digitalWrite(AXIS3_MOTOR_PULSE, LOW);
        digitalWrite(AXIS4_MOTOR_PULSE, LOW);
        digitalWrite(AXIS5_MOTOR_PULSE, LOW);

        stepcounter ++;
        rt_task_set_periodic(&sync_task, TM_NOW, speed_delay);
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
            rt_task_sleep(pulse_width);
            digitalWrite(AXIS1_MOTOR_PULSE, LOW);
            axis_1_homed = 0;
        } else{
            axis_1_homed = 1;
        }
        if (digitalRead(LIMIT2) == limit_2_state){
            digitalWrite(AXIS2_MOTOR_PULSE, HIGH);
            rt_task_sleep(pulse_width);
            digitalWrite(AXIS2_MOTOR_PULSE, LOW);
            axis_2_homed = 0;
        } else {
            axis_2_homed = 1;
        }
        if (digitalRead(LIMIT3) == limit_3_state){
            digitalWrite(AXIS3_MOTOR_PULSE, HIGH);
            rt_task_sleep(pulse_width);
            digitalWrite(AXIS3_MOTOR_PULSE, LOW);
            axis_3_homed = 0;
        } else {
            axis_3_homed = 1;
        }
        if (digitalRead(LIMIT4) == limit_4_state){
            digitalWrite(AXIS4_MOTOR_PULSE, HIGH);
            rt_task_sleep(pulse_width);
            digitalWrite(AXIS4_MOTOR_PULSE, LOW);
            axis_4_homed = 0;
        } else {
            axis_4_homed = 1;
        }
        if (digitalRead(LIMIT5) == limit_5_state){
            digitalWrite(AXIS5_MOTOR_PULSE, HIGH);
            rt_task_sleep(pulse_width);
            digitalWrite(AXIS5_MOTOR_PULSE, LOW);
            axis_5_homed = 0;
        } else {
            axis_5_homed = 1;
        }
        //printf(" %i %i %i %i %i \n", axis_1_homed, axis_2_homed, axis_3_homed, axis_4_homed, axis_5_homed);
        if (axis_1_homed && axis_2_homed && axis_3_homed && axis_4_homed && axis_5_homed) {
        //if (axis_4_homed && axis_5_homed) {
            really_homed_count ++;
            if(really_homed_count > 100){
                printf(" %i %i %i %i %i \n", axis_1_homed, axis_2_homed, axis_3_homed, axis_4_homed, axis_5_homed);
                printf("really homed!\n");
                homed = 1;
            }
        }
        rt_task_set_periodic(&sync_task, TM_NOW, speed_delay);
    }
    
    //read state of switches
    _digitalRead(LIMIT1);
    _digitalRead(LIMIT2);
    _digitalRead(LIMIT3);
    _digitalRead(LIMIT4);
    _digitalRead(LIMIT5);

    //alarm(1);

}

int home(){
    rt_task_set_periodic(&sync_task, TM_NOW, 1000000);
    rt_task_create(&sync_task, "sync-task", 0, 99, 0);
    rt_task_start(&sync_task, &sync_bot, NULL);
    pause();
    rt_task_delete(&sync_task);
    return 0;
}


const int move_max_delay = 400000;
int move_min_delay = 25000;
int slope = 18000;
int start_slope = 18000;
int move_delay = 400000;
int speed_delta = 0; 
int increment = 0;

int steps_to_move_a1 = 0;
int steps_to_move_a2 = 0;
int steps_to_move_a3 = 0;
int steps_to_move_a4 = 0;
int steps_to_move_a5 = 0;

int steps_a1 = 0;
int steps_a2 = 0;
int steps_a3 = 0;
int steps_a4 = 0;
int steps_a5 = 0;

int speed(int total_steps, int steps_left){
    int state = 1; // 1 = easing in, 2 = max speed, 3 = easing out
    if (total_steps - steps_left < slope){
        state = 1; 
    }else if(steps_left < slope){
        state = 3;
    }else{
        state = 2;
    }
    //printf("state : %i delay : %i steps left: %i \n", state, move_delay, steps_left);
    if (state == 1){
        move_delay -= increment;
        if(move_delay < move_min_delay){
           move_delay = move_min_delay;
        }    
    }else if(state == 3){
        move_delay += increment;
        if(move_delay >  move_max_delay){
           move_delay = move_max_delay;
        }    
    }
}

void move_bot(){
    speed_delta = move_max_delay - move_min_delay; 
    increment = speed_delta / start_slope;
    int numsteps1 = steps_to_move_a1;
    int numsteps2 = steps_to_move_a2;
    int numsteps3 = steps_to_move_a3;
    int numsteps4 = steps_to_move_a4;
    int numsteps5 = steps_to_move_a5;
    int done = 0;
    //printf("axis to move %i\n", axis_to_move);
    while(!done){
        rt_task_wait_period(NULL);
        if (numsteps1 > 0){
            digitalWrite(AXIS1_MOTOR_PULSE, HIGH);
            numsteps1 --;
            speed(steps_to_move_a1, numsteps1);
        }
        if (numsteps2 > 0){
            digitalWrite(AXIS2_MOTOR_PULSE, HIGH);
            numsteps2 --;
        }
        if (numsteps3 > 0){
            digitalWrite(AXIS3_MOTOR_PULSE, HIGH);
            numsteps3 --;
        }
        if (numsteps4 > 0){
            digitalWrite(AXIS4_MOTOR_PULSE, HIGH);
            numsteps4 --;
        }
        if (numsteps5 > 0){
            digitalWrite(AXIS5_MOTOR_PULSE, HIGH);
            numsteps5 --;
        }
        rt_task_sleep(2000);
        digitalWrite(AXIS1_MOTOR_PULSE, LOW);
        digitalWrite(AXIS2_MOTOR_PULSE, LOW);
        digitalWrite(AXIS3_MOTOR_PULSE, LOW);
        digitalWrite(AXIS4_MOTOR_PULSE, LOW);
        digitalWrite(AXIS5_MOTOR_PULSE, LOW);
        rt_task_set_periodic(&sync_task, TM_NOW, move_delay);
        done = numsteps1 == 0 && numsteps2 == 0 && numsteps3 == 0 && numsteps4 == 0 && numsteps5 == 0; 
    }
    printf("loop done steps left %i\n",numsteps1);
}


int moveTo(int steps1, int steps2, int steps3, int steps4, int steps5){
    printf("move to %i %i %i %i %i\n", steps1, steps2, steps3, steps4, steps5);
    steps_to_move_a1 = abs(steps1 - steps_a1);
    steps_to_move_a2 = abs(steps2 - steps_a2);
    steps_to_move_a3 = abs(steps3 - steps_a3);
    steps_to_move_a4 = abs(steps4 - steps_a4);
    steps_to_move_a5 = abs(steps5 - steps_a5);
    if(steps_to_move_a1 < (start_slope*2)){
        slope = steps_to_move_a1 / 2; 
    }else{
        slope = start_slope;
    }
    printf("slope %i \n", slope);
    //rt_task_set_periodic(&sync_task, TM_NOW, move_delay);
    //rt_task_create(&sync_task, "sync-task", 0, 99, 0);
    //rt_task_start(&sync_task, &move_bot, NULL);
    if(steps1 < steps_a1){
        digitalWrite(AXIS1_MOTOR_DIR, CCW);
    }else{
        digitalWrite(AXIS1_MOTOR_DIR, CW);
    }
    if(steps2 < steps_a2){
        digitalWrite(AXIS2_MOTOR_DIR, CCW);
    }else{
        digitalWrite(AXIS2_MOTOR_DIR, CW);
    }
    if(steps3 < steps_a3){
        digitalWrite(AXIS3_MOTOR_DIR, CCW);
    }else{
        digitalWrite(AXIS3_MOTOR_DIR, CW);
    }
    if(steps4 < steps_a4){
        digitalWrite(AXIS4_MOTOR_DIR, CCW);
    }else{
        digitalWrite(AXIS4_MOTOR_DIR, CW);
    }
    if(steps5 < steps_a5){
        digitalWrite(AXIS5_MOTOR_DIR, CCW);
    }else{
        digitalWrite(AXIS5_MOTOR_DIR, CW);
    }
    steps_a1 = steps1;
    steps_a2 = steps2;
    steps_a3 = steps3;
    steps_a4 = steps4;
    steps_a5 = steps5;
    move_bot();
    //pause();
    //rt_task_delete(&sync_task);
}

void program(){
    //moveTo(-2000,0, 0, 0, 0);
    moveTo(-70000, 0, 0, 0, 0);
    moveTo(70000, 0, 0, 0, 0);
    //moveTo(18000, 0, 0, 0, 0);
    //moveTo(40000, 0, 0, 0, 0);
    //moveTo(0, 0, -5000, 10000, 5000);
    //moveTo(0, 2000, 5000, 10000, 5000);
    //moveTo(0, 2000, 5000, 10000, 5000);
    //moveTo(0, -2000, -5000, 10000, 5000);
    //moveTo(40000, -2000, -5000, 10000, 5000);
    //moveTo(40000, -22000, 5000, 0, 0);
    //moveTo(40000, 22000, 5000, 0, 0);
    //moveTo(40000, -22000, 5000, 0, 0);
    moveTo(0, 0, 0, 0, 0);
    alarm(1);
}

int runProgram(){
    rt_task_set_periodic(&sync_task, TM_NOW, move_delay);
    rt_task_create(&sync_task, "sync-task", 0, 99, 0);
    rt_task_start(&sync_task, &program, NULL);
    pause();
    rt_task_delete(&sync_task);
}

int main(int argc, char* argv[])
{
    signal(SIGTERM, catch_signal);
    signal(SIGINT, catch_signal);
    /* Avoids memory swapping for this program */
    mlockall(MCL_CURRENT|MCL_FUTURE);
    wiringPiSetup();
    setUp();
    //home();
    runProgram();
    return 0;
}
