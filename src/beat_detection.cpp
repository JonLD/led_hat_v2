#include "beat_detection.h"

#include "driver/i2s.h"

#define FFT_SQRT_APPROXIMATION
#define FFT_SPEED_OVER_PRECISION
#include <arduinoFFT.h>

#include "profiling.h"

#define BEAT_DEBOUNCE_DURATION_MS 200
#define MAX_BASS_FREQUENCY_HZ 140.0f

#define SCL_INDEX 0x00
#define SCL_TIME 0x01
#define SCL_FREQUENCY 0x02
#define SCL_PLOT 0x03

// you shouldn't need to change these settings
#define I2S_MIC_CHANNEL I2S_CHANNEL_FMT_ONLY_RIGHT
#define I2S_MIC_SERIAL_CLOCK GPIO_NUM_32
#define I2S_MIC_LEFT_RIGHT_CLOCK GPIO_NUM_25
#define I2S_MIC_SERIAL_DATA GPIO_NUM_33

const uint16_t numberOfSamples = 1024; // This value MUST ALWAYS be a power of 2
const uint32_t samplingFrequency = 96000;
unsigned long lastBeatTime_ms = 0;
bool isBeatDetected = false;

// don't mess around with this
i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = samplingFrequency,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
    .channel_format = I2S_MIC_CHANNEL,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 2,
    .dma_buf_len = numberOfSamples,
    .use_apll = false,
    .tx_desc_auto_clear = false,
    .fixed_mclk = 0};

// and don't mess around with this
i2s_pin_config_t i2s_mic_pins = {
    .bck_io_num = I2S_MIC_SERIAL_CLOCK,
    .ws_io_num = I2S_MIC_LEFT_RIGHT_CLOCK,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = I2S_MIC_SERIAL_DATA
};

typedef struct freqBandData_t
{
    float averageMagnitude;
    float currentMagnitude;
    uint32_t lowerBinIndex;
    uint32_t upperBinIndex;
    float beatDetectThresholdCoeff;
    float leakyAverageCoeff;
    float minMagnitude;
} freqBandData_s;

static freqBandData_t bassFreqData{
    .averageMagnitude = 0,
    .currentMagnitude = 0,
    .lowerBinIndex = 0,
    .upperBinIndex = 2,
    .beatDetectThresholdCoeff = 1.3,
    .leakyAverageCoeff = 0.125,
    .minMagnitude = 40000000
};

static freqBandData_t midFreqData{
    .averageMagnitude = 0,
    .currentMagnitude = 0,
    .lowerBinIndex = 3,
    .upperBinIndex = 5,
    .beatDetectThresholdCoeff = 1.4,
    .leakyAverageCoeff = 0.125,
    .minMagnitude = 40000000
};

static float frequencyPeak_Hz;
float vImag[numberOfSamples] = {0};
float vReal[numberOfSamples] = {0};
ArduinoFFT<float> FFT = ArduinoFFT<float>(vReal, vImag, numberOfSamples, samplingFrequency, true);

static void PrintVector(float *, uint16_t, uint8_t);
static void analyzeFrequencyBand(freqBandData_t *);
static bool isMagAboveThreshold(freqBandData_t);
static float proportionOfMagAboveAvg(freqBandData_t);

float weighingFactors[numberOfSamples];

int32_t rawMicSamples[numberOfSamples];

bool readMicData()
{
    // read from the I2S device
    size_t bytes_read = 0;
    i2s_read(I2S_NUM_0, rawMicSamples, sizeof(int32_t) * numberOfSamples, &bytes_read, portMAX_DELAY);
    EMIT_PROFILING_EVENT;
    const bool readAllSamples =  bytes_read == (sizeof(int32_t) * numberOfSamples);
    return readAllSamples;
}

static void populateRealAndImag()
{
    for (int i = 0; i < FFT_BUFFER_LENGTH; i++)
    {
        vReal[i] = (float)rawMicSamples[i];
#ifdef OUTPUT_AUDIO
        Serial.print(rawMicSamples[i]);
#endif
    }
    // The audio is only real data but the FFT outputs to vImag so it needs to be zeroed each time
    memset(vImag, 0, sizeof(vImag));
    EMIT_PROFILING_EVENT;
}

