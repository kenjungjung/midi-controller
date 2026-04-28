#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "config.h"
#include "ssd1306.h"
#include "potentiometer.h"

extern "C" void app_main(void)
{
    SSD1306 display(hw::display);
    Potentiometer pot({
        .unit    = hw::pot_unit,
        .channel = hw::pot_channel,
    });

    display.draw_text(0, 0, "MIDI Controller");

    int prev_midi = -1;
    while (true) {
        auto [raw, midi] = pot.read();

        if (midi != prev_midi) {
            char buf[32];
            snprintf(buf, sizeof(buf), "CC1: %3d / 127", midi);
            display.draw_text(0, 2, buf);
            prev_midi = midi;
        }
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}
