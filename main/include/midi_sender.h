#pragma once
#include <cstdint>
#include <cstddef>

/** @brief MIDI 送信の抽象インターフェース */
class IMidiSender {
public:
    virtual ~IMidiSender() = default;

    /** @brief CC メッセージを送信する
     *  @param ch   MIDIチャンネル（1–16）
     *  @param cc   CC番号（0–127）
     *  @param val  CC値（0–127）
     */
    virtual void send_cc(uint8_t ch, uint8_t cc, uint8_t val) = 0;

    /** @brief Note On を送信する
     *  @param ch    MIDIチャンネル（1–16）
     *  @param note  ノート番号（0–127）
     *  @param vel   ベロシティ（0–127）
     */
    virtual void send_note_on(uint8_t ch, uint8_t note, uint8_t vel) = 0;

    /** @brief Note Off を送信する
     *  @param ch    MIDIチャンネル（1–16）
     *  @param note  ノート番号（0–127）
     */
    virtual void send_note_off(uint8_t ch, uint8_t note) = 0;

    /** @brief USB MIDI として PC に接続済みなら true */
    virtual bool is_connected() const = 0;

    /** @brief 完全な SysEx メッセージを1件読み出す（F0〜F7）
     *  @param buf      受信バッファ
     *  @param out_len  受信バイト数（F0・F7含む）
     *  @param max_len  buf のサイズ
     *  @return 完全な SysEx を受信したら true
     */
    virtual bool read_sysex(uint8_t* buf, size_t* out_len, size_t max_len) = 0;
};
