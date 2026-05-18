#pragma once
#include "config.h"

// ── Color enum ───────────────────────────────────────────────
enum Color {
  UNKNOWN,
  BLUE,
  YELLOW,
  GREEN,
  RED,
  BROWN,
  COLOR_COUNT
};

extern const char* colorNames[];

// ── Calibration data (defined in calibration.cpp) ────────────
extern int baselineRef[NUM_CHANNELS];
extern int blueVal[NUM_CHANNELS];
extern int yellowVal[NUM_CHANNELS];
extern int greenVal[NUM_CHANNELS];
extern int redVal[NUM_CHANNELS];
extern int brownVal[NUM_CHANNELS];
extern int rgbc[NUM_CHANNELS];

extern bool live;  // toggles raw RGBC serial output

// ── Detection result ─────────────────────────────────────────
struct DetectionResult {
  Color color;
  long  bestDist;
  long  secondBest;
  long  separation;
  int   r, g, b, c;
};

// ── Functions ────────────────────────────────────────────────
void            readRGBC();
long            colorDistanceSq(const int a[NUM_CHANNELS], const int b[NUM_CHANNELS]);
DetectionResult detectColorDetailed(const int current[NUM_CHANNELS]);
DetectionResult readStableColor();
void            takeAverageReading(int outAvg[NUM_CHANNELS], int samples);
