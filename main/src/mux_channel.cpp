#include "mux_channel.h"

MuxChannel::MuxChannel(MuxController& ctrl, MuxBus bus, uint8_t ch)
    : ctrl_(ctrl), bus_(bus), ch_(ch) {}

uint16_t MuxChannel::read()
{
    return ctrl_.read(bus_, ch_);
}
