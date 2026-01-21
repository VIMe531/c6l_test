#include <RadioLib.h>
#include <M5Unified.h>

// SX1262: CS, IRQ, NRST, BUSY
SX1262 radio = new Module(23, 7, RADIOLIB_NC, 19);

bool receiveFlag = false;  // flag to indicate that a packet was received

// function to be called when a complete packet is received
void setFlag(void) {
  receiveFlag = true;
}

void setup() {
  Serial.begin(115200);
  M5.begin();
  M5.Lcd.begin();
  M5.Lcd.setTextSize(1);
  M5.Lcd.clear();
  delay(1000);

  auto& ioe = M5.getIOExpander(0);

  // ---- Modified: reset sequence stabilized (same style as TX) ----
  pinMode(7, OUTPUT);  // SX_NRST
  pinMode(6, OUTPUT);  // SX_ANT_SW
  pinMode(5, OUTPUT);  // SX_LNA_EN

  ioe.digitalWrite(7, false);
  delay(100);
  ioe.digitalWrite(7, true);  // re-enable SX_NRST
  ioe.digitalWrite(6, true);  // enable SX_ANT_SW
  ioe.digitalWrite(5, true);  // enable SX_LNA_EN

  // initialize SX1262
  M5.Lcd.setCursor(0, 10);
  M5.Lcd.printf("[SX1262] Initializing... ");
  Serial.println("test point Init");
  while (true) {
    Serial.println("test point while");
    // frequency MHz, bandwidth kHz, spreading factor, coding rate denominator, sync word,
    // output power dBm, preamble length, TCXO reference voltage, useRegulatorLDO
    int beginState = radio.begin(920.6, 125.0, 7, 5, 0x34, 13, 20, 3.0, true);

    Serial.print("begin State : ");
    Serial.println(beginState);

    if (beginState == RADIOLIB_ERR_NONE) {
      M5.Lcd.clear();
      M5.Lcd.setCursor(0, 10);
      M5.Lcd.printf("[SX1262]\nSucceeded! ");
      break;
    } else {
      M5.Lcd.clear();
      M5.Lcd.setCursor(0, 10);
      M5.Lcd.printf("[SX1262]\nFailed: %d", beginState);
      delay(1000);
    }
  }

  // set the function to be called when a new packet is received
  radio.setPacketReceivedAction(setFlag);

  // start listening for LoRa packets
  M5.Lcd.clear();
  M5.Lcd.setCursor(0, 10);
  M5.Lcd.printf("[SX1262]\nStarting to listen... ");

  int receiveState = radio.startReceive();
  if (receiveState == RADIOLIB_ERR_NONE) {
    M5.Lcd.clear();
    M5.Lcd.setCursor(0, 10);
    M5.Lcd.printf("[SX1262]\nListening...");
  } else {
    M5.Lcd.clear();
    M5.Lcd.setCursor(0, 10);
    M5.Lcd.printf("[SX1262]\nFailed: %d", receiveState);
    while (true) { delay(100); }
  }
}

void loop() {
  Serial.println(receiveFlag);
  if (receiveFlag) {      // check if a new packet is received
    receiveFlag = false;  // reset the flag

    String str;  // read the received data as an Arduino String
    int readState = radio.readData(str);

    if (readState == RADIOLIB_ERR_NONE) {  // packet was received successfully
      M5.Lcd.clear();
      M5.Lcd.setCursor(0, 0);
      M5.Lcd.printf("[SX1262] RX OK\n");

      M5.Lcd.printf("Data: %s\n", str.c_str());
/*      M5.Lcd.printf("RSSI: %.1f dBm\n", radio.getRSSI());
      M5.Lcd.printf("SNR : %.1f dB\n",  radio.getSNR());
      M5.Lcd.printf("FreqErr: %.0f Hz\n", radio.getFrequencyError());

    } else if (readState == RADIOLIB_ERR_CRC_MISMATCH) {  // packet was received but malformed
      M5.Lcd.clear();
      M5.Lcd.setCursor(0, 0);
      M5.Lcd.printf("[SX1262] CRC error!\n");
      M5.Lcd.printf("RSSI: %.1f dBm\n", radio.getRSSI());
      M5.Lcd.printf("SNR : %.1f dB\n",  radio.getSNR());

    } else {  // some other error occurred
      M5.Lcd.clear();
      M5.Lcd.setCursor(0, 0);
      M5.Lcd.printf("[SX1262] RX Failed\n");
      M5.Lcd.printf("code: %d\n", readState);
  */  }

// ---- Added: receive restart ----
int st = radio.startReceive();
if (st != RADIOLIB_ERR_NONE) {
  // 受信再開に失敗した場合も表示（ただし停止はしない）
  M5.Lcd.printf("restart RX err:%d\n", st);
}
  }
}
