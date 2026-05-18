#pragma once
#include "config.h"

// Forward-declare DetectionResult so this header stays self-contained
struct DetectionResult;

void jsonEvent(const char* e);
void jsonState(const char* s);
void jsonCommand(const char* cmd);
void jsonCalibrationSaved();
void jsonCalibrationPoint(const char* label, const int v[NUM_CHANNELS], long dist);
void jsonDetectionSample(const DetectionResult& d, int attempt, int consecutive);
void jsonDetectionFinal(const DetectionResult& d, bool stableHit, int maxVotes);
