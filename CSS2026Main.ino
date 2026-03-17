#include <EEPROM.h>

// Stepper Motor 1
const int stepPin1 = 6;
const int dirPin1  = 5;
const int enPin1   = 7;

// Stepper Motor 2
const int stepPin2 = 3;
const int dirPin2  = 2;
const int enPin2   = 7;

// Color Sensor (TCS3200)
#define S0         8
#define S1         9
#define S2         10
#define S3         11
#define SENSOR_OUT 12

const int RATE_STEPPER1 = 170000;
const int RATE_STEPPER2 = 2000;

const int STEPS_PER_90 = 50;   // tune for your mechanism
const int INDEX_DIR    = LOW;  // LOW=CW, HIGH=CCW (CW = Clock Wise, CCW = Counter Clock Wise)

const int TIME_BETWEEN_ACTIONS = 500;

#define CALIBRATION_SAMPLES        30
#define STABLE_READS               3
#define MIN_SEPARATION             8
#define CALIBRATION_MIN_DISTANCE   10
#define NUM_CHANNELS               4

enum Color {
  UNKNOWN,
  BLUE,
  YELLOW,
  GREEN,
  RED,
  BROWN,
  COLOR_COUNT
};

const char* colorNames[] = {
  "UNKNOWN", "BLUE", "YELLOW", "GREEN", "RED", "BROWN"
};

// Format: {R,G,B,C}
int baselineRef[NUM_CHANNELS] = {10, 17, 20, 15};
int blueVal[NUM_CHANNELS]     = {14, 20, 14, 12};
int yellowVal[NUM_CHANNELS]   = {10, 15, 18,  9};
int greenVal[NUM_CHANNELS]    = {12, 14, 17, 11};
int redVal[NUM_CHANNELS]      = {15, 79, 20, 13};
int brownVal[NUM_CHANNELS]    = {12, 18, 22, 14};

int rgbc[NUM_CHANNELS] = {0, 0, 0, 0};

struct ContainerConfig {
  int steps;
  int dirLevel;
};

const ContainerConfig containerMap[COLOR_COUNT] = {
  {  0, HIGH }, // UNKNOWN
  { 34, HIGH }, // BLUE
  { 29, HIGH }, // YELLOW
  {  0, HIGH }, // GREEN
  { 29, LOW  }, // RED
  { 29, LOW  }, // BROWN
};

int numberOffSteps         = 0;
int directionStepperMotor2 = 0;

struct DetectionResult {
  Color color;
  long bestDist;
  long secondBest;
  long separation;
  int r;
  int g;
  int b;
  int c;
};

void setupStepperMotor1();
void setupStepperMotor2();
void setupColorSensor();

void stepMotor(int stepPin, int steps, int rateUs);
void index90ToNextStation();
void indexToScanner();
void indexToDeposit();

void stepperMotor2ToContainer(Color c);
void stepperMotor2ReturnToBase();

void readRGBC();
long colorDistanceSq(const int a[NUM_CHANNELS], const int b[NUM_CHANNELS]);
DetectionResult detectColorDetailed(const int current[NUM_CHANNELS]);
DetectionResult readStableColorDetailed();

void loadCalibration();
void saveCalibration();
void calibrateColorsIndexed();
void takeAverageReading(int outAvg[NUM_CHANNELS], int samples);
void countdownSeconds(int seconds);

// JSON helpers
void jsonEvent(const char* type);
void jsonState(const char* state);
void jsonDetectionSample(const DetectionResult &d, int attempt, int consecutive);
void jsonDetectionFinal(const DetectionResult &d, bool stableHit, int maxVotes);
void jsonCalibrationPoint(const char* label, const int v[NUM_CHANNELS], long distFromBaseline);
void jsonCalibrationSaved();
void jsonCommand(const char* cmd);
void jsonError(const char* msg);

void setup() {
  Serial.begin(9600);

  setupStepperMotor1();
  setupStepperMotor2();
  setupColorSensor();
  loadCalibration();

  jsonEvent("boot");
}

void loop() {
  if (Serial.available()) {
    char cmd = Serial.read();
    while (Serial.available()) Serial.read();

    if (cmd == 'C' || cmd == 'c') {
      jsonCommand("calibrate");
      calibrateColorsIndexed();
      return;
    } else if (cmd == 'V' || cmd == 'v') {
      jsonCommand("view_calibration");
      jsonCalibrationPoint("baseline", baselineRef, 0);
      jsonCalibrationPoint("blue", blueVal, colorDistanceSq(baselineRef, blueVal));
      jsonCalibrationPoint("yellow", yellowVal, colorDistanceSq(baselineRef, yellowVal));
      jsonCalibrationPoint("green", greenVal, colorDistanceSq(baselineRef, greenVal));
      jsonCalibrationPoint("red", redVal, colorDistanceSq(baselineRef, redVal));
      jsonCalibrationPoint("brown", brownVal, colorDistanceSq(baselineRef, brownVal));
      return;
    }
  }

  delay(500);

  jsonState("index_to_scanner");
  indexToScanner();
  delay(TIME_BETWEEN_ACTIONS);

  DetectionResult finalDet = readStableColorDetailed();

  jsonState("set_container");
  stepperMotor2ToContainer(finalDet.color);

  jsonState("index_to_deposit");
  delay(TIME_BETWEEN_ACTIONS);
  indexToDeposit();

  delay(300);

  jsonState("container_return_base");
  stepperMotor2ReturnToBase();

  jsonState("cycle_done");
  delay(500);
}

