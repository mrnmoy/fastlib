#include <SPI.h>
#include <fastlib.h>

#define SCK 7
#define MISO 5
#define MOSI 16
#define SS 18

CC1101 radio(CC1101_MOD_2FSK,
             433.8,
             4.0,
             CC1101_POWER_1MW,
             0,
             4,
             CC1101_SYNC_MODE_16_16,
             0x1234,
             64,
             true,
             false,
             true,
             false,
             true,
             false,
             false,
             SS,
             MISO);

byte rxBuff[1];

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
  if (!radio.read(rxBuff, 1, 2000)) {
    Serial.println("Receiving rxBuff timeout");
  } else {
    Serial.printf("Received: [%d], Rssi: %d, Lqi: %d\n", rxBuff[0], radio.rssi, radio.lqi);
  }
}
