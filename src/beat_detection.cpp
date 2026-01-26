#include "beat_detection.h"

#include <math.h>
#include <string.h>

#include "esp_dsp.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "timing.h"
#include "profiling.h"

static const char *TAG = "beat_detection";

#define BEAT_DEBOUNCE_DURATION_MS 200
#define MAX_BASS_FREQUENCY_HZ 140.0f

uint32_t lastBeatTime_ms = 0;
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
    .beatDetectThresholdCoeff = 1.3,
    .leakyAverageCoeff = 0.125,
    .minMagnitude = 100000000
};

// FFT State Buffers
// =================
// Time-domain input buffer: audio samples (preprocessed in-place: DC removal, windowing)
static float samples[FFT_BUFFER_LENGTH] = {0};

// Frequency-domain buffer: interleaved [real0, imag0, real1, imag1, ...]
// LIFECYCLE:
//   Before FFT: Complex input (reals=samples, imags=0)
//   After FFT:  Complex spectrum
//   After cplx2reC: Magnitudes in even indices [mag0, _, mag1, _, mag2, ...]
// Access magnitudes as: fftBuffer[i*2] for bin i
static float fftBuffer[FFT_BUFFER_LENGTH * 2] = {0};

// Window coefficients (precomputed once, reused every FFT)
static float windowCoeffs[FFT_BUFFER_LENGTH] = {0};

// Initialize FFT and generate window coefficients
// MUST be called once at startup before calling ComputeFFT()
void InitBeatDetection()
{
    dsps_fft2r_init_fc32(NULL, FFT_BUFFER_LENGTH);
    // Generate Hann window coefficients (similar to Hamming)
    dsps_wind_hann_f32(windowCoeffs, FFT_BUFFER_LENGTH);
}

static void AnalyzeFrequencyBand(freqBandData_t *);
static inline bool IsMagAboveThreshold(freqBandData_t *);
static inline float ProportionOfMagAboveAvg(freqBandData_t *);
static void ConvertToInterleavedComplex(float *realData, float *complexData, int length);
static float FindMajorPeakFrequency();


void ComputeFFT(int32_t rawMicSamples[FFT_BUFFER_LENGTH])
{
    // Step 1: Convert int32 samples to float
    for (int i = 0; i < FFT_BUFFER_LENGTH; i++)
    {
        samples[i] = (float)rawMicSamples[i];
    }

    // Step 2: DC removal - calculate mean manually
    float mean = 0.0f;
    for (int i = 0; i < FFT_BUFFER_LENGTH; i++)
    {
        mean += samples[i];
    }
    mean /= FFT_BUFFER_LENGTH;
    // Subtract mean using ESP-DSP (in-place operation)
    // dsps_addc_f32_ae32(input, output, len, C, step_in, step_out)
    dsps_addc_f32_ae32(samples, samples, FFT_BUFFER_LENGTH, -mean, 1, 1);

    // Step 3: Apply Hann window (in-place)
    // dsps_mul_f32_ae32(input1, input2, output, len, step1, step2, step_out)
    dsps_mul_f32_ae32(samples, windowCoeffs, samples, FFT_BUFFER_LENGTH, 1, 1, 1);

    // Step 4: Convert to interleaved complex format [real0, imag0, real1, imag1, ...]
    ConvertToInterleavedComplex(samples, fftBuffer, FFT_BUFFER_LENGTH);

    // Step 5: Perform FFT
    dsps_fft2r_fc32(fftBuffer, FFT_BUFFER_LENGTH);

    // Step 6: Bit reverse order output
    dsps_bit_rev_fc32(fftBuffer, FFT_BUFFER_LENGTH);

    // Step 7: Convert complex spectrum to magnitudes (in-place)
    // After this, fftBuffer contains [mag0, _, mag1, _, mag2, ...] at even indices
    dsps_cplx2reC_fc32(fftBuffer, FFT_BUFFER_LENGTH);

    // Magnitudes are now in fftBuffer at stride-2 (every even index)
    // No copy needed - AnalyzeFrequencyBand will access them directly

    AnalyzeFrequencyBand(&bassFreqData);
    AnalyzeFrequencyBand(&midFreqData);
}

