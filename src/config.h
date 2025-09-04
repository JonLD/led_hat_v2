#ifndef CONFIG_H
#define CONFIG_H

// Advanced Jellyfish Geometry Configuration
// ===========================================

// Layout: Center tentacle (up+down) -> Connector -> Rim -> Edge tentacles around perimeter
// LED strip order: [Center tentacle] -> [Center-to-rim connector] -> [Rim + Edge tentacles alternating]

// Center tentacle (double length - down then up)
#define CENTER_TENTACLE_LENGTH 15    // Length of tentacle (total LEDs = 2x this)
#define CENTER_TENTACLE_LEDS (CENTER_TENTACLE_LENGTH * 2)

// Center to rim connector
#define CENTER_TO_RIM_CONNECTOR_LEDS 3

// Edge tentacles (around perimeter)
#define NUM_EDGE_TENTACLES 8         // Normal tentacles around edge
#define EDGE_TENTACLE_LENGTH 15      // Length of tentacle (total LEDs = 2x this)
#define EDGE_TENTACLE_LEDS_EACH (EDGE_TENTACLE_LENGTH * 2)

// Rim LEDs (connecting sections between tentacles)
#define RIM_LEDS_PER_SECTION 5       // LEDs between each tentacle on rim

// Total calculations
#define TOTAL_RIM_LEDS (NUM_EDGE_TENTACLES * RIM_LEDS_PER_SECTION)
#define TOTAL_EDGE_TENTACLE_LEDS (NUM_EDGE_TENTACLES * EDGE_TENTACLE_LEDS_EACH)
#define NUM_LEDS (CENTER_TENTACLE_LEDS + CENTER_TO_RIM_CONNECTOR_LEDS + TOTAL_RIM_LEDS + TOTAL_EDGE_TENTACLE_LEDS)

// Legacy compatibility (for existing effects that use matrix coordinates)
#define NUMBER_X_LEDS 20   // Maintain for compatibility
#define NUMBER_Y_LEDS 15   // Maintain for compatibility  
#define MAX_X_INDEX (NUMBER_X_LEDS - 1)
#define MAX_Y_INDEX (NUMBER_Y_LEDS - 1)

#define BRIGHTNESS_RE_PIN_A 19
#define BRIGHTNESS_RE_PIN_B 18

// Geometry configuration flags
#define ADVANCED_JELLYFISH_GEOMETRY  // Enable advanced geometry mapping
#define VERTICAL_ZIGZAG              // Define this for jellyfish (vertical zigzag), comment out for hat (horizontal zigzag)

#endif // CONFIG_H