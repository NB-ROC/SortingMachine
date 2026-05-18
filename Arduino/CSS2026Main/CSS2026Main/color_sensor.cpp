#include "color_sensor.h"
#include "serial_json.h"
#include "motors.h"   // for computeConfidence
#include "pins.h"
#include <Arduino.h>

// ── Globals ──────────────────────────────────────────────────
int rgbc[NUM_CHANNELS] = { 0, 0, 0, 0 };
bool live = false;

// ── Raw sensor read ──────────────────────────────────────────
void readRGBC() {
  digitalWrite(S2, LOW);  digitalWrite(S3, LOW);
  rgbc[0] = pulseIn(SENSOR_OUT, LOW); delay(2);  // R

  digitalWrite(S2, HIGH); digitalWrite(S3, HIGH);
  rgbc[1] = pulseIn(SENSOR_OUT, LOW); delay(2);  // G

  digitalWrite(S2, LOW);  digitalWrite(S3, HIGH);
  rgbc[2] = pulseIn(SENSOR_OUT, LOW); delay(2);  // B

  digitalWrite(S2, HIGH); digitalWrite(S3, LOW);
  rgbc[3] = pulseIn(SENSOR_OUT, LOW); delay(2);  // C

  if (live) {
    Serial.println();
    Serial.print(" R: "); Serial.print(rgbc[0]);
    Serial.print(" G: "); Serial.print(rgbc[1]);
    Serial.print(" B: "); Serial.print(rgbc[2]);
    Serial.print(" C: "); Serial.print(rgbc[3]);
    Serial.println();
    Serial.println();
  }
}

// ── Distance metric ──────────────────────────────────────────
long colorDistanceSq(const int a[NUM_CHANNELS], const int b[NUM_CHANNELS]) {
  long sum = 0;
  for (int i = 0; i < NUM_CHANNELS; i++) {
    long d = (long)a[i] - (long)b[i];
    sum += d * d;
  }
  return sum;
}

// ── Single-sample classification ─────────────────────────────
DetectionResult detectColorDetailed(const int current[NUM_CHANNELS]) {
  const int* presets[5] = { blueVal, yellowVal, greenVal, redVal, brownVal };
  Color      colors[5]  = { BLUE, YELLOW, GREEN, RED, BROWN };

  long  best = 999999, second = 999999;
  Color bestColor = UNKNOWN;

  for (int i = 0; i < 5; i++) {
    long d = colorDistanceSq(current, presets[i]);
    if (d < best)        { second = best; best = d; bestColor = colors[i]; }
    else if (d < second) { second = d; }
  }

  DetectionResult out;
  out.bestDist   = best;
  out.secondBest = second;
  out.separation = second - best;
  out.r = current[0];
  out.g = current[1];
  out.b = current[2];
  out.c = current[3];
  out.color = (out.bestDist > MAX_BEST_DIST || out.separation < MIN_SEPARATION)
              ? UNKNOWN : bestColor;
  return out;
}

// ── Stable multi-sample classification ───────────────────────
DetectionResult readStableColor() {
  int votes[COLOR_COUNT] = { 0 };
  int consecutive = 0, maxVotes = 0;
  Color last = UNKNOWN, winner = UNKNOWN;

  DetectionResult lastResult = { UNKNOWN, 999999, 999999, 0, 0, 0, 0, 0 };

  for (int i = 0; i < 20; i++) {
    readRGBC();
    DetectionResult d = detectColorDetailed(rgbc);
    lastResult = d;

    if (d.c >= 34) consecutive += 2;

    votes[d.color]++;
    if (d.color == last) consecutive++;
    else { last = d.color; consecutive = 1; }

    if (votes[d.color] > maxVotes) { maxVotes = votes[d.color]; winner = d.color; }

    jsonDetectionSample(d, i + 1, consecutive);

    if (consecutive >= STABLE_READS) {
      jsonDetectionFinal(d, true, maxVotes);
      return d;
    }
  }

  lastResult.color = (maxVotes >= STABLE_READS) ? winner : UNKNOWN;
  jsonDetectionFinal(lastResult, false, maxVotes);
  return lastResult;
}

// ── Averaged reading for calibration ─────────────────────────
void takeAverageReading(int outAvg[NUM_CHANNELS], int samples) {
  long sum[NUM_CHANNELS] = { 0, 0, 0, 0 };
  for (int i = 0; i < samples; i++) {
    readRGBC();
    for (int ch = 0; ch < NUM_CHANNELS; ch++) sum[ch] += rgbc[ch];
    delay(50);
  }
  for (int ch = 0; ch < NUM_CHANNELS; ch++) outAvg[ch] = (int)(sum[ch] / samples);
}
