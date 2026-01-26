#include "wifi.h"
#include "esp_now.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_log.h"
#include "interface.h"
#include <string.h>

static const char *TAG = "wifi";

void OnDataRecv(const esp_now_recv_info_t *esp_now_info, const uint8_t *inputData, int data_len);

void InitWifi()
{
    ESP_LOGI(TAG, "Initializing WESP_LOGI(TAG, WiFi subsystem...");

    // Initialize TCP/IP network interface (required for WiFi)
    ESP_ERROR_CHECK(esp_netif_init());

    // Create default event loop (handles WiFi/network events)
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Initialize WiFi with default config
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // Set to station mode (client, not access point)
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

    // Start WiFi driver
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "WiFi initialized in STA mode");
}

void InitEspNow()
{
    ESP_LOGI(TAG, "Initializing ESP-NOW...");

    // Initialize ESP-NOW protocol
    ESP_ERROR_CHECK(esp_now_init());

    // Register callback for incoming data
    ESP_ERROR_CHECK(esp_now_register_recv_cb(OnDataRecv));

    ESP_LOGI(TAG, "ESP-NOW initialized, ready to receive");
}

void OnDataRecv(const esp_now_recv_info_t *esp_now_info,
                const uint8_t *incomingData,
                int data_len)
{
    memcpy(&radioData, incomingData, sizeof(radioData_t));
    ESP_LOGI(TAG, "Bytes received: %u", data_len);
    ESP_LOGI(TAG, "Effect enum: %d", radioData.effect);
    ESP_LOGI(TAG, "Colour enum: %d", radioData.colour);
    ESP_LOGI(TAG, "Ambient override: %d", radioData.ambientOverride);
}
