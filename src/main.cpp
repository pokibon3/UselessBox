#include <Arduino.h>
#include "ch32v00x_pwr.h"
#include "ch32v00x_rcc.h"


#define LED_BUILTIN 2

#define SERVO_PIN  3  // サーボ制御ピン（配線に合わせて変更）
#define SWITCH_PIN 4  // トグルスイッチ入力ピン（配線に合わせて変更）
                      // INPUT_PULLUP使用: LOW=ON, HIGH=OFF

#define SERVO_MAX_DEG 180  // サーボの最大角度

void onSwitchOn() {
  // 割り込みハンドラ: STANDBYから復帰後に呼ばれる
}

// サーボにPWMパルスを指定時間送り続ける
void servoWrite(int pin, int degrees, unsigned long durationMs) {
  int pulseWidth = map(degrees, 0, SERVO_MAX_DEG, 600, 2500);
  unsigned long start = millis();
  while (millis() - start < durationMs) {
    digitalWrite(pin, HIGH);
    delayMicroseconds(pulseWidth);
    digitalWrite(pin, LOW);
    delayMicroseconds(20000 - pulseWidth);
  }
}

// サーボを0度からtoDegまで少しずつ動かしながらスイッチを監視する
// スイッチがOFFになったらその場で止まる
void sweepUntilSwitchOff(int toDeg) {
  for (int deg = 0; deg <= toDeg; deg += 2) {
    servoWrite(SERVO_PIN, deg, 40);
    if (digitalRead(SWITCH_PIN) == HIGH) {  // スイッチがOFFになった
      return;
    }
  }
  // 最大角度に達してもスイッチがOFFにならない場合はそのまま待機
  while (digitalRead(SWITCH_PIN) == LOW) {
    servoWrite(SERVO_PIN, toDeg, 20);
  }
}

void goToSleep() {
  digitalWrite(LED_BUILTIN, LOW);

  // PWRクロックを有効化してSTANDBYモードへ
  // 復帰時はリセット扱いになるため setup() から再開する
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);
  PWR_EnterSTANDBYMode(PWR_STANDBYEntry_WFI);
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(SERVO_PIN, OUTPUT);
  pinMode(SWITCH_PIN, INPUT_PULLUP);

  // EXTI割り込みを設定（STANDBYからの復帰トリガー）
  // STANDBYはリセット扱いなので setup() で一度設定すれば十分
  // CH32V003固有シグネチャ: (pin, GPIOMode, callback, EXTIMode, EXTITrigger)
  attachInterrupt(digitalPinToInterrupt(SWITCH_PIN), GPIO_Mode_IPU, onSwitchOn, EXTI_Mode_Interrupt, EXTI_Trigger_Falling);

  // 初期位置: 90度
  servoWrite(SERVO_PIN, 180 , 500);
  delay(1000);
  // 起動時にスイッチがすでにONなら即動作、OFFならスリープ
  if (digitalRead(SWITCH_PIN) == HIGH) {
//    goToSleep();
  }
}

void loop() {
  // スイッチON: LED点灯
  digitalWrite(LED_BUILTIN, HIGH);

  if (digitalRead(SWITCH_PIN) == LOW) {  // スイッチがOFFになった
    delay(500);  // デバウンス遅延
    // スイッチがOFFになるまでサーボを回す（最大120度）
    // sweepUntilSwitchOff(SERVO_MAX_DEG);
    servoWrite(SERVO_PIN, 145, 100);

    // スイッチOFF: LED点灯
    digitalWrite(LED_BUILTIN, LOW);

    // 0度に戻す
    servoWrite(SERVO_PIN, 180, 100);
  }
  // ディープスリープへ
 // goToSleep();
}
