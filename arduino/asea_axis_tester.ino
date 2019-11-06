

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



boolean DIR = HIGH;
boolean INVERTED_DIR = LOW;
int LEFTENGINE= HIGH;
int lastBstate = LOW;
unsigned long currentTime;
unsigned long loopTime;
unsigned long basecount = 300 * 158;

void setup() {
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


  digitalWrite(AXIS1_MOTOR_DIR, DIR);
  digitalWrite(AXIS2_MOTOR_DIR, DIR);
  digitalWrite(AXIS3_MOTOR_DIR, DIR);
  digitalWrite(AXIS4_MOTOR_DIR, DIR);
  digitalWrite(AXIS5_MOTOR_DIR, DIR);
  
  //Serial.begin (9600);
}

void loop(){ //Do stuff here
  digitalWrite(AXIS1_MOTOR_DIR, INVERTED_DIR);
  digitalWrite(AXIS2_MOTOR_DIR, DIR);
  digitalWrite(AXIS3_MOTOR_DIR, DIR);
  digitalWrite(AXIS4_MOTOR_DIR, DIR);
  digitalWrite(AXIS5_MOTOR_DIR, DIR);
  delay(5000);
  
  unsigned long i = 0;  
  int count = 24000;
  // 2400 line/revolution
  // 158 base gearbox
  // 128 wrist gboxes
  //5/2pi ball screw
  // 3/2 crown
  2400
  
  //Serial.println(basecount);
  
  for(i=0;i<189600;i++){
    
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
      delayMicroseconds(50);
  }

  if (DIR == LOW) {
      DIR = HIGH;
      INVERTED_DIR = LOW;
    } else {
      DIR = LOW;
      INVERTED_DIR = HIGH;
    }

}
