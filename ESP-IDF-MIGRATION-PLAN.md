# ESP-IDF Migration Plan - Synesthetix Light Device

**Document Version:** 1.0
**Date:** 2026-01-24
**Target Device:** Audio-reactive LED matrix (wearable/fixture)

---

## Executive Summary

Migration from Arduino framework to ESP-IDF to achieve:
- **Professional-grade OTA updates** with dual-partition rollback
- **Better performance** with proper FreeRTOS task architecture
- **Production readiness** for potential commercial deployment
- **Resume/portfolio value** demonstrating embedded systems expertise

**Estimated Effort:** 2-4 weeks (can be done incrementally)

---

## Current Architecture Analysis

### Core Components (Arduino Framework)

| Component | Current Implementation | Lines of Code | Complexity |
|-----------|----------------------|---------------|------------|
| Main Loop | `light_device.cpp` (setup/loop) | ~268 | Medium |
| LED Control | FastLED library | ~30 direct calls | High dependency |
| Audio Input | I2S microphone (i2s_mic.cpp) | ~50 | Low (already low-level) |
| Beat Detection | ArduinoFFT + custom logic | ~100 | Medium |
| Effects Engine | Custom effect system (effects.cpp) | ~280 | Low (pure logic) |
| Wireless | ESP-NOW receive | ~15 | Low (already ESP-IDF) |
| Profiling | Custom timing (profiling.cpp/h) | ~40 | Low |

### Hardware Configuration

- **Board:** ESP32 (AZ-Delivery DevKit v4)
- **LEDs:** 170 pixels (34x5 matrix) via FastLED
- **Audio:** I2S MEMS microphone, 48kHz sampling
- **Wireless:** ESP-NOW for remote control (receive only)
- **Power:** USB powered (5V)

### Current Dependencies

```ini
Arduino framework
FastLED (LED control abstraction)
ArduinoFFT (frequency analysis)
ESP-NOW (already native ESP-IDF)
WiFi (for ESP-NOW initialization)
```

---

## ESP-IDF Target Architecture

### Proposed FreeRTOS Task Structure

```
┌─────────────────────────────────────────────────────┐
│                   app_main()                        │
│  - Initialize NVS, WiFi, I2S, RMT, ESP-NOW         │
│  - Create tasks                                     │
│  - Setup OTA partition monitoring                   │
└─────────────────────────────────────────────────────┘
                        │
        ┌───────────────┼───────────────┬──────────────┐
        ▼               ▼               ▼              ▼
┌──────────────┐ ┌──────────────┐ ┌──────────┐ ┌────────────┐
│ Audio Task   │ │ Effect Task  │ │ LED Task │ │ OTA Task   │
│ (Priority 3) │ │ (Priority 2) │ │(Priority1)│ │(Priority 0)│
├──────────────┤ ├──────────────┤ ├──────────┤ ├────────────┤
│- Read I2S    │ │- Beat detect │ │- Render  │ │- Listen    │
│- Fill buffer │ │- Select FX   │ │  effects │ │  for OTA   │
│- Compute FFT │ │- Update state│ │- RMT     │ │  trigger   │
│              │ │              │ │  output  │ │- Download  │
└──────────────┘ └──────────────┘ └──────────┘ └────────────┘
       │                 │              │              │
       └─────────────────┴──────────────┴──────────────┘
                         │
                    Event Groups
                  Queue Messages
```

### Component Mapping

| Arduino Component | ESP-IDF Replacement | Migration Effort |
|------------------|---------------------|------------------|
| `Serial.print()` | `ESP_LOG*()` macros | Easy (find/replace) |
| `setup()/loop()` | `app_main()` + tasks | Medium (architectural) |
| FastLED | RMT driver (WS2812) | Hard (rewrite) |
| ArduinoFFT | ESP-DSP library | Medium (API change) |
| ESP-NOW | esp_now.h (native) | Easy (already compatible) |
| I2S (Arduino) | I2S driver (ESP-IDF) | Easy-Medium (minimal change) |
| `delay()` | `vTaskDelay()` | Easy (find/replace) |
| `millis()` | `esp_timer_get_time()` | Easy (find/replace) |

---

## Migration Strategy

### Phase 1: Foundation (Week 1)
**Goal:** Get basic ESP-IDF project building

- [ ] Create ESP-IDF project structure
- [ ] Port config.h and interface.h (pure C compatible)
- [ ] Implement app_main() skeleton
- [ ] Setup logging system
- [ ] Port timing utilities to ESP-IDF timers
- [ ] Basic GPIO blink test

