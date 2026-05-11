#pragma once
#include "esp_adc/adc_oneshot.h"
#include "iadc_unit.h"

/** @brief ADC2ユニットハンドルの唯一のオーナー
 *
 *  ESP-IDFの制約により adc_oneshot_new_unit() は同一ユニットにつき1回のみ呼べる。
 *  複数の入力クラスでこのクラスを共有することで競合を防ぐ。
 *  ESP32-S3では ADC2 は Wi-Fi 使用中に利用不可である点に注意。
 */
class Adc2Unit : public IAdcUnit {
public:
    /** @brief ADC_UNIT_2 を初期化する */
    Adc2Unit();
    ~Adc2Unit() override;

    void config_channel(adc_channel_t ch, adc_atten_t atten) override;
    int  read_raw(adc_channel_t ch) const override;
    adc_oneshot_unit_handle_t handle() const override { return handle_; }
    adc_unit_t unit_id() const override { return ADC_UNIT_2; }

private:
    adc_oneshot_unit_handle_t handle_;
};
