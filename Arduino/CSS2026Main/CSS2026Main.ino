#include <EEPROM.h>

const int stepPin1 = 6;
const int dirPin1 = 5;
const int enPin1 = 7;

const int stepPin2 = 3;
const int dirPin2 = 2;
const int enPin2 = 7;

#define S0 8
#define S1 9
#define S2 10
#define S3 11
#define SENSOR_OUT 12

const int RATE_STEPPER1 = 170000;
const int RATE_STEPPER2 = 2000;

const double STEPS_PER_90 = 50;
const int INDEX_DIR = LOW;  // LOW=CW, HIGH=CCW (CW = Clock Wise, CCW = Counter Clock Wise)

const int LOOP_DELAY_MS = 300;

#define NUM_CHANNELS 4
#define CALIBRATION_SAMPLES 30
#define STABLE_READS 3
#define MIN_SEPARATION 8
#define CALIBRATION_MIN_DISTANCE 10
#define MAX_BEST_DIST 150

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

// [R,G,B,C]
int baselineRef[NUM_CHANNELS] = { 10, 17, 20, 15 };
int blueVal[NUM_CHANNELS] = { 14, 20, 14, 12 };
int yellowVal[NUM_CHANNELS] = { 10, 15, 18, 9 };
int greenVal[NUM_CHANNELS] = { 12, 14, 17, 11 };
int redVal[NUM_CHANNELS] = { 15, 79, 20, 13 };
int brownVal[NUM_CHANNELS] = { 12, 18, 22, 14 };

int rgbc[NUM_CHANNELS] = { 0, 0, 0, 0 };

struct ContainerConfig {
  int steps;
  int dirLevel;
};

const ContainerConfig containerMap[COLOR_COUNT] = {
  { 0, HIGH },   // UNKNOWN
  { 38, HIGH },  // BLUE
  { 20, HIGH },  // YELLOW
  { 20, LOW },   // GREEN
  { 0, HIGH },   // RED
  { 38, LOW }    // BROWN
};

struct DetectionResult {
  Color color;
  long bestDist;
  long secondBest;
  long separation;
  int r, g, b, c;
};

// stepper2 state
double numberOfSteps2 = 0;
int direction2 = 0;  // 1=CCW(HIGH), 2=CW(LOW)

void stepMotor(int stepPin, int steps, int rateUs);
void index90Step1();
void moveStep2ToColor(Color c);
void returnStep2();

void readRGBC();
long colorDistanceSq(const int a[NUM_CHANNELS], const int b[NUM_CHANNELS]);
DetectionResult detectColorDetailed(const int current[NUM_CHANNELS]);
DetectionResult readStableColor();

void loadCalibration();
void saveCalibration();
void calibrateManual();

void takeAverageReading(int outAvg[NUM_CHANNELS], int samples);

// JSON
void jsonEvent(const char* e);
void jsonState(const char* s);
void jsonDetectionSample(const DetectionResult& d, int attempt, int consecutive);
void jsonDetectionFinal(const DetectionResult& d, bool stableHit, int maxVotes);
void jsonCalibrationPoint(const char* label, const int v[NUM_CHANNELS], long dist);
void jsonCalibrationSaved();
void jsonCommand(const char* cmd);

void setup() {
  Serial.begin(9600);

  pinMode(stepPin1, OUTPUT);
  pinMode(dirPin1, OUTPUT);
  pinMode(enPin1, OUTPUT);
  digitalWrite(enPin1, LOW);
  digitalWrite(dirPin1, INDEX_DIR);

  pinMode(stepPin2, OUTPUT);
  pinMode(dirPin2, OUTPUT);
  pinMode(enPin2, OUTPUT);
  digitalWrite(enPin2, LOW);

  pinMode(S0, OUTPUT);
  pinMode(S1, OUTPUT);
  pinMode(S2, OUTPUT);
  pinMode(S3, OUTPUT);
  pinMode(SENSOR_OUT, INPUT);
  digitalWrite(S0, HIGH);
  digitalWrite(S1, HIGH);

  loadCalibration();
  jsonEvent("boot");
}

