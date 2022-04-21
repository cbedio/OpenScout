#include <util/atomic.h>

#define NMOTORS 4

#define UMAX 255
#define PGAIN 2
#define DGAIN 0.015
#define IGAIN 5

#define VMAX 12

//motor[k].setParams(70,0.1,5,255);

// Declare Pins
// RB, LB, RF, LF
const int ENC_A[] = {21,20,19,18};
const int ENC_B[] = {51,53,50,52};
const int PWM[] = {8,9,10,11};
const int INP_1[] = {22,24,27,29};
const int INP_2[] = {23,25,26,28};
const int JOY = A0;
const int CH1 = 2; // defines the steering channel
const int CH2 = 3; // defines the throttling channel

//const double maxRPS = 9.0; // maximum rads per second
const float linearMax = 1.0;
const float angularMax = 1.5;

const int CH1_CENTER_LIMITS[] = {1490,1510};
const int CH2_CENTER_LIMITS[] = {1480,1500};
const int CH1_LIMITS[] = {1000, 2000};
const int CH2_LIMITS[] = {1000, 1990};

// volatile variables (in channel callback)
volatile long timestart_ch1;
volatile long timedelta_ch1;
volatile long timestart_ch2;
volatile long timedelta_ch2;

// volatile variables (in encoder callback)
volatile long pos_i[] = {0,0,0,0};
//volatile long previous_pos_i[] = {0,0,0,0};
//volatile double velocity_i[] = {0,0,0,0};
//volatile long prevT_i[] = {0,0,0,0};
//volatile float deltaT_i[] = {0,0,0,0};

long previous_pos[] = {0,0,0,0};
float velocity[] = {0,0,0,0};
long prevT = 0;
float deltaT = 0;

// Robot variables
int N = 1440;
double wheelRadius = 0.056; // m
double wheelDistance = 0.395; // m

int joyValue = 0;
float steeringValue;
float throttleValue;
float targetSpeed[] = {0,0,0,0};
float currentSpeed[] = {0,0,0,0};
float previousSpeed[] = {0,0,0,0};

// A class to compute the control signal
class SimplePID{
  private:
    float kp, kd, ki, umax; // Parameters
    float eprev, eintegral; // Storage

  public:
  // Constructor
  SimplePID() : kp(2), kd(1), ki(5), umax(15), eprev(0.0), eintegral(0.0){}

  // A function to set the parameters
  void setParams(float kpIn, float kdIn, float kiIn, float umaxIn){
    kp = kpIn; kd = kdIn; ki = kiIn; umax = umaxIn;
  }

  // A function to compute the control signal
  void evalu(float value, float target, float deltaT, int &pwr, int &dir){
    // error
    float e = target - value;
  
    // derivative
    float dedt = (e-eprev)/(deltaT);
  
    // integral
    eintegral = eintegral + (0.5*(eprev+e)*deltaT); // 1/2 * (a + b) * h
  
    // control signal
    float u = kp*e + kd*dedt + ki*eintegral;

    // motor direction
    dir = 1;
    if(u<0){
      dir = -1;
    }

    // motor power
    pwr = int(255 * (abs(u)/VMAX));
    if( pwr > umax ){
      pwr = umax;
    } else if ( pwr < 20) {
      pwr = 0;
    }

    if (eintegral > 10) {
      eintegral = 10;
    } else if (eintegral < -10) {
      eintegral = -10;
    }
    
    // store previous error
    eprev = e;

    //Serial.print(e);
    //Serial.print('\t');
    //Serial.print(eintegral);
    //Serial.println();
  }
  
};

// PID class instances
SimplePID motor[NMOTORS];

void setMotor(int dir, int PWMVal, int PWM, int INP_1, int INP_2){
  analogWrite(PWM,PWMVal);
  if(dir == 1){
    digitalWrite(INP_1,LOW);
    digitalWrite(INP_2,HIGH);
  }
  else if(dir == -1){
    digitalWrite(INP_1,HIGH);
    digitalWrite(INP_2,LOW);
  }
  else{
    digitalWrite(INP_1,LOW);
    digitalWrite(INP_2,LOW);
  }

  /*Serial.print(dir);
  Serial.print('\t');
  Serial.print(PWMVal);
  Serial.print('\t');*/
}

