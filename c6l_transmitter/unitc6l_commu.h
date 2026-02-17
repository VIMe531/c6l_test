#include <M5Unified.h>
#include <RadioLib.h>

// -----------------------------
// LoRa radio configuration (MUST match RX)
// -----------------------------
static constexpr float LORA_FREQ_MHZ = 921.6f;
static constexpr float LORA_BW_KHZ = 125.0f;
static constexpr uint8_t LORA_SF = 7;
static constexpr uint8_t LORA_CR_DENOM = 5;  // 4/5 -> denom=5
static constexpr uint8_t LORA_SYNC_WORD = 0x34;
static constexpr int8_t TX_POWER_DBM = 13;  // 送信出力，日本国内では13 dBm以下
static constexpr uint16_t PREAMBLE_LEN = 20;
static constexpr float TCXO_VOLT = 3.0f;
static constexpr bool USE_LDO = true;

// -----------------------------
// Regulatory-minded timing constraints (per ARIB STD-T108 intent)
// -----------------------------
static constexpr uint32_t CARRIER_SENSE_TIME_MS = 5;  // キャリアセンス時間：5 ms以上
static constexpr float CS_THRESHOLD_DBM = -80.0f;     // キャリアセンスレベル：-80 dBm
static constexpr uint32_t MAX_TX_TIME_MS = 4000;      // 最大送信時間：920.5MHz~923.5 MHzでは4 s，923.5~928.1 MHzでは400 ms
static constexpr uint32_t TX_OFF_TIME_MS = 50;        // 送信後の休止時間：920.5MHz~923.5 MHzでは50 ms以上，923.5~928.1 MHzでは2 ms以上

// random backoff / retry settings
static constexpr uint32_t BACKOFF_MIN_MS = 0;
static constexpr uint32_t BACKOFF_MAX_MS = 50;          // random backoff range
static constexpr uint32_t MAX_LBT_RETRY_TIME_MS = 100;  // total retry window

static constexpr int PIN_SX_NRST = 7;
static constexpr int PIN_SX_ANT_SW = 6;
static constexpr int PIN_SX_LNA_EN = 5;

#ifndef RADIOLIB_ERR_CHANNEL_BUSY
#define RADIOLIB_ERR_CHANNEL_BUSY -711
#endif
#ifndef RADIOLIB_ERR_INVALID_PACKET_LENGTH
#define RADIOLIB_ERR_INVALID_PACKET_LENGTH -305
#endif
#ifndef RADIOLIB_ERR_RX_TIMEOUT
#define RADIOLIB_ERR_RX_TIMEOUT -706
#endif

class UnitC6L_Commu {
public:
  // SX1262: CS, IRQ, NRST, BUSY  (Module(cs, dio1, rst, busy))
  SX1262 radio;
public:
  UnitC6L_Commu()
    : radio(new Module(23, 7, RADIOLIB_NC, 19)) {}

  void begin_transmit() {
    auto& ioe = M5.getIOExpander(0);

    ioe.digitalWrite(7, false);
    delay(100);
    ioe.digitalWrite(7, true);  // SX_NRST
    ioe.digitalWrite(6, true);  // SX_ANT_SW
    ioe.digitalWrite(5, true);  // SX_LNA_EN
    delay(100);

    Serial.printf("BUSY(before reset)=%d\n", digitalRead(19));

    int beginState = radio.begin(
      LORA_FREQ_MHZ, LORA_BW_KHZ, LORA_SF, LORA_CR_DENOM,
      LORA_SYNC_WORD, TX_POWER_DBM, PREAMBLE_LEN,
      TCXO_VOLT, USE_LDO);

    M5.Lcd.begin();
    M5.Lcd.setTextSize(1);
    M5.Lcd.clear();
    M5.Lcd.setCursor(0, 0);
    M5.Lcd.printf("[SX1262] begin() = ");
    M5.Lcd.printf("%d \n", beginState);

    if (beginState != RADIOLIB_ERR_NONE) {
      M5.Lcd.setCursor(0, 0);
      M5.Lcd.printf("[SX1262] init failed. Check pins.\n");
      while (true) delay(1000);
    }

    radio.standby();

    M5.Lcd.clear();
    M5.Lcd.setCursor(0, 0);
    M5.Lcd.printf("TX Ready\n");
    Serial.println("[TX] Ready.");
  }

