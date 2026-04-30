#pragma once
#include <cstdint>
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "analog_input.h"

/** @brief 74HC4052AP デュアル4chマルチプレクサのコントローラー
 *
 *  S0/S1 の2ビットで4チャンネルを選択し、ADC1で読み取る。
 *  /E (Active Low Enable) は初期化時にLowに固定する。
 *  ADC1 unitハンドルを唯一所有するため、他の AdcAnalogInput と併用不可。
 */
class MuxController {
public:
    /** @brief 初期化
     *  @param unit    ADCユニット（ADC_UNIT_1 のみ）
     *  @param x       X（Xセクションコモン）が接続されたADCチャンネル
     *  @param atten   入力レンジ
     *  @param pin_a   セレクト A GPIO
     *  @param pin_b   セレクト B GPIO
     */
    MuxController(adc_unit_t unit, adc_channel_t x, adc_atten_t atten,
                  gpio_num_t pin_a, gpio_num_t pin_b);
    ~MuxController();

    /** @brief 指定チャンネルを選択してADC値（0–4095）を返す */
    uint16_t read(uint8_t mux_ch);

private:
    /** @brief S0/S1 GPIO を mux_ch に応じてセット */
    void select(uint8_t mux_ch);

    adc_oneshot_unit_handle_t adc_handle_;
    adc_cali_handle_t         cali_handle_;
    bool                      cali_valid_;
    adc_channel_t             x_;
    gpio_num_t                pin_a_;
    gpio_num_t                pin_b_;
};
