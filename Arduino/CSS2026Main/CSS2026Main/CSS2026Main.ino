#include <ArduinoJson.h>

#include "pins.h"
#include "config.h"
#include "color_sensor.h"
#include "calibration.h"
#include "motors.h"
#include "serial_json.h"

bool paused = false;

int wrongCounter = 0;


// Function to set up all the starting values and base setup
void setup() {
  Serial.begin(9600);

  pinMode(stepPin1, OUTPUT);
  pinMode(dirPin1, OUTPUT);
  pinMode(enPin1, OUTPUT);
  digitalWrite(enPin1, LOW);
  digitalWrite(dirPin1, INDEX_DIR);

  pinMode(stepPin2, OUTPUT);
  pinMode(dirPin2, OUTPUT);
  pinMode(enPin2, OUTPUT);
  digitalWrite(enPin2, LOW);

  pinMode(S0, OUTPUT);
  pinMode(S1, OUTPUT);
  pinMode(S2, OUTPUT);
  pinMode(S3, OUTPUT);
  pinMode(SENSOR_OUT, INPUT);
  digitalWrite(S0, HIGH);
  digitalWrite(S1, HIGH);

  getToPoint();
  loadCalibration();
  jsonEvent("boot");
}
// The loop to handle all logic
void loop() {
  if (Serial.available()) {
    // Receives messages from the serial monitor or the esp
    String cmd = Serial.readStringUntil('\n');  // char not string
    if (cmd.length() == 0) return;

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, cmd);

    if (err) {

      // Not JSON, treat as single-char command
      char charCMD = cmd[0];
      if (charCMD == 'C' || charCMD == 'c') {
        // Calibrates the colors
        jsonCommand("calibrate");
        calibrateManual();
      } else if (charCMD == 'V' || charCMD == 'v') {
        // Prints the calibration values
        jsonCommand("view_calibration");
        jsonCalibrationPoint("baseline", baselineRef, 0);
        jsonCalibrationPoint("blue", blueVal, colorDistanceSq(baselineRef, blueVal));
        jsonCalibrationPoint("yellow", yellowVal, colorDistanceSq(baselineRef, yellowVal));
        jsonCalibrationPoint("green", greenVal, colorDistanceSq(baselineRef, greenVal));
        jsonCalibrationPoint("red", redVal, colorDistanceSq(baselineRef, redVal));
        jsonCalibrationPoint("brown", brownVal, colorDistanceSq(baselineRef, brownVal));
      } else if (charCMD == 'L' || charCMD == 'l') {
        // Makes it so it prints the rgbc values when it's being detected
        live = !live;
      } else if (charCMD == 'P' || charCMD == 'p') {
        paused = !paused;
        //Pauses the code
      } else if (charCMD == 'R' || charCMD == 'r') {
        getToPoint();
        // Calibrates the position
      }
      return;
    }

    // Valid JSON
    String action = doc["command"].as<String>();
    String type = doc["type"].as<String>();

    //Translates the input from the esp

    Serial.println(cmd);

    if (action == "set_speed") {
      const double k = 1000.0;
      RATE_STEPPER1 = 165000 + k * (100 - doc["value"].as<int>());

      //Sets the machine's rotation speed

    } else if (action == "calibrate_wheel") {
      getToPoint();

      // Calibrates the location of the wheel
    } else if (action == "calibrate_colors") {
      // Calibrates the colors
      jsonCommand("calibrate");
      calibrateManual();
    } else if (action == "toggle_pause") {
      paused = !paused;
      //Pauses the machine or unpauses it
    } else if (type == "color_order") {

      //Attempts to change the order of the colors
      JsonObject boxes = doc["values"].as<JsonObject>();
      int index = 0;
      for (JsonPair box : boxes) {
        JsonObject boxObj = box.value().as<JsonObject>();
        bool goesLeft = boxObj["direction"].as<String>() != "right";
        JsonArray colors = boxObj["colors"].as<JsonArray>();
        for (JsonVariant color : colors) {
          int colorId = getColorId(color.as<String>());
          if (colorId != -1) {
            setContainer(index, colorId, goesLeft);
          }
        }
        index++;
      }
    }
  }

  if (!paused) {

    // The main loop that it runs. Rotate 90 degrees, scan the color, sort and repeat.

    jsonState("index_90");
    index90Step1();
    jsonState("scan");
    DetectionResult det = readStableColor();
    jsonState("sort_return");
    delay(500);
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
