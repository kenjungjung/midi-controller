#pragma once
#include <cstdint>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "midi_event.h"
#include "analog_input.h"
#include "button.h"
#include "midi_sender.h"
// #include "display.h"

/** @brief ADC・ボタン読み取りと MIDI 送信・表示更新を管理するコントローラー
 *
 *  InputTask から input_loop() を、MidiTask から midi_loop() を呼び出す。
 *  midi_queue のライフタイムは呼び出し元（main.cpp）が管理する。
 */
class Controller {
public:
    /** @brief 依存オブジェクトをすべてコンストラクタで受け取る
     *  @param input      アナログ入力（フェーダー）
     *  @param button     ボタン入力
     *  @param sender     MIDI 送信インターフェース
     *  @param display    OLED 表示インターフェース
     *  @param midi_queue タスク間キュー（main.cpp で xQueueCreate したもの）
     */
    Controller(IAnalogInput& input, IButton& button,
               IMidiSender& sender, /*IDisplay& display,*/ QueueHandle_t midi_queue);

    /** @brief InputTask のメインループ: 10ms ごとに ADC とボタンを読む */
    void input_loop();

    /** @brief MidiTask のメインループ: queue からイベントを取り出して送信し表示を更新する */
    void midi_loop();

    /** @brief USB 再接続時に呼ぶ: 次のポーリングで全CC を強制再送信する */
    void reset_prev_cc();

    /** @brief 12bit ADC 値を 7bit MIDI CC 値に変換する
     *  @param raw  ADC スケール値（0-4095）
     *  @return     MIDI CC 値（0-127）
     */
    static uint8_t to_midi_cc(uint16_t raw);

private:
    IAnalogInput&  input_;
    IButton&       button_;
    IMidiSender&   sender_;
    // IDisplay&      display_;
    QueueHandle_t  midi_queue_;

    uint8_t prev_cc_;   ///< 直前の CC 値。0xFF = 未送信（強制送信フラグ）
    bool    prev_btn_;  ///< 直前のボタン状態
};
