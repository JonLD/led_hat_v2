#include "beat_detection.h"

#include "driver/i2s.h"

#define FFT_SQRT_APPROXIMATION
#define FFT_SPEED_OVER_PRECISION
#include <arduinoFFT.h>

#include "profiling.h"

#define BEAT_DEBOUNCE_DURATION_MS 200
#define MAX_BASS_FREQUENCY_HZ 140.0f


// you shouldn't need to change these settings
#define I2S_MIC_CHANNEL I2S_CHANNEL_FMT_ONLY_RIGHT
#define I2S_MIC_SERIAL_CLOCK GPIO_NUM_32
#define I2S_MIC_LEFT_RIGHT_CLOCK GPIO_NUM_25
#define I2S_MIC_SERIAL_DATA GPIO_NUM_33

unsigned long lastBeatTime_ms = 0;
bool isBeatDetected = false;

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
    .lowerBinIndex = 1,
    .upperBinIndex = 2,
    .beatDetectThresholdCoeff = 1.4,
    .leakyAverageCoeff = 0.125,
    .minMagnitude = 100000000
};

static freqBandData_t midFreqData{
    .averageMagnitude = 0,
    .currentMagnitude = 0,
    .lowerBinIndex = 3,
    .upperBinIndex = 3,
    .beatDetectThresholdCoeff = 1.5,
    .leakyAverageCoeff = 0.125,
    .minMagnitude = 100000000
};

float vImag[FFT_BUFFER_LENGTH] = {0};
float vReal[FFT_BUFFER_LENGTH] = {0};
ArduinoFFT<float> FFT = ArduinoFFT<float>(vReal, vImag, FFT_BUFFER_LENGTH, SAMPLING_FREQUENCY_HZ, true);

static void analyzeFrequencyBand(freqBandData_t *);
static bool isMagAboveThreshold(freqBandData_t);
static float proportionOfMagAboveAvg(freqBandData_t);


static void populateRealAndImag(int32_t rawMicSamples[FFT_BUFFER_LENGTH])
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

void computeFFT(int32_t rawMicSamples[FFT_BUFFER_LENGTH])
{
    populateRealAndImag(rawMicSamples);
    FFT.dcRemoval();
    FFT.windowing(FFT_WIN_TYP_HAMMING, FFT_FORWARD);
    FFT.compute(FFTDirection::Forward);
    FFT.complexToMagnitude();
    
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
    const bool isNoRecentBeat = ((millis() - lastBeatTime_ms) > (BEAT_DEBOUNCE_DURATION_MS));
    const bool peakIsBass = (FFT.majorPeak() < MAX_BASS_FREQUENCY_HZ);
    const bool isAvgBassAboveMin = (bassFreqData.averageMagnitude > bassFreqData.minMagnitude);
    const float proportionBassAboveAvg = proportionOfMagAboveAvg(bassFreqData);
    const float proportionMidAboveAvg = proportionOfMagAboveAvg(midFreqData);

    isBeatDetected = (isNoRecentBeat && isBassAboveAvg && peakIsBass && isAvgBassAboveMin && isMidAboveAvg);

#ifdef PRINT_CURRENT_BASS_MAG
    Serial.println(bassFreqData.currentMagnitude);
#endif
#ifdef PRINT_NOT_BEAT_DETECTED_REASON
    if (!isNoRecentBeat)
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

