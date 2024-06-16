#include "beat_detection.h"

#include "driver/i2s.h"

#define FFT_SQRT_APPROXIMATION
#define FFT_SPEED_OVER_PRECISION
#include <arduinoFFT.h>

#include "timing.h"
#include "profiling.h"

#define BEAT_DEBOUNCE_DURATION_MS 200
#define MAX_BASS_FREQUENCY_HZ 140.0f

#define HISTORICAL_DATA_LENGTH 3
#define HISTORICAL_VAR_LENGTH 3

template <typename T, size_t N>
class HistoricData {
    public:
        T data[N] = { 0 };
        int frontIdx = 0;
        int rearIdx = N-1;
        T mean = 0;
        float variance = 0.0;

        void update(T sample)
        {
            insert(sample);
            mean = Mean();
            variance = Variance();
        }

    private:
        void insert(T sample) 
        {
            if(++rearIdx == N) {
                rearIdx = 0;
            }
            if(++frontIdx == N) {
                frontIdx = 0;
            }
            data[rearIdx] = sample;
        }

        uint64_t TotalMagnitute(void)
        {
            uint64_t sum = 0;
            for(int i = 0; i<N ; i++){
                sum+=(uint64_t)data[i];
            }
            return sum;
        }

        T Mean(void)
        {
            return TotalMagnitute() / N;
        }

        float Variance(void)
        {
            mean = Mean();
            uint64_t squaredDifferenceSum = 0;
            for(int i = 0; i < N ; i++)
            {
                squaredDifferenceSum+= data[i]*data[i];
            }
            return sqrt((double)(squaredDifferenceSum / N));
        }
};


unsigned long lastBeatTime_ms = 0;
bool isBeatDetected = false;

typedef struct freqBandData_t
{
    HistoricData<uint64_t, HISTORICAL_DATA_LENGTH> historicData;
    float averageMagnitude;
    float currentMagnitude;
    uint32_t lowerBinIndex;
    uint32_t upperBinIndex;
    float beatDetectThresholdCoeff;
    float leakyAverageCoeff;
    float minMagnitude;
    uint32_t varianceThreshold;
} freqBandData_s;

static freqBandData_t bassFreqData{
    .averageMagnitude = 0,
    .currentMagnitude = 0,
    .lowerBinIndex = 1,
    .upperBinIndex = 2,
    .beatDetectThresholdCoeff = 1.4,
    .leakyAverageCoeff = 0.125,
    .minMagnitude = 40000000,
    .varianceThreshold = 400000000
};

static freqBandData_t midFreqData{
    .averageMagnitude = 0,
    .currentMagnitude = 0,
    .lowerBinIndex = 3,
    .upperBinIndex = 3,
    .beatDetectThresholdCoeff = 1.3,
    .leakyAverageCoeff = 0.125,
    .minMagnitude = 40000000,
    .varianceThreshold = 400000000

};

float vImag[FFT_BUFFER_LENGTH] = {0};
float vReal[FFT_BUFFER_LENGTH] = {0};
ArduinoFFT<float> FFT = ArduinoFFT<float>(vReal, vImag, FFT_BUFFER_LENGTH, SAMPLING_FREQUENCY_HZ, true);

static void AnalyzeFrequencyBand(freqBandData_t *);
static inline bool IsMagAboveThreshold(freqBandData_t *);
static inline float ProportionOfMagAboveAvg(freqBandData_t *);
static void PopulateRealAndImag(int32_t rawMicSamples[FFT_BUFFER_LENGTH]);


void ComputeFFT(int32_t rawMicSamples[FFT_BUFFER_LENGTH])
{
    PopulateRealAndImag(rawMicSamples);
    FFT.dcRemoval();
    FFT.windowing(FFT_WIN_TYP_HAMMING, FFT_FORWARD);
    FFT.compute(FFTDirection::Forward);
    FFT.complexToMagnitude();

    AnalyzeFrequencyBand(&bassFreqData);
    AnalyzeFrequencyBand(&midFreqData);
}

static void AnalyzeFrequencyBand(freqBandData_t *freqBand)
{
    // Calculate current magnitude by averaging bins in frequency range
    freqBand->currentMagnitude = 0;
    for (int binIndex = freqBand->lowerBinIndex; binIndex <= freqBand->upperBinIndex; ++binIndex)
    {
        freqBand->currentMagnitude += vReal[binIndex];
    }
    uint32_t numberOfBins = (1 + freqBand->upperBinIndex - freqBand->lowerBinIndex);
    freqBand->currentMagnitude /= numberOfBins;
    freqBand->historicData.update(freqBand->currentMagnitude);


    // Calulate leaky average
    freqBand->averageMagnitude += (freqBand->currentMagnitude - freqBand->averageMagnitude) * (freqBand->leakyAverageCoeff);
}

void DetectBeat()
{
    const bool isBassAboveAvg = IsMagAboveThreshold(&bassFreqData);
    const bool isMidAboveAvg = IsMagAboveThreshold(&midFreqData);
    const bool isNoRecentBeat = (GetMillis() - lastBeatTime_ms) > (BEAT_DEBOUNCE_DURATION_MS);
    const bool peakIsBass = (FFT.majorPeak() < MAX_BASS_FREQUENCY_HZ);
    const bool isAvgBassAboveMin = (bassFreqData.averageMagnitude > bassFreqData.minMagnitude);
    const float proportionBassAboveAvg = ProportionOfMagAboveAvg(&bassFreqData);
    const float proportionMidAboveAvg = ProportionOfMagAboveAvg(&midFreqData);
  
    Serial.printf("%f\t", bassFreqData.historicData.variance);
    Serial.println(midFreqData.historicData.variance);


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
        lastBeatTime_ms = GetMillis();
#ifdef PRINT_BIN_MAGNITUDES
        PrintVector(vReal, NUMBER_OF_SAMPLES, SCL_FREQUENCY);
        delay(20000);
#endif
    }
}

static void PopulateRealAndImag(int32_t rawMicSamples[FFT_BUFFER_LENGTH])
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

static inline bool IsMagAboveThreshold(freqBandData_t *freqBandData)
{
    return (freqBandData->currentMagnitude > (freqBandData->averageMagnitude * freqBandData->beatDetectThresholdCoeff));
}

static inline float ProportionOfMagAboveAvg(freqBandData_t *freqBandData)
{
    return (freqBandData->currentMagnitude / freqBandData->averageMagnitude);
}
