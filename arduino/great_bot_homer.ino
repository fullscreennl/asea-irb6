

#define AXIS1_MOTOR_PULSE 4
#define AXIS1_MOTOR_DIR 5

#define AXIS2_MOTOR_PULSE 6
#define AXIS2_MOTOR_DIR 7

#define AXIS3_MOTOR_PULSE 8
#define AXIS3_MOTOR_DIR 9

#define AXIS4_MOTOR_PULSE 10
#define AXIS4_MOTOR_DIR 11

#define AXIS5_MOTOR_PULSE 12
#define AXIS5_MOTOR_DIR 13

#define LIMIT1 2
#define LIMIT2 3
#define LIMIT3 1
#define LIMIT4 0
#define LIMIT5 A0

boolean DIR_AXIS1 = HIGH; // CW
boolean DIR_AXIS2 = HIGH; // CW
boolean DIR_AXIS3 = HIGH; // CW
boolean DIR_AXIS4 = HIGH; // CW
boolean DIR_AXIS5 = HIGH; // CW

boolean CW = HIGH;
boolean CCW = LOW;

boolean moved_from_home = LOW;

unsigned long currentTime;
unsigned long loopTime;
unsigned long basecount = 300 * 158;

boolean last_limit_1_state = LOW;
boolean last_limit_2_state = LOW;
boolean last_limit_3_state = LOW;
boolean last_limit_4_state = LOW;
boolean last_limit_5_state = LOW;

boolean axis_1_homed = LOW;
boolean axis_2_homed = LOW;
boolean axis_3_homed = LOW;
boolean axis_4_homed = LOW;
boolean axis_5_homed = LOW;
boolean homed = LOW;

void setup() {
//  Serial.begin (9600);
  
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
  pinMode(LIMIT1, INPUT_PULLUP);
  pinMode(LIMIT2, INPUT_PULLUP);
  pinMode(LIMIT3, INPUT_PULLUP);
  pinMode(LIMIT4, INPUT_PULLUP);
  pinMode(LIMIT5, INPUT_PULLUP);
//  digitalWrite(AXIS2_MOTOR_DIR, DIR);
//  digitalWrite(AXIS3_MOTOR_DIR, DIR);
//  digitalWrite(AXIS4_MOTOR_DIR, DIR);
//  digitalWrite(AXIS5_MOTOR_DIR, DIR);
  last_limit_1_state = digitalRead(LIMIT1);
  last_limit_2_state = digitalRead(LIMIT2);
  last_limit_3_state = digitalRead(LIMIT3);
  last_limit_4_state = digitalRead(LIMIT4);
  last_limit_5_state = digitalRead(LIMIT5);
  if (digitalRead(LIMIT1) == HIGH) {
     DIR_AXIS1 = CW;
  } else {
     DIR_AXIS1 = CCW;
  }
  if (digitalRead(LIMIT2) == HIGH) {
     DIR_AXIS2 = CW;
  } else {
     DIR_AXIS2 = CCW;
  }
  if (digitalRead(LIMIT3) == HIGH) {
     DIR_AXIS3 = CW;
  } else {
     DIR_AXIS3 = CCW;
  }
  if (digitalRead(LIMIT4) == HIGH) {
     DIR_AXIS4 = CCW;
  } else {
     DIR_AXIS4 = CW;
  }
  if (digitalRead(LIMIT5) == HIGH) {
     DIR_AXIS5 = CW;
  } else {
     DIR_AXIS5 = CCW;
  }
  delay(1000);  
  digitalWrite(AXIS1_MOTOR_DIR, DIR_AXIS1);
  digitalWrite(AXIS2_MOTOR_DIR, DIR_AXIS2);
  digitalWrite(AXIS3_MOTOR_DIR, DIR_AXIS3);
  digitalWrite(AXIS4_MOTOR_DIR, DIR_AXIS4);
  digitalWrite(AXIS5_MOTOR_DIR, DIR_AXIS5);
}

