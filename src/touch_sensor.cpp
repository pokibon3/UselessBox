#include "touch_sensor.h"

namespace {
uint8_t gTouchPin = A0;
TouchConfig gConfig;

bool gPressed = false;
bool gInitialized = false;
uint16_t gRaw = 0;
uint16_t gBaseline = 0;

uint16_t measureChargeAdc() {
  // 1) 完全放電
  pinMode(gTouchPin, OUTPUT);
  digitalWrite(gTouchPin, LOW);
  delayMicroseconds(200);

  // 2) 内蔵プルアップで短時間だけ充電
  pinMode(gTouchPin, INPUT_PULLUP);
  delayMicroseconds(35);

  // 3) 電圧をADCで読む
  return static_cast<uint16_t>(analogRead(gTouchPin));
}

uint16_t sampleAverage() {
  uint32_t sum = 0;
  for (uint8_t i = 0; i < gConfig.sampleCount; ++i) {
    sum += measureChargeAdc();
  }
  return static_cast<uint16_t>(sum / gConfig.sampleCount);
}
}  // namespace

void touchInit(uint8_t pin, const TouchConfig& config) {
  gTouchPin = pin;
  gConfig = config;
  gPressed = false;
  gInitialized = false;
  gRaw = 0;
  gBaseline = 0;

  pinMode(gTouchPin, INPUT_PULLUP);
}

void touchUpdate() {
  gRaw = sampleAverage();

  if (!gInitialized) {
    gBaseline = gRaw;
    gInitialized = true;
    return;
  }

  // ADC方式ではタッチ時に raw が下がる想定なので、差分は baseline - raw
  const int16_t delta = static_cast<int16_t>(gBaseline) - static_cast<int16_t>(gRaw);

  if (!gPressed) {
    if (delta >= static_cast<int16_t>(gConfig.onThreshold)) {
      gPressed = true;
      return;
    }
    const int16_t drift = static_cast<int16_t>(gRaw) - static_cast<int16_t>(gBaseline);
    gBaseline += drift >> gConfig.baselineShiftIdle;
    return;
  }

  if (delta <= static_cast<int16_t>(gConfig.offThreshold)) {
    gPressed = false;
  }
  const int16_t drift = static_cast<int16_t>(gRaw) - static_cast<int16_t>(gBaseline);
  gBaseline += drift >> gConfig.baselineShiftPressed;
}

bool touchIsPressed() {
  return gPressed;
}

uint16_t touchRaw() {
  return gRaw;
}

uint16_t touchBaseline() {
  return gBaseline;
}

int16_t touchDelta() {
  return static_cast<int16_t>(gBaseline) - static_cast<int16_t>(gRaw);
}
