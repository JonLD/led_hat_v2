#include "beat_detection.h"

#include "driver/i2s.h"

#define FFT_SQRT_APPROXIMATION
#define FFT_SPEED_OVER_PRECISION
#include <arduinoFFT.h>

#include "bpm_detection.h"
#include "profiling.h"
#include "timing.h"

#define BEAT_DEBOUNCE_DURATION_MS 200
#define MAX_BASS_FREQUENCY_HZ 140.0f

#define HISTORICAL_MAG_LENGTH 1
#define HISTORIC_VAR_LENGTH 40

#define MAXIMUM_BPM                 155u
#define MINIMUM_DELAY_BETWEEN_BEATS (60000 / MAXIMUM_BPM)
#define SINGLE_BEAT_DURATION_MS     100u // good value range is [50:150]


template <typename T, size_t N>
class HistoricData {
    public:
        T data[N] = { 0 };
        int frontIdx = 0;
        int rearIdx = N-1;
        T mean = 0;
        float energy = 0.0;
        T recent_max = 0;


        void Update(T sample)
        {
            insert(sample);
            mean = Mean();
            energy = Energy();
            recent_max = max_element();
        }

    private:
        T max_element() 
        {
            T largest = data[0];
            for (int i = 1; i < N; ++i) {
                largest = max(data[i], largest);
            }
            return largest;
        }

        void insert(T sample) 
        {
            if(++rearIdx == N) {
                rearIdx = 0;
            }
            if(++frontIdx == N) {
                frontIdx = 0;
            }
            data[frontIdx] = sample;
        }

        float TotalMagnitute(void)
        {
            float sum = 0;
            for(int i = 0; i<N ; i++){
                sum+=(float)data[i];
            }
            return sum;
        }

        T Mean(void)
        {
            return TotalMagnitute() / N;
        }


        float Energy(void)
        {
            mean = Mean();
            float squaredDifferenceSum = 0;
            for(int i = 0; i < N ; i++)
            {
                squaredDifferenceSum+= data[i]*data[i];
            }
            return (double)(squaredDifferenceSum / N);
        }
};



unsigned long lastBeatTime_ms = 0;
bool isBeatDetected = false;

typedef struct freqBandData_t
{
    HistoricData<float, HISTORICAL_MAG_LENGTH> historicMag;
    HistoricData<float, HISTORIC_VAR_LENGTH> historicEnergy;
    float averageMagnitude;
    float currentMagnitude;
    float averageVariance;
    uint8_t lowerBinIndex;
    uint8_t upperBinIndex;
    float beatDetectThresholdCoeff;
    float leakyAverageCoeff;
    float minMagnitude;
    uint32_t varianceThreshold;
} freqBandData_s;

static freqBandData_t subFreqData{
    .averageMagnitude = 0,
    .currentMagnitude = 0,
    .lowerBinIndex = 1,
    .upperBinIndex = 1,
    .beatDetectThresholdCoeff = 1.2,
    .leakyAverageCoeff = 0.100,
    .minMagnitude = 100000000,
    .varianceThreshold = 200000000
};

static freqBandData_t bassFreqData{
    .averageMagnitude = 0,
    .currentMagnitude = 0,
    .lowerBinIndex = 2,
    .upperBinIndex = 2,
    .beatDetectThresholdCoeff = 1.2,
    .leakyAverageCoeff = 0.100,
    .minMagnitude = 40000000,
    .varianceThreshold = 200000000

};

static freqBandData_t midFreqData{
    .averageMagnitude = 0,
    .currentMagnitude = 0,
    .lowerBinIndex = 3,
    .upperBinIndex = 3,
    .beatDetectThresholdCoeff = 1.2,
    .leakyAverageCoeff = 0.100,
    .minMagnitude = 40000000,
    .varianceThreshold = 200000000

};

static freqBandData_t frequencyBands[] = {
    subFreqData,
    bassFreqData,
    midFreqData
};

float vImag[FFT_BUFFER_LENGTH] = {0};
float vReal[FFT_BUFFER_LENGTH] = {0};
ArduinoFFT<float> FFT = ArduinoFFT<float>(vReal, vImag, FFT_BUFFER_LENGTH, SAMPLING_FREQUENCY_HZ, true);