**Deliverable:** Blinking LED with ESP-IDF

### Phase 2: I2S Audio (Week 1-2)
**Goal:** Get microphone data flowing

- [ ] Configure I2S driver for MEMS mic
- [ ] Create audio input task
- [ ] Port i2s_mic.cpp to ESP-IDF API
- [ ] Implement circular buffer for samples
- [ ] Test with serial output of audio levels

**Deliverable:** Audio sampling working, visible in logs

### Phase 3: DSP & Beat Detection (Week 2)
**Goal:** Replicate beat detection functionality

- [ ] Integrate ESP-DSP library
- [ ] Port ArduinoFFT calls to ESP-DSP
- [ ] Port beat_detection.cpp logic
- [ ] Create effect selection task
- [ ] Test beat detection accuracy vs Arduino version

**Deliverable:** Beat detection working, logged to serial

### Phase 4: LED Control (Week 2-3) - **CRITICAL PATH**
**Goal:** Replace FastLED with RMT driver

**Option A: Use ESP-IDF RMT Driver Directly**
- [ ] Configure RMT peripheral for WS2812B timing
- [ ] Implement RGB pixel buffer
- [ ] Port color conversion logic
- [ ] Rewrite FastLED.show() equivalent
- [ ] Test single LED first, then full matrix

**Option B: Use led_strip Component**
- [ ] Use ESP-IDF's led_strip component (wraps RMT)
- [ ] Simpler API but less control
- [ ] May need custom color correction

**Recommendation:** Start with Option B, fall back to A if needed

**Tasks:**
- [ ] Port effects.cpp to new LED API
- [ ] Test each effect individually
- [ ] Verify color accuracy
- [ ] Performance profiling

**Deliverable:** Full LED matrix effects working

### Phase 5: ESP-NOW Integration (Week 3)
**Goal:** Wireless control restored

- [ ] Initialize WiFi in STA mode
- [ ] Configure ESP-NOW callbacks
- [ ] Port radioData receive handler
- [ ] Test remote control functionality

**Deliverable:** Device responds to ESP-NOW commands

### Phase 6: OTA Implementation (Week 3-4)
**Goal:** Professional OTA update system

- [ ] Configure partition table for OTA
  ```
  nvs:      24K
  otadata:  8K
  ota_0:    1.5M  (factory)
  ota_1:    1.5M  (update)
  ```
- [ ] Implement HTTPS OTA from server
- [ ] Add rollback on boot failure
- [ ] Version management in NVS
- [ ] OTA trigger via ESP-NOW or web
- [ ] Test update/rollback cycle

**Deliverable:** Working OTA update mechanism

### Phase 7: Polish & Testing (Week 4)
**Goal:** Production-ready code

- [ ] Code cleanup and documentation
- [ ] Performance optimization
- [ ] Memory leak testing (valgrind-like tools)
- [ ] Power consumption profiling
- [ ] Create comprehensive README
- [ ] Build/flash documentation

**Deliverable:** Fully migrated, documented project

---

## OTA Implementation Deep Dive

### Partition Table Design

```
# partitions.csv
# Name,   Type, SubType, Offset,  Size,    Flags
nvs,      data, nvs,     0x9000,  0x6000,
otadata,  data, ota,     0xf000,  0x2000,
phy_init, data, phy,     0x11000, 0x1000,
ota_0,    app,  ota_0,   0x20000, 0x180000,
ota_1,    app,  ota_1,   0x1A0000,0x180000,
```

### OTA Update Flow

```
1. Device boots → Check otadata → Run ota_0 (factory)
2. OTA trigger received (ESP-NOW or WiFi)
3. Download firmware to ota_1 partition
4. Verify SHA256 checksum
5. Mark ota_1 as valid, set next boot
6. Reboot
7. Boot from ota_1
8. If boot fails → rollback to ota_0
9. If boot succeeds → mark ota_1 permanent
```

### OTA Trigger Options

**Option 1: ESP-NOW Command**
- Send magic packet to trigger OTA
- Includes HTTPS URL to firmware
- Fast, no infrastructure needed

**Option 2: Web Interface**
- ESP32 runs lightweight HTTP server
- Upload .bin file via browser
- Good for local development

**Option 3: Automatic Updates**
- Periodically check server for updates
- HTTPS with certificate pinning
- Production-grade approach

### Security Considerations

