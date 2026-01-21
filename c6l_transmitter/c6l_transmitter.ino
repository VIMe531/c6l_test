#include <RadioLib.h>
#include <M5Unified.h>

// SX1262: CS, IRQ, NRST, BUSY
SX1262 radio = new Module(23, 7, RADIOLIB_NC, 19);

int transmitState = RADIOLIB_ERR_NONE;  // save transmission state between loops
bool transmitFlag = false;              // flag to indicate that a packet was sent
int count = 0;                          // counter of transmitted packets

// ---- Added: regulatory timing constraints ----
static constexpr uint32_t CARRIER_SENSE_TIME_MS = 5;
static constexpr uint32_t MAX_TX_TIME_MS = 4000;
static constexpr uint32_t TX_OFF_TIME_MS = 50;

// ---- Added: random backoff / retry settings ----
static constexpr uint32_t BACKOFF_MIN_MS = 0;
static constexpr uint32_t BACKOFF_MAX_MS = 50;           // random backoff range
static constexpr uint32_t MAX_LBT_RETRY_TIME_MS = 1000;  // total time allowed for LBT retries

// Some RadioLib versions may not define these; provide a fallback.
#ifndef RADIOLIB_ERR_CHANNEL_BUSY
#define RADIOLIB_ERR_CHANNEL_BUSY -711
#endif
#ifndef RADIOLIB_ERR_INVALID_PACKET_LENGTH
#define RADIOLIB_ERR_INVALID_PACKET_LENGTH -305
#endif

#include <math.h>

// LoRa time-on-air calculator (ms) for current fixed config:
// BW=125kHz, SF=12, CR=4/5, preamble=20, explicit header, CRC on
static float calcLoRaToAms(size_t payloadLen) {
  const float bw = 125000.0f;  // Hz
  const int sf = 12;
  const int crDenom = 5;  // 4/5 -> denom=5
  const int preambleLen = 20;

  const int ih = 0;   // explicit header
  const int crc = 1;  // CRC on
  const int de = 1;   // low data rate optimization (SF>=11 & BW=125k)

  // CR in the formula is (denom - 4): 4/5 => 1, 4/6 => 2, 4/7 => 3, 4/8 => 4
  const int cr = crDenom - 4;

  const float ts = (float)(1UL << sf) / bw;  // symbol duration (s)

  // preamble time
  const float tPreamble = (preambleLen + 4.25f) * ts;

  // payload symbols
  const float tmp = (8.0f * payloadLen - 4.0f * sf + 28.0f + 16.0f * crc - 20.0f * ih);
  const float denom = 4.0f * (sf - 2.0f * de);
  const float ceilVal = (tmp > 0.0f) ? ceilf(tmp / denom) : 0.0f;
  const float nPayload = 8.0f + ceilVal * (cr + 4);

  const float tPayload = nPayload * ts;

  return (tPreamble + tPayload) * 1000.0f;  // ms
}

// carrier sense (LBT/CAD): try to confirm channel is free within 5ms
static int carrierSense5ms() {
  uint32_t start = millis();
  while ((millis() - start) < CARRIER_SENSE_TIME_MS) {
    // scanChannel() uses CAD/LBT-like mechanism on SX126x LoRa
    int state = radio.scanChannel();
    if (state == RADIOLIB_ERR_NONE) {
      return RADIOLIB_ERR_NONE;  // free
    }
    if (state != RADIOLIB_ERR_CHANNEL_BUSY) {
      // unexpected error
      return state;
    }
    // busy -> keep trying until 5ms expires
  }
  return RADIOLIB_ERR_CHANNEL_BUSY;
}

static int checkMaxTxTime(const size_t payloadLen) {
  float toaMs = calcLoRaToAms(payloadLen);

  Serial.printf("[ToA] len=%u -> %.2f ms\n", (unsigned)payloadLen, toaMs);
  if (toaMs > (float)MAX_TX_TIME_MS) {
    return RADIOLIB_ERR_INVALID_PACKET_LENGTH;  // -305
  }
  return RADIOLIB_ERR_NONE;
}

