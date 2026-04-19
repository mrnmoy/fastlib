#pragma once

#ifndef QMC5883L_H
#define QMC5883L_H

#include "../fastutils.h"

#define QMC5883L_CHIP_ID           0xFF
#define QMC5883L_REG_CHIP_ID       0x0D
#define QMC5883L_REG_ADDR          0x1A
#define QMC5883L_REG_DATA          0x00 
#define QMC5883L_REG_TEMP          0x07 
#define QMC5883L_REG_STATUS        0x06 
#define QMC5883L_REG_CTRL          0x09 
#define QMC5883L_REG_CTRL_2        0x0A 
#define QMC5883L_REG_RESET         0x0B 

class QMC5883L {
  public:
    QMC5883L(
        byte drate = 50,
        byte scale = 2,
        uint16_t bandwidth = 512,
        TwoWire &wire = Wire
        ): 
      drate(drate), 
      scale(scale),
      bandwidth(bandwidth),
      bus(QMC5883L_REG_ADDR, wire) {};

  bool begin();
  void update();

  XYZ mag;
  uint16_t temp;

  private: 
    Bus<false> bus;
    byte rawData[6];
    byte drate, scale;
    uint16_t bandwidth;

    void reset();
    void setMode(bool isContinuous);
    void setDrate(byte drate);
    void setScale(byte scale);
    void setBandwidth(uint16_t bandwidth);
};

#endif
