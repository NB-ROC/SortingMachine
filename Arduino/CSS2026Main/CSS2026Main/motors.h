#pragma once
#include "color_sensor.h"  // for Color, DetectionResult

// ── Motor 2 state (defined in motors.cpp) ────────────────────
extern double numberOfSteps2;
extern int    direction2;  // stores dirLevel (HIGH/LOW), -1 = idle

// ── Step execution ────────────────────────────────────────────
void stepMotor(int stepPin, double steps, int rateUs);
void stepMotor(int stepPin, int dirPin, double steps, int rateUs);

// ── High-level motor commands ─────────────────────────────────
void index90Step1();
void moveStep2ToColor(Color c);
void returnStep2();
void getToPoint();

// ── Confidence scoring ────────────────────────────────────────
int computeConfidence(long bestDist, long separation);
