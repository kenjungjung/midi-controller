#include "mux_controller.h"
#include "iadc_unit.h"
#include "esp_rom_sys.h"
#include "esp_log.h"
#include "config.h"

static const char* TAG = "MuxController";

static adc_cali_handle_t create_cali(adc_unit_t unit_id, adc_channel_t ch, adc_atten_t atten, bool& valid)
{
    adc_cali_handle_t handle = nullptr;
    adc_cali_curve_fitting_config_t cfg = {};
    cfg.unit_id  = unit_id;
    cfg.chan     = ch;
    cfg.atten    = atten;
    cfg.bitwidth = ADC_BITWIDTH_12;
    esp_err_t ret = adc_cali_create_scheme_curve_fitting(&cfg, &handle);
    valid = (ret == ESP_OK);
    if (!valid) {
        ESP_LOGW(TAG, "calibration unavailable for ch%d, using raw values", ch);
    }
    return handle;
}

MuxController::MuxController(IAdcUnit& unit, adc_channel_t x, adc_channel_t y,
                             adc_atten_t atten, gpio_num_t pin_a, gpio_num_t pin_b)
    : unit_(unit), cali_x_(nullptr), cali_y_(nullptr),
      cali_x_valid_(false), cali_y_valid_(false),
      x_(x), y_(y), pin_a_(pin_a), pin_b_(pin_b)
{
    raws_prev_x_.fill(0xFFF);
    raws_prev_y_.fill(0xFFF);

    unit_.config_channel(x, atten);
    unit_.config_channel(y, atten);

    cali_x_ = create_cali(unit.unit_id(), x, atten, cali_x_valid_);
    cali_y_ = create_cali(unit.unit_id(), y, atten, cali_y_valid_);

    gpio_config_t io_cfg = {};
    io_cfg.pin_bit_mask = (1ULL << pin_a) | (1ULL << pin_b);
    io_cfg.mode         = GPIO_MODE_OUTPUT;
    io_cfg.pull_up_en   = GPIO_PULLUP_DISABLE;
    io_cfg.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_cfg.intr_type    = GPIO_INTR_DISABLE;
    ESP_ERROR_CHECK(gpio_config(&io_cfg));

    select(0);
}

MuxController::~MuxController()
{
    if (cali_x_valid_) adc_cali_delete_scheme_curve_fitting(cali_x_);
    if (cali_y_valid_) adc_cali_delete_scheme_curve_fitting(cali_y_);
}

void MuxController::select(uint8_t mux_ch) const
{
    gpio_set_level(pin_a_, (mux_ch >> 0) & 1);
    gpio_set_level(pin_b_, (mux_ch >> 1) & 1);
}

uint16_t MuxController::read_com(adc_channel_t ch, adc_cali_handle_t cali, bool cali_valid,
                                  std::array<int, NUM_MUC_CH_MAX>& raws_prev, uint8_t mux_ch)
{
    int raw = unit_.read_raw(ch);

    int normalized = raw;
    if (cali_valid) {
        int voltage_mv = 0;
        adc_cali_raw_to_voltage(cali, raw, &voltage_mv);
        normalized = voltage_mv * 4095 / 3300;
    }
    if (normalized < 0)    normalized = 0;
    if (normalized > 4095) normalized = 4095;

    if (std::abs(raws_prev[mux_ch] - normalized) <= RAW_THRETHOLD) {
        return static_cast<uint16_t>(raws_prev[mux_ch]);
    }
    raws_prev[mux_ch] = normalized;
    return static_cast<uint16_t>(normalized);
}

uint16_t MuxController::read(MuxBus bus, uint8_t mux_ch)
{
    select(mux_ch);
    esp_rom_delay_us(MUX_SETTLE_US);

    if (bus == MuxBus::X) {
        return read_com(x_, cali_x_, cali_x_valid_, raws_prev_x_, mux_ch);
    } else {
        return read_com(y_, cali_y_, cali_y_valid_, raws_prev_y_, mux_ch);
    }
}
