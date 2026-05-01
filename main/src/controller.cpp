#include "controller.h"
#include "config.h"
#include "led.h"
#include <cstdlib>
#include "freertos/task.h"
#include "esp_log.h"

static const char* TAG = "Controller";

static uint8_t to_midi_cc(uint16_t vol, uint16_t vol_min, uint16_t vol_max){
    
    int clamped = vol < vol_min ? vol_min
                : vol > vol_max ? vol_max : vol;
                
    return static_cast<uint8_t>((clamped - vol_min) * 127 / (vol_max - vol_min));
}

Controller::Controller(const ControllerConfig& cfg)
    : cfg_(cfg)
{
    prev_fader_cc_.fill(0xFF);
    prev_knob_cc_.fill(0xFF);
    prev_btn_.fill(false);
}

void Controller::reset_prev_cc() {
    prev_fader_cc_.fill(0xFF);
    prev_knob_cc_.fill(0xFF);
}

void Controller::notify_connected(bool connected) {
    cfg_.display->set_title(connected ? Display::TITLE : Display::DISCONECTED);
}

void Controller::input_loop() {
    bool prev_connected = false;
    while (true) {
        // ここでポーリングせず、usb_event_cbでやるべき
        bool connected = cfg_.sender->is_connected();
        if (connected != prev_connected) {
            notify_connected(connected);
            prev_connected = connected;
        }
        if (connected) {
            // フェーダー
            for (int i = 0; i < NUM_FADERS; ++i) {
                auto vol = cfg_.faders[i]->read();
                auto new_cc = to_midi_cc(vol, FADER_RAW_MIN, FADER_RAW_MAX);
                int delta = (prev_fader_cc_[i] == 0xFFu)
                    ? DEADBAND
                    : std::abs(static_cast<int>(new_cc) - static_cast<int>(prev_fader_cc_[i]));
                if (delta >= DEADBAND) {
                    ESP_LOGI("MIDI out", "fader %d vol=%4d, val=%3d", i, vol, new_cc);
                    MidiEvent ev{MidiEvent::Type::CC, MIDI_CHANNEL,
                                 static_cast<uint8_t>(CC_FADER_1 + i), new_cc};
                    xQueueSend(cfg_.midi_queue, &ev, 0);
                    prev_fader_cc_[i] = new_cc;
                }
            }

            // ノブ
            for (int i = 0; i < NUM_KNOBS; ++i) {
                auto vol = cfg_.knobs[i]->read();
                auto new_cc = to_midi_cc(vol, KNOB_RAW_MIN, KNOB_RAW_MAX);
                int delta = (prev_knob_cc_[i] == 0xFFu)
                    ? DEADBAND
                    : std::abs(static_cast<int>(new_cc) - static_cast<int>(prev_knob_cc_[i]));
                if (delta >= DEADBAND) {
                    ESP_LOGI("MIDI out", "knob %d vol=%4d, val=%3d", i, vol, new_cc);
                    MidiEvent ev{MidiEvent::Type::CC, MIDI_CHANNEL,
                                 static_cast<uint8_t>(CC_KNOB_1 + i), new_cc};
                    xQueueSend(cfg_.midi_queue, &ev, 0);
                    prev_knob_cc_[i] = new_cc;
                }
            }

            // ボタン
            for (int i = 0; i < NUM_BUTTONS; ++i) {
                auto vol = cfg_.buttons[i]->read();
                auto new_cc = to_midi_cc(vol, KNOB_RAW_MIN, KNOB_RAW_MAX);
                bool pressed = (new_cc >= MIDI_BTN_THRETHOLD);
                if (pressed && !prev_btn_[i]) {
                    // cfg_.buttons[i]->set_led(true);                    
                    ESP_LOGI("MIDI out", "button %d vol=%4d, ON", i, vol);
                    MidiEvent ev{MidiEvent::Type::NOTE_ON, MIDI_CHANNEL,
                                 static_cast<uint8_t>(NOTE_BUTTON_1 + i), 127};
                    xQueueSend(cfg_.midi_queue, &ev, 0);
                } else if (!pressed && prev_btn_[i]) {
                    // cfg_.buttons[i]->set_led(false);
                    ESP_LOGI("MIDI out", "button %d vol=%4d, OFF", i, vol);
                    MidiEvent ev{MidiEvent::Type::NOTE_OFF, MIDI_CHANNEL,
                                 static_cast<uint8_t>(NOTE_BUTTON_1 + i), 0};
                    xQueueSend(cfg_.midi_queue, &ev, 0);
                }
                prev_btn_[i] = pressed;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

/** @brief SysEx を解析してLED色を更新する
 *  フォーマット: F0 7D 01 <index> <R> <G> <B> F7
 *  R/G/B は 7bit（0–127）。LED は 8bit に変換して set_color() する。
 */
static void handle_sysex(const uint8_t* buf, size_t len, ILed* led) {
    // 最小長: F0 7D type index data... F7 = 最低7バイト（カラーは8バイト）
    if (len < 7 || buf[0] != 0xF0u || buf[1] != 0x7Du || buf[len - 1] != 0xF7u) return;

    uint8_t type  = buf[2];
    uint8_t index = buf[3];

    if (type == 0x01u && len == 8u) {
        // トラックカラー: F0 7D 01 <index> <R> <G> <B> F7
        if (index >= NUM_LEDS) return;
        uint8_t r = static_cast<uint8_t>(buf[4] * 2u);
        uint8_t g = static_cast<uint8_t>(buf[5] * 2u);
        uint8_t b = static_cast<uint8_t>(buf[6] * 2u);
        ESP_LOGI("SysEx", "track[%d] color R=%d G=%d B=%d", index, r, g, b);
        led->set_color(index, {r, g, b});
        led->refresh();
    }
}

void Controller::midi_loop() {
    MidiEvent ev;
    uint8_t   sysex_buf[64];
    size_t    sysex_len;

    while (true) {
        // SysEx 受信ポーリング（ブロッキング前に処理）
        while (cfg_.sender->read_sysex(sysex_buf, &sysex_len, sizeof(sysex_buf))) {
            handle_sysex(sysex_buf, sysex_len, cfg_.led);
        }

        // MIDI 送信キュー処理（最大待機 10ms でポーリングと交互に回す）
        if (xQueueReceive(cfg_.midi_queue, &ev, pdMS_TO_TICKS(10)) == pdTRUE) {
            switch (ev.type) {
                case MidiEvent::Type::CC:
                    cfg_.sender->send_cc(ev.channel, ev.number, ev.value);
                    cfg_.display->push_event(ev, true);
                    break;
                case MidiEvent::Type::NOTE_ON:
                    cfg_.sender->send_note_on(ev.channel, ev.number, ev.value);
                    cfg_.display->push_event(ev, true);
                    break;
                case MidiEvent::Type::NOTE_OFF:
                    cfg_.sender->send_note_off(ev.channel, ev.number);
                    cfg_.display->push_event(ev, true);
                    break;
            }
        }
    }
}
