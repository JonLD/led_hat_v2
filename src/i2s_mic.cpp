#include "i2s_mic.h"
#include "driver/i2s.h"
#include "profiling.h"

#define I2S_MIC_CHANNEL I2S_CHANNEL_FMT_ONLY_RIGHT
#define I2S_MIC_SERIAL_CLOCK GPIO_NUM_32
#define I2S_MIC_LEFT_RIGHT_CLOCK GPIO_NUM_25
#define I2S_MIC_SERIAL_DATA GPIO_NUM_33

static i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = SAMPLING_FREQUENCY_HZ,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_24BIT,
    .channel_format = I2S_MIC_CHANNEL,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 2,
    .dma_buf_len = FFT_BUFFER_LENGTH,
    .use_apll = false,
    .tx_desc_auto_clear = false,
    .fixed_mclk = 0};

static i2s_pin_config_t i2s_mic_pins = {
    .bck_io_num = I2S_MIC_SERIAL_CLOCK,
    .ws_io_num = I2S_MIC_LEFT_RIGHT_CLOCK,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = I2S_MIC_SERIAL_DATA
};

void i2sInit()
{
    i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
    i2s_set_pin(I2S_NUM_0, &i2s_mic_pins);
}

// Return true if read FFT_BUFFER_LENGTH samples
// @param rawMicSamples[out]    Output buffer to store samples from mic in
bool readMicData(int32_t rawMicSamples[FFT_BUFFER_LENGTH])
{
    size_t bytes_read = 0;
    i2s_read(I2S_NUM_0, rawMicSamples, sizeof(int32_t) * FFT_BUFFER_LENGTH, &bytes_read, portMAX_DELAY);
    const bool successfullyReadAllSamples = (bytes_read / sizeof(int32_t) == FFT_BUFFER_LENGTH);
    EMIT_PROFILING_EVENT;
    return successfullyReadAllSamples;
}