void AnalyzeFrequencyBand(freqBandData_t *freqBand)
{
    // Calculate current magnitude by averaging bins in frequency range
    // Magnitudes are stored at even indices in fftBuffer after cplx2reC
    freqBand->currentMagnitude = 0;
    for (int binIndex = freqBand->lowerBinIndex; binIndex <= freqBand->upperBinIndex; ++binIndex)
    {
        freqBand->currentMagnitude += fftBuffer[binIndex * 2]; // Stride-2: magnitudes at even indices
    }
    uint32_t numberOfBins = (1 + freqBand->upperBinIndex - freqBand->lowerBinIndex);
    freqBand->currentMagnitude /= numberOfBins;

    // Calculate leaky average
    freqBand->averageMagnitude += (freqBand->currentMagnitude - freqBand->averageMagnitude) * (freqBand->leakyAverageCoeff);
}

void DetectBeat()
{
    const bool isBassAboveAvg = IsMagAboveThreshold(&bassFreqData);
    const bool isMidAboveAvg = IsMagAboveThreshold(&midFreqData);
    const bool isNoRecentBeat = (GetMillis() - lastBeatTime_ms) > (BEAT_DEBOUNCE_DURATION_MS);
    const float majorPeakFreq = FindMajorPeakFrequency();
    const bool peakIsBass = (majorPeakFreq < MAX_BASS_FREQUENCY_HZ);
    const bool isAvgBassAboveMin = (bassFreqData.averageMagnitude > bassFreqData.minMagnitude);
    const float proportionBassAboveAvg = ProportionOfMagAboveAvg(&bassFreqData);
    const float proportionMidAboveAvg = ProportionOfMagAboveAvg(&midFreqData);

    isBeatDetected = (isNoRecentBeat && isBassAboveAvg && peakIsBass && isAvgBassAboveMin && isMidAboveAvg);

#ifdef PRINT_CURRENT_BASS_MAG
    ESP_LOGI(TAG, "%f", bassFreqData.currentMagnitude);
#endif
#ifdef PRINT_NOT_BEAT_DETECTED_REASON
    if (!isNoRecentBeat)
    {
        if (isBassAboveAvg && peakIsBass)
        {
            ESP_LOGI(TAG, "isAvgBassAboveMin");
        }
        else if (isBassAboveAvg && isAvgBassAboveMin)
        {
            ESP_LOGI(TAG, "peakIsBass");
        }
        else if (peakIsBass && isAvgBassAboveMin)
        {
            ESP_LOGI(TAG, "isBassAboveAvg");
        }
    }
#endif

    if (isBeatDetected)
    {
        lastBeatTime_ms = GetMillis();
#ifdef PRINT_BIN_MAGNITUDES
        // Print first 20 magnitude bins for debugging
        ESP_LOGI(TAG, "Magnitude spectrum:");
        for (int i = 0; i < 20; i++)
        {
            float freq = (float)i * SAMPLING_FREQUENCY_HZ / FFT_BUFFER_LENGTH;
            float mag = fftBuffer[i * 2];
            ESP_LOGI(TAG, "  Bin %d (%.1f Hz): %.2f", i, freq, mag);
        }
        vTaskDelay(pdMS_TO_TICKS(20000)); // 20 second delay for inspection
#endif
    }
}

// Convert real samples to interleaved complex format for ESP-DSP FFT
static void ConvertToInterleavedComplex(float *realData, float *complexData, int length)
{
    for (int i = 0; i < length; i++)
    {
        complexData[i * 2] = realData[i];      // Real part
        complexData[i * 2 + 1] = 0.0f;         // Imaginary part (zero for real audio signal)
    }
}

static inline bool IsMagAboveThreshold(freqBandData_t *freqBandData)
{
    return (freqBandData->currentMagnitude > (freqBandData->averageMagnitude * freqBandData->beatDetectThresholdCoeff));
}

static inline float ProportionOfMagAboveAvg(freqBandData_t *freqBandData)
{
    return (freqBandData->currentMagnitude / freqBandData->averageMagnitude);
}

// Find the frequency bin with the highest magnitude and convert to Hz
// Accesses fftBuffer directly at stride-2 (magnitudes at even indices)
static float FindMajorPeakFrequency()
{
    float peakMagnitude = 0.0f;
    int peakBinIndex = 0;
    // Search first N/2 bins (Nyquist limit), skip bin 0 (DC component)
    for (int i = 1; i < FFT_BUFFER_LENGTH / 2; i++)
    {
        float magnitude = fftBuffer[i * 2]; // Magnitudes at even indices
        if (magnitude > peakMagnitude)
        {
            peakMagnitude = magnitude;
            peakBinIndex = i;
        }
    }
    // Convert bin index to frequency: f = bin * (sampleRate / FFT_size)
    return (float)peakBinIndex * SAMPLING_FREQUENCY_HZ / FFT_BUFFER_LENGTH;
}
