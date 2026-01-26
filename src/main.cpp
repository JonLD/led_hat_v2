#include <stdio.h>
#include "beat_detection.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "i2s_mic.h"
#include "nvs_flash.h"
#include "wifi.h"

static const char *TAG = "synesthetix";

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "Synesthetix starting up...");

    // Initialize NVS (Non-Volatile Storage) - required for WiFi/ESP-NOW
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "NVS initialized");

    // TODO: Initialize subsystems
    // - WiFi/ESP-NOW
    InitWifi();
    InitEspNow();
    // - I2S microphone
    I2sInit();
    InitBeatDetection();
    // - FastLED
    // - Create FreeRTOS tasks
    //

    gpio_config_t io_conf = {};
    io_conf.pin_bit_mask = (1ULL << GPIO_NUM_2);  // Which pin(s)
    io_conf.mode = GPIO_MODE_OUTPUT;               // Input or output
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    io_conf.intr_type = GPIO_INTR_DISABLE;         // No interrupts
    gpio_config(&io_conf);


    ESP_LOGI(TAG, "Setup complete - entering main loop");

    // Main loop (for now, just blink to show we're alive)
    while (1) {
        ESP_LOGI(TAG, "Still alive!");

        vTaskDelay(pdMS_TO_TICKS(1000));
        gpio_set_level(GPIO_NUM_2, 1);

        vTaskDelay(pdMS_TO_TICKS(1000));
        gpio_set_level(GPIO_NUM_2, 0);
    }
}