template <int j>
void readEncoder(){
  int b = digitalRead(ENC_B[j]);
  int i = 0;
  if ((j == 0) || (j == 2)) { 
    if(b > 0){
      pos_i[j]++;
    }
    else{
      pos_i[j]--;
    }
  } else if ((j == 1) || (j == 3)) {
    if(b > 0){
      pos_i[j]--;
    }
    else{
      pos_i[j]++;
    }
  }
}

volatile int count = 0;
int count_prev = 0;

ISR(TIMER5_COMPA_vect) {
  count++;
}

void setup() {
  
  Serial.begin(115200);

  //Setup timer interrupt using timer 5
  cli();
  TCCR5A = 0;
  TCCR5B = 0;
  TCNT5 = 0;
  OCR5A = 12499; //Prescaler = 64
  TCCR5B |= (1 << WGM12);
  TCCR5B |= (1 << CS11 | 1 << CS10);
  TIMSK5 |= (1 << OCIE5A);
  sei(); // re-enables interrupts

  for(int k = 0; k < NMOTORS; k++){
    pinMode(ENC_A[k],INPUT);
    pinMode(ENC_B[k],INPUT);
    pinMode(PWM[k],OUTPUT);
    pinMode(INP_1[k],OUTPUT);
    pinMode(INP_2[k],OUTPUT);
    
 //   motor[k].setParams(500, 70, 20, 255);
    //motor[k].setParams(70,0.1,5,255);
    motor[k].setParams(PGAIN,DGAIN,IGAIN,UMAX);
  }
  pinMode (CH1, INPUT);// initialises the channels
  pinMode (CH2, INPUT);// initialises the channels
 
  attachInterrupt(digitalPinToInterrupt(ENC_A[0]),readEncoder<0>,RISING);
  attachInterrupt(digitalPinToInterrupt(ENC_A[1]),readEncoder<1>,RISING);
  attachInterrupt(digitalPinToInterrupt(ENC_A[2]),readEncoder<2>,RISING);
  attachInterrupt(digitalPinToInterrupt(ENC_A[3]),readEncoder<3>,RISING);
  attachInterrupt(digitalPinToInterrupt(CH1),readCh1,CHANGE);
  attachInterrupt(digitalPinToInterrupt(CH2),readCh2,CHANGE);
  
}

