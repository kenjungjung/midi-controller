#pragma once
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"

// ─── OLED ─────────────────────────────────────────────────
constexpr gpio_num_t PIN_OLED_SDA = GPIO_NUM_8;
constexpr gpio_num_t PIN_OLED_SCL = GPIO_NUM_9;

// ─── LED ──────────────────────────────────────────────────
constexpr gpio_num_t PIN_LED_DATA = GPIO_NUM_4;
constexpr int        NUM_LEDS     = 5;
constexpr uint8_t    LED_DARKNESS = 50;

// ─── 74HC4052AP マルチプレクサ ────────────────────────────
//   セレクト・禁止ピン（GPIO出力）
constexpr gpio_num_t    PIN_MUX_A    = GPIO_NUM_1;      ///< セレクト A          （ADC1_CH0）
constexpr gpio_num_t    PIN_MUX_B    = GPIO_NUM_2;      ///< セレクト B          （ADC1_CH1）
//   Xコモン（ADC入力） ※PIN_MUX_A/Bと重複しないGPIOを選ぶこと
constexpr adc_channel_t ADC_CH_MUX_X = ADC_CHANNEL_6;  ///< Xコモン（GPIO7）
constexpr adc_atten_t   ADC_ATTEN     = ADC_ATTEN_DB_12; ///< 0–3.3V full range
constexpr int           MUX_SETTLE_US = 10;              ///< チャンネル切替後の整定待ち [µs]

constexpr int NUM_FADERS  = 1; ///< フェーダー本数（Phase4 で 12）
constexpr int NUM_KNOBS   = 1; ///< ノブ本数
constexpr int NUM_BUTTONS = 1; ///< ボタン本数（Phase3 で 10）

// adcテスト用
constexpr adc_channel_t ADC_CH_TEST  = ADC_CHANNEL_5;  ///< GPIO6 = ADC1_CH5


// ─── MIDI ─────────────────────────────────────────────────
constexpr uint8_t MIDI_CHANNEL  = 1;
constexpr uint8_t CC_FADER_1    = 1;   ///< フェーダー CC: 1〜NUM_FADERS
constexpr uint8_t CC_KNOB_1     = 20;  ///< ノブ CC: 20〜(20+NUM_KNOBS-1)
constexpr uint8_t NOTE_BUTTON_1 = 36;  ///< ボタン Note: 36〜(36+NUM_BUTTONS-1)
constexpr int     DEADBAND      = 1;   ///< minimum CC change threshold
constexpr int     FADER_RAW_MIN = 200; ///< fader bottom (measured)
constexpr int     FADER_RAW_MAX = 3972;///< fader top (measured)
constexpr int     KNOB_RAW_MIN  = 0;    ///< knob bottom (measured)
constexpr int     KNOB_RAW_MAX  = 3972; ///< knob top (measured)
constexpr int     BTN_RAW_THRETHOLD = 1600; ///< button vol 127/0 threthold
constexpr int     BTN_MIDI_THRETHOLD = 64; ///< button midi on/off threthold
 
// ─── タスク設定 ───────────────────────────────────────────
constexpr int MIDI_QUEUE_LEN = 32;
constexpr int STACK_USB      = 4096;
constexpr int STACK_INPUT    = 4096;
constexpr int STACK_MIDI     = 4096;
constexpr int STACK_DISPLAY  = 3072;
constexpr int PRIO_USB       = 6;
constexpr int PRIO_INPUT     = 5;
constexpr int PRIO_MIDI      = 4;
constexpr int PRIO_DISPLAY   = 3;
constexpr int CORE_USB       = 1;  ///< UsbTask + MidiTask
constexpr int CORE_INPUT     = 0;  ///< InputTask + DisplayTask

// #define USE_STUBS  // uncomment to replace hardware with stubs
