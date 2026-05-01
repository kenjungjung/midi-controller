#pragma once
#include <cstdint>

/** @brief RGB カラー値 */
struct RgbColor {
    uint8_t r; ///< 赤 (0–255)
    uint8_t g; ///< 緑 (0–255)
    uint8_t b; ///< 青 (0–255)
};

/** @brief LED 制御の抽象インターフェース */
class ILed {
public:
    virtual ~ILed() = default;

    /** @brief 指定インデックスの LED の色を設定する（refresh() まで反映しない）
     *  @param index LED インデックス（0 始まり）
     *  @param color RGB カラー
     */
    virtual void set_color(int index, RgbColor color) = 0;

    /** @brief 全 LED の色をまとめて送信する */
    virtual void refresh() = 0;

    /** @brief 全 LED を消灯する */
    virtual void clear() = 0;
};

/** @brief PL9823-F5 (WS2812B 互換) を RMT で制御する */
class LedManager : public ILed {
public:
    /** @brief RMT チャンネルを初期化する（config.h の PIN_LED_DATA / NUM_LEDS を使用） */
    LedManager();
    ~LedManager() override;

    void set_color(int index, RgbColor color) override;
    void refresh() override;
    void clear() override;

private:
    struct led_strip_t* strip_ = nullptr; ///< led_strip ドライバハンドル
};