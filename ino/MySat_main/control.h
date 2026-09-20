//for communication with Nano (ATmega328p) microcontroller - MySat auxiliary microcontroller
//and for controlling the STAR LED - MySat debug indicator

//Nano (ATmega328p) is used for the operation of subsystems - control of the MySat solar panels movement and other tasks
#pragma once
#include <EEPROM.h>
#include <Adafruit_NeoPixel.h>
#include "sensors_data.h"      //for init_status, pointer_of_sensors
#include "position_sensor.h"   //for calibration struct
extern bool debug_mode_active;

#define SIGNALLED_BRIGHTNESS 20
#define STARLED_BRIGHTNESS 65
#define EEPROM_ADDR_STATE_MOTOR 0

bool stateMotor;
unsigned long lastMotorChange = 0;

const int LED = 14;  //MySat STAR LED
const int SIGNAL_LED = 2;  //MySat SIGNAL LED
const int NUM_LEDS = 1;

Adafruit_NeoPixel signalStrip(NUM_LEDS, SIGNAL_LED, NEO_GRB + NEO_KHZ800);

enum LedMode { LED_OFF,
               LED_SOLID,
               LED_BLINK };

struct SignalLedState {
  uint8_t r, g, b;
  LedMode mode;
  uint8_t brightness;
  uint16_t onInterval;
  uint16_t offInterval;
  unsigned long lastUpdate;
  bool state;
};

SignalLedState signalLed;

void initSignalLed() {   //initializes the SIGNAL LED; used in setup()
  signalStrip.begin();
  signalStrip.setBrightness(SIGNALLED_BRIGHTNESS);
  signalStrip.show();
  signalLed = { 0, 0, 0, LED_OFF, SIGNALLED_BRIGHTNESS, 500, 500, millis(), false };
}

void setSignalLed(uint8_t r, uint8_t g, uint8_t b, LedMode mode,        //configures the SIGNAL LED settings
                   uint8_t brightness = SIGNALLED_BRIGHTNESS, uint16_t onInterval = 500, uint16_t offInterval = 0) {
  uint16_t off = (offInterval == 0) ? onInterval : offInterval;
  //Idempotent: if parameters are unchanged, preserve blink phase — don't reset lastUpdate/state.
  //This lets updateSignalLed() (called every loop iteration) drive the blink independently
  //of the checkSystemState() throttle interval (500ms). Without this, repeated calls with
  //the same parameters reset the blink phase on every checkSystemState cycle, so blink
  //patterns with onInterval >= 500ms never complete a blink cycle (LED stays solid).
  if (r == signalLed.r && g == signalLed.g && b == signalLed.b &&
      mode == signalLed.mode && brightness == signalLed.brightness &&
      onInterval == signalLed.onInterval && off == signalLed.offInterval) {
    return;
  }
  signalLed.r = r;
  signalLed.g = g;
  signalLed.b = b;
  signalLed.mode = mode;
  signalLed.brightness = brightness;
  signalLed.onInterval = onInterval;
  signalLed.offInterval = off;
  signalLed.lastUpdate = millis();
  signalLed.state = true;  //used within LED_BLINK mode to create the blinking effect
  signalStrip.setBrightness(brightness);
}

void updateSignalLed() {      //tracks which LED mode should be used; 
  unsigned long now = millis();
  switch (signalLed.mode) {
    case LED_OFF:
      signalStrip.clear();
      break;
    case LED_SOLID:
      signalStrip.setPixelColor(0, signalStrip.Color(signalLed.r, signalLed.g, signalLed.b));
      break;
    case LED_BLINK:
      uint16_t currentInterval = signalLed.state ? signalLed.onInterval : signalLed.offInterval;
      if (now - signalLed.lastUpdate > currentInterval) {
        signalLed.state = !signalLed.state;
        signalLed.lastUpdate = now;
      }
      signalStrip.setPixelColor(0, signalLed.state ? signalStrip.Color(signalLed.r, signalLed.g, signalLed.b) : 0);
      break;
  }
  signalStrip.show();
}

