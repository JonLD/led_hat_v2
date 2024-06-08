#include <Arduino.h>
#include "arduinoFFT.h"

#define NUM_LOOPS_TO_MEASURE (5000u)

const uint16_t numberOfSamples = 512; // This value MUST ALWAYS be a power of 2
const uint32_t samplingFrequency = 25000;

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
float vReal[numberOfSamples];
ArduinoFFT<float> FFT = ArduinoFFT<float>(vReal, vImag, numberOfSamples, samplingFrequency); /* Create FFT object */

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

void setup()
{
    Serial.begin(115200);
}

// Make some "real" (i.e. nonzero) data for the FFT to operate on.
// Slow, messy, and hacky, but adequate
void generateData()
{
    const float first = (float)random();
    const float second = (float)random();
    const float third = (float)random();
    const float fourth = (float)random();

    for (uint32_t idx = 0u; idx < numberOfSamples; idx++)
    {
        vReal[idx] = first * sin(second * (float)idx) + third * sin(fourth * (float)idx);
    }
}

void loop()
{
    static uint32_t loopCnt = 0u;

    generateData();

    const int64_t startMicros = esp_timer_get_time();
    computeFFT();
    const int64_t endMicros = esp_timer_get_time();
    const int64_t elapsed = endMicros - startMicros;

    if (loopCnt < NUM_LOOPS_TO_MEASURE)
    {
        Serial.printf("%u\n", elapsed);
    }
    else if (loopCnt == NUM_LOOPS_TO_MEASURE)
    {
        Serial.printf("Completed %u loops\n", loopCnt);
    }
    else
    {
        // Loop indefinitely
    }

    loopCnt++;
}
