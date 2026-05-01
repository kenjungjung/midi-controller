#pragma once
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"

/** @brief ADC1ユニットハンドルの唯一のオーナー
 *
 *  ESP-IDFの制約により adc_oneshot_new_unit() は同一ユニットにつき1回のみ呼べる。
 *  AdcAnalogInput と MuxController でこのクラスを共有することで競合を防ぐ。
 */
class Adc1Unit {
public:
    /** @brief ADC_UNIT_1 を初期化する */
    Adc1Unit();
    ~Adc1Unit();

    /** @brief チャンネルを設定する（各入力クラスの初期化時に呼ぶ）
     *  @param ch    ADCチャンネル
     *  @param atten 入力レンジ
     */
    void config_channel(adc_channel_t ch, adc_atten_t atten);

    /** @brief rawADC値（0–4095）を読む */
    int read_raw(adc_channel_t ch) const;

    /** @brief ハンドルを返す（キャリブレーション設定に使用） */
    adc_oneshot_unit_handle_t handle() const { return handle_; }

private:
    adc_oneshot_unit_handle_t handle_;
};
