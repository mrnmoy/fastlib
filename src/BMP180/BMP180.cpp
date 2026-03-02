#include "BMP180.h"

bool BMP180::begin() {
  if (bus.read(BMP180_REG_CHIP_ID) != BMP180_CHIP_ID) return false;

  return true;
}
void BMP180::update() {
  static const CalParams cal = getCP();

  int32_t b5 = getB5(cal);
  temp = getTemp(b5);
  press = getPress(b5, cal);
  alt = getAlt(press);
  Serial.print(" temp: ");
  Serial.print(temp);
  Serial.print(" press: ");
  Serial.print(press);
  Serial.print(" alt: ");
  Serial.println(alt);
}

CalParams BMP180::getCP() {
  CalParams cal;
  byte calData[22];
  bus.readBurst(0xAA, calData, 22);
  cal.ac1 = (calData[0] << 8) | calData[1];
  cal.ac2 = (calData[2] << 8) | calData[3];
  cal.ac3 = (calData[4] << 8) | calData[5];
  cal.ac4 = (calData[6] << 8) | calData[7];
  cal.ac5 = (calData[8] << 8) | calData[9];
  cal.ac6 = (calData[10] << 8) | calData[11];
  cal.b1 = (calData[12] << 8) | calData[13];
  cal.b2 = (calData[14] << 8) | calData[15];
  cal.mb = (calData[16] << 8) | calData[17];
  cal.mc = (calData[18] << 8) | calData[19];
  cal.md = (calData[20] << 8) | calData[21];

  Serial.print("ac1: ");
  Serial.print(cal.ac1);
  Serial.print(" ac2: ");
  Serial.print(cal.ac2);
  Serial.print(" ac3: ");
  Serial.print(cal.ac3);
  Serial.print(" ac4: ");
  Serial.print(cal.ac4);
  Serial.print(" ac5: ");
  Serial.print(cal.ac5);
  Serial.print(" ac6: ");
  Serial.print(cal.ac6);
  Serial.print(" b1: ");
  Serial.print(cal.b1);
  Serial.print(" b2: ");
  Serial.print(cal.b2);
  Serial.print(" mb: ");
  Serial.print(cal.mb);
  Serial.print(" mc: ");
  Serial.print(cal.mc);
  Serial.print(" md: ");
  Serial.println(cal.md);
  return cal;
} 
int32_t BMP180::getUT() {
  bus.write(0xF4, 0x2E);
  delay(4.5);
  return bus.read16(BMP180_REG_MEAS);
}
int32_t BMP180::getUP() {
  bus.write(0xF4, 0x34 | (oss << 6));
  switch (oss) {
    case 3:
      delay(25.5);
      break;
    case 2:
      delay(13.5);
      break;
    case 1:
      delay(7.5);
      break;
    case 0:
      delay(4.5);
      break;
  }
  return bus.read24(BMP180_REG_MEAS) >> (8 - oss);
}
int32_t BMP180::getB5(const CalParams cal) {
  // int32_t x1 = ((getUT() - cal.ac6) * cal.ac5) >> 15;
  // int32_t x2 = (cal.mc << 11) / (x1 + cal.md);
  // return x1 + x2;

  int32_t x1 = ((getUT() - cal.ac6) * cal.ac5) / pow(2, 15);
  int32_t x2 = (cal.mc * pow(2, 11)) / (x1 + cal.md);
  return x1 + x2;
}
float BMP180::getTemp(int32_t b5) {
  // return (float)((b5 + 8) >> 4) / 10.0;
  return ((b5 + 8) / pow(2, 4)) / 10.0;
}
float BMP180::getPress(int32_t b5, const CalParams cal) {
  int32_t x1, x2, x3, b3, b6, p;
  uint32_t b4, b7;

  // b6 = b5 - 4000;
  // x1 = (cal.b2 * (b6 * b6 >> 12)) >> 11;
  // x2 = (cal.ac2 * b6) >> 11;
  // x3 = x1 + x2;
  // b3 = ((((cal.ac1 * 4) + x3) << oss) + 2) / 4;
  // x1 = (cal.ac3 * b6) >> 13;
  // x2 = (cal.b1 * ((b6 * b6) >> 12)) >> 16;
  // x3 = ((x1 + x2) + 2) >> 2;
  // b4 = (cal.ac4 * (uint32_t)(x3 + 32768)) >> 15; 
  // b7 = (uint32_t)(getUP() - b3) * (50000 >> oss);
  // if (b7 < 0x80000000) p = (b7 * 2) / b4;
  // else p = (b7 / b4) * 2;
  // x1 = (p >> 8) * (p >> 8);
  // x1 = (x1 * 3038) >> 16;
  // x2 = (-7357 * p) >> 16;
  // return (p + ((x1 + x2) + 3791)) >> 4;

  b6 = b5 - 4000;
  x1 = (cal.b2 * (b6 * b6 / pow(2, 12))) / pow(2, 11);
  x2 = cal.ac2 * b6 / pow(2, 11);
  x3 = x1 + x2;
  b3 = (((cal.ac1 * 4 + x3) << oss) + 2) / 4;
  x1 = cal.ac3 * b6 / pow(2, 13);
  x2 = (cal.b1 * (b6 * b6 / pow(2, 12))) / pow(2, 10);
  x3 = ((x1 + x2) + 2) / pow(2, 2);
  b4 = cal.ac4 * (uint32_t)(x3 + 32768) / pow(2, 15); 
  b7 = ((uint32_t)getUP() - b3) * (50000 >> oss);
  if (b7 < 0x80000000) p = (b7 * 2) / b4;
  else p = (b7 / b4) * 2;
  x1 = (p / pow(2, 8)) * (p / pow(2, 8));
  x1 = (x1 * 3038) / pow(2, 16);
  x2 = (-7357 * p) / pow(2, 16);
  return p + (x1 + x2 + 3791) / pow(2, 4);
}
float BMP180::getAlt(int32_t p) {
  // return 44330.0 * (1.0 - pow(p / 1013.25, 1 / 5.255));
  return 44330.0 * (1.0 - pow(p / 101919.3, 0.1903));
} 
