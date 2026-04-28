#include "controller.h"
#include "config.h"
#include <cstdlib>
#include "freertos/task.h"

Controller::Controller(IAnalogInput& input, IButton& button,
                       IMidiSender& sender, QueueHandle_t midi_queue)
    : input_(input), button_(button), sender_(sender),
      midi_queue_(midi_queue), prev_cc_(0xFF), prev_btn_(false)
{}

void Controller::reset_prev_cc() {
    prev_cc_ = 0xFF;
}

uint8_t Controller::to_midi_cc(uint16_t raw) {
    return static_cast<uint8_t>(raw >> 5);
}

void Controller::input_loop() {
    while (true) {
        if (sender_.is_connected()) {
            // ── フェーダー ──
            uint8_t new_cc = to_midi_cc(input_.read());
            // prev_cc_ == 0xFF のときは変化量チェックをスキップして強制送信
            int delta = (prev_cc_ == 0xFFu)
                ? DEADBAND
                : std::abs(static_cast<int>(new_cc) - static_cast<int>(prev_cc_));

            if (delta >= DEADBAND) {
                MidiEvent ev{MidiEvent::Type::CC, MIDI_CHANNEL, CC_FADER_1, new_cc};
                xQueueSend(midi_queue_, &ev, 0);  // フル時はイベントを捨てる
                prev_cc_ = new_cc;
            }

            // ── ボタン ──
            bool pressed = button_.is_pressed();
            if (pressed && !prev_btn_) {
                MidiEvent ev{MidiEvent::Type::NOTE_ON, MIDI_CHANNEL, NOTE_BUTTON_1, 127};
                xQueueSend(midi_queue_, &ev, 0);
            } else if (!pressed && prev_btn_) {
                MidiEvent ev{MidiEvent::Type::NOTE_OFF, MIDI_CHANNEL, NOTE_BUTTON_1, 0};
                xQueueSend(midi_queue_, &ev, 0);
            }
            prev_btn_ = pressed;
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void Controller::midi_loop() {
    MidiEvent ev;
    while (true) {
        // イベントが来るまでブロック（タイムアウトなし）
        if (xQueueReceive(midi_queue_, &ev, portMAX_DELAY) == pdTRUE) {
            switch (ev.type) {
                case MidiEvent::Type::CC:
                    sender_.send_cc(ev.channel, ev.number, ev.value);
                    break;
                case MidiEvent::Type::NOTE_ON:
                    sender_.send_note_on(ev.channel, ev.number, ev.value);
                    break;
                case MidiEvent::Type::NOTE_OFF:
                    sender_.send_note_off(ev.channel, ev.number);
                    break;
            }
        }
    }
}
