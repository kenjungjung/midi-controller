# midi-controller

ESP32-S3-DevKitC-1-N8 を使ったMIDIコントローラー。

## ハードウェア

- ESP32-S3-DevKitC-1-N8
- SSD1306 OLED (I2C: SDA=GPIO4, SCL=GPIO5)

## ビルド・書き込み

### 初回 / ターゲット変更後

```bash
idf.py set-target esp32s3
idf.py fullclean
idf.py -p COM5 flash monitor
```

### 通常ビルド

```bash
idf.py -p COM5 flash monitor
```

## BLEログ

ESP32-S3が `ESP32-Logger` という名前でBLEアドバタイズする。
スマホの「Serial Bluetooth Terminal」アプリで接続するとログをリアルタイム確認できる。

- USBシリアル（`idf.py monitor`）にも同時出力される
- 接続できるスマホは1台のみ
