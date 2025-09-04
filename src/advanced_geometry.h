#ifndef ADVANCED_GEOMETRY_H
#define ADVANCED_GEOMETRY_H

#include "config.h"

#ifdef ADVANCED_JELLYFISH_GEOMETRY

// Section identifiers
enum class JellyfishSection {
    CENTER_TENTACLE,
    CENTER_TO_RIM_CONNECTOR,
    RIM,
    EDGE_TENTACLE
};

// Structure to represent a position in jellyfish geometry
struct JellyfishPosition {
    JellyfishSection section;
    int tentacle_id;     // Which tentacle (0-7 for edge tentacles, -1 for center/rim)
    int position;        // Position within the section
    bool is_up_stroke;   // For tentacles: true = going up, false = going down
};

// LED index ranges for each section
struct SectionRange {
    int start_index;
    int end_index;
};

// Get LED index ranges for each section
SectionRange GetCenterTentacleRange();
SectionRange GetConnectorRange();
SectionRange GetRimRange();
SectionRange GetEdgeTentacleRange(int tentacle_id);
SectionRange GetAllEdgeTentaclesRange();

// Main mapping functions
int MapJellyfishPositionToLED(const JellyfishPosition& pos);
JellyfishPosition MapLEDToJellyfishPosition(int led_index);

// Legacy compatibility - map x,y coordinates to advanced geometry
JellyfishPosition MapXYToJellyfishPosition(int x, int y);
int MapXYToAdvancedLED(int x, int y);

// Section-specific helper functions
bool IsLEDInCenterTentacle(int led_index);
bool IsLEDInRim(int led_index);
bool IsLEDInEdgeTentacle(int led_index, int tentacle_id = -1);
int GetTentacleForLED(int led_index);

// Functions to get LED arrays for each section
void GetCenterTentacleLEDs(int* led_indices, int* count);
void GetCenterTentacleDownLEDs(int* led_indices, int* count);
void GetCenterTentacleUpLEDs(int* led_indices, int* count);

void GetConnectorLEDs(int* led_indices, int* count);
void GetRimLEDs(int* led_indices, int* count);

void GetEdgeTentacleLEDs(int tentacle_id, int* led_indices, int* count);
void GetEdgeTentacleDownLEDs(int tentacle_id, int* led_indices, int* count);
void GetEdgeTentacleUpLEDs(int tentacle_id, int* led_indices, int* count);

void GetAllEdgeTentaclesLEDs(int* led_indices, int* count);

#endif // ADVANCED_JELLYFISH_GEOMETRY

#endif // ADVANCED_GEOMETRY_H