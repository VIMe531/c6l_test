#include <RadioLib.h>

// SX1262: CS, IRQ, NRST, BUSY  (Module(cs, dio1, rst, busy))
SX1262 radio = new Module(23, 7, RADIOLIB_NC, 19);

// -----------------------------
// LoRa radio configuration (MUST match RX)
// -----------------------------
static constexpr float LORA_FREQ_MHZ = 920.6f;
static constexpr float LORA_BW_KHZ = 125.0f;
static constexpr uint8_t LORA_SF = 7;
static constexpr uint8_t LORA_CR_DENOM = 5;  // 4/5 -> denom=5
static constexpr uint8_t LORA_SYNC_WORD = 0x34;
static constexpr int8_t TX_POWER_DBM = 13;  // keep conservative (do NOT raise without compliance check)
static constexpr uint16_t PREAMBLE_LEN = 20;
static constexpr float TCXO_VOLT = 3.0f;
static constexpr bool USE_LDO = true;

// -----------------------------
// Regulatory-minded timing constraints (per ARIB STD-T108 intent)
// -----------------------------
static constexpr uint32_t CARRIER_SENSE_TIME_MS = 5;  // >=5 ms
static constexpr float CS_THRESHOLD_DBM = -80.0f;     // busy if RSSI >= -80 dBm
static constexpr uint32_t MAX_TX_TIME_MS = 4000;      // <=4 s
static constexpr uint32_t TX_OFF_TIME_MS = 50;        // >=50 ms

// random backoff / retry settings
static constexpr uint32_t BACKOFF_MIN_MS = 0;
static constexpr uint32_t BACKOFF_MAX_MS = 50;           // random backoff range
static constexpr uint32_t MAX_LBT_RETRY_TIME_MS = 1000;  // total retry window

#ifndef RADIOLIB_ERR_CHANNEL_BUSY
#define RADIOLIB_ERR_CHANNEL_BUSY -711
#endif
#ifndef RADIOLIB_ERR_INVALID_PACKET_LENGTH
#define RADIOLIB_ERR_INVALID_PACKET_LENGTH -305
#endif
#ifndef RADIOLIB_ERR_RX_TIMEOUT
#define RADIOLIB_ERR_RX_TIMEOUT -706
#endif

// -----------------------------
// Helpers
// -----------------------------
static uint32_t deviceId32() {
  // ESP32-C6: eFuse MAC is 48-bit.
  uint64_t mac = ESP.getEfuseMac();
  return (uint32_t)(mac >> 16);  // upper 32 bits
}

