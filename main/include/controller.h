#pragma once
#include <cstdint>
#include <array>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "config.h"
#include "midi_event.h"
#include "analog_input.h"
#include "midi_sender.h"
#include "display.h"
#include "led.h"

/** @brief Controller に渡すハードウェア依存オブジェクトをまとめた設定構造体
 *
 *  デバイスが増えてもこの構造体に追加するだけでよく、
 *  Controller のコンストラクタ引数は変わらない。
 */
struct ControllerConfig {
    // 入力
    std::array<IAnalogInput*, NUM_FADERS>  faders;   ///< フェーダー群
    std::array<IAnalogInput*, NUM_KNOBS>   knobs;    ///< ノブ群
    std::array<IAnalogInput*, NUM_BUTTONS> buttons;  ///< ボタン群（LED付き含む）
    // 出力
    ILed*          led;         ///< RGBステータスLED（PL9823等）
    IDisplay*      display;     ///< OLEDディスプレイ
    // 通信
    IMidiSender*   sender;      ///< USB MIDI 送信
    QueueHandle_t  midi_queue;  ///< タスク間キュー（main.cpp が管理）
};

/** @brief ADC・ボタン読み取りと MIDI 送信・表示更新を管理するコントローラー
 *
 *  InputTask から input_loop() を、MidiTask から midi_loop() を呼び出す。
 */
class Controller {
public:
    /** @brief ControllerConfig を受け取って初期化する */
    explicit Controller(const ControllerConfig& cfg);

    /** @brief InputTask のメインループ: 10ms ごとにフェーダー・ノブ・ボタンを読む */
    void input_loop();

    /** @brief MidiTask のメインループ: queue からイベントを取り出して送信し表示を更新する */
    void midi_loop();

    /** @brief USB 再接続時に呼ぶ: 次のポーリングで全CC を強制再送信する */
    void reset_prev_cc();

    /** @brief USB 接続状態変化時に呼ぶ: タイトル行を更新する */
    void notify_connected(bool connected);

private:
    ControllerConfig cfg_;

    std::array<uint8_t, NUM_FADERS>  prev_fader_cc_; ///< デッドバンド用前回値（0xFF=強制送信）
    std::array<uint8_t, NUM_KNOBS>   prev_knob_cc_;  ///< デッドバンド用前回値
    std::array<bool,    NUM_BUTTONS> prev_btn_;       ///< ボタン前回状態
};