float mapf(float x, float in_min, float in_max, float out_min, float out_max) {
     float result;
     result = (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
     return result;
}

void readCh1() 
{
     unsigned long value;
     value = digitalRead (CH1);
     if (value == 1) {
      timestart_ch1 = micros();
     } else if (value == 0) {
      long timestop = micros();
      timedelta_ch1 = timestop - timestart_ch1; 
     }
}

void readCh2() 
{
     unsigned long value;
     value = digitalRead (CH2);
     if (value == 1) {
      timestart_ch2 = micros();
     } else if (value == 0) {
      long timestop = micros();
      timedelta_ch2 = timestop - timestart_ch2; 
     }
}

void loop() {

  // T = 0.05s
  if (count > count_prev) {
    // Compute time
    long currT = micros();
    float deltaT = ((float) (currT-prevT))/1.0e6;

    // set target position
    //joyValue = analogRead(A0);
  
      
    float delta_pos[] = {0.0, 0.0, 0.0, 0.0};
  
    // Read the velocity in an atomic block to avoid a potential misread
    // and convert from counts/s to rad/s
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
      for (int k=0; k<NMOTORS; k++){
        delta_pos[k] = pos_i[k] - previous_pos[k];
        velocity[k] = delta_pos[k] / N / deltaT * 2 * PI;
        previous_pos[k] = pos_i[k];
      }
    }
    
    // Read transmitter
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
      throttleValue = mapf(timedelta_ch2,CH2_LIMITS[0],CH2_LIMITS[1],-1,1);; //maxRPS*sin(prevT/1e6*1);
      steeringValue = mapf(timedelta_ch1,CH1_LIMITS[0],CH1_LIMITS[1],1,-1);; //maxRPS*sin(prevT/1e6*1);
      // Make a dead-zone for throttling
      if ((throttleValue >= -0.10) && (throttleValue <= 0.10)) {
        throttleValue = 0.0;
      }
      // Make a dead-zone for steering
      if ((steeringValue >= -0.25) && (steeringValue <= 0.25)) {
        steeringValue = 0.0;
      }
    }


    // Reset Speeds
    targetSpeed[0] = 0.0;
    targetSpeed[2] = 0.0;
    targetSpeed[1] = 0.0;
    targetSpeed[3] = 0.0;

    Serial.print(throttleValue);
    Serial.print('\t');
    Serial.println(steeringValue);

    // if controller is off when robot is turned on. throttleValue received is ~ -3. Going to reset this to zero
    if (throttleValue < -1.5) {
      throttleValue = 0.0;
    }

    // if controller is off when robot is turned on. steeringValue received is ~ 3. Going to reset this to zero
    if (steeringValue > 1.5) {
      steeringValue = 0.0;
    }

    // Map desired speed to motor speeds
    float linearSpeed = linearMax * throttleValue;
    //linearSpeed = 0.0;
    float angularSpeed = angularMax * steeringValue;
    //float angularSpeed = 0.0;
    //float rightSpeed = (1/wheelRadius) * linearSpeed - (wheelDistance/wheelRadius) * angularSpeed;
    //float leftSpeed = (1/wheelRadius) * linearSpeed + (wheelDistance/wheelRadius) * angularSpeed;

    float rightSpeed = ((linearSpeed + (wheelDistance * angularSpeed)) / wheelRadius);
    float leftSpeed = ((linearSpeed - (wheelDistance * angularSpeed)) / wheelRadius);

    
    targetSpeed[0] = rightSpeed;
    targetSpeed[2] = rightSpeed;
    targetSpeed[1] = leftSpeed;
    targetSpeed[3] = leftSpeed;
    

    /*if (count < 5 * 20) 
    {
      targetSpeed[0] = 0;
      targetSpeed[2] = 0;
      targetSpeed[1] = 0;
      targetSpeed[3] = 0;
    } 
    else if (count > 25 * 20)
    {
      targetSpeed[0] = 0;
      targetSpeed[2] = 0;
      targetSpeed[1] = 0;
      targetSpeed[3] = 0;
    }
    else
    {
      targetSpeed[0] = rightSpeed;
      targetSpeed[2] = rightSpeed;
      targetSpeed[1] = leftSpeed;
      targetSpeed[3] = leftSpeed;
    }*/
    
    
    // Low-pass filter (25 Hz cutoff)
    for (int k=0; k<NMOTORS; k++) {
      currentSpeed[k] = 0.854*currentSpeed[k] + 0.0728*velocity[k] + 0.0728*previousSpeed[k];
      previousSpeed[k] = velocity[k];
    }

    
    // Set speeds
    for (int k=0; k<NMOTORS; k++) {
      int pwr, dir;
      motor[k].evalu(velocity[k], targetSpeed[k], deltaT, pwr, dir);

      if ((targetSpeed[k] < 0.1) && (targetSpeed[k] > -0.1) ) {
        pwr = 0;
        dir = 0;
      }
      setMotor(dir, pwr, PWM[k], INP_1[k], INP_2[k]);
    }

    // RB, LB, RF, LF

    // calculate forward kinematics from read wheel velocities
    float rightVirtualWheelSpeed = (velocity[0] + velocity[2]) * 0.5;
    float leftVirtualWheelSpeed = (velocity[1] + velocity[3]) * 0.5;

    float feedbackAngularVelocity = (((rightVirtualWheelSpeed * wheelRadius) + ( -1* leftVirtualWheelSpeed * wheelRadius)) / ( 2 * wheelDistance));
    


//    Serial.println();
  
    //Serial.print(targetSpeed[0]);
    //Serial.print("\t");
    //Serial.print(velocity[0]);
//    Serial.print("\t");
//    Serial.print(velocity[1]);
//    Serial.print("\t");
//    Serial.print(velocity[2]);
//    Serial.print("\t");
//    Serial.print(velocity[3]);
    //Serial.println();

//    Serial.print(throttleValue);
//    Serial.print("\t");
//    Serial.print(steeringValue);
//    Serial.println();

      /*Serial.print(linearSpeed);
      Serial.print("\t");
      Serial.print(angularSpeed);
      Serial.print("\t");
      Serial.print(leftSpeed);
      Serial.print("\t");
      Serial.print(rightSpeed);
      Serial.println();*/

    prevT = currT;
    count_prev = count;
  }

}
