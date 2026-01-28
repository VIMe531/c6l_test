#include <M5Unified.h>
#include "unitc6l_commu.h"

// =============================
//  M5 Unit C6L (ESP32-C6) + SX1262
//  RX (ArduinoIDE / RadioLib)
//  - Polling receive() (no DIO1 interrupt required)
//  - Parses payload: "<8-hex-ID>,<counter>"
// =============================

void setup() {
  int beginState;

  Serial.begin(115200);
  M5.begin();
  beginState = radio.begin(LORA_FREQ_MHZ, LORA_BW_KHZ, LORA_SF, LORA_CR_DENOM,
                           LORA_SYNC_WORD, TX_POWER_DBM, PREAMBLE_LEN,
                           TCXO_VOLT, USE_LDO);

  M5.Lcd.begin();
  M5.Lcd.setTextSize(1);
  M5.Lcd.clear();
  delay(300);

  auto& ioe = M5.getIOExpander(0);
  // reset + RF switch enable (as in your original sketch)
  ioe.digitalWrite(7, false);
  delay(100);
  ioe.digitalWrite(7, true);  // SX_NRST
  ioe.digitalWrite(6, true);  // SX_ANT_SW
  ioe.digitalWrite(5, true);  // SX_LNA_EN

  M5.Lcd.setCursor(0, 0);
  M5.Lcd.printf("[SX1262] Init...\n");
  Serial.println("[SX1262] Initializing...");

  Serial.print("[SX1262] begin() = ");
  Serial.println(beginState);

  if (beginState != RADIOLIB_ERR_NONE) {
    M5.Lcd.printf("Init FAIL: %d\n", beginState);
    Serial.println("[SX1262] init failed. Check wiring/pins and power.");
    while (true) delay(1000);
  }

  Serial.println("before startReceive");
  radio.startReceive();
  Serial.println("after startReceive");


  M5.Lcd.printf("RX Ready\n");
  Serial.println("[RX] Ready.");
}

void loop() {
  String str;
  // receive() is blocking until a packet arrives or timeout occurs.
  // Use a short timeout to keep UI responsive.
  int st = radio.readData(str);
  Serial.printf("startReceive st=%d\n", st);
  if (st == RADIOLIB_ERR_NONE) {
    // Expected format: "<8-hex-ID>,<counter>"
    int comma = str.indexOf(',');
    String idHex = (comma >= 0) ? str.substring(0, comma) : String();
    String cnt = (comma >= 0) ? str.substring(comma + 1) : String();

    float rssi = radio.getRSSI();
    float snr = radio.getSNR();

    M5.Lcd.clear();
    M5.Lcd.setCursor(0, 0);
//    M5.Lcd.printf("[RX] OK\n");
    M5.Lcd.printf("Raw: %s\n", str.c_str());
    M5.Lcd.printf("RSSI: %.1f\n", rssi);
    M5.Lcd.printf("SNR : %.1f\n", snr);

    Serial.printf("[RX] \"%s\" RSSI=%.1f SNR=%.1f\n", str.c_str(), rssi, snr);

    if (isHex8(idHex)) {
      Serial.printf("[RX] ID=%s CNT=%s\n", idHex.c_str(), cnt.c_str());
    } else {
      Serial.println("[RX] WARNING: payload has no valid 32-bit hex ID");
    }

  } else if (st == RADIOLIB_ERR_RX_TIMEOUT) {
    // no packet; do nothing
  } else if (st == RADIOLIB_ERR_CRC_MISMATCH) {
    Serial.println("[RX] CRC mismatch");
  } else {
    Serial.printf("[RX] FAIL code=%d\n", st);
  }
  // small UI refresh pacing
  delay(50);
}
