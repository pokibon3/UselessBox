#include <Arduino.h>
#include "VarSpeedServo.h"
#include "touch_sensor.h"
#include "ch32v00x_pwr.h"
#include "ch32v00x_rcc.h"

#define LED_PIN 2

#define SERVO_PIN  3  // サーボ制御ピン（配線に合わせて変更）
#define SWITCH_PIN 4  // トグルスイッチ入力ピン（配線に合わせて変更）
#define TOUCH_PIN  A0 // タッチ電極入力ピン
#define TOUCH_ACTIVE_RAW_THRESHOLD 300
#define SWITCH_DEBOUNCE_MS 30
#define TOUCH_RELEASE_STABLE_MS 150
#define TOUCH_RELEASE_RESET_MS 250
#define FIRE_RANDOM_MIN_MS 100
#define FIRE_RANDOM_MAX_MS 3000
#define FIRE_MAX_ATTEMPTS 3
#define FEINT_INTERVAL_MIN_MS 300
#define FEINT_INTERVAL_MAX_MS 3000
#define FEINT_RETURN_MIN_MS 60
#define FEINT_RETURN_MAX_MS 3000
#define SERVO_REST_ANGLE 172
#define SERVO_ATTACK_ANGLE 115
#define FEINT_RATIO_MIN_PERCENT 52
#define FEINT_RATIO_MAX_PERCENT 72
                      // INPUT_PULLUP使用: LOW=ON, HIGH=OFF

VarSpeedServo servo;

void onSwitchOn() {
  // 割り込みハンドラ: STANDBYから復帰後に呼ばれる
}

void doFeint() {
  const int fullStroke = SERVO_REST_ANGLE - SERVO_ATTACK_ANGLE;
  const long ratio = random(FEINT_RATIO_MIN_PERCENT, FEINT_RATIO_MAX_PERCENT + 1);
  const int feintStroke = (fullStroke * static_cast<int>(ratio)) / 100;
  const int feintTarget = SERVO_REST_ANGLE - feintStroke;
  const uint32_t returnDelayMs = static_cast<uint32_t>(
      random(FEINT_RETURN_MIN_MS, FEINT_RETURN_MAX_MS + 1));

  servo.write(feintTarget, 255, true);
  delay(returnDelayMs);
  servo.write(SERVO_REST_ANGLE, 255, true);
  delay(40);
}

uint32_t nextFeintDelayMs() {
  return static_cast<uint32_t>(
      random(FEINT_INTERVAL_MIN_MS, FEINT_INTERVAL_MAX_MS + 1));
}

// サーボを0度からtoDegまで少しずつ動かしながらスイッチを監視する
// スイッチがOFFになったらその場で止まる
void sweepUntilSwitchOff(int toDeg) {
  for (int deg = 0; deg <= toDeg; deg += 2) {
    servo.write(deg, 72, false);
    delay(40);
    if (digitalRead(SWITCH_PIN) == HIGH || touchIsPressed()) {
      // スイッチがOFF、またはタッチ検出中は停止
      return;
    }
  }
  // 最大角度に達してもスイッチがOFFにならない場合はそのまま待機
  while (digitalRead(SWITCH_PIN) == LOW) {
    delay(20);
  }
}

void goToSleep() {
  digitalWrite(LED_PIN, LOW);

  // PWRクロックを有効化してSTANDBYモードへ
  // 復帰時はリセット扱いになるため setup() から再開する
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);
  PWR_EnterSTANDBYMode(PWR_STANDBYEntry_WFI);
}

void setup() {
  pinMode(LED_PIN, OUTPUT);
  pinMode(SWITCH_PIN, INPUT_PULLUP);
  servo.attach(SERVO_PIN);

  TouchConfig touchConfig;
  touchConfig.onThreshold = 20;
  touchConfig.offThreshold = 8;
  touchConfig.sampleCount = 24;
  touchConfig.maxCount = 1200;
  touchInit(TOUCH_PIN, touchConfig);

  // EXTI割り込みを設定（STANDBYからの復帰トリガー）
  // STANDBYはリセット扱いなので setup() で一度設定すれば十分
  // CH32V003固有シグネチャ: (pin, GPIOMode, callback, EXTIMode, EXTITrigger)
  attachInterrupt(digitalPinToInterrupt(SWITCH_PIN), GPIO_Mode_IPU, onSwitchOn, EXTI_Mode_Interrupt, EXTI_Trigger_Falling);

  // 初期位置: 180度
  servo.write(SERVO_REST_ANGLE, 0, false);
  delay(1000);

  // タッチ基準値を初期化
  for (uint8_t i = 0; i < 120; ++i) {
    touchUpdate();
    delay(2);
  }
  randomSeed(micros() ^ touchRaw());

  // 起動時にスイッチがすでにONなら即動作、OFFならスリープ
  if (digitalRead(SWITCH_PIN) == HIGH) {
//    goToSleep();
  }
}