- [ ] HTTPS only for firmware downloads
- [ ] Signature verification (ESP32 secure boot)
- [ ] Encrypted flash storage (optional)
- [ ] Version downgrade protection
- [ ] Certificate pinning for server

---

## Testing Strategy

### Unit Testing
- ESP-IDF supports Unity test framework
- Test individual components (DSP, effects logic)
- Run on device or host (for pure logic)

### Integration Testing
- Full system tests with real hardware
- Audio input → beat detection → LED output
- ESP-NOW → effect change → visual verification
- OTA update cycle testing

### Performance Benchmarks
- Frame rate (target: 60+ FPS)
- FFT computation time
- Task latency measurements
- Memory usage profiling

### Regression Testing
- Visual comparison with Arduino version
- Beat detection accuracy
- Color accuracy
- Effect timing

---

## Risks & Mitigations

| Risk | Impact | Mitigation |
|------|--------|------------|
| FastLED replacement breaks visuals | High | Incremental testing, keep Arduino branch |
| FFT performance degradation | Medium | ESP-DSP is optimized, should be faster |
| RMT timing issues with LEDs | High | Use proven led_strip component |
| OTA bricks device | High | Bootloader rollback, serial recovery |
| Timeline overrun | Low | Incremental approach, can pause anytime |

---

## Success Criteria

✅ Device boots and runs reliably
✅ All effects work identically to Arduino version
✅ Beat detection accuracy matches or exceeds current
✅ OTA updates work with rollback protection
✅ Performance: >60 FPS LED refresh, <50ms FFT
✅ Memory: <80% heap usage under normal operation
✅ Code quality: Clean, documented, professional

---

## Tools & Resources

### ESP-IDF Documentation
- https://docs.espressif.com/projects/esp-idf/
- ESP-DSP: https://github.com/espressif/esp-dsp
- OTA Updates: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/ota.html

### Development Tools
- ESP-IDF v5.x (latest stable)
- PlatformIO ESP-IDF platform support
- ESP-IDF Monitor for debugging
- ESP-IDF Size Analysis tools

### Reference Projects
- ESP-IDF examples/bluetooth/esp_hid_device
- ESP-IDF examples/system/ota
- led_strip component examples

---

## Next Steps

1. **Review this plan** - Adjust timeline/approach as needed
2. **Setup ESP-IDF environment** - Install toolchain
3. **Create feature branch** - Keep Arduino version safe
4. **Start Phase 1** - Basic project structure
5. **Document progress** - Blog posts, commit messages

---

## Questions to Resolve

- [ ] Do we need Bluetooth for future control options?
- [ ] Should we add a web config interface?
- [ ] What OTA server infrastructure? (Self-hosted vs cloud)
- [ ] Battery power support in future?
- [ ] Need for remote logging/telemetry?

---

## Appendix A: File Structure Comparison

### Current (Arduino)
```
Synesthetix/
├── src/
│   ├── light_device.cpp      (main)
│   ├── beat_detection.cpp/h
│   ├── effects.cpp/h
│   ├── i2s_mic.cpp/h
│   ├── interface.h
│   ├── config.h
│   ├── timing.h
│   └── profiling.cpp/h
└── platformio.ini
```

### Proposed (ESP-IDF)
```
Synesthetix/
├── main/
│   ├── main.c                (app_main)
│   ├── audio_task.c/h        (I2S + DSP)
│   ├── effect_task.c/h       (beat detection + selection)
│   ├── led_task.c/h          (RMT output)
│   ├── ota_task.c/h          (OTA updates)
│   ├── effects.c/h           (effect implementations)
│   ├── config.h              (hardware config)
│   └── interface.h           (ESP-NOW protocol)
├── components/               (custom components)
│   └── led_matrix/          (LED abstraction layer)
├── CMakeLists.txt
├── sdkconfig                 (ESP-IDF configuration)
└── partitions.csv           (partition table)
```

---

## Appendix B: Memory Budget

**ESP32 Resources:**
- Flash: 4MB
- RAM: 520KB (SRAM)
- PSRAM: None (current board)

**Estimated Usage:**
- Firmware (ota_0): ~800KB
- Firmware (ota_1): ~800KB
- NVS: 24KB
- Runtime heap: ~150KB
- FreeRTOS stacks: ~32KB
- LED buffer (170 pixels × 3): ~512 bytes
- FFT buffer: ~4KB
- Audio buffer: ~16KB

**Total: ~2MB flash, ~200KB RAM** ✅ Well within limits

---

*End of Migration Plan*
