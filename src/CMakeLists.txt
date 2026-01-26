# Main component CMakeLists.txt

idf_component_register(
    SRCS
        "main.cpp"
        "interface.cpp"
        "wifi.cpp"
        "profiling.cpp"
        "i2s_mic.cpp"
        "beat_detection.cpp"
        # Add your .cpp files here as you create them
        # "effects.cpp"
    INCLUDE_DIRS
        "."
    REQUIRES
        nvs_flash
        esp_wifi
        esp_event
        esp_netif
        driver
        esp_timer
        esp-dsp
        # FastLED-idf - will add after basic build works
        # Add more ESP-IDF components as needed
)

# C++ compile options
target_compile_options(${COMPONENT_LIB} PRIVATE
    -std=gnu++17
    -fno-exceptions
    -fno-rtti
)
