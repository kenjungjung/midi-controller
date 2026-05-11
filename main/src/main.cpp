#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "tinyusb.h"
#include "usb_descriptors.h"
#include "config.h"
#include "adc1_unit.h"
#include "adc2_unit.h"
#include "analog_input.h"
#include "mux_controller.h"
#include "mux_channel.h"
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
    
    static Adc1Unit         adc1;
    static Adc2Unit         adc2;
    // U2: ポテンショメータ RV5-RV8（X-COM=GPIO5/CH4, Y-COM=GPIO6/CH5）
    static MuxController    muxU2(adc1, ADC_CH_U2_X, ADC_CH_U2_Y, ADC_ATTEN, PIN_MUX_A, PIN_MUX_B);
    // U5: ポテンショメータ RV9-RV12（Y-COM=GPIO14/ADC2_CH3、X側は未接続）
    static MuxController    muxU5(adc2, ADC_CH_U5_X, ADC_CH_U5_Y, ADC_ATTEN, PIN_U5_A, PIN_U5_B);
    // U2 Xセクション: X0=RV7, X1=RV6, X2=RV5, X3=RV8
    static MuxChannel       knob_RV5(muxU2, MuxBus::X, 2, 0, 3972);
    static MuxChannel       knob_RV6(muxU2, MuxBus::X, 1, 0, 3972);
    static MuxChannel       knob_RV7(muxU2, MuxBus::X, 0, 0, 3972);
    static MuxChannel       knob_RV8(muxU2, MuxBus::X, 3, 0, 3972);
    // U5 Yセクション: Y0=RV9, Y1=RV11, Y2=RV10, Y3=RV12
    static MuxChannel       knob_RV9 (muxU5, MuxBus::Y, 0, 0, 3872);
    static MuxChannel       knob_RV10(muxU5, MuxBus::Y, 2, 0, 3872);
    static MuxChannel       knob_RV11(muxU5, MuxBus::Y, 1, 0, 3872);
    static MuxChannel       knob_RV12(muxU5, MuxBus::Y, 3, 0, 3872);

    static LedManager       led;
    static UsbMidiSender    sender;
    static Display          display;

    midi_queue = xQueueCreate(MIDI_QUEUE_LEN, sizeof(MidiEvent));

    static ControllerConfig cfg = {
        .faders   = {},
        .knobs    = { &knob_RV5, &knob_RV6, &knob_RV7, &knob_RV8,
                      &knob_RV9, &knob_RV10, &knob_RV11, &knob_RV12 },
        .buttons  = {},
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
