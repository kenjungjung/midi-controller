#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "tinyusb.h"
#include "usb_descriptors.h"
#include "config.h"
#include "analog_input.h"
#include "mux_controller.h"
#include "mux_channel.h"
#include "button.h"
#include "led.h"
#include "usb_midi.h"
#include "display.h"
#include "controller.h"
#include "esp_log.h"

static QueueHandle_t  midi_queue;
static Controller*    g_controller;

static void usb_event_cb(tinyusb_event_t* event, void* /*arg*/)
{
    ESP_LOGI("usb_event_cb", "%d", event->id);
    switch (event->id) {
        case TINYUSB_EVENT_DETACHED:
            if (midi_queue) xQueueReset(midi_queue);
            if (g_controller) g_controller->notify_connected(false);
            break;
        case TINYUSB_EVENT_ATTACHED:
            if (g_controller) g_controller->reset_prev_cc();
            if (g_controller) g_controller->notify_connected(true);
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
    tusb_cfg.event_cb               = usb_event_cb; // これ反応しない。。後でやる
    tusb_cfg.event_arg              = nullptr;
    ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));

#ifndef USE_STUBS
    static MuxController    mux(ADC_UNIT_MUX, ADC_CH_MUX_X, ADC_ATTEN,
                                PIN_MUX_A, PIN_MUX_B);
    static MuxChannel       fader(mux, 0, FADER_RAW_MIN, FADER_RAW_MAX);
    static MuxChannel       knob1(mux, 1, KNOB_RAW_MIN, KNOB_RAW_MAX);
    static LedGpioButton    btn(PIN_LED_BTN_SW, PIN_LED_BTN_LED);
    static LedManager       led;
    static UsbMidiSender    sender;
    static Display          display;
#else
    static StubAnalogInput  fader;
    static StubAnalogInput  knob1, knob2, knob3;
    static StubButtonLed    btn;
    static StubLed          led;
    static UsbMidiSender    sender;
    static StubDisplay      display;
#endif

    midi_queue = xQueueCreate(MIDI_QUEUE_LEN, sizeof(MidiEvent));

    static ControllerConfig cfg = {
        .faders   = { &fader },
        .knobs    = {}, //{ &knob1 },// { &knob1, &knob2, &knob3 },
        .buttons  = { &btn },
        .led      = &led,
        .display  = &display,
        .sender   = &sender,
        .midi_queue = midi_queue,
    };
    static Controller controller(cfg);
    g_controller = &controller;

    xTaskCreatePinnedToCore(input_task,   "input",   STACK_INPUT,   nullptr,  PRIO_INPUT,   nullptr, CORE_INPUT);
    xTaskCreatePinnedToCore(midi_task,    "midi",    STACK_MIDI,    nullptr,  PRIO_MIDI,    nullptr, CORE_USB);
    xTaskCreatePinnedToCore(display_task, "display", STACK_DISPLAY, &display, PRIO_DISPLAY, nullptr, CORE_INPUT);
}
