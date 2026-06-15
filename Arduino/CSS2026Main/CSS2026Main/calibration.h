#pragma once
#include "color_sensor.h"  // for Color, NUM_CHANNELS

// ── Color names (indexed by Color enum) ──────────────────────
// Defined in calibration.cpp
extern const char* colorNames[];

// ── Calibration reference values [R,G,B,C] ───────────────────
extern int baselineRef[NUM_CHANNELS];
extern int blueVal[NUM_CHANNELS];
extern int yellowVal[NUM_CHANNELS];
extern int greenVal[NUM_CHANNELS];
extern int redVal[NUM_CHANNELS];
extern int brownVal[NUM_CHANNELS];

// ── Bin position config ──────────────────────────────────────
struct ContainerConfig {
  int steps;
  int dirLevel;
};

extern ContainerConfig containerMap[COLOR_COUNT];

// ── Functions ────────────────────────────────────────────────
void calibrateManual();
void loadCalibration();
void saveCalibration();
int getColorId(const String& color);
void setContainer(int index, int colorId,  bool goesLeft);
