#pragma once

#ifndef MPU6500_H
#define MPU6500_H

#include "../fastutils.h"

#define MPU6500_CHIP_ID           0x70
#define MPU6500_REG_ADDR          0x68
#define MPU6500_REG_WHO_AM_I      0x75
#define MPU6500_REG_DATA          0x3B 
#define MPU6500_REG_CONFIG        0x1A 
#define MPU6500_REG_CONFIG_GYRO   0x1B 
#define MPU6500_REG_CONFIG_ACCEL  0x1C 
#define MPU6500_REG_CONFIG_ACCEL2 0x1D 
#define MPU6500_REG_OFFSET_ACCEL  0x77 
#define MPU6500_REG_OFFSET_GYRO   0x13 
#define MPU6500_REG_CTRL          0x6A
#define MPU6500_REG_PWR_MGMT_1    0x6B
#define MPU6500_REG_PWR_MGMT_2    0x6C

template <bool isSPI>
class MPU6500 {
  public:
    MPU6500(uint8_t accelScale = 2, uint16_t gyroScale = 250, TwoWire &wire = Wire): 
      accelScale(accelScale), gyroScale(gyroScale), bus(MPU6500_REG_ADDR, wire) {};
    MPU6500(
        uint8_t accelScale = 2, uint16_t gyroScale = 250, byte ss = SS, byte miso = MISO, SPIClass &spi = SPI
        ): accelScale(accelScale), gyroScale(gyroScale), bus(ss, miso, spi) {};

  bool begin();
  void update();

  XYZ accel, gyro, accelOffset, gyroOffset;
  uint16_t temp;

  private: 
    Bus<isSPI> bus;
    byte rawData[14];
    byte rawAccelOffset[6];
    byte rawGyroOffset[6];
    uint8_t accelScale;
    uint16_t gyroScale;

    void reset();
    void setAccelScale(uint8_t scale);
    void setGyroScale(uint16_t scale);
};

template <bool isSPI>
bool MPU6500<isSPI>::begin() {
  if(bus.read(MPU6500_REG_WHO_AM_I) != MPU6500_CHIP_ID) return false;

  reset();

  setAccelScale(accelScale);
  setGyroScale(gyroScale);

  bus.readBurst(MPU6500_REG_OFFSET_ACCEL, rawAccelOffset, 6);
  accelOffset.x = (rawData[0] << 8) | rawData[1];
  accelOffset.y = (rawData[2] << 8) | rawData[3];
  accelOffset.z = (rawData[4] << 8) | rawData[5];

  bus.readBurst(MPU6500_REG_OFFSET_GYRO, rawGyroOffset, 6);
  gyroOffset.x = (rawData[0] << 8) | rawData[1];
  gyroOffset.y = (rawData[2] << 8) | rawData[3];
  gyroOffset.z = (rawData[4] << 8) | rawData[5];

  update();

  return true;
}

template <bool isSPI>
void MPU6500<isSPI>::update() {
  bus.readBurst(MPU6500_REG_DATA, rawData, 14);
  accel.x = (rawData[0] << 8) | rawData[1];
  accel.y = (rawData[2] << 8) | rawData[3];
  accel.z = (rawData[4] << 8) | rawData[5];
  temp = (rawData[6] << 8) | rawData[7];
  gyro.x = (rawData[8] << 8) | rawData[9];
  gyro.y = (rawData[10] << 8) | rawData[11];
  gyro.z = (rawData[12] << 8) | rawData[13];
}

template <bool isSPI>
void MPU6500<isSPI>::reset() {
  bus.writeField(MPU6500_REG_PWR_MGMT_1, 1, 7, 7);
}

template <bool isSPI>
void MPU6500<isSPI>::setAccelScale(uint8_t scale) {
  byte scaleIdx;
  switch (scale) {
    case 2:
      scaleIdx = 0;
      break;
    case 4:
      scaleIdx = 1;
      break;
    case 8:
      scaleIdx = 2;
      break;
    case 16:
      scaleIdx = 3;
      break;
    default:
      return;
  }
  bus.writeField(MPU6500_REG_CONFIG_ACCEL, scaleIdx, 4, 3);
}

template <bool isSPI>
void MPU6500<isSPI>::setGyroScale(uint16_t scale) {
  byte scaleIdx;
  switch (scale) {
    case 250:
      scaleIdx = 0;
      break;
    case 500:
      scaleIdx = 1;
      break;
    case 1000:
      scaleIdx = 2;
      break;
    case 2000:
      scaleIdx = 3;
      break;
    default:
      return;
  }
  bus.writeField(MPU6500_REG_CONFIG_GYRO, scaleIdx, 4, 3);
}

#endif
