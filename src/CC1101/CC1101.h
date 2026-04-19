#pragma once

#ifndef CC1101_H
#define CC1101_H

#include "../fastutils.h"

#define CC1101_PARTNUM            0x00
#define CC1101_VERSION            0x14
#define CC1101_VERSION_LEGACY     0x04
#define CC1101_FIFO_SIZE          64    
#define CC1101_CRYSTAL_FREQ       26   
#define CC1101_RSSI_OFFSET        74

#define CC1101_READ               0x80
#define CC1101_WRITE              0x00
#define CC1101_READ_BURST         0xC0
#define CC1101_WRITE_BURST        0x40

/* Status registers */
#define CC1101_REG_PARTNUM        0x30
#define CC1101_REG_VERSION        0x31
#define CC1101_REG_TXBYTES        0x3a
#define CC1101_REG_RXBYTES        0x3b
#define CC1101_REG_RCCTRL0_STATUS 0x3d

/* Command strobes */
#define CC1101_REG_RES            0x30  
#define CC1101_REG_RX             0x34  
#define CC1101_REG_TX             0x35  
#define CC1101_REG_IDLE           0x36  
#define CC1101_REG_FRX            0x3a  
#define CC1101_REG_FTX            0x3b  
#define CC1101_REG_NOP            0x3d  

/* Registers */
#define CC1101_REG_IOCFG0         0x02
#define CC1101_REG_SYNC1          0x04  
#define CC1101_REG_SYNC0          0x05  
#define CC1101_REG_PKTLEN         0x06
#define CC1101_REG_PKTCTRL1       0x07
#define CC1101_REG_PKTCTRL0       0x08  
#define CC1101_REG_ADDR           0x09
#define CC1101_REG_CHANNR         0x0a
#define CC1101_REG_FREQ2          0x0d
#define CC1101_REG_FREQ1          0x0e
#define CC1101_REG_MDMCFG4        0x10
#define CC1101_REG_MDMCFG3        0x11
#define CC1101_REG_MDMCFG2        0x12  
#define CC1101_REG_MDMCFG1        0x13
#define CC1101_REG_MDMCFG0        0x14
#define CC1101_REG_DEVIATN        0x15
#define CC1101_REG_FREQ0          0x0f
#define CC1101_REG_MCSM2          0x16
#define CC1101_REG_MCSM1          0x17
#define CC1101_REG_MCSM0          0x18
#define CC1101_REG_FREND0         0x22  
#define CC1101_REG_PATABLE        0x3e
#define CC1101_REG_FIFO           0x3f

enum CC1101_Modulation {
  CC1101_MOD_2FSK    = 0,
  CC1101_MOD_GFSK    = 1,
  CC1101_MOD_ASK_OOK = 3,
  CC1101_MOD_4FSK    = 4,
  CC1101_MOD_MSK     = 7
};

enum CC1101_PowerMW {
  CC1101_POWER_1MW   = 4,  /* 1mw / 0dbm */
  CC1101_POWER_3MW   = 5,  /* 3.16mw / 5dbm */
  CC1101_POWER_5MW   = 6,  /* 5.01mw / 7dbm */
  CC1101_POWER_10MW  = 7, /* 10mw / 10dbm */
};

enum CC1101_SyncMode {
  CC1101_SYNC_MODE_NO_PREAMBLE    = 0,  /* No preamble/sync */
  CC1101_SYNC_MODE_15_16          = 1,  /* 15/16 sync word bits detected */
  CC1101_SYNC_MODE_16_16          = 2,  /* 16/16 sync word bits detected */
  CC1101_SYNC_MODE_30_32          = 3,  /* 30/32 sync word bits detected */
  CC1101_SYNC_MODE_NO_PREAMBLE_CS = 4,  /* No preamble/sync, carrier-sense above threshold */
  CC1101_SYNC_MODE_15_16_CS       = 5,  /* 15/16 + carrier-sense above threshold */
  CC1101_SYNC_MODE_16_16_CS       = 6,  /* 16/16 + carrier-sense above threshold */
  CC1101_SYNC_MODE_30_32_CS       = 7,  /* 30/32 + carrier-sense above threshold */
};

class CC1101 {
  public:
    CC1101(
        CC1101_Modulation mod = CC1101_MOD_2FSK,
        double freq = 433.8,
        double drate = 4.0,
        CC1101_PowerMW pwr = CC1101_POWER_1MW,
        uint8_t addr = 0,
        uint8_t pktLen = 4,
        CC1101_SyncMode syncMode = CC1101_SYNC_MODE_16_16,
        uint16_t syncWord = 0x1234,
        uint8_t preambleLen = 64,
        bool isCRC = true, 
        bool isFEC = false,
        bool isAutoCalib = true,
        bool isManchester = false,
        bool isAppendStatus = true,
        bool isDataWhitening = false,
        bool isVariablePktLen = false,
        uint8_t ss = SS,
        uint8_t miso = MISO,
        SPIClass &spi = SPI
        ):
      mod(mod),
      freq(freq),
      drate(drate),
      pwr(pwr),
      addr(addr),
      pktLen(pktLen),
      syncMode(syncMode),
      syncWord(syncWord),
      preambleLen(preambleLen),
      isCRC(isCRC), 
      isFEC(isFEC),
      isAutoCalib(isAutoCalib),
      isManchester(isManchester),
      isAppendStatus(isAppendStatus),
      isDataWhitening(isDataWhitening),
      isVariablePktLen(isVariablePktLen),
      ss(ss),
      bus(ss, miso, spi) {};

