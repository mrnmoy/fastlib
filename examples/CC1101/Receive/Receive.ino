#include <SPI.h>
#include <fastlib.h>

#define SCK 7
#define MISO 5
#define MOSI 16
#define SS 18

CC1101 radio(CC1101_MOD_2FSK,         // mod
             433.8,                   // freq
             4.0,                     // drate
             CC1101_POWER_3MW,        // pwr
             0,                       // addr
             4,                       // pktlen
             CC1101_SYNC_MODE_16_16,  // sync mode
             0x1234,                  // sync word
             64,                      // preamble length
             false,                   // crc
             false,                   // fec
             true,                    // auto calib
             false,                   // manchester
             true,                    // append status
             false,                   // data whitening
             false,                   // variable packet length
             SS,                      // ss/cs pin
             MISO);                   // miso pin

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

byte buff[4];
void loop() {
  if (!radio.read(buff)) {
    Serial.print("Receiving rxBuff timeout");
  } else {
    Serial.printf("Received: [%d, %d, %d, %d], Rssi: %d, Lqi: %d\n", buff[0], buff[1], buff[2], buff[3], radio.rssi, radio.lqi);
  }

  Serial.println();
}