void setupStepperMotor1() {
  pinMode(stepPin1, OUTPUT);
  pinMode(dirPin1,  OUTPUT);
  pinMode(enPin1,   OUTPUT);
  digitalWrite(enPin1, LOW);
  digitalWrite(dirPin1, INDEX_DIR);
}

void setupStepperMotor2() {
  pinMode(stepPin2, OUTPUT);
  pinMode(dirPin2,  OUTPUT);
  pinMode(enPin2,   OUTPUT);
  digitalWrite(enPin2, LOW);
}

void setupColorSensor() {
  pinMode(S0, OUTPUT);
  pinMode(S1, OUTPUT);
  pinMode(S2, OUTPUT);
  pinMode(S3, OUTPUT);
  pinMode(SENSOR_OUT, INPUT);

  digitalWrite(S0, HIGH);
  digitalWrite(S1, HIGH);
}

void stepMotor(int stepPin, int steps, int rateUs) {
  for (int i = 0; i < steps; i++) {
    digitalWrite(stepPin, HIGH);
    delayMicroseconds(rateUs);
    digitalWrite(stepPin, LOW);
    delayMicroseconds(rateUs);
  }
}

void index90ToNextStation() {
  stepMotor(stepPin1, STEPS_PER_90, RATE_STEPPER1);
}

void indexToScanner() { index90ToNextStation(); }
void indexToDeposit() { index90ToNextStation(); }

void stepperMotor2ToContainer(Color c) {
  if (c <= UNKNOWN || c >= COLOR_COUNT) {
    numberOffSteps = 0;
    directionStepperMotor2 = 0;
    return;
  }

  if (c == GREEN) {
    numberOffSteps = 0;
    directionStepperMotor2 = 0;
    return;
  }

  const ContainerConfig& cfg = containerMap[c];
  numberOffSteps = cfg.steps;
  digitalWrite(dirPin2, cfg.dirLevel);
  directionStepperMotor2 = (cfg.dirLevel == HIGH) ? 1 : 2;
  stepMotor(stepPin2, numberOffSteps, RATE_STEPPER2);
}

void stepperMotor2ReturnToBase() {
  if (numberOffSteps <= 0) return;

  if (directionStepperMotor2 == 1) {
    digitalWrite(dirPin2, LOW);
  } else if (directionStepperMotor2 == 2) {
    digitalWrite(dirPin2, HIGH);
  } else {
    return;
  }

  stepMotor(stepPin2, numberOffSteps, RATE_STEPPER2);
  numberOffSteps = 0;
  directionStepperMotor2 = 0;
}

void readRGBC() {
  // Red
  digitalWrite(S2, LOW);  digitalWrite(S3, LOW);
  rgbc[0] = pulseIn(SENSOR_OUT, LOW); delay(5);

  // Green
  digitalWrite(S2, HIGH); digitalWrite(S3, HIGH);
  rgbc[1] = pulseIn(SENSOR_OUT, LOW); delay(5);

  // Blue
  digitalWrite(S2, LOW);  digitalWrite(S3, HIGH);
  rgbc[2] = pulseIn(SENSOR_OUT, LOW); delay(5);

  // Clear
  digitalWrite(S2, HIGH); digitalWrite(S3, LOW);
  rgbc[3] = pulseIn(SENSOR_OUT, LOW); delay(5);
}

long colorDistanceSq(const int a[NUM_CHANNELS], const int b[NUM_CHANNELS]) {
  long sum = 0;
  for (int i = 0; i < NUM_CHANNELS; i++) {
    long d = (long)a[i] - (long)b[i];
    sum += d * d;
  }
  return sum;
}

