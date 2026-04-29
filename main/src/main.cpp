#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "tinyusb.h"
#include "usb_descriptors.h"
#include "config.h"
#include "analog_input.h"
#include "button.h"
#include "usb_midi.h"
#include "controller.h"
// #include "display.h"

static QueueHandle_t  midi_queue;
static Controller*    g_controller;

static void usb_event_cb(tinyusb_event_t* event, void* /*arg*/)
{
    switch (event->id) {
        case TINYUSB_EVENT_DETACHED:
            if (midi_queue) xQueueReset(midi_queue);
            break;
        case TINYUSB_EVENT_ATTACHED:
            if (g_controller) g_controller->reset_prev_cc();
            break;
        default:
            break;
    }
}

static void input_task(void* /*arg*/) {
    g_controller->input_loop();
}

static void midi_task(void* /*arg*/) {
    g_controller->midi_loop();
}


static void display_task(void* arg) {
    IDisplay* display = static_cast<IDisplay*>(arg);
    while (true) {
        display->render();
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}


extern "C" void app_main(void) {
    tinyusb_config_t tusb_cfg = {};
    tusb_cfg.task.size              = STACK_USB;
    tusb_cfg.task.priority          = PRIO_USB;
    tusb_cfg.task.xCoreID           = CORE_USB;
    tusb_cfg.descriptor.full_speed_config = desc_fs_cfg;
    tusb_cfg.event_cb               = usb_event_cb;
    tusb_cfg.event_arg              = nullptr;
    ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));

#ifndef USE_STUBS
    static AdcAnalogInput  fader(ADC_UNIT_FADER, ADC_CH_FADER_1, ADC_ATTEN);
    static GpioButton      button(PIN_BUTTON_1);
    static UsbMidiSender   sender;
    static Display  display;
#else
    static StubAnalogInput fader;
    static StubButton      button;
    static UsbMidiSender   sender;
    static StubDisplay     display;
#endif

    midi_queue   = xQueueCreate(MIDI_QUEUE_LEN, sizeof(MidiEvent));
    static Controller controller(fader, button, sender, display, midi_queue);
    g_controller = &controller;

    xTaskCreatePinnedToCore(input_task,   "input",   STACK_INPUT,   nullptr,  PRIO_INPUT,   nullptr, CORE_INPUT);
    xTaskCreatePinnedToCore(midi_task,    "midi",    STACK_MIDI,    nullptr,  PRIO_MIDI,    nullptr, CORE_USB);
    xTaskCreatePinnedToCore(display_task, "display", STACK_DISPLAY, &display, PRIO_DISPLAY, nullptr, CORE_INPUT);
}
