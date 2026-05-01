#include "mux_controller.h"
#include "esp_rom_sys.h"
#include "esp_log.h"
#include "config.h"

static const char* TAG = "MuxAdc";

MuxController::MuxController(adc_channel_t x, adc_atten_t atten,
                             gpio_num_t pin_a, gpio_num_t pin_b)
    : cali_handle_(nullptr), cali_valid_(false),
      x_(x), pin_a_(pin_a), pin_b_(pin_b)
{
    // ADC unit 初期化
    adc_oneshot_unit_init_cfg_t unit_cfg = {};
    unit_cfg.unit_id   = ADC_UNIT_1;
    unit_cfg.ulp_mode  = ADC_ULP_MODE_DISABLE;
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&unit_cfg, &adc_handle_));

    // チャンネル設定
    adc_oneshot_chan_cfg_t chan_cfg = {};
    chan_cfg.atten    = atten;
    chan_cfg.bitwidth = ADC_BITWIDTH_12;
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle_, x, &chan_cfg));

    // キャリブレーション（ESP32-S3: curve fitting のみ）
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

    // セレクト GPIO 初期化
    gpio_config_t io_cfg = {};
    io_cfg.pin_bit_mask = (1ULL << pin_a) | (1ULL << pin_b);
    io_cfg.mode         = GPIO_MODE_OUTPUT;
    io_cfg.pull_up_en   = GPIO_PULLUP_DISABLE;
    io_cfg.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_cfg.intr_type    = GPIO_INTR_DISABLE;
    ESP_ERROR_CHECK(gpio_config(&io_cfg));

    // デフォルトチャンネル 0 を選択
    select(0);
}

MuxController::~MuxController()
{
    if (cali_valid_) {
        adc_cali_delete_scheme_curve_fitting(cali_handle_);
    }
    adc_oneshot_del_unit(adc_handle_);
}

void MuxController::select(uint8_t mux_ch)
{
    gpio_set_level(pin_a_, (mux_ch >> 0) & 1);
    gpio_set_level(pin_b_, (mux_ch >> 1) & 1);
}

uint16_t MuxController::read(uint8_t mux_ch)
{
    select(mux_ch);
    esp_rom_delay_us(MUX_SETTLE_US);

    int raw = 0;
    adc_oneshot_read(adc_handle_, x_, &raw);

    if (cali_valid_) {
        int voltage_mv = 0;
        adc_cali_raw_to_voltage(cali_handle_, raw, &voltage_mv);
        return static_cast<uint16_t>(voltage_mv * 4095 / 3300);
    }
    
    return static_cast<uint16_t>(raw);
}