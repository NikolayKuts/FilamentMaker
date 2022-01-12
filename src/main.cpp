#include <Arduino.h>
#include <GyverPID.h>
#include <PIDtuner.h>
#include <PIDtuner2.h>
#include <GyverButton.h>
#include "FilamentMakerMenu.h"
#include <GyverNTC.h>

// Encoder
#define ENCODER_PIN_1 4
#define ENCODER_PIN_2 2
#define BUTTON_PIN 3
    // Motor
#define SPEED_STEP 1
#define DEFAULT_SPEED_LEVEL 60
    // Temperature
#define HEATING_PIN 9
#define TEMP_SENSOR_PIN A0
#define DEFAULT_HEATINT_TEMP 200
#define MIN_HEATING_TEMP 150
#define MAX_HEATING_TEMP 300
#define AVERAGE_TEMP_COUNTER_LIMIT 10
// #define TEMP_POINTER_LENGTH 10
    //  Termistor
#define RESISTANCE 4000
#define B_COEFFICIENT 3950 // 3950 by datasheet
#define TIME_INTERVAL 500

GButton button(BUTTON_PIN);
FilamentMakerMenu menu;
GyverNTC thermister(TEMP_SENSOR_PIN, 100000, B_COEFFICIENT, 25, RESISTANCE);
GyverPID pidObject(5.0, 0.1, 1.3, TIME_INTERVAL);

// Temperature
// int8_t averageTemperatureCounter = 0;
// int temperatureSum = 0;
float cerrentAverageTemperatur = 0;
volatile int setHeatingTemperature = DEFAULT_HEATINT_TEMP;
volatile bool isHeatingAllowed = true;
bool isHeated = true;
byte tempPinterLength = 10;
int temperaturePointers[11];
byte temperaturePointerCounter = 0;
int cerrentHetingTemperature = 0;
// Encoder
volatile boolean state0, lastState, turnFlag;
bool isButtonReleased = true;
// Motor
volatile uint8_t speedLevel = DEFAULT_SPEED_LEVEL;
// General
volatile bool isWorkAllowed = false;
long lastTime = 0;

///////////////////////// ENCODER /////////////////////////////////////
void switchHeating() { isHeatingAllowed = !isHeatingAllowed; }

void setTemperaturePointers() {
  byte delimeter;
  int startTemperaturePoint;
  
  if (setHeatingTemperature - cerrentAverageTemperatur > 150) {
    tempPinterLength = 10;
    delimeter = 9;
    startTemperaturePoint = setHeatingTemperature / delimeter * (delimeter - 2);

    int step = (setHeatingTemperature - startTemperaturePoint) / tempPinterLength;
    int temperery = startTemperaturePoint;
    for (int index = 0; index < tempPinterLength; index++) {
    temperaturePointers[index] = temperery;
    temperery += step;
    }
  } else if (setHeatingTemperature - cerrentAverageTemperatur > 80) {
    tempPinterLength = 5;
    delimeter = 7;
    startTemperaturePoint = setHeatingTemperature / delimeter * (delimeter - 1);

    int step = (setHeatingTemperature - startTemperaturePoint) / tempPinterLength;
    int temperery = startTemperaturePoint;
    for (int index = 0; index < tempPinterLength; index++) {
    temperaturePointers[index] = temperery;
    temperery += step;
    }
  } else if (setHeatingTemperature - cerrentAverageTemperatur < 50) {
    tempPinterLength = 1;
    delimeter = 10;
    temperaturePointers[0] = setHeatingTemperature;
  } else {
    tempPinterLength = 2;
    delimeter = 10;
    startTemperaturePoint = setHeatingTemperature / delimeter * (delimeter - 1);

    int step = (setHeatingTemperature - startTemperaturePoint) / tempPinterLength;
    int temperery = startTemperaturePoint;
    for (int index = 0; index < tempPinterLength; index++) {
    temperaturePointers[index] = temperery;
    temperery += step;
  }
}

//  for (int index = 0; index < TEMP_POINTER_LENGTH; index++) {
//    Serial.print("index:"); Serial.print(index); Serial.print("-->"); Serial.println(temperaturePointers[index]);
//  }
  temperaturePointerCounter = 0;
}

void switchPower() {
  isWorkAllowed = !isWorkAllowed;
  setTemperaturePointers();
}

