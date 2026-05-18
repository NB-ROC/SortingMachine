#include "motors.h"
#include "calibration.h"
#include "serial_json.h"
#include "pins.h"
#include "config.h"
#include <Arduino.h>

// ── Motor 2 state ─────────────────────────────────────────────
double numberOfSteps2 = 0;
int    direction2     = -1;  // stores dirLevel (HIGH/LOW), -1 = idle

// ── Mutable speed (declared here, extern'd via config.h) ──────
int RATE_STEPPER1 = 170000;

// ── Low-level step pulse (no direction control) ───────────────
void stepMotor(int stepPin, double steps, int rateUs) {
  if (steps > MAX_STEPS || steps < -MAX_STEPS) return;
  if (steps < 0) steps = -steps;

  for (int i = 0; i < (int)steps; i++) {
    digitalWrite(stepPin, HIGH); delayMicroseconds(rateUs);
    digitalWrite(stepPin, LOW);  delayMicroseconds(rateUs);
  }
}

// ── Low-level step pulse (with direction control) ─────────────
void stepMotor(int stepPin, int dirPin, double steps, int rateUs) {
  if (steps > MAX_STEPS || steps < -MAX_STEPS) return;

  if (steps < 0) {
    digitalWrite(dirPin, !digitalRead(dirPin));
    steps = -steps;
  }

  for (int i = 0; i < (int)steps; i++) {
    digitalWrite(stepPin, HIGH); delayMicroseconds(rateUs);
    digitalWrite(stepPin, LOW);  delayMicroseconds(rateUs);
  }

  // Only reset to home direction for motor 1
  if (stepPin == stepPin1) {
    digitalWrite(dirPin, INDEX_DIR);
  }
}

// ── Motor 1: advance one index position (90°) ────────────────
void index90Step1() {
  stepMotor(stepPin1, dirPin1, STEPS_PER_90, RATE_STEPPER1);
}

// ── Motor 2: move to the bin for a detected color ─────────────
void moveStep2ToColor(Color c) {
  numberOfSteps2 = 0;
  direction2     = -1;

  if (c <= UNKNOWN || c >= COLOR_COUNT) return;

  const ContainerConfig& cfg = containerMap[c];
  numberOfSteps2 = cfg.steps;
  direction2     = cfg.dirLevel;
  digitalWrite(dirPin2, cfg.dirLevel);

  stepMotor(stepPin2, numberOfSteps2, RATE_STEPPER2);
}

// ── Motor 2: return to centre ─────────────────────────────────
void returnStep2() {
  if (numberOfSteps2 <= 0 || direction2 == -1) return;

  digitalWrite(dirPin2, !direction2);
  stepMotor(stepPin2, numberOfSteps2, RATE_STEPPER2);
  numberOfSteps2 = 0;
  direction2     = -1;
}

// ── Motor 1: seek the index mark using the color sensor ───────
void getToPoint() {
  DetectionResult result;
  int loopAmount = 0;

  do {
    stepMotor(stepPin1, dirPin1, 4, RATE_STEPPER1);
    result = readStableColor();
    loopAmount++;
    if (loopAmount > 360) break;
  } while (result.c <= 32);

  int    currentPos = 0, bestPos = 0;
  int    bestC      = result.c;
  double offset     = 8;

  while (offset >= 0.5) {
    int target = bestPos + (int)offset;
    stepMotor(stepPin1, dirPin1, target - currentPos, RATE_STEPPER1);
    currentPos = target;
    DetectionResult r = readStableColor();
    if (r.c > bestC) { bestC = r.c; bestPos = currentPos; }

    target = bestPos - (int)offset;
    stepMotor(stepPin1, dirPin1, target - currentPos, RATE_STEPPER1);
    currentPos = target;
    r = readStableColor();
    if (r.c > bestC) { bestC = r.c; bestPos = currentPos; }

    offset /= 2;
  }

  delay(100);
  stepMotor(stepPin1, dirPin1, bestPos - currentPos, RATE_STEPPER1);
  jsonState("location_calibrated");
  delay(100);
}

// ── Detection confidence score (0–100) ───────────────────────
int computeConfidence(long bestDist, long separation) {
  if (bestDist > MAX_BEST_DIST || separation < MIN_SEPARATION) return 0;

  int  distScore = 100 - (int)((bestDist * 100L) / MAX_BEST_DIST);
  long sepCap    = (long)MIN_SEPARATION * 3;
  if (separation > sepCap) separation = sepCap;
  int sepScore = (int)((separation * 100L) / sepCap);

  return (distScore * 60 + sepScore * 40) / 100;
}
