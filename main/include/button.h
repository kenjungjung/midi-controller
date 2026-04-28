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
