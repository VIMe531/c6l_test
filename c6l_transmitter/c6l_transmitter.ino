#include <M5Unified.h>
#include "unitc6l_commu.h"

UnitC6L_Commu c6l;

// =============================
//  M5 Unit C6L (ESP32-C6) + SX1262
//  TX (ArduinoIDE / RadioLib)
// =============================

// ----------------------------------
// static uint32_t deviceId32()
//
// MACアドレスを取得し、識別コードとする
//
// 入力：無し
// 出力：MACアドレス（uint32_t）
// ----------------------------------
static uint32_t deviceId32() {
  // ESP32-C6: eFuse MAC is 48-bit.
  uint64_t mac = ESP.getEfuseMac();
  return (uint32_t)(mac >> 16);  // upper 32 bits
}

void setup() {
  Serial.begin(115200);
  M5.begin();

  randomSeed((uint32_t)micros());

  // BUSY pin (SX1262 BUSY = GPIO19)
  pinMode(19, INPUT);

  c6l.begin_transmit();

  uint32_t id = deviceId32();
  Serial.printf("[ID] deviceId32 = %08lX\n", (unsigned long)id);
}

void loop() {
  static uint32_t counter = 0;
  // payload format: "<8-hex-ID>,<counter>"
  uint32_t id = deviceId32();
  String payload = String(id, HEX);
  payload.toUpperCase();
  payload += ",";
  payload += String(counter);

  Serial.printf("BUSY(before)=%d\n", digitalRead(19));

  int st = c6l.transmitLoRa(payload, 20);

  Serial.println(c6l.radiolibErrToStr(st));

  M5.Lcd.setCursor(0, 0);
  M5.Lcd.printf("[TX] \"%s\" \n", payload.c_str());

  if (st == RADIOLIB_ERR_NONE) {
    M5.Lcd.println("[TX] OK");
  } else if (st == RADIOLIB_ERR_CHANNEL_BUSY) {
    M5.Lcd.println("[TX] BUSY (LBT)");
  } else if (st == RADIOLIB_ERR_INVALID_PACKET_LENGTH) {
    M5.Lcd.println("[TX] payload too long (ToA > 4s)");
  } else {
    M5.Lcd.printf("[TX] FAIL code=%d\n", st);
  }

  counter++;

  delay(50);
}
