//for INA3221 - voltage and current sensor 

//used to control battery charge and solar panels voltage
#pragma once

#include <Beastdevices_INA3221.h>
#include <pgmspace.h>

Beastdevices_INA3221 ina(INA3221_ADDR40_GND);

//Li-ion 18650 discharge curve: voltage → state-of-charge (%).
//Linear interpolation between points; clamped to 0% below 3.00V and 100% above 4.20V.
const float SOC_LUT[][2] PROGMEM = {
  {3.00, 0}, {3.10, 5}, {3.20, 10}, {3.30, 15}, {3.40, 20},
  {3.50, 30}, {3.60, 40}, {3.70, 50}, {3.80, 65}, {3.90, 75},
  {4.00, 85}, {4.10, 95}, {4.20, 100}
};
const uint8_t SOC_LUT_SIZE = sizeof(SOC_LUT) / sizeof(SOC_LUT[0]);

uint8_t estimateSoC(float voltage) {
  if (voltage <= pgm_read_float(&SOC_LUT[0][0])) return 0;
  if (voltage >= pgm_read_float(&SOC_LUT[SOC_LUT_SIZE - 1][0])) return 100;
  for (uint8_t i = 0; i < SOC_LUT_SIZE - 1; i++) {
    float v1 = pgm_read_float(&SOC_LUT[i][0]);
    float s1 = pgm_read_float(&SOC_LUT[i][1]);
    float v2 = pgm_read_float(&SOC_LUT[i + 1][0]);
    float s2 = pgm_read_float(&SOC_LUT[i + 1][1]);
    if (voltage >= v1 && voltage <= v2) {
      float frac = (voltage - v1) / (v2 - v1);
      return (uint8_t)(s1 + frac * (s2 - s1));
    }
  }
  return 0;
}

struct ina_struct {
  float batteryVoltage;
  float batteryCurrent;
  float SolarPanelVoltage;
  float leftSolarPanelCurrent;
  float rightSolarPanelCurrent;
  uint8_t battery_soc;
} ina_data;

bool initINA() {
  ina.begin();
  ina.setShuntRes(100, 100, 100);
  return true;
}

//
// CH2 = LEFT
// CH3 = RIGHT
//

ina_struct* get_ina_data() {
  ina_data.batteryVoltage = ina.getVoltage(INA3221_CH1);
  ina_data.batteryCurrent = ina.getCurrent(INA3221_CH1) * 1000;

  ina_data.SolarPanelVoltage  = (ina.getVoltage(INA3221_CH2) + ina.getVoltage(INA3221_CH3)) / 2;

  ina_data.leftSolarPanelCurrent = ina.getCurrent(INA3221_CH2) * 1000;
  ina_data.rightSolarPanelCurrent = ina.getCurrent(INA3221_CH3) * 1000;
  ina_data.battery_soc = estimateSoC(ina_data.batteryVoltage);
  return &ina_data;
}
