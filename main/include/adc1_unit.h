#pragma once
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "iadc_unit.h"

/** @brief ADC1ユニットハンドルの唯一のオーナー
 *
 *  ESP-IDFの制約により adc_oneshot_new_unit() は同一ユニットにつき1回のみ呼べる。
 *  AdcAnalogInput と MuxController でこのクラスを共有することで競合を防ぐ。
 */
class Adc1Unit : public IAdcUnit {
public:
    /** @brief ADC_UNIT_1 を初期化する */
    Adc1Unit();
    ~Adc1Unit() override;

    void config_channel(adc_channel_t ch, adc_atten_t atten) override;
    int  read_raw(adc_channel_t ch) const override;
    adc_oneshot_unit_handle_t handle() const override { return handle_; }
    adc_unit_t unit_id() const override { return ADC_UNIT_1; }

private:
    adc_oneshot_unit_handle_t handle_;
};
