#include <util/atomic.h>

#define NMOTORS 4
#define UMAX 255
#define PGAIN 2
#define DGAIN 0.015
#define IGAIN 5
#define VMAX 12

const int ENC_A[] = {21,20,19,18};
const int ENC_B[] = {51,53,50,52};
const int PWM[] = {8,9,10,11};
const int INP_1[] = {22,24,27,29};
const int INP_2[] = {23,25,26,28};
const int JOY = A0;
const int CH1 = 2;
const int CH2 = 3;

const float linearMax = 1.0;
const float angularMax = 1.5;

const int CH1_CENTER_LIMITS[] = {1490,1510};
const int CH2_CENTER_LIMITS[] = {1480,1500};
const int CH1_LIMITS[] = {1000, 2000};
const int CH2_LIMITS[] = {1000, 1990};

volatile long timestart_ch1;
volatile long timedelta_ch1;
volatile long timestart_ch2;
volatile long timedelta_ch2;
volatile long pos_i[] = {0,0,0,0};

long previous_pos[] = {0,0,0,0};
float velocity[] = {0,0,0,0};
long prevT = 0;
float deltaT = 0;

int N = 1440;
double wheelRadius = 0.056;
double wheelDistance = 0.395;

int joyValue = 0;
float steeringValue;
float throttleValue;
float targetSpeed[] = {0,0,0,0};
float currentSpeed[] = {0,0,0,0};
float previousSpeed[] = {0,0,0,0};

class SimplePID{
  private:
    float kp, kd, ki, umax;
    float eprev, eintegral;

  public:
    SimplePID() : kp(2), kd(1), ki(5), umax(15), eprev(0.0), eintegral(0.0){}

    void setParams(float kpIn, float kdIn, float kiIn, float umaxIn){
      kp = kpIn; kd = kdIn; ki = kiIn; umax = umaxIn;
    }

    void evalu(float value, float target, float deltaT, int &pwr, int &dir){
      float e = target - value;
      float dedt = (e-eprev)/(deltaT);
      eintegral = eintegral + (0.5*(eprev+e)*deltaT);
      float u = kp*e + kd*dedt + ki*eintegral;

      dir = 1;
      if(u<0){
        dir = -1;
      }

      pwr = int(255 * (abs(u)/VMAX));
      if(pwr > umax){
        pwr = umax;
      } else if(pwr < 20){
        pwr = 0;
      }

      if(eintegral > 10){
        eintegral = 10;
      } else if(eintegral < -10){
        eintegral = -10;
      }
      
      eprev = e;
    }
};

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
}

template <int j>
void readEncoder(){
  int b = digitalRead(ENC_B[j]);
  if((j == 0) || (j == 2)){
    if(b > 0){
      pos_i[j]++;
    }
    else{
      pos_i[j]--;
    }
  } else if((j == 1) || (j == 3)){
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

ISR(TIMER5_COMPA_vect){
  count++;
}

void setup(){
  Serial.begin(9600);

  cli();
  TCCR5A = 0;
  TCCR5B = 0;
  TCNT5 = 0;
  OCR5A = 12499;
  TCCR5B |= (1 << WGM12);
  TCCR5B |= (1 << CS11 | 1 << CS10);
  TIMSK5 |= (1 << OCIE5A);
  sei();

  for(int k = 0; k < NMOTORS; k++){
    pinMode(ENC_A[k],INPUT);
    pinMode(ENC_B[k],INPUT);
    pinMode(PWM[k],OUTPUT);
    pinMode(INP_1[k],OUTPUT);
    pinMode(INP_2[k],OUTPUT);
    motor[k].setParams(PGAIN,DGAIN,IGAIN,UMAX);
  }
  pinMode(CH1, INPUT);
  pinMode(CH2, INPUT);
 
  attachInterrupt(digitalPinToInterrupt(ENC_A[0]),readEncoder<0>,RISING);
  attachInterrupt(digitalPinToInterrupt(ENC_A[1]),readEncoder<1>,RISING);
  attachInterrupt(digitalPinToInterrupt(ENC_A[2]),readEncoder<2>,RISING);
  attachInterrupt(digitalPinToInterrupt(ENC_A[3]),readEncoder<3>,RISING);
  attachInterrupt(digitalPinToInterrupt(CH1),readCh1,CHANGE);
  attachInterrupt(digitalPinToInterrupt(CH2),readCh2,CHANGE);
}

float mapf(float x, float in_min, float in_max, float out_min, float out_max){
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void readCh1(){
  unsigned long value = digitalRead(CH1);
  if(value == 1){
    timestart_ch1 = micros();
  } else if(value == 0){
    timedelta_ch1 = micros() - timestart_ch1;
  }
}

void readCh2(){
  unsigned long value = digitalRead(CH2);
  if(value == 1){
    timestart_ch2 = micros();
  } else if(value == 0){
    timedelta_ch2 = micros() - timestart_ch2;
  }
}

void loop(){
  if(count > count_prev){
    long currT = micros();
    float deltaT = ((float)(currT-prevT))/1.0e6;
    joyValue = analogRead(A0);
    float delta_pos[] = {0.0, 0.0, 0.0, 0.0};
  
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
      for(int k=0; k<NMOTORS; k++){
        delta_pos[k] = pos_i[k] - previous_pos[k];
        velocity[k] = delta_pos[k] / N / deltaT * 2 * PI;
        previous_pos[k] = pos_i[k];
      }
    }
    
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
      throttleValue = mapf(timedelta_ch2,CH2_LIMITS[0],CH2_LIMITS[1],-1,1);
      steeringValue = mapf(timedelta_ch1,CH1_LIMITS[0],CH1_LIMITS[1],1,-1);
      
      if((throttleValue >= -0.10) && (throttleValue <= 0.10)){
        throttleValue = 0.0;
      }
      if((steeringValue >= -0.25) && (steeringValue <= 0.25)){
        steeringValue = 0.0;
      }
    }

    for(int i=0; i<4; i++){
      targetSpeed[i] = 0.0;
    }

    Serial.print(throttleValue);
    Serial.print('\t');
    Serial.println(steeringValue);

    if(throttleValue < -1.5){
      throttleValue = 0.0;
    }
    if(steeringValue > 1.5){
      steeringValue = 0.0;
    }

    float linearSpeed = linearMax * throttleValue;
    float angularSpeed = angularMax * steeringValue;
    float rightSpeed = ((linearSpeed + (wheelDistance * angularSpeed)) / wheelRadius);
    float leftSpeed = ((linearSpeed - (wheelDistance * angularSpeed)) / wheelRadius);

    targetSpeed[0] = rightSpeed;
    targetSpeed[2] = rightSpeed;
    targetSpeed[1] = leftSpeed;
    targetSpeed[3] = leftSpeed;
    
    for(int k=0; k<NMOTORS; k++){
      currentSpeed[k] = 0.854*currentSpeed[k] + 0.0728*velocity[k] + 0.0728*previousSpeed[k];
      previousSpeed[k] = velocity[k];
    }

    for(int k=0; k<NMOTORS; k++){
      int pwr, dir;
      motor[k].evalu(velocity[k], targetSpeed[k], deltaT, pwr, dir);

      if((targetSpeed[k] < 0.1) && (targetSpeed[k] > -0.1)){
        pwr = 0;
        dir = 0;
      }
      setMotor(dir, pwr, PWM[k], INP_1[k], INP_2[k]);
    }

    float rightVirtualWheelSpeed = (velocity[0] + velocity[2]) * 0.5;
    float leftVirtualWheelSpeed = (velocity[1] + velocity[3]) * 0.5;

    prevT = currT;
    count_prev = count;
  }
}
