#pragma once
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"

constexpr gpio_num_t PIN_BUTTON_1     = GPIO_NUM_16; ///< タクトスイッチ（既存）

constexpr gpio_num_t PIN_LED_BTN_SW  = GPIO_NUM_17; ///< ST12-401FCG スイッチ入力（アクティブLow）
constexpr gpio_num_t PIN_LED_BTN_LED = GPIO_NUM_18; ///< ST12-401FCG LED出力（High=点灯、68Ω直列）

constexpr gpio_num_t PIN_OLED_SDA = GPIO_NUM_8;
constexpr gpio_num_t PIN_OLED_SCL = GPIO_NUM_9;

constexpr adc_unit_t    ADC_UNIT_FADER  = ADC_UNIT_1;
constexpr adc_channel_t ADC_CH_FADER_1  = ADC_CHANNEL_6;  ///< GPIO7 = ADC1_CH6
constexpr adc_atten_t   ADC_ATTEN       = ADC_ATTEN_DB_12; ///< 0-3.3V full range

constexpr uint8_t MIDI_CHANNEL  = 1;
constexpr uint8_t CC_FADER_1    = 1;   ///< フェーダー CC: 1〜NUM_FADERS
constexpr uint8_t CC_KNOB_1     = 20;  ///< ノブ CC: 20〜(20+NUM_KNOBS-1)
constexpr uint8_t NOTE_BUTTON_1 = 36;  ///< ボタン Note: 36〜(36+NUM_BUTTONS-1)
constexpr int     DEADBAND      = 4;   ///< minimum CC change threshold
constexpr int     FADER_RAW_MIN = 400;  ///< fader bottom (measured)
constexpr int     FADER_RAW_MAX = 3700; ///< fader top (measured)

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

constexpr int NUM_FADERS  = 1;  ///< フェーダー本数（Phase4 で 12）
constexpr int NUM_KNOBS   = 3;  ///< ノブ本数（Bank1/2 共通）
constexpr int NUM_BUTTONS = 1;  ///< ボタン本数（Phase3 で 10）

constexpr gpio_num_t PIN_LED_DATA = GPIO_NUM_5; ///< PL9823-F5 data line (RMT) ※GPIO4損傷疑いで変更
constexpr int        NUM_LEDS     = 1;           ///< PL9823-F5 count

// #define USE_STUBS  // uncomment to replace hardware with stubs
