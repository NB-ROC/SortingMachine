#pragma once

// ── Stepper Motor 1 ──────────────────────────────────────────
const int stepPin1 = 6;
const int dirPin1  = 5;
const int enPin1   = 7;

// ── Stepper Motor 2 ──────────────────────────────────────────
const int stepPin2 = 3;
const int dirPin2  = 2;
const int enPin2   = 7;

// ── Color Sensor (TCS230 / TCS3200) ──────────────────────────
#define S0         8
#define S1         9
#define S2         10
#define S3         11
#define SENSOR_OUT 12

// ── Motor limits ─────────────────────────────────────────────
#define MAX_STEPS  180