// ---- Added: LBT with random backoff and retry ----
static int lbtWithRandomBackoff() {
  uint32_t t0 = millis();
  while ((millis() - t0) < MAX_LBT_RETRY_TIME_MS) {
    int st = carrierSense5ms();
    if (st == RADIOLIB_ERR_NONE) {
      return RADIOLIB_ERR_NONE;  // channel free
    }
    if (st != RADIOLIB_ERR_CHANNEL_BUSY) {
      return st;  // unexpected error
    }

    // BUSY: random backoff then retry
    uint32_t backoff = (uint32_t)random((long)BACKOFF_MIN_MS, (long)BACKOFF_MAX_MS + 1);
    delay(backoff);
  }
  return RADIOLIB_ERR_CHANNEL_BUSY;  // still busy within allowed retry window
}

// wrapper: max TX time + LBT(backoff retry) + startTransmit
static int startTransmitWithReg(String& s) {
  int st = checkMaxTxTime(s.length());
  if (st == RADIOLIB_ERR_NONE) {
    return st;
  }

  st = lbtWithRandomBackoff();
  if (st != RADIOLIB_ERR_NONE) {
    return st;
  }

  return radio.startTransmit(s);
}

// function to be called when a complete packet is transmitted
void setFlag(void) {
  transmitFlag = true;
}

void setup() {
  Serial.begin(115200);
  M5.begin();
  delay(1000);

  // ---- Added: seed random for backoff ----
  randomSeed((uint32_t)micros());

  auto& ioe = M5.getIOExpander(0);
  ioe.digitalWrite(7, false);
  delay(100);
  ioe.digitalWrite(7, true);  // re-enable SX_NRST
  ioe.digitalWrite(6, true);  // enable SX_ANT_SW
  ioe.digitalWrite(5, true);  // enable SX_LNA_EN

  // initialize SX1262
  Serial.print("\n[SX1262] Initializing... ");
  // frequency MHz, bandwidth kHz, spreading factor, coding rate denominator, sync word,
  // output power dBm, preamble length, TCXO reference voltage, useRegulatorLDO
  int beginState = radio.begin(920.6, 125.0, 7, 5, 0x34, 13, 20, 3.0, true);
  if (beginState == RADIOLIB_ERR_NONE) {
    Serial.println("Succeeded!");
  } else {
    Serial.print("Failed, code: ");
    Serial.println(beginState);
    while (true) { delay(100); }
  }

  // set the function to be called when packet transmission is finished
  radio.setPacketSentAction(setFlag);

  // start transmitting the first packet
  Serial.print("[SX1262] Sending the first packet... ");

  // you can transmit C-string or Arduino string up to 256 characters long
  //  transmitState = radio.startTransmit("Hello world from M5Stack! #0");
  // setup() 内
  String first = "#0";
  transmitState = startTransmitWithReg(first);

  Serial.print("transmitState : ");
  Serial.println(transmitState);

  // ---- Added: enforce TX_OFF_TIME_MS even if TX did not start (BUSY / error) ----
  if (transmitState == RADIOLIB_ERR_NONE) {
    transmitFlag = true;  // drive loop() to run finishTransmit + off time
  }
}

void loop() {
  if (transmitFlag) {      // check if the previous transmission is finished
    transmitFlag = false;  // reset the flag

    if (transmitState == RADIOLIB_ERR_NONE) {  // packet was sent successfully
      Serial.println("Succeeded!");
    } else {
      Serial.print("Failed, code: ");
      Serial.println(transmitState);
    }

    // clean up after transmission is finished. This will ensure transmitter is disabled, RF switch is powered down, etc.
    radio.finishTransmit();

    // ---- Added: off time (minimum idle time after TX) - apply even on failure ----
    delay(TX_OFF_TIME_MS);

    delay(1000);

    // send another packet
    count++;
    Serial.printf("[SX1262] Sending packet #%d... ", count);
    //    String str = "Hello world from C6L! #" + String(count);
    String str = "#" + String(count);
    transmitState = startTransmitWithReg(str);

    // ---- Added: if TX didn't start, still enforce finishTransmit + off time ----
    if (transmitState == RADIOLIB_ERR_NONE) {
      transmitFlag = true;
    }
  }
}
