

#include <ESP32Servo.h>



const int PIN_STEP_IN1 = 14;
const int PIN_STEP_IN2 = 27;
const int PIN_STEP_IN3 = 26;
const int PIN_STEP_IN4 = 25;

const int PIN_SERVO    = 18;

const int PIN_TRIG     = 17;
const int PIN_ECHO     = 19;  



const long  STEPS_PER_REV   = 4096;   
const int   STEP_DELAY_US   = 1400;   
const float PAN_MIN_DEG     = -90.0;  
const float PAN_MAX_DEG     =  90.0;



const int TILT_MIN_DEG      = 70;     
const int TILT_MAX_DEG      = 130;
const int TILT_CENTER_DEG   = 90;    
const int TILT_SETTLE_MS    = 120;    



const float AMBIENT_TEMP_C  = 21.0;  
const unsigned long ECHO_TIMEOUT_US = 30000UL; 
const int   PING_SETTLE_MS  = 60;    
const float DIST_MAX_CM     = 400.0;
const float DIST_MIN_CM     = 2.0;
const float DIST_INVALID    = -1.0;


Servo tiltServo;

long  panSteps    = 0;      
int   tiltAngle   = TILT_CENTER_DEG;
float speedOfSound_cm_us;   

const uint8_t STEP_SEQ[8][4] = {
  {1,0,0,0},
  {1,1,0,0},
  {0,1,0,0},
  {0,1,1,0},
  {0,0,1,0},
  {0,0,1,1},
  {0,0,0,1},
  {1,0,0,1}
};
int seqIndex = 0;


void coilsOff() {
  digitalWrite(PIN_STEP_IN1, LOW);
  digitalWrite(PIN_STEP_IN2, LOW);
  digitalWrite(PIN_STEP_IN3, LOW);
  digitalWrite(PIN_STEP_IN4, LOW);
}

void applyStep(int idx) {
  digitalWrite(PIN_STEP_IN1, STEP_SEQ[idx][0]);
  digitalWrite(PIN_STEP_IN2, STEP_SEQ[idx][1]);
  digitalWrite(PIN_STEP_IN3, STEP_SEQ[idx][2]);
  digitalWrite(PIN_STEP_IN4, STEP_SEQ[idx][3]);
}


void stepBy(long steps) {
  int dir = (steps >= 0) ? 1 : -1;
  long n  = labs(steps);

  for (long i = 0; i < n; i++) {
    seqIndex = (seqIndex + dir + 8) % 8;
    applyStep(seqIndex);
    delayMicroseconds(STEP_DELAY_US);
    panSteps += dir;
  }
  coilsOff();  
}

long degToSteps(float deg) {
  return (long)((deg / 360.0) * STEPS_PER_REV);
}

float stepsToDeg(long steps) {
  return ((float)steps / (float)STEPS_PER_REV) * 360.0;
}

void panToDeg(float deg) {
  if (deg < PAN_MIN_DEG) deg = PAN_MIN_DEG;
  if (deg > PAN_MAX_DEG) deg = PAN_MAX_DEG;

  long target = degToSteps(deg);
  stepBy(target - panSteps);
}

float panCurrentDeg() {
  return stepsToDeg(panSteps);
}


void tiltTo(int deg) {
  if (deg < TILT_MIN_DEG) deg = TILT_MIN_DEG;
  if (deg > TILT_MAX_DEG) deg = TILT_MAX_DEG;

  tiltAngle = deg;
  tiltServo.write(tiltAngle);
  delay(TILT_SETTLE_MS);
}


float pingOnce() {
  digitalWrite(PIN_TRIG, LOW);
  delayMicroseconds(3);
  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);

  unsigned long dur = pulseIn(PIN_ECHO, HIGH, ECHO_TIMEOUT_US);
  if (dur == 0) return DIST_INVALID;          

  float cm = (dur * speedOfSound_cm_us) / 2.0;
  if (cm < DIST_MIN_CM || cm > DIST_MAX_CM) return DIST_INVALID;
  return cm;
}