void loop() {
  touchUpdate();

  const uint16_t raw = touchRaw();
  const bool touchActive = (raw >= TOUCH_ACTIVE_RAW_THRESHOLD);
  const bool switchOnRaw = (digitalRead(SWITCH_PIN) == LOW);

  static bool switchOnStable = false;
  static bool lastSwitchOnRaw = false;
  static uint32_t switchDebounceStartMs = 0;

  if (switchOnRaw != lastSwitchOnRaw) {
    lastSwitchOnRaw = switchOnRaw;
    switchDebounceStartMs = millis();
  } else if (switchOnStable != switchOnRaw &&
             (millis() - switchDebounceStartMs >= SWITCH_DEBOUNCE_MS)) {
    switchOnStable = switchOnRaw;
  }

  const bool switchOn = switchOnStable;
  digitalWrite(LED_PIN, touchActive ? HIGH : LOW);

  static bool pendingRun = false;
  static bool switchHandled = false;
  static uint32_t nonTouchStartMs = 0;
  static uint32_t fireAtMs = 0;
  static uint32_t feintAtMs = 0;
  static uint32_t touchReleaseStartMs = 0;
  static uint8_t fireAttempts = 0;

  // SWがONなら一度だけ起動する。立ち上がり瞬間を取り逃がしても、
  // ONが続いている限り次のループで拾えるようにする。
  if (switchOn && !switchHandled) {
    pendingRun = true;
    switchHandled = true;
    nonTouchStartMs = 0;
    fireAtMs = millis() + static_cast<uint32_t>(
        random(FIRE_RANDOM_MIN_MS, FIRE_RANDOM_MAX_MS + 1));
    feintAtMs = 0;
    touchReleaseStartMs = 0;
    fireAttempts = 0;
  }

  // SW ON中かつ保留中:
  // タッチが連続して外れている時間が一定以上になったら実行
  if (switchOn && pendingRun) {
    if (touchActive) {
      nonTouchStartMs = 0;
      touchReleaseStartMs = 0;

      if (feintAtMs == 0) {
        feintAtMs = millis() + nextFeintDelayMs();
      }

      if (feintAtMs != 0 && static_cast<int32_t>(millis() - feintAtMs) >= 0) {
        doFeint();
        feintAtMs = millis() + nextFeintDelayMs();
      }
    } else {
      if (touchReleaseStartMs == 0) {
        touchReleaseStartMs = millis();
      } else if (millis() - touchReleaseStartMs >= TOUCH_RELEASE_RESET_MS) {
        feintAtMs = 0;
      }

      if (nonTouchStartMs == 0) {
        nonTouchStartMs = millis();
      }

      const bool touchReleasedStable =
          (nonTouchStartMs != 0) &&
          (millis() - nonTouchStartMs >= TOUCH_RELEASE_STABLE_MS);

      if (touchReleasedStable &&
          fireAtMs != 0 &&
          static_cast<int32_t>(millis() - fireAtMs) >= 0) {
        ++fireAttempts;
        servo.write(SERVO_ATTACK_ANGLE, 255, true);
        servo.write(SERVO_REST_ANGLE, 255, true);
        fireAtMs = 0;

        if (digitalRead(SWITCH_PIN) == HIGH) {
          pendingRun = false;
        } else if (fireAttempts >= FIRE_MAX_ATTEMPTS) {
          pendingRun = false;
        } else {
          nonTouchStartMs = millis();
          fireAtMs = millis() + static_cast<uint32_t>(
              random(FIRE_RANDOM_MIN_MS, FIRE_RANDOM_MAX_MS + 1));
        }
      }
    }
  }

  // SW OFFで保留状態をクリア
  if (!switchOn) {
    pendingRun = false;
    switchHandled = false;
    nonTouchStartMs = 0;
    fireAtMs = 0;
    feintAtMs = 0;
    touchReleaseStartMs = 0;
    fireAttempts = 0;
  }

  // ディープスリープへ
 // goToSleep();
}
