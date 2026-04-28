#pragma once
#include "esp_adc/adc_oneshot.h"
#include "ssd1306.h"

// ハードウェア構成を一箇所で管理する。ピンを変えるときはここだけ触る。
namespace hw {

inline constexpr SSD1306Config display = {
    .sda_pin = 4,
    .scl_pin = 5,
};

// GPIO1,2,39-42はモータードライバ予約
inline constexpr adc_unit_t    pot_unit    = ADC_UNIT_1;
inline constexpr adc_channel_t pot_channel = ADC_CHANNEL_5; // GPIO6

} // namespace hw