  void begin_receive() {
    auto& ioe = M5.getIOExpander(0);

    ioe.digitalWrite(7, false);
    delay(100);
    ioe.digitalWrite(7, true);  // SX_NRST
    ioe.digitalWrite(6, true);  // SX_ANT_SW
    ioe.digitalWrite(5, true);  // SX_LNA_EN
    delay(100);

    Serial.printf("BUSY(before reset)=%d\n", digitalRead(19));

    int beginState = radio.begin(
      LORA_FREQ_MHZ, LORA_BW_KHZ, LORA_SF, LORA_CR_DENOM,
      LORA_SYNC_WORD, TX_POWER_DBM, PREAMBLE_LEN,
      TCXO_VOLT, USE_LDO);

    M5.Lcd.begin();
    M5.Lcd.setTextSize(1);
    M5.Lcd.clear();
    M5.Lcd.setCursor(0, 0);
    M5.Lcd.printf("[SX1262] begin() = ");
    M5.Lcd.printf("%d \n", beginState);

    if (beginState != RADIOLIB_ERR_NONE) {
      M5.Lcd.setCursor(0, 0);
      M5.Lcd.printf("[SX1262] init failed. Check pins.\n");
      while (true) delay(1000);
    }

    radio.startReceive();

    M5.Lcd.clear();
    M5.Lcd.setCursor(0, 0);
    M5.Lcd.printf("RX Ready\n");
    Serial.println("[RX] Ready.");
  }
  // ------------------------------------------
  // const char* radiolibErrToStr( int )
  //
  // エラーメッセージを返すための関数
  //
  // 入力：エラーコード（int）
  // 出力：エラーメッセージ（char*）
  // ------------------------------------------
  const char* radiolibErrToStr(int st) {
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

  // ---------------------------------------
  // float calcLoRaToAms( size_t )
  //
  // LoRaパケットの通信時間（ToA）を計算する関数
  //
  // 入力：パケットの長さ（size_t）
  // 出力：通信時間 [ms]（float）
  // ---------------------------------------
  float calcLoRaToAms(size_t payloadLen) {
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

  // ------------------------------------------
  // int checkMaxTxTime( size_t )
  //
  // ToAが最大通信時間を超えないか確認する関数
  // 中でcalcLoRaToAms( size_t )を呼び出している
  //
  // 入力：通信パケットのサイズ（size_t）
  // 出力：エラーコード（int）
  // ------------------------------------------
  int checkMaxTxTime(size_t payloadLen) {
    float toa = this->calcLoRaToAms(payloadLen);
    if (toa > (float)MAX_TX_TIME_MS) {
      return RADIOLIB_ERR_INVALID_PACKET_LENGTH;
    }
    return RADIOLIB_ERR_NONE;
  }

  // ------------------------------------
  // int carrierSense()
  //
  // キャリアセンスを行う関数
  // RSSIを観測して，閾値以上ならBUSY
  //
  // 入力：無し
  // 出力：エラーコード（int）
  // ------------------------------------
  int carrierSense() {
    uint32_t start = millis();
    float maxRssi = -200.0f;

    // 受信開始（失敗したらBUSY扱い）
    int st = radio.startReceive();
    if (st != RADIOLIB_ERR_NONE) {
      radio.standby();
      delay(1);
      return RADIOLIB_ERR_CHANNEL_BUSY;
    }

    // 規定時間だけRSSIを観測
    while (millis() - start < CARRIER_SENSE_TIME_MS) {
      float rssi = radio.getRSSI();
      if (rssi > maxRssi) maxRssi = rssi;
      delay(1);
    }

    radio.standby();
    delay(1);

    // RSSI閾値でBUSY判定
    if (maxRssi >= CS_THRESHOLD_DBM) {
      return RADIOLIB_ERR_CHANNEL_BUSY;
    }
    return RADIOLIB_ERR_NONE;
  }

  // --------------------------------------------------
  // int lbtWithRandomBackoff()
  //
  // チャンネルが混んでいる場合の送信処理を行う関数
  // 混んでいる場合は，ランダムバックオフしながら再試行する
  // 最大時間までに空かなければ，BUSY扱いで終了する
  //
  // 入力：無し
  // 出力：エラーコード（int）
  // --------------------------------------------------
  int lbtWithRandomBackoff() {
    const uint32_t t0 = millis();

    while ((millis() - t0) < MAX_LBT_RETRY_TIME_MS) {
      int st = this->carrierSense();
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

  // ------------------------------------------------------------------
  // void waitForToAms( size_t , uint32_t , uint32_t )
  //
  // calcLoRaToAms( size_t )で計算したToA [ms]だけ待機する関数
  //
  // 入力：パケットのサイズ，追加待機時間 [ms]，ループ1回辺りの待機時間 [ms]
  // 出力：無し
  // ------------------------------------------------------------------
  void waitForToAms(size_t payloadLen, uint32_t extra_margin_ms, uint32_t slice_delay_ms) {
    float toa_ms_f = this->calcLoRaToAms(payloadLen);
    uint32_t wait_ms = (uint32_t)ceilf(toa_ms_f) + extra_margin_ms;

    //    Serial.print("toa [ms]: ");
    //    Serial.println(toa_ms_f);

    uint32_t start = millis();
    while ((millis() - start) < wait_ms) {
      delay(slice_delay_ms);
      yield();  // ESP系でのタスク切替・WDT回避
    }
  }

  // -----------------------------------
  // void TxIntervals( uint32_t )
  //
  // 設定した休止時間分，送信を休止する関数
  //
  // 入力：休止時間 [ms]（uint32_t）
  // 出力：無し
  // -----------------------------------
  void TxIntervals(uint32_t off_time_ms = TX_OFF_TIME_MS) {
    uint32_t start = millis();
    while ((millis() - start) < off_time_ms) {
      delay(1);
      yield();
    }
  }

  // --------------------------------------------------------------------------------------------------------------------
  // int transmitLoRa( const String&, bool always_wait_toa, bool ignore_tx_timeout, uint32_t )
  //
  // 一連の通信動作を行う関数
  //
  // 入力：パケット（const String&），追加待機時間（uint32_t）
  // 出力：エラーコード（ int）
  // --------------------------------------------------------------------------------------------------------------------
  int transmitLoRa(const String& payload, uint32_t extra_margin_ms) {
    String tmp = payload;

    // 1) 送りたいパケットの送信に必要な時間が最大送信時間を超えていないか確認
    int st = this->checkMaxTxTime(tmp.length());
    if (st != RADIOLIB_ERR_NONE) return st;

    // 2) チャンネルが空いているか確認
    radio.standby();
    st = this->lbtWithRandomBackoff();
    if (st != RADIOLIB_ERR_NONE) {
      // busy のときは送らずに戻る
      Serial.println(radiolibErrToStr(st));
      return st;
    }
    radio.standby();
    // 3) 送信
    int ret = radio.transmit(tmp);

    // 4) ToA分待機，送信失敗ならそのまま 5)へ
    if (ret == RADIOLIB_ERR_NONE || ret == RADIOLIB_ERR_TX_TIMEOUT) {
      this->waitForToAms(tmp.length(), extra_margin_ms, 10);
    }

    // 5) 送信後の休止時間
    this->TxIntervals(TX_OFF_TIME_MS);

    // 6) TX_TIMEOUTのとき，エラーメッセージを表示して次へ
    if (ret == RADIOLIB_ERR_TX_TIMEOUT) {
      M5.Lcd.setCursor(0, 0);
      //      M5.Lcd.printf("WARN: TX_TIMEOUT\n");
      return RADIOLIB_ERR_NONE;
    }

    return ret;
  }
};
