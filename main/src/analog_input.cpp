#include "analog_input.h"
#include "esp_log.h"
#include "esp_err.h"

static const char* TAG = "AdcAnalogInput";

AdcAnalogInput::AdcAnalogInput(adc_unit_t unit, adc_channel_t channel,
                               adc_atten_t atten)
    : adc_handle_(nullptr), cali_handle_(nullptr),
      channel_(channel), cali_valid_(false)
{
    // ADC ユニット初期化
    adc_oneshot_unit_init_cfg_t unit_cfg = {
        .unit_id  = unit,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&unit_cfg, &adc_handle_));

    // チャンネル設定
    adc_oneshot_chan_cfg_t chan_cfg = {
        .atten    = atten,
        .bitwidth = ADC_BITWIDTH_12,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle_, channel_, &chan_cfg));

    // ESP32-S3 は curve_fitting のみ対応
    adc_cali_curve_fitting_config_t cali_cfg = {
        .unit_id  = unit,
        .chan     = channel_,
        .atten    = atten,
        .bitwidth = ADC_BITWIDTH_12,
    };
    esp_err_t ret = adc_cali_create_scheme_curve_fitting(&cali_cfg, &cali_handle_);
    if (ret == ESP_OK) {
        cali_valid_ = true;
    } else {
        ESP_LOGW(TAG, "ADC: calibration not available (err=0x%x), using raw values", ret);
    }
}

AdcAnalogInput::~AdcAnalogInput() {
    if (cali_valid_ && cali_handle_) {
        adc_cali_delete_scheme_curve_fitting(cali_handle_);
    }
    if (adc_handle_) {
        adc_oneshot_del_unit(adc_handle_);
    }
}

uint16_t AdcAnalogInput::read() const {
    int raw = 0;
    ESP_ERROR_CHECK(adc_oneshot_read(adc_handle_, channel_, &raw));

    if (cali_valid_) {
        int voltage_mv = 0;
        ESP_ERROR_CHECK(adc_cali_raw_to_voltage(cali_handle_, raw, &voltage_mv));
        // mV（0–3300）を 0–4095 にスケール
        return static_cast<uint16_t>(voltage_mv * 4095 / 3300);
    }
    return static_cast<uint16_t>(raw);
}
