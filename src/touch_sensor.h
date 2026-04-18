#pragma once

#include <Arduino.h>

struct TouchConfig {
  uint16_t onThreshold = 28;
  uint16_t offThreshold = 12;
  uint8_t baselineShiftIdle = 4;
  uint8_t baselineShiftPressed = 7;
  uint8_t sampleCount = 8;
  uint16_t maxCount = 600;
};

void touchInit(uint8_t pin, const TouchConfig& config = TouchConfig());
void touchUpdate();
bool touchIsPressed();
uint16_t touchRaw();
uint16_t touchBaseline();
int16_t touchDelta();
