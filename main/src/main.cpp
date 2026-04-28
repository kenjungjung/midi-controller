#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "tinyusb.h"
#include "config.h"
#include "analog_input.h"
#include "button.h"
#include "usb_midi.h"
#include "controller.h"

// ── グローバルオブジェクト（タスク間で共有） ─────────────────
static QueueHandle_t  midi_queue;
static Controller*    g_controller;

// ── USB ホットプラグコールバック ──────────────────────────────

/** @brief USB 接続/切断時に esp_tinyusb から呼ばれるコールバック */
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

// ── FreeRTOS タスク ───────────────────────────────────────────

static void input_task(void* /*arg*/) {
    g_controller->input_loop();
}

static void midi_task(void* /*arg*/) {
    g_controller->midi_loop();
}

// ── エントリポイント ──────────────────────────────────────────

extern "C" void app_main(void) {
    // TinyUSB 初期化（内部でUSBタスクを起動する）
    const tinyusb_config_t tusb_cfg = {
        .event_cb  = usb_event_cb,
        .event_arg = nullptr,
    };
    ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));

    // ハードウェア初期化
#ifndef USE_STUBS
    static AdcAnalogInput fader(ADC_UNIT_FADER, ADC_CH_FADER_1, ADC_ATTEN);
    static GpioButton     button(PIN_BUTTON_1);
    static UsbMidiSender  sender;
#else
    static StubAnalogInput fader;
    static StubButton      button;
    static UsbMidiSender   sender;
#endif

    // queue と controller を生成
    midi_queue   = xQueueCreate(MIDI_QUEUE_LEN, sizeof(MidiEvent));
    static Controller controller(fader, button, sender, midi_queue);
    g_controller = &controller;

    // タスク起動（UsbTask は tinyusb_driver_install が内部で起動する）
    xTaskCreatePinnedToCore(input_task, "input", STACK_INPUT, nullptr, PRIO_INPUT, nullptr, CORE_INPUT);
    xTaskCreatePinnedToCore(midi_task,  "midi",  STACK_MIDI,  nullptr, PRIO_MIDI,  nullptr, CORE_USB);
}