void evaluateSystemState(pointer_of_sensors* data) {     //monitors the system state and triggers appropriate LED indication; 
  //Priority order, first match wins: all-missing > WiFi > low battery > BME > MPU/cal > INA > RTC > ADS > debug > all OK
  if (!init_status.bme_ && !init_status.mpu_ && !init_status.ads_
      && !init_status.ina_ && !init_status.rtc_) {
    setSignalLed(255, 0, 0, LED_BLINK, SIGNALLED_BRIGHTNESS, 200, 200);
    return;
  }

  if (WiFi.status() != WL_CONNECTED) {
    setSignalLed(0, 0, 255, LED_BLINK, SIGNALLED_BRIGHTNESS, 800, 200);
    return;
  }

  if (init_status.ina_ && data && data->ina_
      && data->ina_->batteryVoltage < 3.3) {
    setSignalLed(255, 0, 0, LED_SOLID);
    return;
  }

  if (!init_status.bme_) {
    setSignalLed(255, 255, 0, LED_BLINK, SIGNALLED_BRIGHTNESS, 500, 500);
    return;
  }

  if (!init_status.mpu_ || !calibration.valid) {
    setSignalLed(255, 255, 0, LED_SOLID);
    return;
  }

  if (!init_status.ina_) {
    setSignalLed(255, 255, 0, LED_BLINK, SIGNALLED_BRIGHTNESS, 200, 200);
    return;
  }

  if (!init_status.rtc_) {
    setSignalLed(255, 255, 0, LED_BLINK, SIGNALLED_BRIGHTNESS, 1000, 1000);
    return;
  }

  if (!init_status.ads_) {
    setSignalLed(0, 255, 255, LED_BLINK, SIGNALLED_BRIGHTNESS, 500, 500);
    return;
  }

  if (debug_mode_active) {
    setSignalLed(180, 0, 255, LED_SOLID);
    return;
  }

  setSignalLed(0, 0, 255, LED_SOLID);
}

unsigned long lastSystemCheck = 0;

void checkSystemState(pointer_of_sensors* data){
  if(millis() - lastSystemCheck > 500){
    evaluateSystemState(data);
    lastSystemCheck = millis();
  }
  updateSignalLed();
}

void control_light(bool state_light) {  //turns the STAR LED on or off based on the input parameter (true/false)
  if (state_light) {
    ledcWrite(LED, STARLED_BRIGHTNESS);
  } else {
    ledcWrite(LED, 0);
  }
}

void initStarLed() {  //initializes the STAR LED; used in setup()
  ledcAttach(LED, 5000, 8);
  control_light(false);
}

struct BlinkLedState{
  bool active;
  unsigned long lastBlink;
  uint8_t step;
  bool returnState;
};

BlinkLedState blinkLedState = {false, 0, 0, false};

void startBlink(bool current_state_light){
  blinkLedState.active = true;
  blinkLedState.lastBlink = millis();
  blinkLedState.step = 0;
  blinkLedState.returnState = current_state_light;
}

void updateBlinkStarLed(){
  if(!blinkLedState.active) return;

  unsigned long now = millis();

  if(now - blinkLedState.lastBlink < 300) return;

  switch(blinkLedState.step){
    case 0: control_light(false); break; 
    case 1: control_light(true); break;  
    case 2: control_light(false); break;  
    case 3: control_light(true); break;   
    case 4: control_light(false); break;  
    case 5:
      control_light(blinkLedState.returnState);  
      blinkLedState.active = false;
      return;
  }

  blinkLedState.step++;
  blinkLedState.lastBlink = now;
}

void control_motor(bool state_motor) {  //deploys or retracts the solar panels depending on the input parameter (true/false)
  if (state_motor) {
    Wire.beginTransmission(8);
    Wire.write(byte(0));
    Wire.endTransmission();
  } else {
    Wire.beginTransmission(8);
    Wire.write(byte(1));
    Wire.endTransmission();
  }
}

void loadStateMotor() {
  EEPROM.begin(4);  
  byte val = EEPROM.read(EEPROM_ADDR_STATE_MOTOR);
  stateMotor = (val == 1);
}

void saveStateMotor() {
  EEPROM.write(EEPROM_ADDR_STATE_MOTOR, stateMotor ? 1 : 0);
  EEPROM.commit(); 
}

bool setStateMotor(bool newState) {
  unsigned long now = millis();
  if (now - lastMotorChange < 2200) return false;  
  if (stateMotor != newState) {
    stateMotor = newState;
    saveStateMotor();
    control_motor(stateMotor);
    lastMotorChange = now;
    return true;
  }
  return false;
}

void setRadio(){
  Wire.beginTransmission(8);
  Wire.write(byte(3));
  Wire.endTransmission();
}