void loop() {
  if (Serial.available()) {
    char cmd = Serial.read();
    while (Serial.available()) Serial.read();

    if (cmd == 'C' || cmd == 'c') {
      jsonCommand("calibrate");
      calibrateManual();
      return;
    }

    if (cmd == 'V' || cmd == 'v') {
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

  jsonState("index_90");
  index90Step1();

  jsonState("scan");
  DetectionResult det = readStableColor();

  jsonState("sort_return");
  returnStep2();

  delay(100);

  jsonState("sort_move");
  moveStep2ToColor(det.color);

  jsonState("cycle_delay");
  delay(LOOP_DELAY_MS);
}

void stepMotor(int stepPin, double steps, int rateUs) {
  for (int i = 0; i < steps; i++) {
    digitalWrite(stepPin, HIGH);
    delayMicroseconds(rateUs);
    digitalWrite(stepPin, LOW);
    delayMicroseconds(rateUs);
  }
}

void index90Step1() {
  stepMotor(stepPin1, STEPS_PER_90, RATE_STEPPER1);
}

void moveStep2ToColor(Color c) {
  numberOfSteps2 = 0;
  direction2 = 0;

  if (c <= UNKNOWN || c >= COLOR_COUNT) return;
  // if (c == GREEN) return;

  const ContainerConfig& cfg = containerMap[c];
  numberOfSteps2 = cfg.steps;
  digitalWrite(dirPin2, cfg.dirLevel);
  direction2 = (cfg.dirLevel == HIGH) ? 1 : 2;

  stepMotor(stepPin2, numberOfSteps2, RATE_STEPPER2);
}

void returnStep2() {
  if (numberOfSteps2 <= 0) return;

  if (direction2 == 1) digitalWrite(dirPin2, LOW);
  else if (direction2 == 2) digitalWrite(dirPin2, HIGH);
  else return;

  stepMotor(stepPin2, numberOfSteps2, RATE_STEPPER2);
  numberOfSteps2 = 0;
  direction2 = 0;
}

void readRGBC() {
  digitalWrite(S2, LOW);
  digitalWrite(S3, LOW);
  rgbc[0] = pulseIn(SENSOR_OUT, LOW);
  delay(5);  // R
  digitalWrite(S2, HIGH);
  digitalWrite(S3, HIGH);
  rgbc[1] = pulseIn(SENSOR_OUT, LOW);
  delay(5);  // G
  digitalWrite(S2, LOW);
  digitalWrite(S3, HIGH);
  rgbc[2] = pulseIn(SENSOR_OUT, LOW);
  delay(5);  // B
  digitalWrite(S2, HIGH);
  digitalWrite(S3, LOW);
  rgbc[3] = pulseIn(SENSOR_OUT, LOW);
  delay(5);  // C
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
  Color colors[5] = { BLUE, YELLOW, GREEN, RED, BROWN };

  long best = 999999, second = 999999;
  Color bestColor = UNKNOWN;

  for (int i = 0; i < 5; i++) {
    long d = colorDistanceSq(current, presets[i]);
    if (d < best) {
      second = best;
      best = d;
      bestColor = colors[i];
    } else if (d < second) second = d;
  }

  DetectionResult out;
  out.bestDist = best;
  out.secondBest = second;
  out.separation = second - best;
  out.r = current[0];
  out.g = current[1];
  out.b = current[2];
  out.c = current[3];

  if (out.bestDist > MAX_BEST_DIST || out.separation < MIN_SEPARATION) {
    out.color = UNKNOWN;
  } else {
    out.color = bestColor;
  }

  return out;
}

DetectionResult readStableColor() {
  int votes[COLOR_COUNT] = { 0 };
  int consecutive = 0;
  int maxVotes = 0;
  Color last = UNKNOWN, winner = UNKNOWN;

  DetectionResult lastResult = { UNKNOWN, 999999, 999999, 0, 0, 0, 0, 0 };

  for (int i = 0; i < 12; i++) {
    readRGBC();
    DetectionResult d = detectColorDetailed(rgbc);
    lastResult = d;

    if (d.color != UNKNOWN) {
      votes[d.color]++;
      if (d.color == last) consecutive++;
      else {
        last = d.color;
        consecutive = 1;
      }

      if (votes[d.color] > maxVotes) {
        maxVotes = votes[d.color];
        winner = d.color;
      }
    } else {
      consecutive = 0;
    }

    jsonDetectionSample(d, i + 1, consecutive);

    if (consecutive >= STABLE_READS) {
      jsonDetectionFinal(d, true, maxVotes);
      return d;
    }
    delay(120);
  }

  if (maxVotes >= STABLE_READS) lastResult.color = winner;
  else lastResult.color = UNKNOWN;

  jsonDetectionFinal(lastResult, false, maxVotes);
  return lastResult;
}

void takeAverageReading(int outAvg[NUM_CHANNELS], int samples) {
  long sum[NUM_CHANNELS] = { 0, 0, 0, 0 };
  for (int i = 0; i < samples; i++) {
    readRGBC();
    for (int ch = 0; ch < NUM_CHANNELS; ch++) sum[ch] += rgbc[ch];
    delay(50);
  }
  for (int ch = 0; ch < NUM_CHANNELS; ch++) outAvg[ch] = (int)(sum[ch] / samples);
}

void calibrateManual() {
  Serial.println(F("\n======= CALIBRATION ======="));
  Serial.println(F("Keep motor 1 still. Place colors at scanner."));
  Serial.println(F("Step 1: Remove all objects (baseline)."));
  delay(3000);

  int avg[NUM_CHANNELS];
  takeAverageReading(avg, CALIBRATION_SAMPLES);
  for (int ch = 0; ch < NUM_CHANNELS; ch++) baselineRef[ch] = avg[ch];

  Serial.print(F("Baseline: "));
  Serial.print(baselineRef[0]);
  Serial.print(",");
  Serial.print(baselineRef[1]);
  Serial.print(",");
  Serial.print(baselineRef[2]);
  Serial.print(",");
  Serial.println(baselineRef[3]);

  struct CalItem {
    Color c;
    int* target;
  };
  CalItem items[] = {
    { BLUE, blueVal }, { YELLOW, yellowVal }, { GREEN, greenVal }, { RED, redVal }, { BROWN, brownVal }
  };

  for (unsigned int i = 0; i < sizeof(items) / sizeof(items[0]); i++) {
    Serial.print(F("Hold "));
    Serial.print(colorNames[items[i].c]);
    Serial.println(F(" in front of scanner (3 sec)..."));
    delay(3000);

    takeAverageReading(avg, CALIBRATION_SAMPLES);
    for (int ch = 0; ch < NUM_CHANNELS; ch++) items[i].target[ch] = avg[ch];

    long dist = colorDistanceSq(baselineRef, avg);
    Serial.print(F("Saved "));
    Serial.print(colorNames[items[i].c]);
    Serial.print(F(": "));
    Serial.print(avg[0]);
    Serial.print(",");
    Serial.print(avg[1]);
    Serial.print(",");
    Serial.print(avg[2]);
    Serial.print(",");
    Serial.print(avg[3]);
    Serial.print(F(" d="));
    Serial.println(dist);

    if (dist < CALIBRATION_MIN_DISTANCE) {
      Serial.println(F("Warning: close to baseline."));
    }
  }

  saveCalibration();
  Serial.println(F("Calibration saved."));
  jsonCalibrationSaved();
}

void loadCalibration() {
  if (EEPROM.read(0) == 0xA5) {
    int addr = 1;
    for (int i = 0; i < NUM_CHANNELS; i++) baselineRef[i] = EEPROM.read(addr++);
    for (int i = 0; i < NUM_CHANNELS; i++) blueVal[i] = EEPROM.read(addr++);
    for (int i = 0; i < NUM_CHANNELS; i++) yellowVal[i] = EEPROM.read(addr++);
    for (int i = 0; i < NUM_CHANNELS; i++) greenVal[i] = EEPROM.read(addr++);
    for (int i = 0; i < NUM_CHANNELS; i++) redVal[i] = EEPROM.read(addr++);
    for (int i = 0; i < NUM_CHANNELS; i++) brownVal[i] = EEPROM.read(addr++);
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

void jsonEvent(const char* e) {
  Serial.print(F("{\"type\":\"event\",\"event\":\""));
  Serial.print(e);
  Serial.println(F("\"}"));
}
void jsonState(const char* s) {
  Serial.print(F("{\"type\":\"state\",\"state\":\""));
  Serial.print(s);
  Serial.println(F("\"}"));
}
void jsonCommand(const char* cmd) {
  Serial.print(F("{\"type\":\"command\",\"cmd\":\""));
  Serial.print(cmd);
  Serial.println(F("\"}"));
}
void jsonCalibrationSaved() {
  Serial.println(F("{\"type\":\"calibration_saved\"}"));
}
void jsonCalibrationPoint(const char* label, const int v[NUM_CHANNELS], long dist) {
  Serial.print(F("{\"type\":\"calibration_point\",\"label\":\""));
  Serial.print(label);
  Serial.print(F("\",\"distFromBaseline\":"));
  Serial.print(dist);
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
void jsonDetectionSample(const DetectionResult& d, int attempt, int consecutive) {
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
void jsonDetectionFinal(const DetectionResult& d, bool stableHit, int maxVotes) {
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
