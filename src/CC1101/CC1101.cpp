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
  setSync(syncMode, syncWord, preambleLen);
  setPktLenMode(isVariablePktLen);
  setPktLen(pktLen);

  return true;
}

bool CC1101::read(uint8_t *buff, size_t timeoutMs){
  uint8_t len = !isVariablePktLen ? pktLen : sizeof(buff);
  setIdleState();
  flushRxBuff();
  setRxState();
  if(!waitForRxBytes(len, timeoutMs)) return false; //timeout
  if(!readRxFifo(buff, len)) return false; // crc mismatch
  waitForState();
  return true;
};
bool CC1101::read(uint8_t *buff, uint8_t len, size_t timeoutMs){
  setIdleState();
  setPktLen(len);
  flushRxBuff();
  setRxState();
  if(!waitForRxBytes(len, timeoutMs)) return false; //timeout
  if(!readRxFifo(buff, len)) return false; // crc mismatch
  waitForState();
  return true;
};
bool CC1101::write(uint8_t *buff){
  uint8_t len = !isVariablePktLen ? pktLen : sizeof(buff);
  setIdleState();
  flushTxBuff();
  setTxState();
  writeTxFifo(buff, len);
  waitForState();
  return true;
};
bool CC1101::write(uint8_t *buff, uint8_t len){
  setIdleState();
  setPktLenMode(false);
  setPktLen(len);
  flushTxBuff();
  setTxState();
  writeTxFifo(buff, len);
  waitForState();
  return true;
};
bool CC1101::link(uint8_t *txBuff, uint8_t *rxBuff, size_t timeoutMs) {
  uint32_t timer = millis();
  setIdleState();
  flushTxBuff();
  setTxState();
  writeTxFifo(txBuff, pktLen);
  waitForState();
  flushRxBuff();
  setRxState();
  while (!enoughRxBytes(pktLen)) {
    if (timer + timeoutMs < millis()) {
      return false; // timeout
    }
    delay(1); // avoid watchdog
  }
  readRxFifo(rxBuff, pktLen);
  waitForState();
  return true;
};
void CC1101::link2(uint8_t *txBuff, uint8_t *rxBuff, size_t timeoutMs) {
  uint32_t timer;
  setIdleState();
  setTwoWay();
  setTxState();
  while(true) {
    flushTxBuff();
    writeTxFifo(txBuff, pktLen);
    waitForState(STATE_RX);
    Serial.println("Sent packet.");
    flushRxBuff();
    timer = millis();
    while (true) { /* state goes to tx even when fifo is empty */
      Serial.print("state: ");
      Serial.println(getState());
      if (bus.readField(CC1101_REG_RXBYTES | CC1101_READ_BURST, 6, 0) != 0) {
        Serial.println("rxbytes > 0");
        readRxFifo(rxBuff, pktLen);
        waitForState(STATE_TX);
        Serial.println("Received packet.");
        break;
      // } else if (timer + timeoutMs < millis()) {
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

  bus.strobe(CC1101_REG_RES | CC1101_WRITE_BURST);
}
void CC1101::flushRxBuff(){
  if(getState() != (STATE_IDLE || STATE_RXFIFO_OVERFLOW)) return;
  bus.strobe(CC1101_REG_FRX | CC1101_WRITE_BURST);
};
void CC1101::flushTxBuff(){
  if(getState() != (STATE_IDLE || STATE_TXFIFO_UNDERFLOW)) return;
  bus.strobe(CC1101_REG_FTX);
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
bool CC1101::getFreqBand(float freq, const float freqTable[][2]) {
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
  bus.writeField(CC1101_REG_PKTCTRL0, CC1101_READ, CC1101_WRITE, 2, 2, (byte)en); /* CRC_EN */
  bus.writeField(CC1101_REG_PKTCTRL1, CC1101_READ, CC1101_WRITE, 3, 3, (byte)en); /* Autoflush */
};
void CC1101::setFEC(bool en) {
  if(isVariablePktLen) return;
  bus.writeField(CC1101_REG_MDMCFG1, CC1101_READ, CC1101_WRITE, 7, 7, (byte)en);
};
void CC1101::setAddr(byte addr) {
  bus.writeField(CC1101_REG_PKTCTRL1, CC1101_READ, CC1101_WRITE, 1, 0, addr > 0 ? 1 : 0);
  bus.write(CC1101_REG_ADDR | CC1101_WRITE, addr);
};
void CC1101::setSync(CC1101_SyncMode syncMode, uint16_t syncWord, uint8_t preambleLen) {
  bus.writeField(CC1101_REG_MDMCFG2, CC1101_READ, CC1101_WRITE, 2, 0, syncMode);

  bus.write(CC1101_REG_SYNC1 | CC1101_WRITE, syncWord >> 8);
  bus.write(CC1101_REG_SYNC0 | CC1101_WRITE, syncWord & 0xff);

  bus.writeField(CC1101_REG_MDMCFG1, CC1101_READ, CC1101_WRITE, 6, 4, getPreambleIdx(preambleLen));
};
void CC1101::setAutoCalib(bool en) {
  bus.writeField(CC1101_REG_MCSM0, CC1101_READ, CC1101_WRITE, 5, 4, (byte)en);
};
void CC1101::setManchester(bool en) {
  if(mod != CC1101_MOD_MSK || mod != CC1101_MOD_4FSK)
    bus.writeField(CC1101_REG_MDMCFG2, CC1101_READ, CC1101_WRITE, 3, 3, (byte)en);
};
void CC1101::setAppendStatus(bool en) {
  bus.writeField(CC1101_REG_PKTCTRL1, CC1101_READ, CC1101_WRITE, 2, 2, (byte)en);
};
void CC1101::setDataWhitening(bool en) {
  bus.writeField(CC1101_REG_PKTCTRL0, CC1101_READ, CC1101_WRITE, 6, 6, (byte)en);
};
void CC1101::setPktLen(uint8_t len) {
  bus.write(CC1101_REG_PKTLEN | CC1101_WRITE, len);
};
void CC1101::setPktLenMode(bool isVariablePktLen) {
  bus.writeField(CC1101_REG_PKTCTRL0, CC1101_READ, CC1101_WRITE, 1, 0, (byte)isVariablePktLen); /* TODO: infinite */
};
void CC1101::setMod(CC1101_Modulation mod){
  bus.writeField(CC1101_REG_MDMCFG2, CC1101_READ, CC1101_WRITE, 6, 4, (byte)mod);
};
void CC1101::setFreq(float freq){
  uint32_t f = ((freq * 65536.0) / CC1101_CRYSTAL_FREQ); 

  bus.write(CC1101_REG_FREQ0 | CC1101_WRITE, f & 0xff);
  bus.write(CC1101_REG_FREQ1 | CC1101_WRITE, (f >> 8) & 0xff);
  bus.write(CC1101_REG_FREQ2 | CC1101_WRITE, (f >> 16) & 0xff);

  /* TODO Deviation */ 
  // bus.writeField(CC1101_REG_DEVIATN, CC1101_READ, CC1101_WRITE, 6, 4, devE);
  // bus.writeField(CC1101_REG_DEVIATN, CC1101_READ, CC1101_WRITE, 2, 0, devM);
};
void CC1101::setDrate(float drate){
  uint32_t xosc = CC1101_CRYSTAL_FREQ * 1000;
  uint8_t e = log2((drate * (double)((uint32_t)1 << 20)) / xosc);
  uint32_t m = round(drate * ((double)((uint32_t)1 << (28 - e)) / xosc) - 256.0);

  if (m == 256) {
    m = 0;
    e++;
  }

  bus.writeField(CC1101_REG_MDMCFG4, CC1101_READ, CC1101_WRITE, 3, 0, e);
  bus.writeField(CC1101_REG_MDMCFG3, CC1101_READ, CC1101_WRITE, 7, 0, (byte)m);
  // bus.write(CC1101_REG_MDMCFG3 | CC1101_WRITE, (byte)m);
};
void CC1101::setPwr(CC1101_FreqBand freqBand, CC1101_PowerMW pwr, const uint8_t pwrTable[][8]){
  // if(mod == CC1101_MOD_ASK_OOK) {
  //   uint8_t paTable[2] = {CC1101_WRITE, pwrTable[freqBand][pwr]};
  //   bus.writeBurst(CC1101_REG_PATABLE | CC1101_WRITE_BURST, paTable, 2);
  //   bus.writeField(CC1101_REG_FREND0, CC1101_READ, CC1101_WRITE, 2, 0, 1);
  // } else {
  //   bus.write(CC1101_REG_PATABLE | CC1101_WRITE, pwrTable[freqBand][pwr]);
  //   bus.writeField(CC1101_REG_FREND0, CC1101_READ, CC1101_WRITE, 2, 0, 0);
  // }
  if(mod == CC1101_MOD_ASK_OOK) {
    bus.writeField(CC1101_REG_FREND0, CC1101_READ, CC1101_WRITE, 2, 0, 1);
  } else {
    bus.writeField(CC1101_REG_FREND0, CC1101_READ, CC1101_WRITE, 2, 0, 0);
  }
  bus.write(CC1101_REG_PATABLE | CC1101_WRITE, pwrTable[freqBand][pwr]);
};
void CC1101::setIdleState() {
  if (getState() == STATE_IDLE) return;
  bus.strobe(CC1101_REG_IDLE);
  while (getState() != STATE_IDLE) {
    delayMicroseconds(50);
  }
};
void CC1101::setRxState() {
    byte state = getState();
    if (state == STATE_RX) return; 
    else if (state == STATE_RXFIFO_OVERFLOW) bus.strobe(CC1101_REG_FRX);
    else if (state != (STATE_CALIB || STATE_SETTLING)) bus.strobe(CC1101_REG_RX);
    while (getState() != STATE_RX) {
      delayMicroseconds(50);
    }
};
void CC1101::setTxState() {
    byte state = getState();
    if (state == STATE_TX) return;
    else if (state == STATE_TXFIFO_UNDERFLOW) bus.strobe(CC1101_REG_FTX);
    else if (state != (STATE_CALIB || STATE_SETTLING)) bus.strobe(CC1101_REG_TX);
    while (getState() != STATE_TX) {
      delayMicroseconds(50);
    }
};
void CC1101::setTwoWay() {
    bus.writeField(CC1101_REG_MCSM1, CC1101_READ, CC1101_WRITE, 5, 4, 0); // Disabl CCA
    bus.writeField(CC1101_REG_MCSM1, CC1101_READ, CC1101_WRITE, 1, 0, 3); // Set TXOFF to RX
    bus.writeField(CC1101_REG_MCSM1, CC1101_READ, CC1101_WRITE, 3, 2, 2); // Set RXOFF to TX
};

bool CC1101::enoughRxBytes(uint8_t len) {
  // if (bus.readField(CC1101_REG_RXBYTES | CC1101_READ_BURST, 6, 0) < (len + (isVariablePktLen ? 1 : 0) + (addr > 0 ? 1 : 0))) return false;
  if (isVariablePktLen) len++;
  if (addr > 0) len++;
  if (bus.readField(CC1101_REG_RXBYTES | CC1101_READ_BURST, 6, 0) < len) return false;
  return true;
};
bool CC1101::waitForRxBytes(uint8_t len, size_t timeoutMs) {
  if (timeoutMs) {
    uint32_t timer = millis();
    while(!enoughRxBytes(len)) {
      if((timer + timeoutMs) < millis()) {
        Serial.println("Waiting for rxbytes timeout");
        setIdleState();
        // getState(); // Magic trick
        return false;
      }
      delayMicroseconds(50);
    }
  } else {
    while (!enoughRxBytes(len)) delayMicroseconds(50);
  }
  return true;
};
bool CC1101::readRxFifo(uint8_t *buff, uint8_t len) {
  if(isVariablePktLen) {
    len = bus.read(CC1101_REG_FIFO | CC1101_READ);
  }
  bus.readBurst(CC1101_REG_FIFO | CC1101_READ_BURST, buff, len);
  if(isAppendStatus) {
    uint8_t r = bus.read(CC1101_REG_FIFO | CC1101_READ);
    if(r >= 128) rssi = ((rssi - 256) / 2) - CC1101_RSSI_OFFSET;
    else rssi = (rssi / 2) - CC1101_RSSI_OFFSET;
    lqi = bus.read(CC1101_REG_FIFO | CC1101_READ) & 0x7f;
    if(!(r >> 7) & 1) { 
      Serial.println("CRC Mismatch");
      return false; // CRC Mismatch
    } 
  }
  return true;
};
void CC1101::writeTxFifo(uint8_t *buff, uint8_t len) {
  if(isVariablePktLen) {
    bus.write(CC1101_REG_FIFO | CC1101_WRITE, len);
  }  
  if(addr > 0) {
    bus.write(CC1101_REG_FIFO | CC1101_WRITE, addr);
  }
  bus.writeBurst(CC1101_REG_FIFO | CC1101_WRITE_BURST, buff, len);
};
