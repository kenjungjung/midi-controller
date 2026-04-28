# CLAUDE.md — ESP32-S3 USB MIDIコントローラー

> 詳細設計（全体確認が必要なときのみ）: [docs/design/overview.md](docs/design/overview.md)

## 現在のフェーズ

**Phase 1 実装中** → [docs/design/phase1_usb_midi.md](docs/design/phase1_usb_midi.md) を読むこと
（他のPhaseファイルは不要）

---

## 実装状況

| モジュール | ファイル | 状態 |
|-----------|--------|------|
| USB MIDI | usb_midi.cpp/.h | 未着手 |
| ADCスライダー | analog_input.cpp/.h | 未着手 |
| ボタン | button.cpp/.h | 未着手 |
| LED制御 | led.cpp/.h | 未着手 |
| ディスプレイ | display_manager.cpp/.h | 未着手 |
| コントローラー本体 | controller.cpp/.h | 未着手 |
| Ableton Remote Script | remote-script/ | 未着手 |

---

## Claude Codeへの指示

- コードはC++、ESP-IDF v5.x APIを使うこと
- `adc_legacy` APIは使用禁止。`adc_oneshot` + `adc_cali` を使うこと
- TinyUSBのMIDIクラスドライバを使うこと（自前MIDI実装禁止）
- ディスプレイは `esp_lcd` コンポーネントを使うこと（外部ライブラリ禁止）
- ADC値は `>> 5` で7bit MIDIに変換し、deadband=4で変化検出すること
- ESP32-S3はADC1（GPIO1–10）のみ使うこと
- ピン番号は `constexpr` で `config.h` にまとめること
- ハードウェアはすべてインターフェース経由で抽象化し、スタブと差し替え可能にすること
- すべての関数・クラス・メンバ変数にDoxygenコメントを付けること（`/** @brief ... */` 形式）
- FreeRTOSタスク構成は [docs/design/phase3_full.md](docs/design/phase3_full.md) §2 に従うこと

---

## 開発環境

- エディタ: VSCode + Claude Code拡張
- PCB設計: KiCad / 発注先: JLCPCB