  int8_t partnum = -1, version = -1;
  uint8_t rssi, lqi;

  bool begin();

  bool read(uint8_t *buff, const int32_t timeoutMs = -1);
  bool read(uint8_t *buff, uint8_t len, const int32_t timeoutMs = -1);
  bool write(uint8_t *buff);
  bool write(uint8_t *buff, uint8_t len);
  bool link(uint8_t *txBuff, uint8_t *rxBuff, const int32_t timeoutMs = 500);
  void link2(uint8_t *txBuff, uint8_t *rxBuff, const int32_t timeoutMs = 500);

  private: 
    enum State {
      STATE_IDLE              = 0,
      STATE_RX                = 1,
      STATE_TX                = 2,
      STATE_FSTXON            = 3,
      STATE_CALIB             = 4,
      STATE_SETTLING          = 5,
      STATE_RXFIFO_OVERFLOW   = 6,
      STATE_TXFIFO_UNDERFLOW  = 7,
    };
    enum CC1101_FreqBand {
      CC1101_FREQ_BAND_315 = 0,
      CC1101_FREQ_BAND_433 = 1,
      CC1101_FREQ_BAND_868 = 2,
      CC1101_FREQ_BAND_915 = 3,
    };
     
    inline static const double freqTable[][2] = {
      [CC1101_FREQ_BAND_315] = { 300.0, 348.0 },
      [CC1101_FREQ_BAND_433] = { 387.0, 464.0 },
      [CC1101_FREQ_BAND_868] = { 779.0, 891.5 },
      [CC1101_FREQ_BAND_915] = { 896.6, 928.0 },
    };
    inline static const double drateTable[][2] = {
      [CC1101_MOD_2FSK]    = {  0.6, 500.0 },
      [CC1101_MOD_GFSK]    = {  0.6, 250.0 },
      [2]           = {  0.0, 0.0   }, 
      [CC1101_MOD_ASK_OOK] = {  0.6, 250.0 },
      [CC1101_MOD_4FSK]    = {  0.6, 300.0 },
      [5]           = {  0.0, 0.0   },
      [6]           = {  0.0, 0.0   },
      [CC1101_MOD_MSK]     = { 26.0, 500.0 }
    };
    inline static const uint8_t powerTable[][8] = {
      [CC1101_FREQ_BAND_315] = { 0x12, 0x0d, 0x1c, 0x34, 0x51, 0x85, 0xcb, 0xc2 },
      [CC1101_FREQ_BAND_433] = { 0x12, 0x0e, 0x1d, 0x34, 0x60, 0x84, 0xc8, 0xc0 },
      [CC1101_FREQ_BAND_868] = { 0x03, 0x0f, 0x1e, 0x27, 0x50, 0x81, 0xcb, 0xc2 },
      [CC1101_FREQ_BAND_915] = { 0x03, 0x0e, 0x1e, 0x27, 0x8e, 0xcd, 0xc7, 0xc0 },
    };
     
    Bus<true> bus;
    byte ss;

    CC1101_Modulation mod;
    CC1101_SyncMode syncMode;
    CC1101_PowerMW pwr;
    CC1101_FreqBand freqBand;
    double freq, drate;
    uint8_t pktLen, preambleLen;
    uint16_t syncWord;
    byte addr;
    bool isCRC, 
         isFEC,
         isAutoCalib,
         isManchester,
         isAppendStatus,
         isDataWhitening,
         isVariablePktLen;
    int8_t pwrIdx = -1, preambleIdx = -1;

    void reset();
    void flushRxBuff();
    void flushTxBuff();
    byte readStatus(byte addr);
    void waitForState(State state = STATE_IDLE);
     
    byte getState();
    bool getChipInfo();
    bool getFreqBand(double freq, const double freqTable[][2]);
    uint8_t getPreambleIdx(uint8_t len);

    void setCRC(bool en);
    void setFEC(bool en);
    void setAddr(byte addr);
    void setSync(CC1101_SyncMode syncMode, uint16_t syncWord, uint8_t preambleLen);
    void setAutoCalib(bool en);
    void setManchester(bool en);
    void setAppendStatus(bool en);
    void setDataWhitening(bool en);
    void setPktLen(uint8_t len);
    void setPktLenMode(bool en);
    void setMod(CC1101_Modulation mod);
    void setFreq(double freq);
    void setDrate(double drate);
    void setPwr(CC1101_FreqBand freqBand, CC1101_PowerMW pwr, const uint8_t pwrTable[][8]);
    void setRxState();
    void setTxState();
    void setIdleState();
    void setTwoWay();

    bool enoughRxBytes(uint8_t len);
    bool waitForRxBytes(uint8_t len, uint32_t timeoutMs);
    void readRxFifo(uint8_t *buff);
    void writeTxFifo(uint8_t *buff);
};

#endif
