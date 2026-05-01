#include "mux_channel.h"
#include "esp_log.h"

MuxChannel::MuxChannel(MuxController& ctrl, uint8_t ch)
    : ctrl_(ctrl), ch_(ch) {}

uint16_t MuxChannel::read() const 
{
    return ctrl_.read(ch_);
}
