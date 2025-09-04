# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Synesthetix is an ESP32-based LED controller system that creates visual effects synchronized with audio beats. The project consists of two main components:
- **Hat**: The LED matrix display device that shows visual effects based on audio input
- **Controller**: A control interface using NeoTrellis keypads to send commands to the hat

## Build System

This project uses PlatformIO with the Arduino framework for ESP32 development.

### Common Commands

```bash
# Build the hat firmware
pio run -e hat

# Build the controller firmware  
pio run -e controller

# Upload to hat (default COM7)
pio run -e hat -t upload

# Upload to controller (default COM10)
pio run -e controller -t upload

# Monitor serial output
pio device monitor -b 115200

# Run tests
pio test

# Clean build files
pio run -t clean
```

## Architecture

### Key Components

**Communication**: ESP-NOW wireless protocol for controller-to-hat communication
- Controller MAC: `0x0C, 0xB8, 0x15, 0xF8, 0xE6, 0x40`
- Hat MAC: `0x0C, 0xB8, 0x15, 0xF8, 0xF6, 0x80`

**LED Configuration**: 
- Matrix: 34x5 LEDs (170 total) using FastLED library
- Controlled via effects system with ambient and beat-reactive modes

**Audio Processing**:
- I2S microphone input at 48kHz sampling
- FFT-based beat detection (1024 sample buffer)
- Beat effects triggered on audio transients

**Effects System**:
- Ambient effects: Continuous animations (twinkle, waves, etc.)
- Beat effects: Triggered by beat detection (bars, waves, flashes)
- Effect selection via controller keypad or automatic rotation

**Interface Protocol** (`radioData_t` structure):
- Effect and color selection
- Brightness control
- Ambient override toggle
- Beat timing information

### Source Filter Configuration

The build system uses source filters to compile device-specific code:
- Hat builds exclude `controller.cpp`
- Controller builds exclude `hat.cpp`

### Hardware Dependencies

- Adafruit NeoTrellis (controller only)
- I2S microphone (hat only)
- FastLED compatible LED strip (hat only)
- ESP32 DevKit V4 boards