#include "advanced_geometry.h"
#include "effects.h"

#ifdef ADVANCED_JELLYFISH_GEOMETRY

// Functions to get LED arrays for each section

void GetCenterTentacleLEDs(int* led_indices, int* count) {
    *count = CENTER_TENTACLE_LEDS;
    for (int i = 0; i < CENTER_TENTACLE_LEDS; i++) {
        led_indices[i] = i;
    }
}

void GetCenterTentacleDownLEDs(int* led_indices, int* count) {
    *count = CENTER_TENTACLE_LENGTH;
    for (int i = 0; i < CENTER_TENTACLE_LENGTH; i++) {
        led_indices[i] = i;
    }
}

void GetCenterTentacleUpLEDs(int* led_indices, int* count) {
    *count = CENTER_TENTACLE_LENGTH;
    for (int i = 0; i < CENTER_TENTACLE_LENGTH; i++) {
        led_indices[i] = CENTER_TENTACLE_LENGTH + i;
    }
}

void GetConnectorLEDs(int* led_indices, int* count) {
    int start = CENTER_TENTACLE_LEDS;
    *count = CENTER_TO_RIM_CONNECTOR_LEDS;
    for (int i = 0; i < CENTER_TO_RIM_CONNECTOR_LEDS; i++) {
        led_indices[i] = start + i;
    }
}

void GetRimLEDs(int* led_indices, int* count) {
    int start = CENTER_TENTACLE_LEDS + CENTER_TO_RIM_CONNECTOR_LEDS;
    *count = TOTAL_RIM_LEDS;
    for (int i = 0; i < TOTAL_RIM_LEDS; i++) {
        led_indices[i] = start + i;
    }
}

void GetEdgeTentacleLEDs(int tentacle_id, int* led_indices, int* count) {
    if (tentacle_id < 0 || tentacle_id >= NUM_EDGE_TENTACLES) {
        *count = 0;
        return;
    }
    
    int start = CENTER_TENTACLE_LEDS + CENTER_TO_RIM_CONNECTOR_LEDS + TOTAL_RIM_LEDS + (tentacle_id * EDGE_TENTACLE_LEDS_EACH);
    *count = EDGE_TENTACLE_LEDS_EACH;
    for (int i = 0; i < EDGE_TENTACLE_LEDS_EACH; i++) {
        led_indices[i] = start + i;
    }
}

void GetEdgeTentacleDownLEDs(int tentacle_id, int* led_indices, int* count) {
    if (tentacle_id < 0 || tentacle_id >= NUM_EDGE_TENTACLES) {
        *count = 0;
        return;
    }
    
    int start = CENTER_TENTACLE_LEDS + CENTER_TO_RIM_CONNECTOR_LEDS + TOTAL_RIM_LEDS + (tentacle_id * EDGE_TENTACLE_LEDS_EACH);
    *count = EDGE_TENTACLE_LENGTH;
    for (int i = 0; i < EDGE_TENTACLE_LENGTH; i++) {
        led_indices[i] = start + i;
    }
}

void GetEdgeTentacleUpLEDs(int tentacle_id, int* led_indices, int* count) {
    if (tentacle_id < 0 || tentacle_id >= NUM_EDGE_TENTACLES) {
        *count = 0;
        return;
    }
    
    int start = CENTER_TENTACLE_LEDS + CENTER_TO_RIM_CONNECTOR_LEDS + TOTAL_RIM_LEDS + (tentacle_id * EDGE_TENTACLE_LEDS_EACH) + EDGE_TENTACLE_LENGTH;
    *count = EDGE_TENTACLE_LENGTH;
    for (int i = 0; i < EDGE_TENTACLE_LENGTH; i++) {
        led_indices[i] = start + i;
    }
}

void GetAllEdgeTentaclesLEDs(int* led_indices, int* count) {
    int start = CENTER_TENTACLE_LEDS + CENTER_TO_RIM_CONNECTOR_LEDS + TOTAL_RIM_LEDS;
    *count = TOTAL_EDGE_TENTACLE_LEDS;
    for (int i = 0; i < TOTAL_EDGE_TENTACLE_LEDS; i++) {
        led_indices[i] = start + i;
    }
}

// Legacy compatibility function for existing effects
int MapXYToAdvancedLED(int x, int y) {
    // Simple mapping for backward compatibility
    if (x < 2) {
        // Center tentacle
        if (x == 0) {
            return y; // Down stroke
        } else {
            return CENTER_TENTACLE_LENGTH + y; // Up stroke
        }
    } else {
        // Edge tentacles
        int tentacle_id = (x - 2) / 2;
        if (tentacle_id >= NUM_EDGE_TENTACLES) {
            tentacle_id = NUM_EDGE_TENTACLES - 1;
        }
        
        int tentacle_base = CENTER_TENTACLE_LEDS + CENTER_TO_RIM_CONNECTOR_LEDS + TOTAL_RIM_LEDS + (tentacle_id * EDGE_TENTACLE_LEDS_EACH);
        
        if ((x - 2) % 2 == 0) {
            return tentacle_base + y; // Down stroke
        } else {
            return tentacle_base + EDGE_TENTACLE_LENGTH + y; // Up stroke
        }
    }
}

#endif // ADVANCED_JELLYFISH_GEOMETRY