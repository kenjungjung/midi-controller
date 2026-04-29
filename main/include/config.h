#pragma once
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"

constexpr gpio_num_t PIN_BUTTON_1 = GPIO_NUM_16;

constexpr gpio_num_t PIN_OLED_SDA = GPIO_NUM_8;
constexpr gpio_num_t PIN_OLED_SCL = GPIO_NUM_9;

constexpr adc_unit_t    ADC_UNIT_FADER  = ADC_UNIT_1;
constexpr adc_channel_t ADC_CH_FADER_1  = ADC_CHANNEL_6;  ///< GPIO7 = ADC1_CH6
constexpr adc_atten_t   ADC_ATTEN       = ADC_ATTEN_DB_12; ///< 0-3.3V full range

constexpr uint8_t MIDI_CHANNEL  = 1;
constexpr uint8_t CC_FADER_1    = 1;
constexpr uint8_t NOTE_BUTTON_1 = 36;
constexpr int     DEADBAND      = 4;   ///< minimum CC change threshold
constexpr int     FADER_RAW_MIN = 0;    ///< fader bottom (measured)
constexpr int     FADER_RAW_MAX = 3900; ///< fader top (measured)

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
