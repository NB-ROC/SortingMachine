#include "serial_json.h"
#include "color_sensor.h"  // for DetectionResult definition
#include "motors.h"        // for computeConfidence
#include <Arduino.h>

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
  Serial.print(F(",\"accuracy\":"));
  Serial.print(computeConfidence(d.bestDist, d.separation));
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