void setTemperature() {
  if (digitalRead(ENCODER_PIN_2) != lastState) {
    setHeatingTemperature = constrain(setHeatingTemperature + 1, MIN_HEATING_TEMP, MAX_HEATING_TEMP);
  }
  else {
    setHeatingTemperature = constrain(setHeatingTemperature - 1, MIN_HEATING_TEMP, MAX_HEATING_TEMP);
  }
  setTemperaturePointers();
}

void setSpeedLevel() {
  if (digitalRead(ENCODER_PIN_2) != lastState) {
    speedLevel = constrain(speedLevel + SPEED_STEP, 0, 100);
  }
  else {
    speedLevel = constrain(speedLevel - SPEED_STEP, 0, 100);
  }
}

void setHeatingSettings() { pinMode(HEATING_PIN, OUTPUT); }

void onEncoderChanges() {
  state0 = digitalRead(ENCODER_PIN_1);
  if (state0 != lastState) {
    turnFlag = !turnFlag;
    if (turnFlag) {
      if (menu.isSettingModeTurnedOn()) {
        switch (menu.getPointerPosition()) {
        case TEMP_POSITION: setTemperature(); break;
        case HEATING_POSITION: switchHeating(); break;
        case SPEED_POSITION: setSpeedLevel(); break;
        case POWER_POSITION: switchPower(); break;
        }
      }
      else {
        if (digitalRead(ENCODER_PIN_2) != lastState) { menu.incrementPointer(); }
        else { menu.decrementPointer(); }
      }
    }
    lastState = state0;
  }
}

float getCurrentTemperature() { return thermister.getTemp(); }

void showGraph() {
  int pidValue = pidObject.output;
  Serial.print(cerrentHetingTemperature);
  Serial.print(",");
  Serial.print(cerrentAverageTemperatur);
  Serial.print(",");
  Serial.println(pidValue);
}

void manageTemperature() {
  float currentTemperature = getCurrentTemperature();
  static float kof = 0.1;
  cerrentAverageTemperatur += (currentTemperature - cerrentAverageTemperatur) * kof;

  if (isWorkAllowed && isHeatingAllowed) {

    if (millis() - lastTime > TIME_INTERVAL) {

      if (
        cerrentAverageTemperatur >= setHeatingTemperature
        || temperaturePointerCounter >= tempPinterLength
        ) {
        pidObject.setpoint = (float)setHeatingTemperature;
        cerrentHetingTemperature = setHeatingTemperature;
        temperaturePointerCounter = tempPinterLength;
//        Serial.println("OVER_COUNTER");
      }
      else if ((float)temperaturePointers[temperaturePointerCounter] < cerrentAverageTemperatur) {
        static float lastAverageTemperature = 0;
        if (lastAverageTemperature > cerrentAverageTemperatur) {
          temperaturePointerCounter++;
        }
        lastAverageTemperature = cerrentAverageTemperatur;
//        Serial.println("CHANGE_COUNTER");
      }
      else {
        // pidObject.setpoint = (float)setHeatingTemperature;
        pidObject.setpoint = (float)temperaturePointers[temperaturePointerCounter];
        cerrentHetingTemperature = temperaturePointers[temperaturePointerCounter];
        
//        Serial.print("POINTER_HEATING ------->"); Serial.println((float)temperaturePointers[temperaturePointerCounter]);
      }

      pidObject.input = cerrentAverageTemperatur;
      pidObject.getResult();
      int pidValue = pidObject.output;

      showGraph();

      analogWrite(HEATING_PIN, pidValue);
      lastTime = millis();
    }
  }
  else {
    analogWrite(HEATING_PIN, 0);
  }

}

void driveMotor(uint8_t speedLevel) {
  if (isWorkAllowed) {
    analogWrite(10, map(speedLevel, 0, 100, 0, 225));
  }
  else {
    analogWrite(10, 0);
  }
}

void setup() {
  setHeatingSettings();
  menu.initialize();
  button.setDebounce(10);
  attachInterrupt(0, onEncoderChanges, CHANGE);

  pidObject.setDirection(NORMAL);


  setTemperaturePointers();
  Serial.begin(9600);
}

void loop() {
  button.tick();
  if (button.isClick() or button.isHold()) {
    menu.switchSettingMode();
  }
  menu.showData(
    cerrentAverageTemperatur,
    setHeatingTemperature,
    isHeated,
    isHeatingAllowed,
    speedLevel,
    isWorkAllowed
  );

  manageTemperature();
  driveMotor(speedLevel);
}