static bool isHex8(const String& s) {
  if (s.length() != 8) return false;
  for (size_t i = 0; i < 8; i++) {
    char c = s[i];
    bool ok = (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f');
    if (!ok) return false;
  }
  return true;
}

// LoRa time-on-air calculator (ms) for our fixed config.
// Formula based on Semtech LoRa ToA model (explicit header, CRC on).
static float calcLoRaToAms(size_t payloadLen) {
  const float bw = LORA_BW_KHZ * 1000.0f;  // Hz
  const int sf = (int)LORA_SF;

  const int ih = 0;   // explicit header
  const int crc = 1;  // CRC on

  // low data rate optimization: typically enabled for SF>=11 at BW=125kHz
  const int de = (sf >= 11 && (int)LORA_BW_KHZ <= 125) ? 1 : 0;

  // CR value in formula is (CR_denom - 4)
  const int cr = (int)LORA_CR_DENOM - 4;

  const float tSym = (float)(1UL << sf) / bw;  // seconds

  const float tPreamble = (PREAMBLE_LEN + 4.25f) * tSym;

  const float tmp = (8.0f * (float)payloadLen - 4.0f * sf + 28.0f + 16.0f * crc - 20.0f * ih);
  const float denom = 4.0f * (sf - 2.0f * de);
  const float nPayload = 8.0f + (denom > 0 ? (ceilf(tmp / denom) * (cr + 4.0f)) : 0.0f);

  const float tPayload = nPayload * tSym;

  return (tPreamble + tPayload) * 1000.0f;  // ms
}

static int checkMaxTxTime(size_t payloadLen) {
  float toa = calcLoRaToAms(payloadLen);
  if (toa > (float)MAX_TX_TIME_MS) {
    return RADIOLIB_ERR_INVALID_PACKET_LENGTH;
  }
  return RADIOLIB_ERR_NONE;
}

static inline void forceStandbyAndClearIrq() {
  radio.standby();
  // RadioLib(SX126x)で提供されることが多いIRQクリア
  // radio.clearIrqStatus();
  delay(1);
}

static inline int carrierSense5ms_cad() {
  uint32_t start = millis();
  while (millis() - start < CARRIER_SENSE_TIME_MS) {
    int st = radio.scanChannel();
    if (st == RADIOLIB_ERR_NONE) {
      return RADIOLIB_ERR_NONE;      // free
    }
    if (st != RADIOLIB_ERR_CHANNEL_BUSY) {
      return st;                     // error
    }
    delay(1);
  }
  return RADIOLIB_ERR_CHANNEL_BUSY;  // busy
}

static inline int lbtWithRandomBackoff() {
  const uint32_t t0 = millis();

  while ((millis() - t0) < MAX_LBT_RETRY_TIME_MS) {
    int st = carrierSense5ms_cad();
    if (st == RADIOLIB_ERR_NONE) {
      return RADIOLIB_ERR_NONE;
    }
    if (st != RADIOLIB_ERR_CHANNEL_BUSY) {
      return st;
    }

    const uint32_t backoff = (uint32_t)random(BACKOFF_MIN_MS, BACKOFF_MAX_MS + 1);
    delay(backoff);
  }
  return RADIOLIB_ERR_CHANNEL_BUSY;
}

// =============================
// ToA wait helpers
// =============================

// ToA[ms] + 余裕分を待機する（delayを刻んでWDT回避）
// extra_margin_ms : 計算誤差や処理遅延分の余裕
// slice_delay_ms  : 1回あたりのdelay刻み（小さいほど応答性が上がる）
static inline void waitForToAms(size_t payloadLen,
                               uint32_t extra_margin_ms = 20,
                               uint32_t slice_delay_ms = 10) {
  float toa_ms_f = calcLoRaToAms(payloadLen);
  uint32_t wait_ms = (uint32_t)ceilf(toa_ms_f) + extra_margin_ms;

  Serial.print("toa [ms]: ");
  Serial.println(toa_ms_f);

  uint32_t start = millis();
  while ((millis() - start) < wait_ms) {
    delay(slice_delay_ms);
    yield();  // ESP系でのタスク切替・WDT回避
  }
}

// 送信後の「最低休止時間（例：50ms）」を保証する
static inline void enforceTxOffTime(uint32_t off_time_ms = TX_OFF_TIME_MS) {
  uint32_t start = millis();
  while ((millis() - start) < off_time_ms) {
    delay(1);
    yield();
  }
}

// =============================
// transmit + ToA wait
// =============================
//
// RadioLibがTX_DONE IRQを取りこぼすと TX_TIMEOUT が出ることがあるため、
// 「ToA分待つ」ことで実際の送信時間を担保する運用を支援する関数。
// 返り値は radio.transmit() の戻り値をそのまま返す。
//
// - always_wait_toa: trueなら transmit() 成否に関わらず ToA待ちする
//                    falseなら st==RADIOLIB_ERR_NONE のときだけ待つ
// - ignore_tx_timeout: trueなら TX_TIMEOUT を警告扱いにして成功(=0)に丸める
//
static inline int transmitWithRegAndToAWait(const String& payload,
                                           bool always_wait_toa = true,
                                           bool ignore_tx_timeout = true,
                                           uint32_t extra_margin_ms = 20) {
  String tmp = payload;

  // 1) max TX time check (<=4s)
  int st = checkMaxTxTime(tmp.length());
  if (st != RADIOLIB_ERR_NONE) return st;

  // 2) 送信前に一旦Standbyへ（状態を整える）
  radio.standby();

  Serial.println("TX: about to transmit");
  int ret = radio.transmit(tmp);
  Serial.println("TX: transmit returned");

  // 3) ToA待機（IRQに依存しない送信時間担保）
  if (always_wait_toa || ret == RADIOLIB_ERR_NONE) {
    waitForToAms(tmp.length(), extra_margin_ms, 10);
  }

  // 4) 送信後オフタイム（>=50ms）を保証
  enforceTxOffTime(TX_OFF_TIME_MS);

  // 5) TX_TIMEOUTを「取りこぼし」とみなす運用（受信側で届いていればOK）
  if (ignore_tx_timeout && ret == RADIOLIB_ERR_TX_TIMEOUT) {
    Serial.println("[TX] WARN: TX_TIMEOUT ignored (treated as OK after ToA wait)");
    return RADIOLIB_ERR_NONE;
  }

  return ret;
}

static const char* radiolibErrToStr(int st) {
  switch (st) {
    case RADIOLIB_ERR_NONE: return "RADIOLIB_ERR_NONE";
    case RADIOLIB_ERR_CHIP_NOT_FOUND: return "RADIOLIB_ERR_CHIP_NOT_FOUND";
    case RADIOLIB_ERR_PACKET_TOO_LONG: return "RADIOLIB_ERR_PACKET_TOO_LONG";
    case RADIOLIB_ERR_TX_TIMEOUT: return "RADIOLIB_ERR_TX_TIMEOUT";
    case RADIOLIB_ERR_RX_TIMEOUT: return "RADIOLIB_ERR_RX_TIMEOUT";
    case RADIOLIB_ERR_CRC_MISMATCH: return "RADIOLIB_ERR_CRC_MISMATCH";
#ifdef RADIOLIB_ERR_CHANNEL_BUSY
    case RADIOLIB_ERR_CHANNEL_BUSY: return "RADIOLIB_ERR_CHANNEL_BUSY";
#endif
    default: return "UNKNOWN";
  }
}
