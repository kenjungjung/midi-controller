#include "button.h"
#include "esp_err.h"

GpioButton::GpioButton(gpio_num_t pin)
    : pin_(pin), state_(false), last_change_(0)
{
    gpio_config_t cfg = {
        .pin_bit_mask = (1ULL << pin_),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&cfg));
}

bool GpioButton::is_pressed() const {
    // アクティブLow: GPIO Low = 押下
    bool current = (gpio_get_level(pin_) == 0);
    TickType_t now = xTaskGetTickCount();

    if (current != state_ && (now - last_change_) >= DEBOUNCE_TICKS) {
        state_       = current;
        last_change_ = now;
    }
    return state_;
}

LedGpioButton::LedGpioButton(gpio_num_t sw_pin, gpio_num_t led_pin)
    : sw_pin_(sw_pin), led_pin_(led_pin), state_(false), last_change_(0)
{
    gpio_config_t sw_cfg = {
        .pin_bit_mask = (1ULL << sw_pin_),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&sw_cfg));

    gpio_config_t led_cfg = {
        .pin_bit_mask = (1ULL << led_pin_),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&led_cfg));
    gpio_set_level(led_pin_, 0); // 起動時は消灯
}

bool LedGpioButton::is_pressed() const {
    bool current = (gpio_get_level(sw_pin_) == 0);
    TickType_t now = xTaskGetTickCount();

    if (current != state_ && (now - last_change_) >= DEBOUNCE_TICKS) {
        state_       = current;
        last_change_ = now;
    }
    return state_;
}

void LedGpioButton::set_led(bool on) {
    gpio_set_level(led_pin_, on ? 1 : 0);
}
