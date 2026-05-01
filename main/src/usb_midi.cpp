#include "usb_midi.h"
#include "tusb.h"
#include "class/midi/midi_device.h"
#include "esp_log.h"
#include <cstring>

static const char* TAG = "UsbMidi";

bool UsbMidiSender::is_connected() const {
    return tud_ready();
}

void UsbMidiSender::send_cc(uint8_t ch, uint8_t cc, uint8_t val) {
    if (!is_connected()) return;

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
    if (!is_connected()) return;

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
    if (!is_connected()) return;

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

bool UsbMidiSender::read_sysex(uint8_t* buf, size_t* out_len, size_t max_len) {
    uint8_t byte;
    while (tud_midi_stream_read(&byte, 1) == 1) {
        if (byte == 0xF0u) {
            in_sysex_ = true;
            sysex_len_ = 0;
        }
        if (!in_sysex_) continue;

        if (sysex_len_ < SYSEX_BUF_SIZE) {
            sysex_buf_[sysex_len_++] = byte;
        }
        if (byte == 0xF7u) {
            in_sysex_ = false;
            if (sysex_len_ <= max_len) {
                memcpy(buf, sysex_buf_, sysex_len_);
                *out_len = sysex_len_;
                return true;
            }
            ESP_LOGW(TAG, "read_sysex: buffer too small (%u bytes)", (unsigned)sysex_len_);
        }
    }
    return false;
}

bool UsbMidiSender::send_sysex(const uint8_t* data, size_t len) {
    if (!is_connected()) {
        ESP_LOGW(TAG, "send_sysex: not connected");
        return false;
    }

    // フォーマットチェック（最低限）
    if (len < 2 || data[0] != 0xF0u || data[len - 1] != 0xF7u) {
        ESP_LOGW(TAG, "send_sysex: invalid format");
        return false;
    }

    uint32_t written = tud_midi_stream_write(0, data, len);

    if (written != len) {
        ESP_LOGW(TAG, "send_sysex: partial write (%u/%u)",
                 (unsigned)written, (unsigned)len);
        return false;
    }

    return true;
}