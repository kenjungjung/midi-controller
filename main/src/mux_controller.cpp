#include "mux_controller.h"
#include "esp_rom_sys.h"
#include "esp_log.h"
#include "config.h"

static const char* TAG = "MuxController";

MuxController::MuxController(Adc1Unit& unit, adc_channel_t x, adc_atten_t atten,
                             gpio_num_t pin_a, gpio_num_t pin_b)
    : unit_(unit), cali_handle_(nullptr), cali_valid_(false),
      x_(x), pin_a_(pin_a), pin_b_(pin_b)
{
    unit_.config_channel(x, atten);

    adc_cali_curve_fitting_config_t cali_cfg = {};
    cali_cfg.unit_id  = ADC_UNIT_1;
    cali_cfg.chan     = x;
    cali_cfg.atten    = atten;
    cali_cfg.bitwidth = ADC_BITWIDTH_12;
    esp_err_t ret = adc_cali_create_scheme_curve_fitting(&cali_cfg, &cali_handle_);
    cali_valid_ = (ret == ESP_OK);
    if (!cali_valid_) {
        ESP_LOGW(TAG, "calibration unavailable, using raw values");
    }

    gpio_config_t io_cfg = {};
    io_cfg.pin_bit_mask = (1ULL << pin_a) | (1ULL << pin_b);
    io_cfg.mode         = GPIO_MODE_OUTPUT;
    io_cfg.pull_up_en   = GPIO_PULLUP_DISABLE;
    io_cfg.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_cfg.intr_type    = GPIO_INTR_DISABLE;
    ESP_ERROR_CHECK(gpio_config(&io_cfg));

    select(0);
}

MuxController::~MuxController()
{
    if (cali_valid_) {
        adc_cali_delete_scheme_curve_fitting(cali_handle_);
    }
}

void MuxController::select(uint8_t mux_ch) const
{
    gpio_set_level(pin_a_, (mux_ch >> 0) & 1);
    gpio_set_level(pin_b_, (mux_ch >> 1) & 1);
}

uint16_t MuxController::read(uint8_t mux_ch) const
{
    select(mux_ch);
    esp_rom_delay_us(MUX_SETTLE_US);

    int raw = unit_.read_raw(x_);

    if (cali_valid_) {
        int voltage_mv = 0;
        adc_cali_raw_to_voltage(cali_handle_, raw, &voltage_mv);
        return static_cast<uint16_t>(voltage_mv * 4095 / 3300);
    }
    return static_cast<uint16_t>(raw);
}
