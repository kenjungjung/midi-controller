#pragma once
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"

// ── ピン定義 ──────────────────────────────────────
constexpr gpio_num_t PIN_FADER_1  = GPIO_NUM_7;   ///< ADC1_CH6
constexpr gpio_num_t PIN_BUTTON_1 = GPIO_NUM_16;

// ── ADC ───────────────────────────────────────────
constexpr adc_unit_t    ADC_UNIT_FADER   = ADC_UNIT_1;
constexpr adc_channel_t ADC_CH_FADER_1  = ADC_CHANNEL_6;
constexpr adc_atten_t   ADC_ATTEN       = ADC_ATTEN_DB_12; ///< 0–3.3V フルレンジ

// ── MIDI ──────────────────────────────────────────
constexpr uint8_t MIDI_CHANNEL   = 1;
constexpr uint8_t CC_FADER_1     = 1;
constexpr uint8_t NOTE_BUTTON_1  = 36;
constexpr int     DEADBAND       = 4;  ///< CC値変化の最小閾値

// ── FreeRTOS ──────────────────────────────────────
constexpr int MIDI_QUEUE_LEN  = 32;
constexpr int STACK_USB       = 4096;
constexpr int STACK_INPUT     = 4096;
constexpr int STACK_MIDI      = 4096;
constexpr int PRIO_USB        = 6;
constexpr int PRIO_INPUT      = 5;
constexpr int PRIO_MIDI       = 4;
constexpr int CORE_USB        = 1;  ///< UsbTask + MidiTask
constexpr int CORE_INPUT      = 0;  ///< InputTask（USB割り込みと分離）

// ── ビルド切り替え ────────────────────────────────
// #define USE_STUBS  ← 定義するとハードウェアをスタブに差し替え
