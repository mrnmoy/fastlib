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

template <bool isSPI> 
class Bus {
  public:
    Bus(byte i2cAddr, TwoWire &wire = Wire):
      i2cAddr(i2cAddr),
      wire(wire),
      spi(SPI) {};
    Bus(byte ss = SS, byte miso = MISO, SPIClass &spi = SPI, SPISettings spiSettings = SPISettings(SPI_MAX_FREQ, SPI_DATA_ORDER, SPI_DATA_MODE)): 
      wire(Wire),
      ss(ss),
      miso(miso),
      spi(spi),
      spiSettings(spiSettings) {};

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

template <bool isSPI> 
void Bus<isSPI>::spiStart() {
#if defined(CONFIG_IDF_TARGET_ESP32S3) || defined(CONFIG_IDF_TARGET_ESP32C3)
  spi.beginTransaction(spiSettings);
  digitalWrite(ss, LOW);
#else
  spi.beginTransaction(spiSettings);
  digitalWrite(ss, LOW);
  while (digitalRead(miso));
#endif
}

template <bool isSPI> 
void Bus<isSPI>::spiStop() {
  digitalWrite(ss, HIGH);
  spi.endTransaction();
}

template <bool isSPI> 
byte Bus<isSPI>::strobe(byte addr) {
  if constexpr (isSPI) {
    spiStart();
    byte data = spi.transfer(addr);
    spiStop();
    return data;
  } else {
    wire.beginTransmission(i2cAddr);
    byte data = wire.write(addr);
    wire.endTransmission();
    return data;
  }
}

template <bool isSPI> 
uint8_t Bus<isSPI>::read(byte addr) {
  if constexpr (isSPI) {
    spiStart();
    spi.transfer(addr);
    uint8_t data = spi.transfer(0);
    spiStop();
    return data;
  } else {
    wire.beginTransmission(i2cAddr);
    wire.write(addr);
    wire.endTransmission();
    wire.requestFrom(i2cAddr, 1);
    return wire.read();
  }
}

template <bool isSPI> 
uint16_t Bus<isSPI>::read16(byte addr) {
  if constexpr (isSPI) {
    spiStart();
    spi.transfer(addr);
    uint16_t data = (spi.transfer(0) << 8) | spi.transfer(0);
    spiStop();
    return data;
  } else {
    wire.beginTransmission(i2cAddr);
    wire.write(addr);
    wire.endTransmission();
    wire.requestFrom(i2cAddr, 2);
    return (wire.read() << 8) | wire.read();
  }
}

template <bool isSPI> 
uint32_t Bus<isSPI>::read24(byte addr) {
  if constexpr (isSPI) {
    spiStart();
    spi.transfer(addr);
    uint16_t data = ((spi.transfer(0) << 16) | spi.transfer(0) << 8) | spi.transfer(0);
    spiStop();
    return data;
  } else {
    wire.beginTransmission(i2cAddr);
    wire.write(addr);
    wire.endTransmission();
    wire.requestFrom(i2cAddr, 3);
    return ((wire.read() << 16) | wire.read() << 8) | wire.read();
  }
}

template <bool isSPI> 
void Bus<isSPI>::readBurst(byte addr, byte *buff, uint8_t len) {
  if constexpr (isSPI) {
    spiStart();
    spi.transfer(addr);
    for (uint8_t i = 0; i < len; i++) {
      buff[i] = spi.transfer(0);
    }
    spiStop();
  } else {
    wire.beginTransmission(i2cAddr);
    wire.write(addr);
    wire.endTransmission();
    wire.requestFrom(i2cAddr, len);
    for (uint8_t i = 0; i < len; i++) {
      buff[i] = wire.read();
    }
  }
}

template <bool isSPI> 
void Bus<isSPI>::write(byte addr, byte val) {
  if constexpr (isSPI) {
    spiStart();
    spi.transfer(addr);
    spi.transfer(val);
    spiStop();
  } else {
    wire.beginTransmission(i2cAddr);
    wire.write(addr);
    wire.write(val);
    wire.endTransmission();
  }
}

template <bool isSPI> 
void Bus<isSPI>::writeBurst(byte addr, byte *buff, uint8_t len) {
  if constexpr (isSPI) {
  } else {
  }
}

template <bool isSPI> 
uint8_t Bus<isSPI>::readField(byte addr, byte hi, byte lo) {
  return read((addr) >> lo) & ((1 << (hi - lo + 1)) -1);
}

template <bool isSPI> 
void Bus<isSPI>::writeField(byte addr, byte val, byte hi, byte lo) {
  uint8_t mask = ((1 << (hi - lo + 1)) -1) << lo;
  write(addr, (read(addr) & ~mask) | ((val <<= lo) & mask));
}

#endif