DetectionResult detectColorDetailed(const int current[NUM_CHANNELS]) {
  const int* presets[5] = { blueVal, yellowVal, greenVal, redVal, brownVal };
  Color colors[5]       = { BLUE,    YELLOW,    GREEN,    RED,    BROWN    };

  long bestDist = 999999;
  long secondBest = 999999;
  Color bestColor = UNKNOWN;

  for (int i = 0; i < 5; i++) {
    long d = colorDistanceSq(current, presets[i]);
    if (d < bestDist) {
      secondBest = bestDist;
      bestDist = d;
      bestColor = colors[i];
    } else if (d < secondBest) {
      secondBest = d;
    }
  }

  DetectionResult out;
  out.bestDist = bestDist;
  out.secondBest = secondBest;
  out.separation = secondBest - bestDist;
  out.r = current[0];
  out.g = current[1];
  out.b = current[2];
  out.c = current[3];

  if (out.separation < MIN_SEPARATION) out.color = UNKNOWN;
  else out.color = bestColor;

  return out;
}

DetectionResult readStableColorDetailed() {
  int colorVotes[COLOR_COUNT] = {0};
  int consecutive = 0;
  int maxVotes = 0;
  Color last = UNKNOWN;
  Color winner = UNKNOWN;

  DetectionResult lastResult;
  lastResult.color = UNKNOWN;
  lastResult.bestDist = 999999;
  lastResult.secondBest = 999999;
  lastResult.separation = 0;
  lastResult.r = lastResult.g = lastResult.b = lastResult.c = 0;

  delay(200);

  for (int i = 0; i < 12; i++) {
    readRGBC();
    DetectionResult d = detectColorDetailed(rgbc);
    lastResult = d;

    if (d.color != UNKNOWN) {
      colorVotes[d.color]++;
      if (d.color == last) consecutive++;
      else { last = d.color; consecutive = 1; }

      if (colorVotes[d.color] > maxVotes) {
        maxVotes = colorVotes[d.color];
        winner = d.color;
      }

      jsonDetectionSample(d, i + 1, consecutive);

      if (consecutive >= STABLE_READS) {
        jsonDetectionFinal(d, true, maxVotes);
        return d;
      }
    } else {
      consecutive = 0;
      jsonDetectionSample(d, i + 1, consecutive);
    }

    delay(150);
  }

  if (maxVotes >= STABLE_READS) {
    lastResult.color = winner;
  } else {
    lastResult.color = UNKNOWN;
  }

  jsonDetectionFinal(lastResult, false, maxVotes);
  return lastResult;
}

void countdownSeconds(int seconds) {
  for (int i = seconds; i > 0; i--) delay(1000);
}

void takeAverageReading(int outAvg[NUM_CHANNELS], int samples) {
  long sum[NUM_CHANNELS] = {0, 0, 0, 0};
  for (int i = 0; i < samples; i++) {
    readRGBC();
    for (int ch = 0; ch < NUM_CHANNELS; ch++) sum[ch] += rgbc[ch];
    delay(50);
  }
  for (int ch = 0; ch < NUM_CHANNELS; ch++) outAvg[ch] = (int)(sum[ch] / samples);
}

void calibrateColorsIndexed() {
  jsonState("calibration_start");

  // Baseline
  countdownSeconds(3);
  indexToScanner();
  delay(300);

  int avg[NUM_CHANNELS];
  takeAverageReading(avg, CALIBRATION_SAMPLES);
  for (int ch = 0; ch < NUM_CHANNELS; ch++) baselineRef[ch] = avg[ch];
  jsonCalibrationPoint("baseline", baselineRef, 0);

  // Move away from scanner to load station
  index90ToNextStation();
  delay(300);

  struct CalItem { Color c; int* target; };
  CalItem items[] = {
    { BLUE,   blueVal   },
    { YELLOW, yellowVal },
    { GREEN,  greenVal  },
    { RED,    redVal    },
    { BROWN,  brownVal  },
  };

  for (unsigned int i = 0; i < sizeof(items) / sizeof(items[0]); i++) {
    countdownSeconds(3);
    indexToScanner();
    delay(300);

    takeAverageReading(avg, CALIBRATION_SAMPLES);
    for (int ch = 0; ch < NUM_CHANNELS; ch++) items[i].target[ch] = avg[ch];

    long dist = colorDistanceSq(baselineRef, avg);
    jsonCalibrationPoint(colorNames[items[i].c], items[i].target, dist);

    index90ToNextStation();
    delay(300);
  }

  saveCalibration();
  jsonCalibrationSaved();
  jsonState("calibration_done");
}

void loadCalibration() {
  if (EEPROM.read(0) == 0xA5) {
    int addr = 1;
    for (int i = 0; i < NUM_CHANNELS; i++) baselineRef[i] = EEPROM.read(addr++);
    for (int i = 0; i < NUM_CHANNELS; i++) blueVal[i]     = EEPROM.read(addr++);
    for (int i = 0; i < NUM_CHANNELS; i++) yellowVal[i]   = EEPROM.read(addr++);
    for (int i = 0; i < NUM_CHANNELS; i++) greenVal[i]    = EEPROM.read(addr++);
    for (int i = 0; i < NUM_CHANNELS; i++) redVal[i]      = EEPROM.read(addr++);
    for (int i = 0; i < NUM_CHANNELS; i++) brownVal[i]    = EEPROM.read(addr++);
    jsonEvent("calibration_loaded");
  } else {
    jsonEvent("no_calibration");
  }
}

