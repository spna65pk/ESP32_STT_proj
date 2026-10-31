idf_component_register(
    SRCS
        "main.c"
        "../code_ICS43434/sources/ics43434.c"
        "../DSP_filter/sources/dsp_filter.c"
        "../Audio_stats/sources/audio_stats_serial.c"

    INCLUDE_DIRS
        "."
        "../code_ICS43434/headers"
        "../DSP_filter/headers"
        "../Audio_stats/headers/"

    REQUIRES
        esp_driver_i2s
        esp_driver_gpio
)