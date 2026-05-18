#include <ArduinoJson.h>

#include "pins.h"
#include "config.h"
#include "color_sensor.h"
#include "calibration.h"
#include "motors.h"
#include "serial_json.h"

bool paused  = false;

int wrongCounter = 0;


// ── Arduino entry points ──────────────────────────────────────
void setup() {
  Serial.begin(9600);

  pinMode(stepPin1, OUTPUT); pinMode(dirPin1, OUTPUT); pinMode(enPin1, OUTPUT);
  digitalWrite(enPin1, LOW); digitalWrite(dirPin1, INDEX_DIR);

  pinMode(stepPin2, OUTPUT); pinMode(dirPin2, OUTPUT); pinMode(enPin2, OUTPUT);
  digitalWrite(enPin2, LOW);

  pinMode(S0, OUTPUT); pinMode(S1, OUTPUT);
  pinMode(S2, OUTPUT); pinMode(S3, OUTPUT);
  pinMode(SENSOR_OUT, INPUT);
  digitalWrite(S0, HIGH); digitalWrite(S1, HIGH);

  getToPoint();
  loadCalibration();
  jsonEvent("boot");
}

void loop() {
  if (Serial.available()) {
    String cmd = Serial.readStringUntil("\n");
    if (cmd.length() == 0) return;

    Serial.println(cmd);

    JsonDocument doc;
    deserializeJson(doc, cmd);

    String action = doc["action"];
    String value  = doc["value"];

    if (action == "set_speed") {
      const double k = 1000.0;
      RATE_STEPPER1 = 165000 + k * (100 - value.toInt());
      return;
    } else if (action == "calibrate_wheel") {
      getToPoint(); return;
    } else if (action == "calibrate_colors") {
      jsonCommand("calibrate"); calibrateManual(); return;
    } else if (action == "toggle_pause") {
      paused = !paused; return;
    }

    char charCMD = cmd[0];

    if (charCMD == 'C' || charCMD == 'c') { jsonCommand("calibrate"); calibrateManual(); return; }
    if (charCMD == 'V' || charCMD == 'v') {
      jsonCommand("view_calibration");
      jsonCalibrationPoint("baseline", baselineRef, 0);
      jsonCalibrationPoint("blue",     blueVal,     colorDistanceSq(baselineRef, blueVal));
      jsonCalibrationPoint("yellow",   yellowVal,   colorDistanceSq(baselineRef, yellowVal));
      jsonCalibrationPoint("green",    greenVal,    colorDistanceSq(baselineRef, greenVal));
      jsonCalibrationPoint("red",      redVal,      colorDistanceSq(baselineRef, redVal));
      jsonCalibrationPoint("brown",    brownVal,    colorDistanceSq(baselineRef, brownVal));
      return;
    }
    if (charCMD == 'L' || charCMD == 'l') { live    = !live;    return; }
    if (charCMD == 'P' || charCMD == 'p') { paused  = !paused;  return; }
    if (charCMD == 'R' || charCMD == 'r') { getToPoint();        return; }
  }

  if (!paused) {
    jsonState("index_90");
    index90Step1();

    jsonState("scan");
    DetectionResult det = readStableColor();

    jsonState("sort_return");
    returnStep2();

    if (det.color != UNKNOWN) {
      delay(500);
      jsonState("sort_move");
      moveStep2ToColor(det.color);
    }

    jsonState("cycle_delay");
    delay(LOOP_DELAY_MS);
  }
}
