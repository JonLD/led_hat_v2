#ifndef BEAT_DETECTION_H
#define BEAT_DETECTION_H

#include <driver/i2s.h>

#define FFT_SQRT_APPROXIMATION
#define FFT_SPEED_OVER_PRECISION
#include <arduinoFFT.h>

// you shouldn't need to change these settings
#define I2S_MIC_CHANNEL I2S_CHANNEL_FMT_ONLY_RIGHT
#define I2S_MIC_SERIAL_CLOCK GPIO_NUM_32
#define I2S_MIC_LEFT_RIGHT_CLOCK GPIO_NUM_25
#define I2S_MIC_SERIAL_DATA GPIO_NUM_33

#define BEAT_DEBOUNCE_DURATION_MS 200 // Debounce the beat
#define MAX_BASS_FREQUENCY_HZ 140.0f

// Uncomment to enable print debugging (only enable one at a time)
// #define PRINT_PROFILING
// #define PRINT_BIN_MAGNITUDES
// #define PRINT_NOT_BEAT_DETECTED_REASON
// #define PRINT_CURRENT_BASS_MAG

const uint16_t numberOfSamples = 512; // This value MUST ALWAYS be a power of 2
const uint32_t samplingFrequency = 25000;

unsigned long lastBeatTime_ms = 0;
bool isBeatDetected = false;
unsigned long initialMicros;

typedef struct freqBandData_t
{
    double averageMagnitude;
    double currentMagnitude;
    uint32_t lowerBinIndex;
    uint32_t upperBinIndex;
    double beatDetectThresholdCoeff;
    double leakyAverageCoeff;
    double minMagnitude;
} freqBandData_s;

freqBandData_t bassFreqData{
    .averageMagnitude = 0,
    .currentMagnitude = 0,
    .lowerBinIndex = 0,
    .upperBinIndex = 1,
    .beatDetectThresholdCoeff = 1.2,
    .leakyAverageCoeff = 0.125,
    .minMagnitude = 200000000};

freqBandData_t midFreqData{
    .averageMagnitude = 0,
    .currentMagnitude = 0,
    .lowerBinIndex = 2,
    .upperBinIndex = 3,
    .beatDetectThresholdCoeff = 1.3,
    .leakyAverageCoeff = 0.125,
    .minMagnitude = 200000000};

float frequencyPeak_Hz;

float vImag[numberOfSamples] = {0};
float vReal[numberOfSamples] = {0};
ArduinoFFT<float> FFT = ArduinoFFT<float>(vReal, vImag, numberOfSamples, samplingFrequency, true);

#define SCL_INDEX 0x00
#define SCL_TIME 0x01
#define SCL_FREQUENCY 0x02
#define SCL_PLOT 0x03

void PrintVector(double *, uint16_t, uint8_t);
void computeFFT();
void detectBeat();
void controlLed(bool);
void analyzeFrequencyBand(freqBandData_t *);
void readMicData();
bool isMagAboveThreshold(freqBandData_t);
double proportionOfMagAboveAvg(freqBandData_t);

float weighingFactors[numberOfSamples];

// don't mess around with this
i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = samplingFrequency,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
    .channel_format = I2S_MIC_CHANNEL,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 4,
    .dma_buf_len = 1024,
    .use_apll = false,
    .tx_desc_auto_clear = false,
    .fixed_mclk = 0};

// and don't mess around with this
i2s_pin_config_t i2s_mic_pins = {
    .bck_io_num = I2S_MIC_SERIAL_CLOCK,
    .ws_io_num = I2S_MIC_LEFT_RIGHT_CLOCK,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = I2S_MIC_SERIAL_DATA};

int32_t rawMicSamples[numberOfSamples];

void readMicData()
{
    // read from the I2S device
    size_t bytes_read = 0;
    i2s_read(I2S_NUM_0, rawMicSamples, sizeof(int32_t) * numberOfSamples, &bytes_read, portMAX_DELAY);
    int samples_read = bytes_read / sizeof(int32_t);
    // Serial.println(samples_read);

#ifdef PRINT_PROFILING
    Serial.print(" Read: ");
    Serial.print(micros() - initialMicros);
#endif

    // dump the samples out to the serial channel.
    for (int i = 0; i < numberOfSamples; i++)
    {
        vReal[i] = (float)rawMicSamples[i];
    }
}

void computeFFT()
{
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
    const double proportionBassAboveAvg = proportionOfMagAboveAvg(bassFreqData);
    const double proportionMidAboveAvg = proportionOfMagAboveAvg(midFreqData);

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
    if (isBeatDetectedisBeatDetected)
    {
        Serial.print("\n");
        Serial.println("isBeatDetectedisBeatDetected");
        Serial.print("\n");
    }
#endif

    if (isBeatDetected)
    {
        lastBeatTime_ms = millis();
#ifdef PRINT_BIN_MAGNITUDES
        PrintVector(vReal, numberOfSamples, SCL_FREQUENCY);
        delay(20000);
#endif
    }
}

bool isMagAboveThreshold(freqBandData_t freqBandData)
{
    const bool magIsAboveThreshold = (bassFreqData.currentMagnitude > (bassFreqData.averageMagnitude * bassFreqData.beatDetectThresholdCoeff));
    return magIsAboveThreshold;
}

double proportionOfMagAboveAvg(freqBandData_t freqBandData)
{
    const double proportionAboveAvg = (bassFreqData.currentMagnitude / bassFreqData.averageMagnitude);
    return proportionAboveAvg;
}

// Print various data for debugging
void PrintVector(double *vData, uint16_t bufferSize, uint8_t scaleType)
{
    for (uint16_t i = 0; i < bufferSize; i++)
    {
        double abscissa;
        /* Print abscissa value */
        switch (scaleType)
        {
        case SCL_INDEX:
            abscissa = (i * 1.0);
            break;
        case SCL_TIME:
            abscissa = ((i * 1.0) / samplingFrequency);
            break;
        case SCL_FREQUENCY:
            abscissa = ((i * 1.0 * samplingFrequency) / numberOfSamples);
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

#endif
