#pragma once
#include "esp_adc/adc_oneshot.h"

class Potentiometer {
public:
    struct Config {
        adc_unit_t     unit;
        adc_channel_t  channel;
        adc_atten_t    atten    = ADC_ATTEN_DB_12;
        adc_bitwidth_t bitwidth = ADC_BITWIDTH_12;
        int            raw_max  = 4095; // 2^bitwidth - 1
        int            midi_max = 127;
    };

    struct Reading {
        int raw;
        int midi;
    };

    explicit Potentiometer(const Config &cfg);
    ~Potentiometer();

    Reading read() const;

private:
    Config                    cfg_;
    adc_oneshot_unit_handle_t adc_;
};
