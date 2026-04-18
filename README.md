# UselessBox

CH32V003 + Arduino + PlatformIO で動かす Useless Box の試作コードです。

トグルスイッチが ON になったらサーボを動かしてスイッチを戻す、シンプルな動作を想定しています。

## Environment

- PlatformIO
- Arduino framework
- CH32V003F4P6
- `minichlink` upload/debug

設定は [platformio.ini](/Users/ooe/src/UselessBox/platformio.ini) にあります。

## Files

- [src/main.cpp](/Users/ooe/src/UselessBox/src/main.cpp): メインの制御コード
- [platformio.ini](/Users/ooe/src/UselessBox/platformio.ini): PlatformIO のボードと書き込み設定

## Pin Assignment

- `LED_BUILTIN = 2`
- `SERVO_PIN = 3`
- `SWITCH_PIN = 4`

`SWITCH_PIN` は `INPUT_PULLUP` を使っているため、`LOW = ON`、`HIGH = OFF` の想定です。

## Current Behavior

現状のコードでは次のように動作します。

1. 起動時にサーボを初期位置へ動かします。
2. ループ中は LED を点灯します。
3. スイッチ入力が `LOW` になったら少し待ってからサーボを動かします。
4. その後サーボを戻します。

スリープ処理や段階的なスイープ処理の関数はありますが、現在はコメントアウトされていて常時有効ではありません。

## Build

```bash
pio run
```

## Upload

```bash
pio run -t upload
```

## Notes

- サーボ角度や動作時間は [src/main.cpp](/Users/ooe/src/UselessBox/src/main.cpp) 内の値を実機に合わせて調整してください。
- 配線に応じて `SERVO_PIN` と `SWITCH_PIN` は変更してください。
- CH32V003 の割り込みや STANDBY 復帰まわりはまだ調整途中の状態です。
