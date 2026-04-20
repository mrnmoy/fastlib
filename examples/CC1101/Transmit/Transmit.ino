#include <SPI.h>
#include <fastlib.h>

#define SCK 7
#define MISO 5
#define MOSI 16
#define SS 18

CC1101 radio(CC1101_MOD_2FSK,         // mod
             433.8,                   // freq
             4.0,                     // drate
             CC1101_POWER_1MW,        // pwr
             0,                       // addr
             4,                       // pktlen
             CC1101_SYNC_MODE_16_16,  // sync mode
             0x1234,                  // sync word
             64,                      // preamble length
             true,                    // crc
             false,                   // fec
             true,                    // auto calib
             false,                   // manchester
             true,                    // append status
             false,                   // data whitening
             false,                   // variable packet length
             SS,                      // ss/cs pin
             MISO);                   // miso pin

byte txBuff[4] = { 200, 201, 202, 203 };

void setup() {
  Serial.begin(115200);

  while (!Serial);

  Serial.println("Namaste CC1101!");

  pinMode(SCK, OUTPUT);
  pinMode(MISO, INPUT);
  pinMode(MOSI, OUTPUT);
  pinMode(SS, OUTPUT);

  digitalWrite(SS, HIGH);

  SPI.begin(SCK, MISO, MOSI, SS);

  Serial.print("Looking for radio");
  while (!radio.begin()) {
    Serial.print(".");
    delay(2000);
  }
  Serial.printf("\nRadio initialized! [%d, %d]\n", radio.partnum, radio.version);
}

void loop() {
  if (!radio.write(txBuff, 4)) {
    Serial.println("Error sending packet");
  } else {
    Serial.printf("Sent pkt: [%d, %d, %d, %d]\n", txBuff[0], txBuff[1], txBuff[2], txBuff[3]);
  }

  delay(2000);
}
