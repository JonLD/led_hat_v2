#include "i2s_mic.h"
#include "driver/i2s_std.h"  // New ESP-IDF 5.x API
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "profiling.h"

static const char *TAG = "i2s_mic";

// I2S microphone GPIO pins
#define I2S_MIC_SERIAL_CLOCK GPIO_NUM_32       // BCK (bit clock)
#define I2S_MIC_LEFT_RIGHT_CLOCK GPIO_NUM_25   // WS (word select / LRCLK)
#define I2S_MIC_SERIAL_DATA GPIO_NUM_33        // DIN (data in)

// I2S channel handle (new API uses handles instead of port numbers)
static i2s_chan_handle_t rx_handle = NULL;

void I2sInit()
{
    ESP_LOGI(TAG, "Initializing I2S microphone...");

    // Step 1: Create I2S channel configuration
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    chan_cfg.dma_desc_num = 2;
    chan_cfg.dma_frame_num = FFT_BUFFER_LENGTH;
    chan_cfg.auto_clear = false;

    // Step 2: Create RX channel (no TX needed for microphone input)
    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, NULL, &rx_handle));

    // Step 3: Configure standard I2S mode
    // Clock configuration
    i2s_std_clk_config_t clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(SAMPLING_FREQUENCY_HZ);

    // Slot configuration (24-bit data in 32-bit slots, mono right channel)
    i2s_std_slot_config_t slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_24BIT, I2S_SLOT_MODE_MONO);
    slot_cfg.slot_bit_width = I2S_SLOT_BIT_WIDTH_32BIT;
    slot_cfg.slot_mask = I2S_STD_SLOT_RIGHT;

    // GPIO configuration
    i2s_std_gpio_config_t gpio_cfg = {
        .mclk = I2S_GPIO_UNUSED,
        .bclk = I2S_MIC_SERIAL_CLOCK,
        .ws = I2S_MIC_LEFT_RIGHT_CLOCK,
        .dout = I2S_GPIO_UNUSED,
        .din = I2S_MIC_SERIAL_DATA,
        .invert_flags = {
            .mclk_inv = false,
            .bclk_inv = false,
            .ws_inv = false,
        },
    };

    // Combine into standard I2S config
    i2s_std_config_t std_cfg = {
        .clk_cfg = clk_cfg,
        .slot_cfg = slot_cfg,
        .gpio_cfg = gpio_cfg,
    };

    // Step 4: Initialize the channel with standard mode configuration
    ESP_ERROR_CHECK(i2s_channel_init_std_mode(rx_handle, &std_cfg));

    // Step 5: Enable the channel to start receiving audio data
    ESP_ERROR_CHECK(i2s_channel_enable(rx_handle));

    ESP_LOGI(TAG, "I2S microphone initialized");
}

// Return true if read FFT_BUFFER_LENGTH samples
// @param rawMicSamples[out]    Output buffer to store samples from mic in
bool ReadMicData(int32_t rawMicSamples[FFT_BUFFER_LENGTH])
{
    size_t bytes_read = 0;

    // Read audio samples from I2S microphone
    ESP_ERROR_CHECK(i2s_channel_read(rx_handle, rawMicSamples, sizeof(int32_t) * FFT_BUFFER_LENGTH, &bytes_read, portMAX_DELAY));

    const bool successfullyReadAllSamples = (bytes_read / sizeof(int32_t) == FFT_BUFFER_LENGTH);
    EMIT_PROFILING_EVENT;
    return successfullyReadAllSamples;
}
