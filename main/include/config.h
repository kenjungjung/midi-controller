#pragma once
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"

// ─── OLED ─────────────────────────────────────────────────
constexpr gpio_num_t PIN_OLED_SDA = GPIO_NUM_47;
constexpr gpio_num_t PIN_OLED_SCL = GPIO_NUM_48;

// ─── LED ──────────────────────────────────────────────────
constexpr gpio_num_t PIN_LED_DATA = GPIO_NUM_42;
constexpr int        NUM_LEDS     = 5;
constexpr uint8_t    LED_DARKNESS = 50;

// ─── 74HC4052AP マルチプレクサ ────────────────────────────
//   全体
constexpr int NUM_FADERS  = 0; ///< フェーダー本数
constexpr int NUM_KNOBS   = 8; ///< ノブ本数（RV5-RV12）
constexpr int NUM_BUTTONS = 0; ///< ボタン本数
constexpr adc_atten_t   ADC_ATTEN     = ADC_ATTEN_DB_12; ///< 0–3.3V full range
constexpr int           MUX_SETTLE_US    = 100;           ///< チャンネル切替後の整定待ち [µs]
constexpr int           ADC_OVERSAMPLE_N = 8;            ///< ADCオーバーサンプリング回数（交互ノイズ平均化）
constexpr int           NUM_MUC_CH_MAX = 4; ///< 4052のch
constexpr gpio_num_t    PIN_MUX_A    = GPIO_NUM_2;      ///< U2 セレクト A（GPIO2）
constexpr gpio_num_t    PIN_MUX_B    = GPIO_NUM_3;      ///< U2 セレクト B（GPIO3）
//   U2 コモン（ADC1）
constexpr adc_channel_t ADC_CH_U2_X = ADC_CHANNEL_4;   ///< U2 X-COM（GPIO5 = ADC1_CH4）
constexpr adc_channel_t ADC_CH_U2_Y = ADC_CHANNEL_5;   ///< U2 Y-COM（GPIO6 = ADC1_CH5）
//   U5セレクトピン（GPIO出力、U2とは独立）
constexpr gpio_num_t    PIN_U5_A     = GPIO_NUM_11;     ///< U5 セレクト A（GPIO11）
constexpr gpio_num_t    PIN_U5_B     = GPIO_NUM_12;     ///< U5 セレクト B（GPIO12）
//   U5 コモン（ADC2）
constexpr adc_channel_t ADC_CH_U5_X = ADC_CHANNEL_2;   ///< U5 X-COM（GPIO13 = ADC2_CH2）
constexpr adc_channel_t ADC_CH_U5_Y = ADC_CHANNEL_3;   ///< U5 Y-COM（GPIO14 = ADC2_CH3）



// adcテスト用
constexpr adc_channel_t ADC_CH_TEST  = ADC_CHANNEL_5;  ///< GPIO6 = ADC1_CH5


// ─── MIDI ─────────────────────────────────────────────────
constexpr uint8_t MIDI_CHANNEL  = 2;
constexpr uint8_t CC_FADER_1    = 1;   ///< フェーダー CC: 1〜NUM_FADERS
constexpr uint8_t CC_KNOB_1     = 20;  ///< ノブ CC: 20〜(20+NUM_KNOBS-1)
constexpr uint8_t NOTE_BUTTON_1 = 36;  ///< ボタン Note: 36〜(36+NUM_BUTTONS-1)
constexpr int     RAW_THRETHOLD  = 16;  ///< minimum adc_raw change threshold
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