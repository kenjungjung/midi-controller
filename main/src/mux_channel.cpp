#include "mux_channel.h"
#include <algorithm>

MuxChannel::MuxChannel(MuxController& ctrl, MuxBus bus, uint8_t ch, int cali_raw_min, int cali_raw_max)
    : ctrl_(ctrl), bus_(bus), ch_(ch), cali_raw_min_(cali_raw_min), cali_raw_max_(cali_raw_max) {}

uint16_t MuxChannel::read()
{
    return static_cast<int>(ctrl_.read(bus_, ch_, cali_raw_min_, cali_raw_max_));
}
