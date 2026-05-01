#pragma once
#include <cstdint>
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

/** @brief アナログ入力の抽象インターフェース */
class IAnalogInput {
public:
    virtual ~IAnalogInput() = default;
    /** @brief ADCキャリブレーション済みの電圧をスケール変換した値を返す
     *  @return mV値をADC12bitレンジ（0–4095）に再スケールした値
     *          （0mV → 0, 3300mV → 4095）
     */
    virtual uint16_t read() const = 0;
};

/** @brief ESP32-S3 ADC1 を使ったアナログ入力実装 */
class AdcAnalogInput : public IAnalogInput {
public:
    /** @brief ADCユニットとチャンネルを指定して初期化する
     *  @param channel ADCチャンネル
     *  @param atten   入力レンジ（デフォルト: ADC_ATTEN_DB_12 = 0–3.3V）
     */
    AdcAnalogInput(adc_channel_t channel,
                   adc_atten_t atten = ADC_ATTEN_DB_12);
    ~AdcAnalogInput() override;

    uint16_t read() const override;

private:
    adc_oneshot_unit_handle_t adc_handle_;
    adc_cali_handle_t         cali_handle_;
    adc_channel_t             channel_;
    bool                      cali_valid_;
};