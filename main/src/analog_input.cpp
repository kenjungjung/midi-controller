#include "analog_input.h"
#include "esp_log.h"

static const char* TAG = "AdcAnalogInput";

AdcAnalogInput::AdcAnalogInput(Adc1Unit& unit, adc_channel_t channel, adc_atten_t atten)
    : unit_(unit), cali_handle_(nullptr),
      channel_(channel), cali_valid_(false)
{
    unit_.config_channel(channel, atten);

    adc_cali_curve_fitting_config_t cali_cfg = {
        .unit_id  = ADC_UNIT_1,
        .chan     = channel,
        .atten    = atten,
        .bitwidth = ADC_BITWIDTH_12,
    };
    esp_err_t ret = adc_cali_create_scheme_curve_fitting(&cali_cfg, &cali_handle_);
    cali_valid_ = (ret == ESP_OK);
    if (!cali_valid_) {
        ESP_LOGW(TAG, "calibration unavailable, using raw values");
    }
}

AdcAnalogInput::~AdcAnalogInput()
{
    if (cali_valid_ && cali_handle_) {
        adc_cali_delete_scheme_curve_fitting(cali_handle_);
    }
}

uint16_t AdcAnalogInput::read() const
{
    int raw = unit_.read_raw(channel_);

    if (!cali_valid_) {
        return static_cast<uint16_t>(raw);
    }

    int voltage_mv = 0;
    adc_cali_raw_to_voltage(cali_handle_, raw, &voltage_mv);
    return static_cast<uint16_t>(voltage_mv);
}
