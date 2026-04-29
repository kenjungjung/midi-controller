#include "led.h"
#include "config.h"
#include "led_strip.h"
#include "esp_log.h"

static const char* TAG = "LedManager";

LedManager::LedManager() {
    led_strip_config_t strip_cfg = {
        .strip_gpio_num          = static_cast<int>(PIN_LED_DATA),
        .max_leds                = static_cast<uint32_t>(NUM_LEDS),
        .led_model               = LED_MODEL_WS2812,             ///< PL9823-F5 は WS2812B 互換タイミング
        .color_component_format  = LED_STRIP_COLOR_COMPONENT_FMT_GRB, ///< PL9823-F5 は GRB 順
        .flags                   = { .invert_out = false },
    };
    led_strip_rmt_config_t rmt_cfg = {
        .clk_src        = RMT_CLK_SRC_DEFAULT,
        .resolution_hz  = 10 * 1000 * 1000, ///< 10 MHz → 0.1 μs 分解能
        .mem_block_symbols = 64,
        .flags          = { .with_dma = false },
    };
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_cfg, &rmt_cfg, &strip_));
    ESP_LOGI(TAG, "init OK: GPIO%d, %d LED(s)", PIN_LED_DATA, NUM_LEDS);
    clear();
}

LedManager::~LedManager() {
    if (strip_) {
        led_strip_del(strip_);
    }
}

void LedManager::set_color(int index, RgbColor color) {
    if (index < 0 || index >= NUM_LEDS) return;
    led_strip_set_pixel(strip_, static_cast<uint32_t>(index), color.r, color.g, color.b);
}

void LedManager::refresh() {
    led_strip_refresh(strip_);
}

void LedManager::clear() {
    led_strip_clear(strip_);
}
