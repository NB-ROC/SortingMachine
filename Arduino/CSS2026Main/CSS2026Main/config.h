#pragma once
#include <Arduino.h>

// ── Motor Speeds (microseconds per step, higher = slower) ────
extern int RATE_STEPPER1;          // mutable – adjusted by set_speed command
const int  RATE_STEPPER2 = 2000;

// ── Motor Behaviour ──────────────────────────────────────────
const double STEPS_PER_90   = 50;
const int    INDEX_DIR      = LOW; // LOW = Clockwise home direction
const int    LOOP_DELAY_MS  = 300;

// ── Color Sensor ─────────────────────────────────────────────
#define NUM_CHANNELS           4
#define CALIBRATION_SAMPLES    30
#define STABLE_READS           3
#define MIN_SEPARATION         8
#define CALIBRATION_MIN_DISTANCE 10
#define MAX_BEST_DIST          150