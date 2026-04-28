#pragma once
#include <stdint.h>
#include <stddef.h>

struct SSD1306Config {
    int      sda_pin;
    int      scl_pin;
    int      i2c_port = 0;       // I2C_NUM_0
    uint8_t  i2c_addr = 0x3C;
    uint32_t freq_hz  = 400'000;
};

class SSD1306 {
public:
    explicit SSD1306(const SSD1306Config &cfg);

    void clear_screen();
    void draw_text(uint8_t col, uint8_t page, const char *str);

private:
    SSD1306Config cfg_;

    void send_cmd(uint8_t cmd);
    void send_data(const uint8_t *data, size_t len);
    void set_cursor(uint8_t col, uint8_t page);
};
