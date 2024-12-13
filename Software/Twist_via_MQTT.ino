#include <util/atomic.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>        // For parsing JSON input


#define NMOTORS 4

#define UMAX 255
#define PGAIN 2
#define DGAIN 0.015
#define IGAIN 5
#define VMAX 12

// WiFi settings
char ssid[] = "";  // Network SSID
char pass[] = "";  // WPA key

// MQTT Broker settings
const char *mqtt_broker = "broker.emqx.io"; // EMQX broker endpoint
const char *mqtt_topic = "Robot/velocity"; // MQTT topic
const char *mqtt_username = "emqx"; // MQTT username for authentication
const char *mqtt_password = "public"; // MQTT password for authentication
const int mqtt_port = 1883; // MQTT port (TCP)

WiFiClient wifiClient;
PubSubClient mqtt_client(wifiClient);

void connectToWiFi();
void connectToMQTTBroker();
void mqttCallback(char *topic, byte *payload, unsigned int length);
void processTwistMessage(const char *payload);




// Declare Pins
// RB, LB, RF, LF
const int ENC_A[] = {21,20,19,18};
const int ENC_B[] = {51,53,50,52};
const int PWM[] = {8,9,10,11};
const int INP_1[] = {22,24,27,29};
const int INP_2[] = {23,25,26,28};

//const double maxRPS = 9.0; // maximum rads per second
const float linearMax = 1.0;
const float angularMax = 1.5;



// volatile variables (in encoder callback)
volatile long pos_i[] = {0,0,0,0};


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
  connectToWiFi();
  mqtt_client.setServer(mqtt_broker, mqtt_port);
  mqtt_client.setCallback(mqttCallback);
  connectToMQTTBroker();

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

  attachInterrupt(digitalPinToInterrupt(ENC_A[0]),readEncoder<0>,RISING);
  attachInterrupt(digitalPinToInterrupt(ENC_A[1]),readEncoder<1>,RISING);
  attachInterrupt(digitalPinToInterrupt(ENC_A[2]),readEncoder<2>,RISING);
  attachInterrupt(digitalPinToInterrupt(ENC_A[3]),readEncoder<3>,RISING);

}

void connectToWiFi() {
    Serial.println("Connecting to WiFi");
    WiFi.begin(ssid, pass);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nConnected to the WiFi network");
}

void connectToMQTTBroker() {
    while (!mqtt_client.connected()) {
        String client_id = "arduino-mega-client"; // Change to esp32-client for clarity
        Serial.print("Connecting to MQTT Broker as ");
        Serial.println(client_id); // Correct usage of Serial.println
        if (mqtt_client.connect(client_id.c_str(), mqtt_username, mqtt_password)) {
            Serial.println("Connected to MQTT broker");
            mqtt_client.subscribe(mqtt_topic);
            // Publish message upon successful connection
           // mqtt_client.publish(mqtt_topic, "publish any message here");
        } else {
            Serial.print("Failed to connect to MQTT broker, rc=");
            Serial.print(mqtt_client.state());
            Serial.println(" try again in 5 seconds");
            delay(5000);
        }
    }
}

void mqttCallback(char *topic, byte *payload, unsigned int length) {
    Serial.print("Message received on topic: ");
    Serial.println(topic);

    // Convert payload (byte array) to a String
    String msg;
    for (unsigned int i = 0; i < length; i++) {
        msg += (char)payload[i];
    }

    Serial.print("Message: ");
    Serial.println(msg);
    Serial.println("-----------------------");

    // Pass the message as a C-string
    processTwistMessage(msg.c_str());
}



float mapf(float x, float in_min, float in_max, float out_min, float out_max) {
     float result;
     result = (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
     return result;
}



void loop() {
    if (!mqtt_client.connected()) {
        connectToMQTTBroker();
    }
    mqtt_client.loop();

  // T = 0.05s
  if (count > count_prev) {
    // Compute time
    long currT = micros();
    float deltaT = ((float) (currT-prevT))/1.0e6;


      
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


      throttleValue = 0.5;
      steeringValue = 0;


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
    



    prevT = currT;
    count_prev = count;
  }

}


// Process the incoming twist message and parse the linear and angular velocities
void processTwistMessage(const char *payload) {
  StaticJsonDocument<200> doc;
  DeserializationError error = deserializeJson(doc, payload);

  if (!error) {
    float linear_x = doc["linear"]["x"];
    float linear_y = doc["linear"]["y"];
    float linear_z = doc["linear"]["z"];
    float angular_x = doc["angular"]["x"];
    float angular_y = doc["angular"]["y"];
    float angular_z = doc["angular"]["z"];

    // For this robot, we care only about the x component of linear and z component of angular velocity
    throttleValue = linear_x;  // Linear velocity (x-axis)
    steeringValue = angular_z; // Angular velocity (z-axis)

    // Print out the velocities to the Serial Monitor
    Serial.print("Throttle (linear x): ");
    Serial.println(throttleValue);
    Serial.print("Steering (angular z): ");
    Serial.println(steeringValue);
  } else {
    Serial.println("Error parsing JSON");
  }
}
