#pragma once
#include <cstdint>
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "adc_unit.h"

/** @brief アナログ入力の抽象インターフェース */
class IAnalogInput {
public:
    virtual ~IAnalogInput() = default;
    virtual uint16_t read() = 0;
};

/** @brief ESP32-S3 ADC1 を使ったアナログ入力実装 */
class AdcAnalogInput : public IAnalogInput {
public:
    /** @brief 初期化
     *  @param unit    共有 Adc1Unit
     *  @param channel ADCチャンネル
     *  @param atten   入力レンジ（デフォルト: ADC_ATTEN_DB_12 = 0–3.3V）
     */
    AdcAnalogInput(Adc1Unit& unit, adc_channel_t channel, adc_atten_t atten = ADC_ATTEN_DB_12);
    ~AdcAnalogInput() override;

    uint16_t read() override;

private:
    Adc1Unit&         unit_;
    adc_cali_handle_t cali_handle_;
    adc_channel_t     channel_;
    bool              cali_valid_;
    int               raw_prev_;
};