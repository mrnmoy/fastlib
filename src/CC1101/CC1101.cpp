#include "CC1101.h"

bool CC1101::begin() { 
  reset();
  delayMicroseconds(50);

  if(!getChipInfo() || 
      !getFreqBand(freq, freqTable) ||
      !(drate > drateTable[mod][0] && drate < drateTable[mod][1])) 
    return false;

  setMod(mod);
  setFreq(freq);
  setDrate(drate);
  setPwr(freqBand, pwr, powerTable);

  setAddr(addr);
  setCRC(isCRC);
  setFEC(isFEC);
  setAutoCalib(isAutoCalib);
  setManchester(isManchester);
  setAppendStatus(isAppendStatus);
  setDataWhitening(isDataWhitening);
  setPktLen(pktLen);
  setPktLenMode(isVariablePktLen);
  setSync(syncMode, syncWord, preambleLen);

  return true;
}

bool CC1101::read(uint8_t *buff, size_t timeoutMs){
  setIdleState();
  flushRxBuff();
  setRxState();
  if(!waitForRxBytes(pktLen, timeoutMs)) return false; //timeout
  readRxFifo(buff, pktLen);
  waitForState();
  return true;
};
bool CC1101::read(uint8_t *buff, uint8_t len, size_t timeoutMs){
  setPktLen(len);
  setIdleState();
  flushRxBuff();
  setRxState();
  if(!waitForRxBytes(len, timeoutMs)) return false; //timeout
  readRxFifo(buff, len);
  waitForState();
  return true;
};
bool CC1101::write(uint8_t *buff){
  setIdleState();
  flushTxBuff();
  setTxState();
  writeTxFifo(buff, pktLen);
  waitForState();
  return true;
};
bool CC1101::write(uint8_t *buff, uint8_t len){
  setPktLen(len);
  setIdleState();
  flushTxBuff();
  setTxState();
  writeTxFifo(buff, len);
  waitForState();
  return true;
};
bool CC1101::link(uint8_t *txBuff, uint8_t *rxBuff, size_t timeoutMs) {
  uint32_t lastMillis = millis();
  setIdleState();
  flushTxBuff();
  setTxState();
  writeTxFifo(txBuff, pktLen);
  waitForState();
  flushRxBuff();
  setRxState();
  while (!enoughRxBytes(pktLen)) {
    if (millis() - lastMillis > timeoutMs) {
      return false; // timeout
    }
    delay(1); // avoid watchdog
  }
  readRxFifo(rxBuff, pktLen);
  waitForState();
  return true;
};
void CC1101::link2(uint8_t *txBuff, uint8_t *rxBuff, size_t timeoutMs) {
  uint32_t lastMillis;
  setIdleState();
  setTwoWay();
  setTxState();
  while(true) {
    flushTxBuff();
    writeTxFifo(txBuff, pktLen);
    waitForState(STATE_RX);
    Serial.println("Sent packet.");
    flushRxBuff();
    lastMillis = millis();
    while (true) { /* state goes to tx even when fifo is empty */
      Serial.print("state: ");
      Serial.println(getState());
      if (bus.readField(CC1101_REG_RXBYTES | CC1101_READ_BURST, 6, 0) != 0) {
        Serial.println("rxbytes > 0");
        readRxFifo(rxBuff, pktLen);
        waitForState(STATE_TX);
        Serial.println("Received packet.");
        break;
      // } else if (millis() - lastMillis > timeoutMs) {
      //   setIdleState();
      //   setTxState();
      //   Serial.println("timeout");
      //   break;
      // } else {
        delay(500);
      }
    }
  }
};

void CC1101::reset() {
  digitalWrite(ss, HIGH);
  delayMicroseconds(5);
  digitalWrite(ss, LOW);
  delayMicroseconds(5);
  digitalWrite(ss, HIGH);
  delayMicroseconds(40);

  bus.strobe(CC1101_REG_RES);
}
void CC1101::flushRxBuff(){
  if(getState() != (STATE_IDLE || STATE_RXFIFO_OVERFLOW)) return;
  bus.strobe(CC1101_REG_FRX);
  delayMicroseconds(50);
};
void CC1101::flushTxBuff(){
  if(getState() != (STATE_IDLE || STATE_TXFIFO_UNDERFLOW)) return;
  bus.strobe(CC1101_REG_FTX);
  delayMicroseconds(50);
};
byte CC1101::readStatus(byte addr){
  return bus.read(addr | CC1101_READ_BURST);
};
void CC1101::waitForState(State state) {
  while (getState() != state) delayMicroseconds(50);
};

