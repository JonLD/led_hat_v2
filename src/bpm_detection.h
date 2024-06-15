#ifndef BPM_DETECTION_H
#define BPM_DETECTION_H

#include <stdint.h>

void BpmDetection_Init(int64_t nowMs);
void BpmDetection_Step(int64_t nowMs);

#endif // BPM_DETECTION_H
