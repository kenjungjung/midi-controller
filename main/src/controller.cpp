#include "controller.h"
#include "config.h"
#include <cstdlib>
#include "freertos/task.h"
#include "esp_log.h"

static const char* TAG = "Controller";

Controller::Controller(IAnalogInput& input, IButton& button,
                       IMidiSender& sender, /*IDisplay& display,*/ QueueHandle_t midi_queue)
    : input_(input), button_(button), sender_(sender), /*display_(display),*/
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
        if (sender_.is_connected()) 
        {
            uint8_t new_cc = to_midi_cc(input_.read());
            int delta = (prev_cc_ == 0xFFu)
                ? DEADBAND
                : std::abs(static_cast<int>(new_cc) - static_cast<int>(prev_cc_));

            if (delta >= DEADBAND) {
                ESP_LOGI(TAG, "fader CC=%3d (raw=%d)", new_cc, input_.read());
                MidiEvent ev{MidiEvent::Type::CC, MIDI_CHANNEL, CC_FADER_1, new_cc};
                xQueueSend(midi_queue_, &ev, 0);
                prev_cc_ = new_cc;
            }

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
        else {
            ESP_LOGI("sender", "disconnected");
        }
    
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void Controller::midi_loop() {
    MidiEvent ev;
    while (true) {
        if (xQueueReceive(midi_queue_, &ev, portMAX_DELAY) == pdTRUE) {
            switch (ev.type) {
                case MidiEvent::Type::CC:
                    sender_.send_cc(ev.channel, ev.number, ev.value);
                    // display_.push_event(ev, true);
                    break;
                case MidiEvent::Type::NOTE_ON:
                    sender_.send_note_on(ev.channel, ev.number, ev.value);
                    // display_.push_event(ev, true);
                    break;
                case MidiEvent::Type::NOTE_OFF:
                    sender_.send_note_off(ev.channel, ev.number);
                    // display_.push_event(ev, true);
                    break;
            }
        }
    }
}