static void AnalyzeFrequencyBand(freqBandData_t *);
static inline bool IsMagAboveThreshold(freqBandData_t *freqBandData);
static inline float ProportionOfMagAboveAvg(freqBandData_t *freqBandData);
static void PopulateRealAndImag(int32_t rawMicSamples[FFT_BUFFER_LENGTH]);

void ComputeFFT(int32_t rawMicSamples[FFT_BUFFER_LENGTH])
{
    PopulateRealAndImag(rawMicSamples);
    FFT.dcRemoval();
    FFT.windowing(FFTWindow::Rectangle, FFTDirection::Forward);
    FFT.compute(FFTDirection::Forward);
    FFT.complexToMagnitude();

    AnalyzeFrequencyBand(&subFreqData);
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
    freqBand->historicMag.Update(freqBand->currentMagnitude);
    freqBand->historicEnergy.Update(freqBand->historicMag.energy);
    
    // Calulate leaky average
    freqBand->averageMagnitude = (freqBand->averageMagnitude * (HISTORIC_VAR_LENGTH-1) / HISTORIC_VAR_LENGTH) + freqBand->currentMagnitude / HISTORIC_VAR_LENGTH;
}

float NormaliseEnergy(freqBandData_t *freqBand)
{
    return freqBand->historicMag.energy / freqBand->historicEnergy.recent_max;
    
}

float PropEnergyOverMean(freqBandData_t *freqBand)
{
    return freqBand->historicMag.energy / freqBand->historicEnergy.mean;
    
}

float RecencyFactor() 
{
    float recencyFactor = 1;
    float referenceDuration = (60000.0 / bpmState.currentBpmEstimate) - 30;
    recencyFactor =  (millis() - lastBeatTime_ms) / referenceDuration;
    Serial.printf("%f\t", (float)bpmState.currentBpmEstimate);

    recencyFactor = constrain(recencyFactor, 0, 1);
    return recencyFactor * recencyFactor * recencyFactor; 
}

void DetectBeat()
{
    const bool isBassAboveAvg = IsMagAboveThreshold(&subFreqData);
    const bool isMidAboveAvg = IsMagAboveThreshold(&bassFreqData);
    const bool isNoRecentBeat = (GetMillis() - lastBeatTime_ms) > (BEAT_DEBOUNCE_DURATION_MS);
    const bool peakIsBass = (FFT.majorPeak() < MAX_BASS_FREQUENCY_HZ);
    const bool isAvgBassAboveMin = (subFreqData.averageMagnitude > subFreqData.minMagnitude);
    const float proportionSubAboveAvg = PropEnergyOverMean(&subFreqData);
    const float proportionBassAboveAvg = PropEnergyOverMean(&bassFreqData);
    const float proportionMidAboveAvg = PropEnergyOverMean(&midFreqData);
    const bool bassVarAboveThreshold = subFreqData.historicMag.energy > subFreqData.varianceThreshold;
    const bool midVarAboveThreshold = midFreqData.historicMag.energy > midFreqData.varianceThreshold;
    float recencyFactor = RecencyFactor();
    float subVarfactor = NormaliseEnergy(&subFreqData);
    float bassVarfactor = NormaliseEnergy(&bassFreqData);
    float midfactor = NormaliseEnergy(&bassFreqData);


    Serial.printf("%f\t%f\t%f\t", proportionSubAboveAvg, proportionBassAboveAvg, proportionMidAboveAvg);

    Serial.printf("%f\t", proportionSubAboveAvg + proportionBassAboveAvg);
    Serial.printf("%f\t", subFreqData.averageMagnitude);
    // Serial.printf("%f\t", bassFreqData.historicMag.energy);
    // Serial.printf("%f\t", midFreqData.historicMag.energy);
    // Serial.printf("%f\t", subFreqData.historicEnergy.energy);
    // Serial.printf("%f\t", bassFreqData.historicEnergy.energy);
    // Serial.printf("%f\t", midFreqData.historicEnergy.energy);
    Serial.print("\n");


    isBeatDetected = (proportionSubAboveAvg > 2) && peakIsBass && isAvgBassAboveMin && isNoRecentBeat;
        // isNoRecentBeat && isBassAboveAvg && peakIsBass && isAvgBassAboveMin 
                    //   && isMidAboveAvg);

#ifdef PRINT_CURRENT_BASS_MAG
    Serial.println(subFreqData.currentMagnitude);
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
        const int64_t nowMs = GetMillis();
        BpmDetection_Step(nowMs);
        lastBeatTime_ms = nowMs;
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
