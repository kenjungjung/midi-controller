#include "mux_channel.h"
#include "esp_log.h"

MuxChannel::MuxChannel(MuxController& ctrl, uint8_t ch, int vol_min, int vol_max)
    : ctrl_(ctrl), ch_(ch), vol_min_(vol_min), vol_max_(vol_max) {}

uint16_t MuxChannel::read_midi_cc() const
{
    int raw = ctrl_.read(ch_);    
    int clamped = raw < vol_min_ ? vol_min_
                : raw > vol_max_ ? vol_max_ : raw;
    ESP_LOGI("muxCannel", "ch=%d, vol=%4d", ch_, raw);
    return static_cast<uint8_t>((clamped - vol_min_) * 127 / (vol_max_ - vol_min_));
}
