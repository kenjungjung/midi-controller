#pragma once
#include "midi_sender.h"
#include <cstddef>

/** @brief TinyUSB MIDI クラスドライバを使った USB MIDI 送受信実装 */
class UsbMidiSender : public IMidiSender {
public:
    UsbMidiSender()  = default;
    ~UsbMidiSender() = default;

    void send_cc(uint8_t ch, uint8_t cc, uint8_t val) override;
    void send_note_on(uint8_t ch, uint8_t note, uint8_t vel) override;
    void send_note_off(uint8_t ch, uint8_t note) override;
    bool is_connected() const override;
    bool read_sysex(uint8_t* buf, size_t* out_len, size_t max_len) override;
    bool send_sysex(const uint8_t* data, size_t len) override;

private:
    static constexpr size_t SYSEX_BUF_SIZE = 64;
    uint8_t sysex_buf_[SYSEX_BUF_SIZE] = {};
    size_t  sysex_len_                  = 0;
    bool    in_sysex_                   = false;
};
