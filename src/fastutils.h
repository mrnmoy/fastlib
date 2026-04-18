#pragma once

#ifndef fastutils
#define fastutils

#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>

#define SPI_MAX_FREQ       6500000  
#define SPI_DATA_ORDER     MSBFIRST
#define SPI_DATA_MODE      SPI_MODE0  

struct XYZ {
  uint16_t x, y, z;
};

class Bus {
  public:
    Bus(byte i2cAddr, TwoWire &wire = Wire):
      isSPI(false),
      i2cAddr(i2cAddr),
      wire(wire),
      spi(SPI) {};
    Bus(byte ss = SS, byte miso = MISO, SPIClass &spi = SPI, SPISettings spiSettings = SPISettings(SPI_MAX_FREQ, SPI_DATA_ORDER, SPI_DATA_MODE)): 
      isSPI(true),
      wire(Wire),
      ss(ss),
      miso(miso),
      spi(spi),
      spiSettings(spiSettings) {};

    bool isSPI;
    byte i2cAddr, ss, miso;
    TwoWire &wire;
    SPIClass &spi;
    SPISettings spiSettings;

    byte strobe(byte addr);
    uint8_t read(byte addr);
    uint16_t read16(byte addr);
    uint32_t read24(byte addr);
    uint8_t readField(byte addr, byte hi, byte lo);
    void readBurst(byte addr, byte *buff, uint8_t len);
    void write(byte addr, byte val);
    void writeField(byte addr, byte val, byte hi, byte lo);
    void writeBurst(byte addr, byte *buff, uint8_t len);

  private:
    void spiStart();
    void spiStop();
};

#endif
