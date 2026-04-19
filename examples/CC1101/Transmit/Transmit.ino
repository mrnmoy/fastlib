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

  while (!radio.begin()) {
    Serial.println("Radio not found!");
    delay(2000);
  }

  Serial.println("Radio initialized!");
}

void loop() {
  if (radio.write(txBuff, 4)) {
    Serial.print("Sent: [");
    for (int i = 0; i < sizeof(txBuff); i++) {
      if (i != 0) Serial.print(", ");
      Serial.print(txBuff[i]);
    }
    Serial.print("] Length: ");
    Serial.println(sizeof(txBuff));
  } else {
    Serial.println("Error sending packet");
  }

  delay(2000);
}