void saveCalibration() {
  EEPROM.write(0, 0xA5);
  int addr = 1;
  for (int i = 0; i < NUM_CHANNELS; i++) EEPROM.write(addr++, baselineRef[i]);
  for (int i = 0; i < NUM_CHANNELS; i++) EEPROM.write(addr++, blueVal[i]);
  for (int i = 0; i < NUM_CHANNELS; i++) EEPROM.write(addr++, yellowVal[i]);
  for (int i = 0; i < NUM_CHANNELS; i++) EEPROM.write(addr++, greenVal[i]);
  for (int i = 0; i < NUM_CHANNELS; i++) EEPROM.write(addr++, redVal[i]);
  for (int i = 0; i < NUM_CHANNELS; i++) EEPROM.write(addr++, brownVal[i]);
}

// Sending json to the esp-32
void jsonEvent(const char* type) {
  Serial.print(F("{\"type\":\"event\",\"event\":\""));
  Serial.print(type);
  Serial.println(F("\"}"));
}

void jsonState(const char* state) {
  Serial.print(F("{\"type\":\"state\",\"state\":\""));
  Serial.print(state);
  Serial.println(F("\"}"));
}

void jsonDetectionSample(const DetectionResult &d, int attempt, int consecutive) {
  Serial.print(F("{\"type\":\"detection_sample\",\"attempt\":"));
  Serial.print(attempt);
  Serial.print(F(",\"color\":\""));
  Serial.print(colorNames[d.color]);
  Serial.print(F("\",\"bestDist\":"));
  Serial.print(d.bestDist);
  Serial.print(F(",\"secondBest\":"));
  Serial.print(d.secondBest);
  Serial.print(F(",\"separation\":"));
  Serial.print(d.separation);
  Serial.print(F(",\"consecutive\":"));
  Serial.print(consecutive);
  Serial.print(F(",\"rgbc\":{\"r\":"));
  Serial.print(d.r);
  Serial.print(F(",\"g\":"));
  Serial.print(d.g);
  Serial.print(F(",\"b\":"));
  Serial.print(d.b);
  Serial.print(F(",\"c\":"));
  Serial.print(d.c);
  Serial.println(F("}}"));
}

void jsonDetectionFinal(const DetectionResult &d, bool stableHit, int maxVotes) {
  Serial.print(F("{\"type\":\"detection_final\",\"color\":\""));
  Serial.print(colorNames[d.color]);
  Serial.print(F("\",\"bestDist\":"));
  Serial.print(d.bestDist);
  Serial.print(F(",\"secondBest\":"));
  Serial.print(d.secondBest);
  Serial.print(F(",\"separation\":"));
  Serial.print(d.separation);
  Serial.print(F(",\"stableHit\":"));
  Serial.print(stableHit ? F("true") : F("false"));
  Serial.print(F(",\"maxVotes\":"));
  Serial.print(maxVotes);
  Serial.print(F(",\"rgbc\":{\"r\":"));
  Serial.print(d.r);
  Serial.print(F(",\"g\":"));
  Serial.print(d.g);
  Serial.print(F(",\"b\":"));
  Serial.print(d.b);
  Serial.print(F(",\"c\":"));
  Serial.print(d.c);
  Serial.println(F("}}"));
}

void jsonCalibrationPoint(const char* label, const int v[NUM_CHANNELS], long distFromBaseline) {
  Serial.print(F("{\"type\":\"calibration_point\",\"label\":\""));
  Serial.print(label);
  Serial.print(F("\",\"distFromBaseline\":"));
  Serial.print(distFromBaseline);
  Serial.print(F(",\"rgbc\":{\"r\":"));
  Serial.print(v[0]);
  Serial.print(F(",\"g\":"));
  Serial.print(v[1]);
  Serial.print(F(",\"b\":"));
  Serial.print(v[2]);
  Serial.print(F(",\"c\":"));
  Serial.print(v[3]);
  Serial.println(F("}}"));
}

void jsonCalibrationSaved() {
  Serial.println(F("{\"type\":\"calibration_saved\"}"));
}

void jsonCommand(const char* cmd) {
  Serial.print(F("{\"type\":\"command\",\"cmd\":\""));
  Serial.print(cmd);
  Serial.println(F("\"}"));
}

void jsonError(const char* msg) {
  Serial.print(F("{\"type\":\"error\",\"message\":\""));
  Serial.print(msg);
  Serial.println(F("\"}"));
}