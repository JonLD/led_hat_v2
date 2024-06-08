#include <Arduino.h>

#define FFT_SQRT_APPROXIMATION
#define FFT_SPEED_OVER_PRECISION

#include "arduinoFFT.h"
#include "dsps_fft2r.h"

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

int16_t inputData[numberOfSamples * 2] = {0};

float vImag[numberOfSamples] = {0};
float vReal[numberOfSamples];
ArduinoFFT<float> FFT = ArduinoFFT<float>(vReal, vImag, numberOfSamples, samplingFrequency, true); /* Create FFT object */

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

const uint16_t minFreq = 1u;

// Some inspiration taken from
// https://stackoverflow.com/questions/78490683/esp32-fft-im-not-getting-the-correct-max-frequency-of-the-input-signal
void newFft(void)
{
    dsps_fft2r_sc16_ae32(inputData, numberOfSamples);
    dsps_bit_rev_sc16_ansi(inputData, numberOfSamples);
    dsps_cplx2reC_sc16(inputData, numberOfSamples);

    uint16_t maxIdx = 0u;
    int16_t maxAmp = INT16_MIN;

    for (uint32_t idx = 0u; idx < numberOfSamples; idx++)
    {
        if (inputData[idx] > minFreq && inputData[idx] > maxAmp)
        {
            maxIdx = idx;
            maxAmp = inputData[idx];
        }
    }
    frequencyPeak_Hz = (float)maxIdx * (float)samplingFrequency / (float)numberOfSamples;

    // Leave these in to make numbers comparable, even though they're operating on a different vector
    analyzeFrequencyBand(&bassFreqData);
    analyzeFrequencyBand(&midFreqData);
}

void setup()
{
    Serial.begin(115200);
    dsps_fft2r_init_sc16(NULL, 1024); // Allow a 1k buffer for sin/cos tables TODO decide how big this should actually be (maybe very small?)
}

// Make some "real" (i.e. nonzero) data for the FFT to operate on.
// Slow, messy, and hacky, but adequate
void generateData()
{
    const float first = (float)random(1, 100) / 100.0;
    const float second = (float)random(1, 100) / 100.0;
    const float third = (float)random(1, 100) / 100.0;
    const float fourth = (float)random(1, 100) / 100.0;

    // Serial.printf("Using %.3fsin(%.3fx) + %.3fsin(%.3fx)\n", first, second, third, fourth);

    for (uint32_t idx = 0u; idx < numberOfSamples; idx++)
    {
        const float val = first * sin(second * (float)idx) + third * sin(fourth * (float)idx);
        vReal[idx] = val;
        inputData[idx * 2] = (int16_t)(val * (float)INT16_MAX / (first + third));
    }
}

void loop()
{
    static uint32_t loopCnt = 0u;

    generateData();

    const int64_t startMicros = esp_timer_get_time();
    // computeFFT();
    newFft();
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
