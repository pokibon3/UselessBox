#include "VarSpeedServo.h"

volatile int VarSpeedServo::sCurrentDeg = 180;
volatile int VarSpeedServo::sTargetDeg = 180;
volatile uint8_t VarSpeedServo::sSpeed = 0;
volatile bool VarSpeedServo::sMoving = false;
VarSpeedServo* VarSpeedServo::sActive = nullptr;
HardwareTimer* VarSpeedServo::sTimer = nullptr;

namespace {
TIM_TypeDef* pickTimerInstance() {
#if defined(TIM1_BASE)
  return TIM1;
#elif defined(TIM2_BASE)
  return TIM2;
#elif defined(TIM3_BASE)
  return TIM3;
#else
#error "No timer instance available for VarSpeedServo"
#endif
}

int clampDeg(int value) {
  return constrain(value, VarSpeedServo::kMinDeg, VarSpeedServo::kMaxDeg);
}

uint32_t degToPulseUs(int degrees) {
  const int clamped = clampDeg(degrees);
  return static_cast<uint32_t>(
      map(clamped, VarSpeedServo::kMinDeg, VarSpeedServo::kMaxDeg,
          VarSpeedServo::kPulseMinUs, VarSpeedServo::kPulseMaxUs));
}

int speedToStep(uint8_t speed) {
  if (speed == 0) {
    return 255;
  }
  // VarSpeedServo互換のため、1=最遅, 255=最速寄りの単純近似
  return max(1, static_cast<int>((speed + 31) / 32));
}
}  // namespace

VarSpeedServo::VarSpeedServo() = default;

HardwareTimer* VarSpeedServo::ensureTimer() {
  if (sTimer != nullptr) {
    return sTimer;
  }

  sTimer = new HardwareTimer(pickTimerInstance());
  sTimer->setMode(kCompareChannel, TIMER_OUTPUT_COMPARE);
  sTimer->setOverflow(kFrameRateHz, HERTZ_FORMAT);
  sTimer->setCaptureCompare(kCompareChannel, 1500, MICROSEC_COMPARE_FORMAT);
  sTimer->attachInterrupt(onFrame);
  sTimer->attachInterrupt(kCompareChannel, onPulseEnd);
  sTimer->resume();
  return sTimer;
}

uint8_t VarSpeedServo::attach(uint8_t pin) {
  pin_ = pin;
  pinMode(pin_, OUTPUT);
  digitalWrite(pin_, LOW);
  attached_ = true;
  sActive = this;
  ensureTimer();
  return pin_;
}

void VarSpeedServo::detach() {
  if (!attached_) {
    return;
  }
  digitalWrite(pin_, LOW);
  attached_ = false;
  if (sActive == this) {
    sActive = nullptr;
    sMoving = false;
  }
}

void VarSpeedServo::write(int value) {
  write(value, 0, false);
}

void VarSpeedServo::write(int value, uint8_t speed, bool wait) {
  const int target = clampDeg(value);
  sTargetDeg = target;
  sSpeed = speed;
  sMoving = (sCurrentDeg != sTargetDeg);
  if (speed == 0) {
    sCurrentDeg = sTargetDeg;
    sMoving = false;
  }

  if (wait) {
    while (isMoving()) {
      delay(1);
    }
  }
}

void VarSpeedServo::slowmove(int value, uint8_t speed) {
  write(value, speed, false);
}

int VarSpeedServo::read() const {
  return static_cast<int>(sCurrentDeg);
}

bool VarSpeedServo::isMoving() const {
  return sMoving;
}

void VarSpeedServo::stop() {
  sTargetDeg = sCurrentDeg;
  sMoving = false;
}

void VarSpeedServo::onFrame() {
  VarSpeedServo* servo = sActive;
  if (servo == nullptr || !servo->attached_) {
    return;
  }

  if (sCurrentDeg != sTargetDeg) {
    const int step = speedToStep(sSpeed);
    if (sTargetDeg > sCurrentDeg) {
      sCurrentDeg = min(sCurrentDeg + step, static_cast<int>(sTargetDeg));
    } else {
      sCurrentDeg = max(sCurrentDeg - step, static_cast<int>(sTargetDeg));
    }
    sMoving = (sCurrentDeg != sTargetDeg);
  } else {
    sMoving = false;
  }

  const uint32_t pulseUs = degToPulseUs(sCurrentDeg);
  sTimer->setCaptureCompare(kCompareChannel, pulseUs, MICROSEC_COMPARE_FORMAT);
  digitalWrite(servo->pin_, HIGH);
}

void VarSpeedServo::onPulseEnd() {
  VarSpeedServo* servo = sActive;
  if (servo == nullptr || !servo->attached_) {
    return;
  }
  digitalWrite(servo->pin_, LOW);
}
