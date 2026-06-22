#include "Stream.h"
#include "calibration.h"
#include "color_sensor.h"
#include "serial_json.h"
#include <EEPROM.h>
#include <Arduino.h>
#include "motors.h"

// ── Color name strings ────────────────────────────────────────
const char* colorNames[] = {
  "UNKNOWN", "BLUE", "YELLOW", "GREEN", "RED", "BROWN"
};

// ── Default calibration values [R,G,B,C] ─────────────────────
int baselineRef[NUM_CHANNELS] = { 10, 17, 20, 15 };
int blueVal[NUM_CHANNELS] = { 14, 20, 14, 12 };
int yellowVal[NUM_CHANNELS] = { 10, 15, 18, 9 };
int greenVal[NUM_CHANNELS] = { 12, 14, 17, 11 };
int redVal[NUM_CHANNELS] = { 15, 79, 20, 13 };
int brownVal[NUM_CHANNELS] = { 12, 18, 22, 14 };

// ── Bin position map ─────────────────────────────────────────
ContainerConfig containerMap[COLOR_COUNT] = {
  { 0, HIGH },   // UNKNOWN
  { 38, HIGH },  // BLUE
  { 20, HIGH },  // YELLOW
  { 20, LOW },   // GREEN
  { 0, HIGH },   // RED
  { 38, LOW }    // BROWN
};

void setContainer(int index, int colorId, bool goesLeft){
  if (colorId < 0 || colorId >= COLOR_COUNT) return;
  int degrees = index * 19;
  containerMap[colorId] = { degrees, goesLeft ? HIGH : LOW };
}
int getColorId(const String& color) {
  for (int i = 0; i < COLOR_COUNT; i++) {
    if (color.equalsIgnoreCase(colorNames[i]))
      return i;
  }
  return -1;
}

// ── Interactive serial calibration ───────────────────────────
void calibrateManual() {
  Serial.println(F("\n======= CALIBRATION ======="));
  Serial.println(F("Keep motor 1 still. Place colors at scanner."));
  Serial.println(F("Step 1: Remove all objects (baseline)."));
  delay(3000);

  int avg[NUM_CHANNELS];
  takeAverageReading(avg, CALIBRATION_SAMPLES);
  for (int ch = 0; ch < NUM_CHANNELS; ch++) baselineRef[ch] = avg[ch];

  index90Step1();

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
    { BLUE, blueVal },
    { YELLOW, yellowVal },
    { GREEN, greenVal },
    { RED, redVal },
    { BROWN, brownVal }
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

    if (dist < CALIBRATION_MIN_DISTANCE)
      Serial.println(F("Warning: close to baseline."));
    index90Step1();
  }

  saveCalibration();
  Serial.println(F("Calibration saved."));
  jsonCalibrationSaved();
}

// ── EEPROM persistence ────────────────────────────────────────
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
