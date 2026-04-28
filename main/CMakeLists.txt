set(SOURCES
    src/main.cpp
    src/analog_input.cpp
    src/button.cpp
    src/usb_midi.cpp
    src/controller.cpp
)

idf_component_register(
    SRCS ${SOURCES}
    INCLUDE_DIRS "include"
    REQUIRES driver esp_adc
)
