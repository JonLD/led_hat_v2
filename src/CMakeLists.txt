# Main component CMakeLists.txt

idf_component_register(
    SRCS
        "main.cpp"
        "interface.cpp"
        "wifi.cpp"
        "profiling.cpp"
        "i2s_mic.cpp"
        # Add your .cpp files here as you create them
        # "beat_detection.cpp"
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
        # FastLED-idf - will add after basic build works
        # Add more ESP-IDF components as needed
)

# C++ compile options
target_compile_options(${COMPONENT_LIB} PRIVATE
    -std=gnu++17
    -fno-exceptions
    -fno-rtti
)
