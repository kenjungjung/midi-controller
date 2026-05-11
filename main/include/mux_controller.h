#pragma once
#include <cstdint>
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "iadc_unit.h"
#include <array>
#include "config.h"

/** @brief CD4052 のどちらのセクションを読むかを指定する */
enum class MuxBus : uint8_t {
    X = 0, ///< Xセクション（X-COM）
    Y = 1, ///< Yセクション（Y-COM）
};

/** @brief 74HC4052AP デュアル4chマルチプレクサのコントローラー
 *
 *  A/B の2ビットで4チャンネルを選択し、X-COM または Y-COM を
 *  Adc1Unit 経由でADC読み取りする。/E はGNDに固定を前提とする。
 */
class MuxController {
public:
    /** @brief 初期化
     *  @param unit  共有 Adc1Unit
     *  @param x     X-COMが接続されたADCチャンネル
     *  @param y     Y-COMが接続されたADCチャンネル
     *  @param atten 入力レンジ
     *  @param pin_a セレクト A GPIO
     *  @param pin_b セレクト B GPIO
     */
    MuxController(IAdcUnit& unit, adc_channel_t x, adc_channel_t y,
                  adc_atten_t atten, gpio_num_t pin_a, gpio_num_t pin_b);
    ~MuxController();

    /** @brief 指定チャンネル・セクションを選択してキャリブレーション済み値（0–4095）を返す
     *  @param bus    MuxBus::X または MuxBus::Y 
     *  @param mux_ch チャンネル番号（0–3）
     */
    uint16_t read(MuxBus bus, uint8_t mux_ch);

private:
    void select(uint8_t mux_ch) const;
    uint16_t read_com(adc_channel_t ch, adc_cali_handle_t cali, bool cali_valid,
                      std::array<int, NUM_MUC_CH_MAX>& raws_prev, uint8_t mux_ch);

    IAdcUnit&          unit_;
    adc_cali_handle_t  cali_x_;
    adc_cali_handle_t  cali_y_;
    bool               cali_x_valid_;
    bool               cali_y_valid_;
    adc_channel_t      x_;
    adc_channel_t      y_;
    const gpio_num_t   pin_a_;
    const gpio_num_t   pin_b_;
    std::array<int, NUM_MUC_CH_MAX> raws_prev_x_; ///< Xセクションのデッドバンド比較用
    std::array<int, NUM_MUC_CH_MAX> raws_prev_y_; ///< Yセクションのデッドバンド比較用
};
