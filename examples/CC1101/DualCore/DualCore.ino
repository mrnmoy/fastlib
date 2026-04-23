#include <SPI.h>
#include <fastlib.h>

// #define FSPI_SCK 7
// #define FSPI_MISO 5
// #define FSPI_MOSI 16
// #define TX_SS 18
#define FSPI_MISO 4
#define FSPI_MOSI 15
#define FSPI_SCK 6
#define TX_SS 17

#define HSPI_MISO 5
#define HSPI_MOSI 18
#define HSPI_SCK 16
#define RX_SS 7

void setup() {
  Serial.begin(115200);

  while (!Serial);

  Serial.println("Namaste CC1101!");

  xTaskCreatePinnedToCore(
    txTask,
    "TX Task",
    10000,
    NULL,
    1,
    NULL,
    0);

  xTaskCreatePinnedToCore(
    rxTask,
    "RX Task",
    10000,
    NULL,
    20,
    NULL,
    1);

  delay(500);
}
void loop() {}

void txTask(void *pvParameters) {
  Serial.print("TX Task running on core ");
  Serial.println(xPortGetCoreID());

  SPIClass fspi(FSPI);
  CC1101 radio(CC1101_MOD_2FSK,         // mod
               433.8,                   // freq
               26.0,                    // drate
               CC1101_POWER_3MW,        // pwr
               0,                       // addr
               64,                      // pktlen
               CC1101_SYNC_MODE_16_16,  // sync mode
               0x1234,                  // sync word
               64,                      // preamble length
               true,                    // crc
               false,                   // fec
               true,                    // auto calib
               false,                   // manchester
               true,                    // append status
               false,                   // data whitening
               true,                    // variable packet length
               TX_SS,                   // ss/cs pin
               FSPI_MISO,               // miso pin
               fspi);                   // spi class

  pinMode(FSPI_SCK, OUTPUT);
  pinMode(FSPI_MISO, INPUT);
  pinMode(FSPI_MOSI, OUTPUT);
  pinMode(TX_SS, OUTPUT);

  digitalWrite(TX_SS, HIGH);

  fspi.begin(FSPI_SCK, FSPI_MISO, FSPI_MOSI, TX_SS);

  while (!radio.begin()) {
    Serial.print("TX Radio not found!");
    Serial.println();
    delay(2000);
  }
  Serial.printf("TX Radio initialized! [%d, %d]", radio.partnum, radio.version);
  Serial.println();

  byte buff[4] = { 200, 201, 202, 203 };

  while (true) {
    if (radio.write(buff, 4)) {
      Serial.printf("Sending pkt: [%d, %d, %d, %d]", buff[0], buff[1], buff[2], buff[3]);
    } else {
      Serial.print("Error sending tx packet");
    }

    Serial.println();
    delay(1000);
  }
}

void rxTask(void *pvParameters) {
  Serial.print("RX Task running on core ");
  Serial.println(xPortGetCoreID());

  SPIClass hspi(HSPI);
  CC1101 radio(CC1101_MOD_2FSK,         // mod
               433.8,                   // freq
               26.0,                    // drate
               CC1101_POWER_3MW,        // pwr
               0,                       // addr
               64,                      // pktlen
               CC1101_SYNC_MODE_16_16,  // sync mode
               0x1234,                  // sync word
               64,                      // preamble length
               true,                    // crc
               false,                   // fec
               true,                    // auto calib
               false,                   // manchester
               true,                    // append status
               false,                   // data whitening
               true,                    // variable packet length
               RX_SS,                   // ss/cs pin
               HSPI_MISO,               // miso pin
               hspi);                   // spi class

  pinMode(HSPI_SCK, OUTPUT);
  pinMode(HSPI_MISO, INPUT);
  pinMode(HSPI_MOSI, OUTPUT);
  pinMode(RX_SS, OUTPUT);

  digitalWrite(RX_SS, HIGH);

  hspi.begin(HSPI_SCK, HSPI_MISO, HSPI_MOSI, RX_SS);

  while (!radio.begin()) {
    Serial.print("RX Radio not found!");
    Serial.println();
    delay(2000);
  }
  Serial.printf("RX Radio initialized! [%d, %d]", radio.partnum, radio.version);
  Serial.println();

  byte buff[4];

  while (true) {
    if (radio.read(buff, 4, 2000)) {
      Serial.printf("Received: [%d, %d, %d, %d], Rssi: %d, Lqi: %d", buff[0], buff[1], buff[2], buff[3], radio.rssi, radio.lqi);
    } else {
      Serial.print("Error receiving rx pkt");
    }
    Serial.println();
  }
}
