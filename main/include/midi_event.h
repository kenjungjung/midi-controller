#pragma once
#include <cstdint>

/** @brief タスク間で受け渡す MIDI イベント */
struct MidiEvent {
    /** @brief イベント種別 */
    enum class Type : uint8_t { CC, NOTE_ON, NOTE_OFF };

    Type    type;
    uint8_t channel;  ///< 1-16
    uint8_t number;   ///< CC番号 or ノート番号
    uint8_t value;    ///< 0-127
};
