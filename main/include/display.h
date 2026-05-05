#pragma once
#include <cstdint>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "driver/i2c_master.h"
#include "midi_event.h"
#include <array>

/** @brief OLED 表示の抽象インターフェース */
class IDisplay {
public:
    virtual ~IDisplay() = default;

    /** @brief MIDI イベントをログに追記する（MidiTask から呼ぶ）
     *  @param ev       イベント
     *  @param outgoing true=送信, false=受信
     */
    virtual void push_event(const MidiEvent& ev, bool outgoing) = 0;

    /** @brief 画面を更新する（DisplayTask から 100ms 周期で呼ぶ） */
    virtual void render() = 0;

    /** @brief タイトル行を更新する（USB 接続状態変化時に呼ぶ） */
    virtual void set_title(const char* title) = 0;
};

/** @brief SSD1306 128x64 OLED: MIDI イベントを 8 行スクロールログで表示する */
class Display : public IDisplay {
public:
    Display();
    ~Display() override;

    void push_event(const MidiEvent& ev, bool outgoing) override;
    void render() override;
    void set_title(const char* title) override;

    static constexpr char TITLE[]      = "   Tp NUM VAL      ";
    static constexpr char DISCONECTED[] = "  DISCONNECTED";
    static constexpr char INITIAL_TITLE[] = "  kenjung MIDI";
    

private:
    static constexpr uint8_t LINES = 7;
    static constexpr uint8_t COLS  = 16;

    i2c_master_bus_handle_t bus_   = nullptr;
    i2c_master_dev_handle_t dev_   = nullptr;
    SemaphoreHandle_t       mutex_ = nullptr;

    char    log_[LINES][COLS + 1] = {};  ///< リングバッファ（各行 16 文字 + null）
    uint8_t head_  = 0;                  ///< 次に書き込む行インデックス
    bool    dirty_ = false;              ///< render() が必要なら true

    void send_cmd(uint8_t cmd);
    void send_data(const uint8_t* data, size_t len);
    void set_cursor(uint8_t col, uint8_t page);
    void draw_line(uint8_t page, const char* str);

    /** @brief MidiEvent を 16 文字の表示行にフォーマットする */
    static std::array<char, Display::COLS + 1> format_event(const MidiEvent& ev, bool outgoing);
};