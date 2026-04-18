#pragma once

#include <Arduino.h>
#include <HardwareTimer.h>

class VarSpeedServo {
 public:
  static constexpr int kMinDeg = 0;
  static constexpr int kMaxDeg = 180;
  static constexpr int kPulseMinUs = 500;
  static constexpr int kPulseMaxUs = 2500;
  static constexpr int kFrameRateHz = 50;
  static constexpr uint32_t kCompareChannel = 1;

  VarSpeedServo();

  uint8_t attach(uint8_t pin);
  void detach();

  void write(int value);
  void write(int value, uint8_t speed, bool wait = false);
  void slowmove(int value, uint8_t speed);

  int read() const;
  bool isMoving() const;
  void stop();

 private:
  static void onFrame();
  static void onPulseEnd();
  static HardwareTimer* ensureTimer();

  static volatile int sCurrentDeg;
  static volatile int sTargetDeg;
  static volatile uint8_t sSpeed;
  static volatile bool sMoving;
  static VarSpeedServo* sActive;
  static HardwareTimer* sTimer;

  uint8_t pin_ = 0;
  bool attached_ = false;
};
