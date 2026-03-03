#pragma once

#ifndef BMP180_H
#define BMP180_H

#include "../fastutils.h"

#define BMP180_REG_ADDR 0x77
#define BMP180_CHIP_ID  0x55

#define BMP180_REG_CHIP_ID     0xD0
#define BMP180_REG_MEAS        0xF6

#define BMP180_REG_TEMPERATURE 0x2E
#define BMP180_REG_OSS_0       0x34
#define BMP180_REG_OSS_1       0x74
#define BMP180_REG_OSS_2       0xB4
#define BMP180_REG_OSS_3       0xF4

#define BMP180_REG_CAL_AC1     0xAA 
#define BMP180_REG_CAL_AC2     0xAC 
#define BMP180_REG_CAL_AC3     0xAE 
#define BMP180_REG_CAL_AC4     0xB0 
#define BMP180_REG_CAL_AC5     0xB2 
#define BMP180_REG_CAL_AC6     0xB4 
#define BMP180_REG_CAL_B1      0xB6 
#define BMP180_REG_CAL_B2      0xB8 
#define BMP180_REG_CAL_MB      0xBA 
#define BMP180_REG_CAL_MC      0xBC 
#define BMP180_REG_CAL_MD      0xBE

struct CalParams {
  int16_t ac1, ac2, ac3;
  uint16_t ac4, ac5, ac6;
  int16_t b1, b2, mb, mc, md;
};

class BMP180 {
  public:
    BMP180(uint8_t oss = 0, float seaLevelPress = 101325.0, TwoWire &wire = Wire): 
      oss(oss), 
      slp(seaLevelPress * 100.0),
      bus(BMP180_REG_ADDR, wire) {};

  float temp, press, alt;

  bool begin();
  void update();

  private: 
    Bus bus;
    const uint8_t oss;
    const float slp;

    CalParams getCP(); 
    int32_t getUT();
    int32_t getUP();
    int32_t getB5(const CalParams cal);
    float getTemp(int32_t b5); 
    float getPress(int32_t b5, const CalParams cal); 
    float getAlt(int32_t p, const float slp); 
};

#endif
