set(SOURCES
    src/main.cpp
    src/adc_unit.cpp
    src/analog_input.cpp
    src/mux_controller.cpp
    src/mux_channel.cpp
    src/button.cpp
    src/usb_midi.cpp
    src/controller.cpp
    src/display.cpp
    src/led.cpp
)

idf_component_register(
    SRCS ${SOURCES}
    INCLUDE_DIRS "include"
    REQUIRES driver esp_adc esp_driver_i2c
)