void loop() { //Do stuff here
   
  unsigned long i = 0;  
  int count = 24000;
  // 2400 line/revolution
  // 158 base gearbox
  // 128 wrist gboxes
  //5/2pi ball screw
  // 3/2 crown
  if (!homed) {
    if (digitalRead(LIMIT1) == last_limit_1_state){
      digitalWrite(AXIS1_MOTOR_PULSE, HIGH);
      delayMicroseconds(2);
      digitalWrite(AXIS1_MOTOR_PULSE, LOW);
    } else{
      axis_1_homed = HIGH;
    }
    if (digitalRead(LIMIT2) == last_limit_2_state){
      digitalWrite(AXIS2_MOTOR_PULSE, HIGH);
      delayMicroseconds(2);
      digitalWrite(AXIS2_MOTOR_PULSE, LOW);
    } else {
      axis_2_homed = HIGH;
    }
    if (digitalRead(LIMIT3) == last_limit_3_state){
      digitalWrite(AXIS3_MOTOR_PULSE, HIGH);
      delayMicroseconds(2);
      digitalWrite(AXIS3_MOTOR_PULSE, LOW);
    } else {
      axis_3_homed = HIGH;
    }
    if (digitalRead(LIMIT4) == last_limit_4_state){
      digitalWrite(AXIS4_MOTOR_PULSE, HIGH);
      delayMicroseconds(2);
      digitalWrite(AXIS4_MOTOR_PULSE, LOW);
    } else {
      axis_4_homed = HIGH;
    }
    if (digitalRead(LIMIT5) == last_limit_5_state){
      digitalWrite(AXIS5_MOTOR_PULSE, HIGH);
      delayMicroseconds(2);
      digitalWrite(AXIS5_MOTOR_PULSE, LOW);
    } else {
      axis_5_homed = HIGH;
    }
    delayMicroseconds(150);
  } else if (!moved_from_home) {
    moved_from_home = HIGH;
    delay(5000);
    for(i=0;i<15000;i++){
      digitalWrite(AXIS1_MOTOR_PULSE, HIGH);
      digitalWrite(AXIS2_MOTOR_PULSE, HIGH);
      digitalWrite(AXIS3_MOTOR_PULSE, HIGH);
      digitalWrite(AXIS4_MOTOR_PULSE, HIGH);    
      digitalWrite(AXIS5_MOTOR_PULSE, HIGH);    
      delayMicroseconds(2);
      digitalWrite(AXIS1_MOTOR_PULSE, LOW);
      digitalWrite(AXIS2_MOTOR_PULSE, LOW);  
      digitalWrite(AXIS3_MOTOR_PULSE, LOW);
      digitalWrite(AXIS4_MOTOR_PULSE, LOW);      
      digitalWrite(AXIS5_MOTOR_PULSE, LOW);      
      delayMicroseconds(550);
    }
  }
  
  if (axis_1_homed && axis_2_homed && axis_3_homed && axis_4_homed && axis_5_homed) {
     homed = HIGH;
  }
  
  
  
//  for(i=0;i<189600;i++){
//    
//      digitalWrite(AXIS1_MOTOR_PULSE, HIGH);
////      digitalWrite(AXIS2_MOTOR_PULSE, HIGH);
////      digitalWrite(AXIS3_MOTOR_PULSE, HIGH);
////      digitalWrite(AXIS4_MOTOR_PULSE, HIGH);    
////      digitalWrite(AXIS5_MOTOR_PULSE, HIGH);    
//      delayMicroseconds(2);
//      digitalWrite(AXIS1_MOTOR_PULSE, LOW);
////      digitalWrite(AXIS2_MOTOR_PULSE, LOW);  
////      digitalWrite(AXIS3_MOTOR_PULSE, LOW);
////      digitalWrite(AXIS4_MOTOR_PULSE, LOW);      
////      digitalWrite(AXIS5_MOTOR_PULSE, LOW);      
//      delayMicroseconds(50);
//  }
//
//  if (DIR == LOW) {
//      DIR = HIGH;
//      INVERTED_DIR = LOW;
//    } else {
//      DIR = LOW;
//      INVERTED_DIR = HIGH;
//    }

}