byte CC1101::getState() {
  return (bus.strobe(CC1101_REG_NOP) >> 4) & 0b00111;
};
bool CC1101::getChipInfo() {
  partnum = readStatus(CC1101_REG_PARTNUM);
  version = readStatus(CC1101_REG_VERSION);

  if(partnum == CC1101_PARTNUM && version == CC1101_VERSION) return true;
  return false;
};
bool CC1101::getFreqBand(double freq, const double freqTable[][2]) {
  for(int i = 0; i < 4; i++) {
    if(freq >= freqTable[i][0] && freq <= freqTable[i][1]) {
      freqBand = (CC1101_FreqBand)i;
      return true;
    }
  }
  return false;
};
uint8_t CC1101::getPreambleIdx(uint8_t len) {
  switch (len) {
    case 16:
      return 0;
    break;
    case 24:
      return 1;
    break;
    case 32:
      return 2;
    break;
    case 48:
      return 3;
    break;
    case 64:
      return 4;
    break;
    case 96:
      return 5;
    break;
    case 128:
      return 6;
    break;
    case 192:
      return 7;
    break;
    default:
      return 0;;
  }
};

void CC1101::setCRC(bool en) {
  bus.writeField(CC1101_REG_PKTCTRL0, (byte)en, 2, 2); /* CRC_EN */
  bus.writeField(CC1101_REG_PKTCTRL1, (byte)en, 3, 3); /* Autoflush */
};
void CC1101::setFEC(bool en) {
  if(isVariablePktLen) return;

  bus.writeField(CC1101_REG_MDMCFG1, (byte)en, 7, 7);
};
void CC1101::setAddr(byte addr) {
  bus.writeField(CC1101_REG_PKTCTRL1, addr > 0 ? 1 : 0, 1, 0);
  bus.write(CC1101_REG_ADDR, addr);
};
void CC1101::setSync(CC1101_SyncMode syncMode, uint16_t syncWord, uint8_t preambleLen) {
  bus.writeField(CC1101_REG_MDMCFG2, syncMode, 2, 0);

  bus.write(CC1101_REG_SYNC1, syncWord >> 8);
  bus.write(CC1101_REG_SYNC0, syncWord & 0xff);

  bus.writeField(CC1101_REG_MDMCFG1, getPreambleIdx(preambleLen), 6, 4);
};
void CC1101::setAutoCalib(bool en) {
  bus.writeField(CC1101_REG_MCSM0, (byte)en, 5, 4);
};
void CC1101::setManchester(bool en) {
  if(mod == CC1101_MOD_MSK || mod == CC1101_MOD_4FSK) return;
  bus.writeField(CC1101_REG_MDMCFG2, (byte)en, 3, 3);
};
void CC1101::setAppendStatus(bool en) {
  bus.writeField(CC1101_REG_PKTCTRL1, (byte)en, 2, 2);
};
void CC1101::setDataWhitening(bool en) {
  bus.writeField(CC1101_REG_PKTCTRL0, (byte)en, 6, 6);
};
void CC1101::setPktLen(uint8_t len) {
  bus.write(CC1101_REG_PKTLEN, pktLen);
};
void CC1101::setPktLenMode(bool en) {
  bus.writeField(CC1101_REG_PKTCTRL0, (byte)en, 1, 0); /* todo: 2 */
};
void CC1101::setMod(CC1101_Modulation mod){
  bus.writeField(CC1101_REG_MDMCFG2, (uint8_t)mod, 6, 4);
};
void CC1101::setFreq(double freq){
  uint32_t f = ((freq * 65536.0) / CC1101_CRYSTAL_FREQ); 

  bus.write(CC1101_REG_FREQ0, f & 0xff);
  bus.write(CC1101_REG_FREQ1, (f >> 8) & 0xff);
  bus.write(CC1101_REG_FREQ2, (f >> 16) & 0xff);

};
void CC1101::setDrate(double drate){
  uint32_t xosc = CC1101_CRYSTAL_FREQ * 1000;
  uint8_t e = log2((drate * (double)((uint32_t)1 << 20)) / xosc);
  uint32_t m = round(drate * ((double)((uint32_t)1 << (28 - e)) / xosc) - 256);

  if (m == 256) {
    m = 0;
    e++;
  }

  bus.writeField(CC1101_REG_MDMCFG4, e, 3, 0);
  bus.writeField(CC1101_REG_MDMCFG3, (uint8_t)m, 7, 0);
  // bus.write(CC1101_REG_MDMCFG3, (uint8_t)m);
};
void CC1101::setPwr(CC1101_FreqBand freqBand, CC1101_PowerMW pwr, const uint8_t pwrTable[][8]){
  // if(mod == MOD_ASK_OOK) {
  //   uint8_t paTable[2] = {WRITE, pwrTable[freqBand][pwr]};
  //   bus.writeBurst(CC1101_REG_PATABLE, paTable, 2);
  //   bus.writeField(CC1101_REG_FREND0, 1, 2, 0);
  // } else {
  //   bus.write(CC1101_REG_PATABLE, pwrTable[freqBand][pwr]);
  //   bus.writeField(CC1101_REG_FREND0, 0, 2, 0);
  // }
  if(mod == CC1101_MOD_ASK_OOK) {
    bus.writeField(CC1101_REG_FREND0, 1, 2, 0);
  } else {
    bus.writeField(CC1101_REG_FREND0, 0, 2, 0);
  }
  bus.write(CC1101_REG_PATABLE, pwrTable[freqBand][pwr]);
};
void CC1101::setRxState() {
  while (true) {
    byte state = getState();
    if (state == STATE_RX) break; 
    else if (state == STATE_RXFIFO_OVERFLOW) bus.strobe(CC1101_REG_FRX);
    else if (state != (STATE_CALIB || STATE_SETTLING)) {
      bus.strobe(CC1101_REG_RX);
    }
    delayMicroseconds(50);
  }
};
void CC1101::setTxState() {
  while (true) {
    byte state = getState();
    if (state == STATE_TX) break;
    else if (state == STATE_TXFIFO_UNDERFLOW) bus.strobe(CC1101_REG_FTX);
    else if (state != (STATE_CALIB || STATE_SETTLING)) bus.strobe(CC1101_REG_TX);
    delayMicroseconds(50);
  }
};
void CC1101::setIdleState() {
  while (true) {
    if (getState() == STATE_IDLE) break;
    bus.strobe(CC1101_REG_IDLE);
    delayMicroseconds(50);
  }
};
void CC1101::setTwoWay() {
    bus.writeField(CC1101_REG_MCSM1, 0, 5, 4); // Disabl CCA
    bus.writeField(CC1101_REG_MCSM1, 3, 1, 0); // Set TXOFF to RX
    bus.writeField(CC1101_REG_MCSM1, 2, 3, 2); // Set RXOFF to TX
};

