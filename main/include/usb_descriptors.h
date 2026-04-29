#pragma once
#include "tusb.h"

// ── インターフェース番号 ───────────────────────────────────────
enum {
    ITF_NUM_MIDI = 0,   ///< Audio Control
    ITF_NUM_MIDI_STREAMING, ///< MIDI Streaming
    ITF_NUM_TOTAL
};

// ── エンドポイント番号 ────────────────────────────────────────
enum {
    EPNUM_MIDI = 1,     ///< MIDI bulk OUT(host→device) / IN(device→host) 共用番号
};

// ── ディスクリプタ総サイズ ─────────────────────────────────────
#define TUSB_DESC_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_MIDI_DESC_LEN)

/** @brief Full-Speed 設定ディスクリプタ（MIDI 1本）*/
static const uint8_t desc_fs_cfg[] = {
    // Config: num_interfaces, string_index, total_len, attribute, power_mA
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, TUSB_DESC_TOTAL_LEN,
                          TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),

    // MIDI: interface_num, string_index, ep_out, ep_in, ep_size
    TUD_MIDI_DESCRIPTOR(ITF_NUM_MIDI, 0,
                        EPNUM_MIDI, (0x80 | EPNUM_MIDI), 64),
};
