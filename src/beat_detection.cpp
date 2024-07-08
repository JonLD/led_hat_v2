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
#define HISTORIC_ENERGY_LENGTH 50
#define HISTORIC_ENERGY_LONG_LENGTH 4000


template <size_t N>
class HistoricData {
    public:
        float data[N] = { 0 };
        int rearIdx = 0;
        float mean = 0;
        float energy = 0.0;
        float recent_max = 0;

        void Update(float sample)
        {
            Insert(sample); // Must insert new sample first
            mean = Mean();
            energy = Energy();
            recent_max = MaxElement();
        }

    private:
        uint32_t NextIndex(uint32_t currentIndex)
        {
            return currentIndex ? ++currentIndex == N : 0;
        }
        uint32_t PreviousIndex(uint32_t currentIndex)
        {
            return currentIndex ? currentIndex-- == 0 : N -1;
        }

        uint32_t GetMultiIndexPrevious(uint32_t currentIndex, uint32_t numIdexPrev)
        {
            for (int i = 0; i < numIdexPrev; i++)
            {
                currentIndex = PreviousIndex(currentIndex);
            }
            return currentIndex;
        }

        float MaxElement(void) 
        {
            float largest = data[0];
            for (int i = 1; i < N; ++i) {
                largest = max(data[i], largest);
            }
            return largest;
        }

        void Insert(float sample) 
        {
            if(++rearIdx == N) 
            {
                rearIdx = 0;
            }
            data[rearIdx] = sample;
        }

        float Mean(void)
        {
            float sum = 0;
            for(int i = 0; i<N ; i++){
                sum+=(float)data[i];
            }
            return sum / N;
        }

        float Energy(void)
        {
            float squaredDifferenceSum = 0;
            for(int i = 0; i < N ; i++)
            {
                squaredDifferenceSum+= data[i]*data[i];
            }
            return squaredDifferenceSum / N;
        }

        float StandardDeviation(void)
        {
            mean = Mean();
            double squaredDifferenceSum = 0;
            for(int i = 0; i < N ; i++)
            {
                squaredDifferenceSum+= (double)((data[i]-mean)*(data[i]-mean));
            }
            return sqrt((squaredDifferenceSum / (double)N));
        }
};



unsigned long lastBeatTime_ms = 0;
bool isBeatDetected = false;

typedef struct freqBandData_t
{
    HistoricData<HISTORICAL_MAG_LENGTH> historicMag;
    HistoricData<HISTORIC_ENERGY_LENGTH> historicEnergy;
    HistoricData<HISTORIC_ENERGY_LONG_LENGTH> historicEnergyLong;
    float averageMagnitude;
    float currentMagnitude;
    float averageVariance;
    uint8_t binIndex;
    float beatDetectThresholdCoeff;
    float minMagnitude;
} freqBandData_s;

static freqBandData_t subFreqData{
    .averageMagnitude = 0,
    .currentMagnitude = 0,
    .binIndex = 1,
    .beatDetectThresholdCoeff = 1.2,
    .minMagnitude = 200000000,
};

float vImag[FFT_BUFFER_LENGTH] = {0};
float vReal[FFT_BUFFER_LENGTH] = {0};
ArduinoFFT<float> FFT = ArduinoFFT<float>(vReal, vImag, FFT_BUFFER_LENGTH, SAMPLING_FREQUENCY_HZ, true);

static void AnalyzeFrequencyBand(freqBandData_t *);
static inline bool IsMagAboveThreshold(freqBandData_t *freqBandData);
static inline float ProportionOfMagAboveAvg(freqBandData_t *freqBandData);
static void PopulateRealAndImag(int32_t rawMicSamples[FFT_BUFFER_LENGTH]);
static float PropEnergyOverMean(freqBandData_t *freqBand);


void ComputeFFT(int32_t rawMicSamples[FFT_BUFFER_LENGTH])
{
    PopulateRealAndImag(rawMicSamples);
    FFT.dcRemoval();
    FFT.windowing(FFTWindow::Rectangle, FFTDirection::Forward);
    FFT.compute(FFTDirection::Forward);
    FFT.complexToMagnitude();

    AnalyzeFrequencyBand(&subFreqData);
}

static void AnalyzeFrequencyBand(freqBandData_t *freqBand)
{
    // Calculate current magnitude by averaging bins in frequency range
    freqBand->currentMagnitude = vReal[freqBand->binIndex];
    freqBand->historicMag.Update(freqBand->currentMagnitude);
    freqBand->historicEnergy.Update(freqBand->historicMag.energy);
    freqBand->historicEnergyLong.Update(freqBand->historicMag.energy);

    // Calulate leaky average
    freqBand->averageMagnitude = (freqBand->averageMagnitude * (HISTORIC_ENERGY_LENGTH-1) / HISTORIC_ENERGY_LENGTH) + freqBand->currentMagnitude / HISTORIC_ENERGY_LENGTH;
}

float PropEnergyOverMean(freqBandData_t *freqBand)
{
    float mean;
    if (GetMillis() - lastBeatTime_ms > 500)
    {
        mean = freqBand->historicEnergyLong.mean;
    }
    else
    {
        mean = freqBand->historicEnergy.mean;
    }

    return freqBand->historicMag.energy / mean;
    
}

float RecencyFactor() 
{
    float recencyFactor = 1;
    float referenceDuration = (60000.0 / bpmState.currentBpmEstimate) - 30;
    recencyFactor =  (millis() - lastBeatTime_ms) / referenceDuration;

    recencyFactor = constrain(recencyFactor, 0, 1);
    return recencyFactor * recencyFactor * recencyFactor; 
}

void DetectBeat()
{
    const bool isBassAboveAvg = IsMagAboveThreshold(&subFreqData);
    const bool isNoRecentBeat = (GetMillis() - lastBeatTime_ms) > (BEAT_DEBOUNCE_DURATION_MS);
    const bool peakIsBass = (FFT.majorPeak() < MAX_BASS_FREQUENCY_HZ);
    const bool isAvgBassAboveMin = (subFreqData.averageMagnitude > subFreqData.minMagnitude);
    const float proportionSubAboveAvg = PropEnergyOverMean(&subFreqData);

    // Serial.printf("%f\t%f\t%f\t", proportionSubAboveAvg);
    // Serial.printf("%f\t", subFreqData.averageMagnitude);
    // Serial.print("\n");

    isBeatDetected = (proportionSubAboveAvg > 2) && peakIsBass && isAvgBassAboveMin && isNoRecentBeat;


    if (isBeatDetected)
    {
        const int64_t nowMs = GetMillis();
        BpmDetection_Step(nowMs);
        lastBeatTime_ms = nowMs;
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