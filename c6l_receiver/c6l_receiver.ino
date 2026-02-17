#include <M5Unified.h>
#include "unitc6l_commu.h"

UnitC6L_Commu c6l;

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
  delay(300);

  pinMode(PIN_SX_NRST, OUTPUT);
  pinMode(PIN_SX_ANT_SW, OUTPUT);
  pinMode(PIN_SX_LNA_EN, OUTPUT);

  c6l.begin_receive();
}

void loop() {
  String str;
  // receive() is blocking until a packet arrives or timeout occurs.
  // Use a short timeout to keep UI responsive.
  int st = c6l.radio.readData(str);
  if (st == RADIOLIB_ERR_NONE) {
    // Expected format: "<8-hex-ID>,<counter>"
    int comma = str.indexOf(',');
    String idHex = (comma >= 0) ? str.substring(0, comma) : String();
    String cnt = (comma >= 0) ? str.substring(comma + 1) : String();

    float rssi = c6l.radio.getRSSI();
    float snr = c6l.radio.getSNR();

    M5.Lcd.clear();
    M5.Lcd.setCursor(0, 0);
    M5.Lcd.printf("Raw: %s\n", str.c_str());
    M5.Lcd.printf("RSSI: %.1f\n", rssi);
    M5.Lcd.printf("SNR : %.1f\n", snr);

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
