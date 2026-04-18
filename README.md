# UselessBox

CH32V003 + Arduino + PlatformIO で動かす Useless Box の試作コードです。

トグルスイッチとタッチ電極を使って、サーボの発火タイミングを制御します。

## Environment

- PlatformIO
- Arduino framework
- CH32V003F4P6
- `minichlink` upload/debug

設定は [platformio.ini](/Users/ooe/src/UselessBox/platformio.ini) にあります。

## Files

- [src/main.cpp](/Users/ooe/src/UselessBox/src/main.cpp): メイン制御
- [src/VarSpeedServo.h](/Users/ooe/src/UselessBox/src/VarSpeedServo.h): VarSpeedServo 互換クラス定義
- [src/VarSpeedServo.cpp](/Users/ooe/src/UselessBox/src/VarSpeedServo.cpp): タイマー駆動サーボ制御実装
- [src/touch_sensor.h](/Users/ooe/src/UselessBox/src/touch_sensor.h): タッチ検出API
- [src/touch_sensor.cpp](/Users/ooe/src/UselessBox/src/touch_sensor.cpp): A0電極のタッチ検出実装
- [platformio.ini](/Users/ooe/src/UselessBox/platformio.ini): PlatformIO のボードと書き込み設定

## Pin Assignment

- `LED_PIN = 2`
- `SERVO_PIN = 3`
- `SWITCH_PIN = 4`
- `TOUCH_PIN = A0`

`SWITCH_PIN` は `INPUT_PULLUP` を使っているため、`LOW = ON`、`HIGH = OFF` の想定です。

## Current Behavior

現状のコードでは次のように動作します。

1. 起動時にサーボを初期位置（`SERVO_REST_ANGLE`）に移動します。
2. `A0` のタッチ状態を毎ループ更新します（LED はタッチ中点灯）。
3. `SW ON` で発火待機に入ります。
4. タッチ中は発火しません。
5. タッチ解除が安定し、ランダム待ち時間を経過したら本発火します。
6. タッチ長押し時（`3秒 + ランダム`）には、1回だけフェイント動作（全ストロークの `60%〜80%`）を行います。

## Build

```bash
pio run
```

## Upload

```bash
pio run -t upload
```

## Notes

- サーボ挙動は [src/main.cpp](/Users/ooe/src/UselessBox/src/main.cpp) の以下で調整できます。
  - `SERVO_REST_ANGLE`
  - `SERVO_ATTACK_ANGLE`
  - `FIRE_RANDOM_MIN_MS` / `FIRE_RANDOM_MAX_MS`
  - `FEINT_HOLD_BASE_MS` / `FEINT_HOLD_RANDOM_MAX_MS`
  - `FEINT_RATIO_MIN_PERCENT` / `FEINT_RATIO_MAX_PERCENT`
- タッチ感度は [src/main.cpp](/Users/ooe/src/UselessBox/src/main.cpp) の `TouchConfig`（`onThreshold`, `offThreshold` など）で調整してください。
- タッチ電極の安定化には `A0` - `GND` 間に `1MΩ` 程度の抵抗追加が有効です。
