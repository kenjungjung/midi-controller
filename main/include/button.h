#pragma once
#include <cstdint>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"

/** @brief ボタン入力の抽象インターフェース */
class IButton {
public:
    virtual ~IButton() = default;
    /** @brief デバウンス済みのボタン状態を返す
     *  @return ボタンが押されていれば true
     */
    virtual bool is_pressed() const = 0;
};

/** @brief LED 付きボタンの抽象インターフェース（IButton を拡張） */
class IButtonLed : public IButton {
public:
    /** @brief 内蔵 LED を点灯 / 消灯する
     *  @param on true = 点灯
     */
    virtual void set_led(bool on) = 0;
};

/** @brief GPIO に直結したタクトスイッチ（内部プルアップ・アクティブLow）
 *
 *  is_pressed() 呼び出しごとに GPIO を読み、DEBOUNCE_MS 以上状態が
 *  継続した場合のみ内部状態を更新するデバウンス処理を行う。
 */
class GpioButton : public IButton {
public:
    /** @brief ピンを指定して初期化する（内部プルアップ有効）
     *  @param pin GPIO ピン番号
     */
    explicit GpioButton(gpio_num_t pin);

    bool is_pressed() const override;

private:
    static constexpr TickType_t DEBOUNCE_TICKS = pdMS_TO_TICKS(10);

    gpio_num_t         pin_;
    mutable bool       state_;         ///< デバウンス済み状態
    mutable TickType_t last_change_;   ///< 最後に状態が変化した tick
};

/** @brief ST12-401FCG などの LED 付き押しボタン
 *
 *  スイッチ入力は GpioButton と同じデバウンス処理を行う。
 *  LED は GPIO 出力（アクティブ High）で点灯 / 消灯する。
 *  電流制限抵抗は基板側で実装すること（ST12-401FCG 赤LED: Vf≈2V, If=20mA → 3.3V系で68Ω推奨）。
 */
class LedGpioButton : public IButtonLed {
public:
    /** @brief スイッチピンと LED ピンを指定して初期化する
     *  @param sw_pin  スイッチ入力 GPIO（内部プルアップ有効、アクティブ Low）
     *  @param led_pin LED 出力 GPIO（High = 点灯）
     */
    LedGpioButton(gpio_num_t sw_pin, gpio_num_t led_pin);

    bool is_pressed() const override;
    void set_led(bool on) override;

private:
    static constexpr TickType_t DEBOUNCE_TICKS = pdMS_TO_TICKS(10);

    gpio_num_t         sw_pin_;
    gpio_num_t         led_pin_;
    mutable bool       state_;         ///< デバウンス済み状態
    mutable TickType_t last_change_;   ///< 最後に状態が変化した tick
};

/** @brief テスト用スタブ: 常に false を返す */
class StubButton : public IButton {
public:
    /** @param pressed is_pressed() が返す固定値（デフォルト: false） */
    explicit StubButton(bool pressed = false) : pressed_(pressed) {}

    bool is_pressed() const override { return pressed_; }

    /** @brief テストコードから状態を上書きする */
    void set_pressed(bool v) { pressed_ = v; }

private:
    bool pressed_;
};

/** @brief テスト用スタブ: LED 状態を記録する */
class StubButtonLed : public IButtonLed {
public:
    /** @param pressed is_pressed() が返す固定値（デフォルト: false） */
    explicit StubButtonLed(bool pressed = false) : pressed_(pressed), led_(false) {}

    bool is_pressed() const override { return pressed_; }
    void set_led(bool on) override { led_ = on; }

    /** @brief テストコードから状態を上書きする */
    void set_pressed(bool v) { pressed_ = v; }

    /** @brief テストコードから LED 状態を取得する */
    bool led_on() const { return led_; }

private:
    bool pressed_; ///< 模擬ボタン状態
    bool led_;     ///< 模擬 LED 状態
};
