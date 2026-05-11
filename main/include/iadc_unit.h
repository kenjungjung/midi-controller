#pragma once
#include "esp_adc/adc_oneshot.h"

/** @brief ADCユニット共通インターフェース
 *
 *  Adc1Unit / Adc2Unit を MuxController から差し替え可能にするための抽象基底。
 */
class IAdcUnit {
public:
    virtual ~IAdcUnit() = default;

    /** @brief チャンネルを設定する */
    virtual void config_channel(adc_channel_t ch, adc_atten_t atten) = 0;

    /** @brief rawADC値（0–4095）を読む */
    virtual int read_raw(adc_channel_t ch) const = 0;

    /** @brief ハンドルを返す（キャリブレーション設定に使用） */
    virtual adc_oneshot_unit_handle_t handle() const = 0;

    /** @brief ADCユニットID（キャリブレーション設定に使用） */
    virtual adc_unit_t unit_id() const = 0;
};
