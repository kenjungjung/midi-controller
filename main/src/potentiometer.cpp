#include "potentiometer.h"

Potentiometer::Potentiometer(const Config &cfg) : cfg_(cfg)
{
    adc_oneshot_unit_init_cfg_t unit_cfg = { .unit_id = cfg_.unit };
    adc_oneshot_new_unit(&unit_cfg, &adc_);

    adc_oneshot_chan_cfg_t ch_cfg = {
        .atten    = cfg_.atten,
        .bitwidth = cfg_.bitwidth,
    };
    adc_oneshot_config_channel(adc_, cfg_.channel, &ch_cfg);
}

Potentiometer::~Potentiometer()
{
    adc_oneshot_del_unit(adc_);
}

Potentiometer::Reading Potentiometer::read() const
{
    int raw = 0;
    adc_oneshot_read(adc_, cfg_.channel, &raw);
    return { raw, raw * cfg_.midi_max / cfg_.raw_max };
}
