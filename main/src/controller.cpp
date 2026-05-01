#include "controller.h"
#include "config.h"
#include "led.h"
#include <cstdlib>
#include "freertos/task.h"
#include "esp_log.h"

// static const char* TAG = "Controller";

enum AdcType{
    eFader,
    eKnob,
    eButton,
};

static uint8_t to_midi_cc(AdcType eType, uint16_t vol) {
    int vol_min = 0;    
    int vol_max = 4095;

    uint8_t midi_cc = 0;

    switch(eType){
        case eFader: vol_min = FADER_RAW_MIN; vol_max = FADER_RAW_MAX;[[fallthrough]];
        case eKnob:  vol_min = KNOB_RAW_MIN;  vol_max = KNOB_RAW_MAX;{
            int clamped = vol < vol_min ? vol_min
                        : vol > vol_max ? vol_max : vol;
            midi_cc = static_cast<uint8_t>((clamped - vol_min) * 127 / (vol_max - vol_min));
            break;
        }
        case eButton:
            midi_cc = static_cast<uint8_t>(vol >= BTN_RAW_THRETHOLD ? 127 : 0);
            break;
        default:
            assert(0);
            break;
    }
    return midi_cc;
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
            if(connected) {
                // abletonにREFRESH要求
                const uint8_t msg[] = {0xF0, 0x7D, 0x02, 0xF7};
                cfg_.sender->send_sysex(msg, sizeof(msg));
                ESP_LOGI("send request", "REFRESH");
            }
            else {
                for (int i = 0; i < NUM_LEDS; ++i) {
                    cfg_.led->set_color(i, {0, 0, 0});
                }
            }
        }
        // 入力読み込み処理
        if (connected) {
            // フェーダー
            for (int i = 0; i < NUM_FADERS; ++i) {
                auto vol = cfg_.faders[i]->read();
                auto new_cc = to_midi_cc(eFader, vol);
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
                auto new_cc = to_midi_cc(eKnob, vol);
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
                auto new_cc = to_midi_cc(eButton, vol);
                bool pressed = (new_cc >= BTN_MIDI_THRETHOLD);
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

/** @brief SysEx を解析してLED色・ボリュームを更新する
 *  カラー:   F0 7D 01 <index> <R> <G> <B> F7  (8バイト)
 *  ボリューム: F0 7D 03 <index> <vol> F7       (6バイト)
 *  R/G/B/vol は 7bit（0–127）。
 */
static void handle_sysex(const uint8_t* buf, size_t len, ILed* led) {
    if (len < 6 || buf[0] != 0xF0u || buf[1] != 0x7Du || buf[len - 1] != 0xF7u) return;

    uint8_t type  = buf[2];
    uint8_t index = buf[3];

    if (type == 0x01u && len == 8u) {
        // トラックカラー: F0 7D 01 <index> <R> <G> <B> F7
        if (index >= NUM_LEDS) return;
        uint8_t r = static_cast<uint8_t>(buf[4] * 2u / LED_DARKNESS);
        uint8_t g = static_cast<uint8_t>(buf[5] * 2u / LED_DARKNESS);
        uint8_t b = static_cast<uint8_t>(buf[6] * 2u / LED_DARKNESS);
        ESP_LOGI("SysEx", "track[%d] color R=%d G=%d B=%d", index, r, g, b);
        // led->set_color(index, {r, g, b});
        // led->refresh();
    } else if (type == 0x03u && len == 6u) {
        // トラックボリューム: F0 7D 03 <index> <vol> F7
        uint8_t vol = buf[4];  // 0–127
        if(index == 0) {
            const int VOL_MIN = 64;  // -12dB
            const int VOL_MAX = 117; // 0dB

            // ボリューム値をLED点灯数に変換（0dBFS以上はすべて点灯）
            int on_num = (vol - VOL_MIN) * NUM_LEDS / (VOL_MAX - VOL_MIN);
            if (vol >= VOL_MAX) on_num = NUM_LEDS; // 0dBFS以上で全点灯
            if (vol > 0 && on_num <= 0) on_num = 1; // 無音でないのにLEDが0本になる場合は最低1本点灯
            if (on_num < 0) on_num = 0; // 負値にならないようにクランプ
            for (uint32_t i = 0; i < NUM_LEDS; i++) {
                if (i < on_num) {
                   led->set_color(i, {10, 10, 10});
                } else {
                    led->set_color(i, {0, 0, 0});
                }
            }
            led->refresh();
            ESP_LOGI("SysEx", "track[%d] volume=%d, on_num=%d", index, vol, on_num);
        }
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
