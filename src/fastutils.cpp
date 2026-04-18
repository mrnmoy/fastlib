#include "fastutils.h"

#if isSPI
  #if defined(CONFIG_IDF_TARGET_ESP32S3) || defined(CONFIG_IDF_TARGET_ESP32C3)
    void Bus::spiStart() {
      spi.beginTransaction(spiSettings);
      digitalWrite(ss, LOW);
    }
  #else
    void Bus::spiStart() {
      spi.beginTransaction(spiSettings);
      digitalWrite(ss, LOW);
      while (digitalRead(miso));
    }
  #endif
  void Bus::spiStop() {
    digitalWrite(ss, HIGH);
    spi.endTransaction();
  }

  byte Bus::strobe(byte addr) {
    spiStart();
    byte data = spi.transfer(addr);
    spiStop();
    return data;
  }
  uint8_t Bus::read(byte addr) {
    Serial.println("Bus::read spi");
    spiStart();
    spi.transfer(addr);
    uint8_t data = spi.transfer(0);
    spiStop();
    return data;
  }
  uint16_t Bus::read16(byte addr) {
    spiStart();
    spi.transfer(addr);
    uint16_t data = (spi.transfer(0) << 8) | spi.transfer(0);
    spiStop();
    return data;
  }
  uint32_t Bus::read24(byte addr) {
    spiStart();
    spi.transfer(addr);
    uint16_t data = ((spi.transfer(0) << 16) | spi.transfer(0) << 8) | spi.transfer(0);
    spiStop();
    return data;
  }
  void Bus::readBurst(byte addr, byte *buff, uint8_t len) {
    spiStart();
    spi.transfer(addr);
    for (uint8_t i = 0; i < len; i++) {
      buff[i] = spi.transfer(0);
    }
    spiStop();
  }
  void Bus::write(byte addr, byte val) {
    spiStart();
    spi.transfer(addr);
    spi.transfer(val);
    spiStop();
  }
  void Bus::writeBurst(byte addr, byte *buff, uint8_t len) {}
#else
  byte Bus::strobe(byte addr) {
    wire.beginTransmission(i2cAddr);
    byte data = wire.write(addr);
    wire.endTransmission();
    return data;
  }
  uint8_t Bus::read(byte addr) {
    Serial.println("Bus::read i2c");
    Serial.printf("isSPI: %d, miso: %d, ss: %d", isSPI, miso, ss);
    wire.beginTransmission(i2cAddr);
    wire.write(addr);
    wire.endTransmission();
    wire.requestFrom(i2cAddr, 1);
    return wire.read();
  }
  uint16_t Bus::read16(byte addr) {
    wire.beginTransmission(i2cAddr);
    wire.write(addr);
    wire.endTransmission();
    wire.requestFrom(i2cAddr, 2);
    return (wire.read() << 8) | wire.read();
  }
  uint32_t Bus::read24(byte addr) {
    wire.beginTransmission(i2cAddr);
    wire.write(addr);
    wire.endTransmission();
    wire.requestFrom(i2cAddr, 3);
    return ((wire.read() << 16) | wire.read() << 8) | wire.read();
  }
  void Bus::readBurst(byte addr, byte *buff, uint8_t len) {
    wire.beginTransmission(i2cAddr);
    wire.write(addr);
    wire.endTransmission();
    wire.requestFrom(i2cAddr, len);
    for (uint8_t i = 0; i < len; i++) {
      buff[i] = wire.read();
    }
  }
  void Bus::write(byte addr, byte val) {
    wire.beginTransmission(i2cAddr);
    wire.write(addr);
    wire.write(val);
    wire.endTransmission();
  }
  void Bus::writeBurst(byte addr, byte *buff, uint8_t len) {}
#endif

uint8_t Bus::readField(byte addr, byte hi, byte lo) {
  return read((addr) >> lo) & ((1 << (hi - lo + 1)) -1);
}
void Bus::writeField(byte addr, byte val, byte hi, byte lo) {
  uint8_t mask = ((1 << (hi - lo + 1)) -1) << lo;
  write(addr, (read(addr) & ~mask) | ((val <<= lo) & mask));
}
