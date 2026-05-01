#pragma once
#include <cstdint>
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "adc_unit.h"

/** @brief 74HC4052AP デュアル4chマルチプレクサのコントローラー
 *
 *  A/B の2ビットで4チャンネルを選択し、Adc1Unit 経由でADC読み取りを行う。
 *  /E はGNDに固定を前提とする。
 */
class MuxController {
public:
    /** @brief 初期化
     *  @param unit  共有 Adc1Unit
     *  @param x     X（Xセクションコモン）が接続されたADCチャンネル
     *  @param atten 入力レンジ
     *  @param pin_a セレクト A GPIO
     *  @param pin_b セレクト B GPIO
     */
    MuxController(Adc1Unit& unit, adc_channel_t x, adc_atten_t atten,
                  gpio_num_t pin_a, gpio_num_t pin_b);
    ~MuxController();

    /** @brief 指定チャンネルを選択してキャリブレーション済み値（0–4095）を返す */
    uint16_t read(uint8_t mux_ch) const;

private:
    void select(uint8_t mux_ch) const;

    Adc1Unit&               unit_;
    adc_cali_handle_t       cali_handle_;
    bool                    cali_valid_;
    adc_channel_t           x_;
    const gpio_num_t      pin_a_;
    const gpio_num_t      pin_b_;
};
