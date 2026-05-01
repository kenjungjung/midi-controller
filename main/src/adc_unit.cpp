#include "adc_unit.h"
#include "esp_log.h"

Adc1Unit::Adc1Unit()
{
    adc_oneshot_unit_init_cfg_t cfg = {};
    cfg.unit_id  = ADC_UNIT_1;
    cfg.ulp_mode = ADC_ULP_MODE_DISABLE;
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&cfg, &handle_));
}

Adc1Unit::~Adc1Unit()
{
    adc_oneshot_del_unit(handle_);
}

void Adc1Unit::config_channel(adc_channel_t ch, adc_atten_t atten)
{
    adc_oneshot_chan_cfg_t cfg = {};
    cfg.atten    = atten;
    cfg.bitwidth = ADC_BITWIDTH_12;
    ESP_ERROR_CHECK(adc_oneshot_config_channel(handle_, ch, &cfg));
}

int Adc1Unit::read_raw(adc_channel_t ch) const
{
    int raw = 0;    
    ESP_ERROR_CHECK(adc_oneshot_read(handle_, ch, &raw)); // ダミーリード
    ESP_ERROR_CHECK(adc_oneshot_read(handle_, ch, &raw));
    return raw;
}
