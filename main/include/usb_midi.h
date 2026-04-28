#pragma once
#include "midi_sender.h"

/** @brief TinyUSB MIDI クラスドライバを使った USB MIDI 送信実装 */
class UsbMidiSender : public IMidiSender {
public:
    UsbMidiSender()  = default;
    ~UsbMidiSender() = default;

    void send_cc(uint8_t ch, uint8_t cc, uint8_t val) override;
    void send_note_on(uint8_t ch, uint8_t note, uint8_t vel) override;
    void send_note_off(uint8_t ch, uint8_t note) override;
    bool is_connected() const override;
};
