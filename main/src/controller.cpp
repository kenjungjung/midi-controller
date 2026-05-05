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

static uint8_t to_midi_cc(AdcType eType, uint16_t raw) {
    int raw_min = 0;
    int raw_max = 4095;

    uint8_t midi_cc = 0;

    switch(eType){
        case eFader: raw_min = FADER_RAW_MIN; raw_max = FADER_RAW_MAX;[[fallthrough]];
        case eKnob:  raw_min = KNOB_RAW_MIN;  raw_max = KNOB_RAW_MAX;{
            int clamped = raw < raw_min ? raw_min
                        : raw > raw_max ? raw_max : raw;
            midi_cc = static_cast<uint8_t>((clamped - raw_min) * 127 / (raw_max - raw_min));
            break;
        }
        case eButton:
            midi_cc = static_cast<uint8_t>(raw >= BTN_RAW_THRETHOLD ? 127 : 0);
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
    reset_prev_cc();
}

void Controller::reset_prev_cc() {
    prev_fader_cc_.fill(0xFF);
    prev_knob_cc_.fill(0xFF);
    prev_btn_.fill(false);

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
                auto raw = cfg_.faders[i]->read();
                auto new_cc = to_midi_cc(eFader, raw);
                if(new_cc != prev_fader_cc_[i]) {
                    ESP_LOGI("MIDI out", "fader %d raw=%4d, val=%3d", i, raw, new_cc);
                    MidiEvent ev{MidiEvent::Type::CC, MIDI_CHANNEL,
                                 static_cast<uint8_t>(CC_FADER_1 + i), new_cc};
                    xQueueSend(cfg_.midi_queue, &ev, 0);
                    prev_fader_cc_[i] = new_cc;
                }
            }

            // ノブ
            for (int i = 0; i < NUM_KNOBS; ++i) {
                auto raw = cfg_.knobs[i]->read();
                auto new_cc = to_midi_cc(eKnob, raw);
                if(new_cc != prev_knob_cc_[i]) {
                    ESP_LOGI("MIDI out", "knob %d raw=%4d, val=%3d", i, raw, new_cc);
                    MidiEvent ev{MidiEvent::Type::CC, MIDI_CHANNEL,
                                 static_cast<uint8_t>(CC_KNOB_1 + i), new_cc};
                    xQueueSend(cfg_.midi_queue, &ev, 0);
                    prev_knob_cc_[i] = new_cc;
                }
            }

            // ボタン
            for (int i = 0; i < NUM_BUTTONS; ++i) {
                auto raw = cfg_.buttons[i]->read();
                auto new_cc = to_midi_cc(eButton, raw);
                bool pressed = (new_cc >= BTN_MIDI_THRETHOLD);
                if(pressed != prev_btn_[i]) {
                    ESP_LOGI("MIDI out", "button %d raw=%4d, %s", i, raw, (pressed ? "ON" : "OFF"));
                    auto note_flag = pressed ? MidiEvent::Type::NOTE_ON : MidiEvent::Type::NOTE_OFF;
                    MidiEvent ev{note_flag, MIDI_CHANNEL,
                                 static_cast<uint8_t>(NOTE_BUTTON_1 + i), new_cc};
                    xQueueSend(cfg_.midi_queue, &ev, 0);
                    prev_btn_[i] = pressed;
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

constexpr uint8_t SYSEX_TRACK_COLOR = 0x01u;
constexpr uint8_t SYSEX_TRACK_VOLUME = 0x03u;

/** @brief SysEx を解析してLED色・ボリュームを更新する
 *  カラー:   F0 7D 01 <index> <R> <G> <B> F7  (8バイト)
 *  ボリューム: F0 7D 03 <index> <vol> F7       (6バイト)
 *  R/G/B/vol は 7bit（0–127）。
 */
static void handle_sysex(const uint8_t* buf, size_t len, ILed* led) {
    if (len < 6 || buf[0] != 0xF0u || buf[1] != 0x7Du || buf[len - 1] != 0xF7u) return;

    uint8_t type  = buf[2];

    switch(type) {
        case SYSEX_TRACK_COLOR:
        case SYSEX_TRACK_VOLUME:
            led->excute(buf, len);
            break;
        default:
            assert(0);
            ESP_LOGI("SysEx", "other message %X", buf);
            break;
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