bool CC1101::enoughRxBytes(uint8_t len) {
  if (bus.readField(CC1101_REG_RXBYTES | CC1101_READ_BURST, 6, 0) < (len + (isVariablePktLen ? 1 : 0) + (addr > 0 ? 1 : 0))) return false;
  return true;
};
bool CC1101::waitForRxBytes(uint8_t len, size_t timeoutMs) {
  if (timeoutMs) {
    uint32_t timer = millis();
    while(!enoughRxBytes(len)) {
      if((timer + timeoutMs) < millis()) {
        setIdleState();
        getState(); // Magic trick
        return false;
      }
      delayMicroseconds(50);
    }
  } else {
    while (!enoughRxBytes(len)) delayMicroseconds(50);
  }
  return true;
};
void CC1101::readRxFifo(uint8_t *buff, uint8_t len) {
  if(isVariablePktLen) {
    len = bus.read(CC1101_REG_FIFO);
  }
  bus.readBurst(CC1101_REG_FIFO | CC1101_READ_BURST, buff, len);
  if(isAppendStatus) {
    uint8_t r = bus.read(CC1101_REG_FIFO);
    if(r >= 128) rssi = ((rssi - 256) / 2) - CC1101_RSSI_OFFSET;
    else rssi = (rssi / 2) - CC1101_RSSI_OFFSET;
    lqi = bus.read(CC1101_REG_FIFO) & 0x7f;
    // if(!(r >> 7) & 1) return false; // CRC Mismatch
  }
};
void CC1101::writeTxFifo(uint8_t *buff, uint8_t len) {
  if(isVariablePktLen) {
    len = sizeof(buff);
    bus.write(CC1101_REG_FIFO, len);
  }
  // if(addr > 0) {
  //   bus.write(CC1101_REG_FIFO, addr);
  // }
  bus.writeBurst(CC1101_REG_FIFO | CC1101_WRITE_BURST, buff, len);
};
