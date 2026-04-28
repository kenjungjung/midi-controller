#include "usb_midi.h"
#include "tusb.h"
#include "class/midi/midi_device.h"
#include "esp_log.h"

static const char* TAG = "UsbMidi";

bool UsbMidiSender::is_connected() const {
    return tud_mounted();
}

void UsbMidiSender::send_cc(uint8_t ch, uint8_t cc, uint8_t val) {
    if (!tud_mounted()) return;

    // MIDI CC: status = 0xB0 | (ch-1)
    uint8_t msg[3] = {
        static_cast<uint8_t>(0xB0u | ((ch - 1u) & 0x0Fu)),
        static_cast<uint8_t>(cc  & 0x7Fu),
        static_cast<uint8_t>(val & 0x7Fu),
    };
    uint32_t written = tud_midi_stream_write(0, msg, sizeof(msg));
    if (written == 0) {
        ESP_LOGW(TAG, "send_cc: write failed (buffer full?)");
    }
}

void UsbMidiSender::send_note_on(uint8_t ch, uint8_t note, uint8_t vel) {
    if (!tud_mounted()) return;

    uint8_t msg[3] = {
        static_cast<uint8_t>(0x90u | ((ch - 1u) & 0x0Fu)),
        static_cast<uint8_t>(note & 0x7Fu),
        static_cast<uint8_t>(vel  & 0x7Fu),
    };
    uint32_t written = tud_midi_stream_write(0, msg, sizeof(msg));
    if (written == 0) {
        ESP_LOGW(TAG, "send_note_on: write failed");
    }
}

void UsbMidiSender::send_note_off(uint8_t ch, uint8_t note) {
    if (!tud_mounted()) return;

    uint8_t msg[3] = {
        static_cast<uint8_t>(0x80u | ((ch - 1u) & 0x0Fu)),
        static_cast<uint8_t>(note & 0x7Fu),
        0x00,
    };
    uint32_t written = tud_midi_stream_write(0, msg, sizeof(msg));
    if (written == 0) {
        ESP_LOGW(TAG, "send_note_off: write failed");
    }
}