float pingMedian3() {
  float a = pingOnce(); delay(PING_SETTLE_MS);
  float b = pingOnce(); delay(PING_SETTLE_MS);
  float c = pingOnce(); delay(PING_SETTLE_MS);

  if (a == DIST_INVALID || b == DIST_INVALID || c == DIST_INVALID) {
    if (a != DIST_INVALID) return a;
    if (b != DIST_INVALID) return b;
    return c;
  }

  float hi = max(a, max(b, c));
  float lo = min(a, min(b, c));
  return a + b + c - hi - lo;
}

//test
void testStepper() {
  Serial.println(F("\n-- stepper: 90 right, 90 left, x3"));
  for (int i = 0; i < 3; i++) {
    panToDeg(90);
    Serial.printf("   at %.1f deg (%ld steps)\n", panCurrentDeg(), panSteps);
    delay(400);
    panToDeg(-90);
    Serial.printf("   at %.1f deg (%ld steps)\n", panCurrentDeg(), panSteps);
    delay(400);
  }
  panToDeg(0);
  Serial.println(F("   returned to home"));
}

void testServo() {
  Serial.println(F("\n-- servo: min, max, center"));
  tiltTo(TILT_MIN_DEG);    Serial.printf("   tilt %d\n", tiltAngle); delay(500);
  tiltTo(TILT_MAX_DEG);    Serial.printf("   tilt %d\n", tiltAngle); delay(500);
  tiltTo(TILT_CENTER_DEG); Serial.printf("   tilt %d\n", tiltAngle);
}

void testUltrasonic() {
  Serial.println(F("\n-- ultrasonic: 10 readings"));
  for (int i = 0; i < 10; i++) {
    float d = pingMedian3();
    if (d == DIST_INVALID) Serial.println(F("   out of range"));
    else                   Serial.printf("   %.1f cm\n", d);
  }
}


void setup() {
  Serial.begin(115200);
  delay(600);

  pinMode(PIN_STEP_IN1, OUTPUT);
  pinMode(PIN_STEP_IN2, OUTPUT);
  pinMode(PIN_STEP_IN3, OUTPUT);
  pinMode(PIN_STEP_IN4, OUTPUT);
  coilsOff();

  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  digitalWrite(PIN_TRIG, LOW);

  tiltServo.setPeriodHertz(50);
  tiltServo.attach(PIN_SERVO, 500, 2400);
  tiltServo.write(TILT_CENTER_DEG);

  speedOfSound_cm_us = (331.3 + 0.606 * AMBIENT_TEMP_C) / 10000.0;

  Serial.println(F("\n=== turret starter ==="));
  Serial.printf("speed of sound: %.1f m/s at %.1f C\n",
                speedOfSound_cm_us * 10000.0, AMBIENT_TEMP_C);
  Serial.printf("steps/rev: %ld   deg/step: %.4f\n",
                STEPS_PER_REV, 360.0 / STEPS_PER_REV);
  Serial.println(F("point the turret where you want, then reset"));
  delay(1500);

  testStepper();
  testServo();
  testUltrasonic();

  Serial.println(F("\n-- entering sweep demo"));
}


void loop() {
  static float deg = PAN_MIN_DEG;
  static int   dir = 1;

  const float STEP_DEG = 5.0;

  panToDeg(deg);
  delay(40);                   

  float d = pingMedian3();

  Serial.printf("pan=%6.1f  tilt=%3d  ", deg, tiltAngle);
  if (d == DIST_INVALID) Serial.println(F("dist=  ---"));
  else                   Serial.printf("dist=%6.1f cm\n", d);

  deg += STEP_DEG * dir;
  if (deg > PAN_MAX_DEG) { deg = PAN_MAX_DEG; dir = -1; }
  if (deg < PAN_MIN_DEG) { deg = PAN_MIN_DEG; dir =  1; }
}
