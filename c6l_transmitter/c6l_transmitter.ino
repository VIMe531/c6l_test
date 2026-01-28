#include <M5Unified.h>
#include "unitc6l_commu.h"

// =============================
//  M5 Unit C6L (ESP32-C6) + SX1262
//  TX (ArduinoIDE / RadioLib)
// =============================

void setup() {
  Serial.begin(115200);
  M5.begin();
  delay(300);

  randomSeed((uint32_t)micros());

  // BUSY pin (SX1262 BUSY = GPIO19)
  pinMode(19, INPUT);

  auto& ioe = M5.getIOExpander(0);
  // Unit C6L IOExpander pins (as used in your original sketch)
  ioe.digitalWrite(7, false);
  delay(100);
  ioe.digitalWrite(7, true);  // SX_NRST
  ioe.digitalWrite(6, true);  // SX_ANT_SW
  ioe.digitalWrite(5, true);  // SX_LNA_EN

  Serial.println("[SX1262] Initializing...");
  int beginState = radio.begin(
      LORA_FREQ_MHZ, LORA_BW_KHZ, LORA_SF, LORA_CR_DENOM,
      LORA_SYNC_WORD, TX_POWER_DBM, PREAMBLE_LEN,
      TCXO_VOLT, USE_LDO);

  Serial.print("[SX1262] begin() = ");
  Serial.println(beginState);

  if (beginState != RADIOLIB_ERR_NONE) {
    Serial.println("[SX1262] init failed. Check wiring/pins and power.");
    while (true) delay(1000);
  }

  uint32_t id = deviceId32();
  Serial.printf("[ID] deviceId32 = %08lX\n", (unsigned long)id);

  // 初期状態を整える（念のため）
  forceStandbyAndClearIrq();

  Serial.println("[TX] Ready.");
}

void loop() {
  static uint32_t counter = 0;

  // payload format: "<8-hex-ID>,<counter>"
  uint32_t id = deviceId32();
  String payload = String(id, HEX);
  payload.toUpperCase();
  payload += ",";
  payload += String(counter);

  Serial.printf("[TX] #%lu payload=\"%s\" ...\n", (unsigned long)counter, payload.c_str());
  Serial.printf("BUSY(before)=%d\n", digitalRead(19));

int st = transmitWithRegAndToAWait(payload, true, true, 0);

  Serial.print("code=");
  Serial.print(st);
  Serial.print(" desc=");
  Serial.println(radiolibErrToStr(st));
  Serial.printf("BUSY(after)=%d\n", digitalRead(19));

  if (st == RADIOLIB_ERR_NONE) {
    Serial.println("[TX] OK");
  } else if (st == RADIOLIB_ERR_CHANNEL_BUSY) {
    Serial.println("[TX] BUSY (LBT)");
  } else if (st == RADIOLIB_ERR_INVALID_PACKET_LENGTH) {
    Serial.println("[TX] payload too long (ToA > 4s)");
  } else {
    Serial.printf("[TX] FAIL code=%d\n", st);
  }

  counter++;

  // Application interval (example). This is separate from the required 50ms off-time.
  // delay(1000);
}
