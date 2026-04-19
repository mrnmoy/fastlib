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
class Bus;

template <> 
class Bus<false> {
  public:
    Bus(byte i2cAddr, TwoWire &wire = Wire):
      i2cAddr(i2cAddr),
      wire(wire) {};

    byte i2cAddr;
    TwoWire &wire;

    byte strobe(byte addr) {
      wire.beginTransmission(i2cAddr);
      byte data = wire.write(addr);
      wire.endTransmission();
      return data;
    }
    uint8_t read(byte addr) {
      wire.beginTransmission(i2cAddr);
      wire.write(addr);
      wire.endTransmission();
      wire.requestFrom(i2cAddr, 1);
      return wire.read();
    }
    uint16_t read16(byte addr) {
      wire.beginTransmission(i2cAddr);
      wire.write(addr);
      wire.endTransmission();
      wire.requestFrom(i2cAddr, 2);
      return (wire.read() << 8) | wire.read();
    }
    uint32_t read24(byte addr) {
      wire.beginTransmission(i2cAddr);
      wire.write(addr);
      wire.endTransmission();
      wire.requestFrom(i2cAddr, 3);
      return ((wire.read() << 16) | wire.read() << 8) | wire.read();
    }
    uint8_t readField(byte addr, byte hi, byte lo) {
      return read((addr) >> lo) & ((1 << (hi - lo + 1)) -1);
    }
    void readBurst(byte addr, byte *buff, uint8_t len) {
      wire.beginTransmission(i2cAddr);
      wire.write(addr);
      wire.endTransmission();
      wire.requestFrom(i2cAddr, len);
      for (uint8_t i = 0; i < len; i++) {
        buff[i] = wire.read();
      }
    }
    void write(byte addr, byte val) {
      wire.beginTransmission(i2cAddr);
      wire.write(addr);
      wire.write(val);
      wire.endTransmission();
    }
    void writeField(byte addr, byte val, byte hi, byte lo) {
      uint8_t mask = ((1 << (hi - lo + 1)) -1) << lo;
      write(addr, (read(addr) & ~mask) | ((val <<= lo) & mask));
    }
    void writeBurst(byte addr, byte *buff, uint8_t len) {}

  private:
};

template <> 
class Bus<true> {
  public:
    Bus(byte ss = SS, byte miso = MISO, SPIClass &spi = SPI, SPISettings spiSettings = SPISettings(SPI_MAX_FREQ, SPI_DATA_ORDER, SPI_DATA_MODE)): 
      ss(ss),
      miso(miso),
      spi(spi),
      spiSettings(spiSettings) {};

    byte ss, miso;
    SPIClass &spi;
    SPISettings spiSettings;

    byte strobe(byte addr) {
      spiStart();
      byte data = spi.transfer(addr);
      spiStop();
      return data;
    }
    uint8_t read(byte addr) {
      spiStart();
      spi.transfer(addr);
      uint8_t data = spi.transfer(0);
      spiStop();
      return data;
    }
    uint16_t read16(byte addr) {
      spiStart();
      spi.transfer(addr);
      uint16_t data = (spi.transfer(0) << 8) | spi.transfer(0);
      spiStop();
      return data;
    }
    uint32_t read24(byte addr) {
      spiStart();
      spi.transfer(addr);
      uint16_t data = ((spi.transfer(0) << 16) | spi.transfer(0) << 8) | spi.transfer(0);
      spiStop();
      return data;
    }
    uint8_t readField(byte addr, byte hi, byte lo) {
      return read((addr) >> lo) & ((1 << (hi - lo + 1)) -1);
    }
    void readBurst(byte addr, byte *buff, uint8_t len) {
      spiStart();
      spi.transfer(addr);
      for (uint8_t i = 0; i < len; i++) {
        buff[i] = spi.transfer(0);
      }
      spiStop();
    }
    void write(byte addr, byte val) {
      spiStart();
      spi.transfer(addr);
      spi.transfer(val);
      spiStop();
    }
    void writeField(byte addr, byte val, byte hi, byte lo) {
      uint8_t mask = ((1 << (hi - lo + 1)) -1) << lo;
      write(addr, (read(addr) & ~mask) | ((val <<= lo) & mask));
    }
    void writeBurst(byte addr, byte *buff, uint8_t len) {}

  private:
    void spiStart() {
      spi.beginTransaction(spiSettings);
      digitalWrite(ss, LOW);
      #if !defined(CONFIG_IDF_TARGET_ESP32S3) || !defined(CONFIG_IDF_TARGET_ESP32C3)
        while (digitalRead(miso));
      #endif
    }
    void spiStop() {
      digitalWrite(ss, HIGH);
      spi.endTransaction();
    }
};

#endif