void computeFFT()
{
    populateRealAndImag();
    FFT.dcRemoval();
    FFT.windowing(FFT_WIN_TYP_HAMMING, FFT_FORWARD);
    FFT.compute(FFTDirection::Forward);
    FFT.complexToMagnitude();
    frequencyPeak_Hz = FFT.majorPeak();
    analyzeFrequencyBand(&bassFreqData);
    analyzeFrequencyBand(&midFreqData);
}

void analyzeFrequencyBand(freqBandData_t *freqBand)
{
    // Calculate current magnitude by averaging bins in frequency range
    freqBand->currentMagnitude = 0;
    for (int binIndex = freqBand->lowerBinIndex; binIndex <= freqBand->upperBinIndex; ++binIndex)
    {
        freqBand->currentMagnitude += vReal[binIndex];
    }
    uint32_t numberOfBins = (1 + freqBand->upperBinIndex - freqBand->lowerBinIndex);
    freqBand->currentMagnitude /= numberOfBins;

    // Calulate leaky average
    freqBand->averageMagnitude += (freqBand->currentMagnitude - freqBand->averageMagnitude) * (freqBand->leakyAverageCoeff);
}

void detectBeat()
{
    const bool isBassAboveAvg = isMagAboveThreshold(bassFreqData);
    const bool isMidAboveAvg = isMagAboveThreshold(midFreqData);
    const bool isRecentBeat = ((millis() - lastBeatTime_ms) < (BEAT_DEBOUNCE_DURATION_MS));
    const bool peakIsBass = (frequencyPeak_Hz < MAX_BASS_FREQUENCY_HZ);
    const bool isAvgBassAboveMin = (bassFreqData.averageMagnitude > bassFreqData.minMagnitude);
    const float proportionBassAboveAvg = proportionOfMagAboveAvg(bassFreqData);
    const float proportionMidAboveAvg = proportionOfMagAboveAvg(midFreqData);

    isBeatDetected = (!isRecentBeat && isBassAboveAvg && peakIsBass && isAvgBassAboveMin && isMidAboveAvg);

#ifdef PRINT_CURRENT_BASS_MAG
    Serial.println(bassFreqData.currentMagnitude);
#endif
#ifdef PRINT_NOT_BEAT_DETECTED_REASON
    if (!isRecentBeat)
    {
        if (isBassAboveAvg && peakIsBass)
        {
            Serial.println("isAvgBassAboveMin");
        }
        else if (isBassAboveAvg && isAvgBassAboveMin)
        {
            Serial.println("peakIsBass");
        }
        else if (peakIsBass && isAvgBassAboveMin)
        {
            Serial.println("isBassAboveAvg");
        }
    }
#endif

    if (isBeatDetected)
    {
        lastBeatTime_ms = millis();
#ifdef PRINT_BIN_MAGNITUDES
        PrintVector(vReal, NUMBER_OF_SAMPLES, SCL_FREQUENCY);
        delay(20000);
#endif
    }
}

static bool isMagAboveThreshold(freqBandData_t freqBandData)
{
    const bool magIsAboveThreshold = (bassFreqData.currentMagnitude > (bassFreqData.averageMagnitude * bassFreqData.beatDetectThresholdCoeff));
    return magIsAboveThreshold;
}

static float proportionOfMagAboveAvg(freqBandData_t freqBandData)
{
    const float proportionAboveAvg = (bassFreqData.currentMagnitude / bassFreqData.averageMagnitude);
    return proportionAboveAvg;
}

// Print various data for debugging
static void PrintVector(float *vData, uint16_t bufferSize, uint8_t scaleType)
{
    for (uint16_t i = 0; i < bufferSize; i++)
    {
        float abscissa;
        /* Print abscissa value */
        switch (scaleType)
        {
        case SCL_INDEX:
            abscissa = (i * 1.0);
            break;
        case SCL_TIME:
            abscissa = ((i * 1.0) / SAMPLING_FREQUENCY_HZ);
            break;
        case SCL_FREQUENCY:
            abscissa = ((i * 1.0 * SAMPLING_FREQUENCY_HZ) / FFT_BUFFER_LENGTH);
            break;
        }
        Serial.print(abscissa, 6);
        if (scaleType == SCL_FREQUENCY)
            Serial.print("Hz");
        Serial.print(" ");
        Serial.println(vData[i] / 1000, 4);
    }
    Serial.println();
}