# Synesthetix Product Roadmap & Technical Plan

**Version:** 2.0
**Date:** 2026-01-25
**Author:** Jonathan Lloyd

---

## Table of Contents

1. [Vision & Products](#vision--products)
2. [Timeline Overview](#timeline-overview)
3. [Product 1: Distributed Wearable System](#product-1-distributed-wearable-system)
4. [Product 2: Professional Lighting Platform](#product-2-professional-lighting-platform)
5. [Technical Architecture](#technical-architecture)
6. [Synchronization Strategy](#synchronization-strategy)
7. [Research Areas](#research-areas)
8. [Libraries & Tools](#libraries--tools)
9. [Learning Resources](#learning-resources)
10. [Risk Assessment](#risk-assessment)

---

## Vision & Products

### Product 1: Distributed Wearable Lighting System
**Target Market:** Festivals, raves, performances, group wearables
**Timeline:** 1-2 months
**Status:** 70% complete (migration needed)

**Core Features:**
- Multiple wearable LED devices (hats, vests, accessories)
- Real-time synchronization across all devices
- Beat-reactive effects using audio analysis
- Wireless coordination via ESP-NOW
- OTA firmware updates
- Low latency (<20ms device-to-device)

**Value Proposition:**
Groups wearing synchronized LED gear that responds to music in perfect unison.

### Product 2: Professional Lighting Control Platform
**Target Market:** DJs, live performers, installations, stages
**Timeline:** 6-12 months
**Status:** 0% (design phase)

**Core Features:**
- GUI timeline editor for programming light shows
- 3D preview/simulation of lighting layout
- Music analysis with automatic feature extraction
- Live sync with DJ software (MIXX integration)
- Pre-programmed sequences synced to tracks
- Latency compensation for wireless systems
- Support for both wearables and fixed installations

**Value Proposition:**
Professional-grade lighting control system with DJ integration, rivaling commercial DMX systems but ESP32-based and affordable.

---

## Timeline Overview

### Phase 1: Foundation (Months 1-2) - Product 1 MVP
**Goal:** Ship working multi-device wearable system

- Week 1-2: ESP-IDF firmware migration
- Week 3-4: Multi-device sync protocol
- Week 5-6: Testing with 2-4 devices
- Week 7-8: OTA implementation, polish, demo

**Deliverable:** Sellable/demonstrable Product 1

### Phase 2: Rust Learning & Architecture (Month 3)
**Goal:** Learn Rust, design Product 2

- Week 9-10: Rust fundamentals (The Book, exercises)
- Week 11-12: Network programming in Rust
- Week 12: Product 2 architecture specification

**Deliverable:** Rust competency, detailed Product 2 spec

### Phase 3: Backend Development (Months 4-5)
**Goal:** Build control server and protocol

- Week 13-16: Rust sync server
- Week 17-18: ESP32 protocol integration
- Week 19-20: Python music analysis pipeline

**Deliverable:** Working backend that controls devices

### Phase 4: Frontend & Integration (Months 6-8)
**Goal:** GUI and end-to-end system

- Week 21-24: GUI timeline editor (Rust + Tauri/egui)
- Week 25-28: 3D preview renderer
- Week 29-32: MIXX/DJ software integration

**Deliverable:** Complete Product 2 alpha

### Phase 5: Polish & Market (Months 9+)
**Goal:** Production-ready, market launch

- Documentation, branding, website
- Beta testing with DJs
- Open-source components
- Product launch

---

## Product 1: Distributed Wearable System

### Technical Specifications

**Hardware per Device:**
- ESP32 (AZ-Delivery DevKit v4 or similar)
- WS2812B LED matrix (configurable: 170 LEDs current, scalable)
- I2S MEMS microphone (optional: leader only)
- LiPo battery + charging circuit
- Power: ~5V @ 2-4A (depending on LED count)

**Firmware Stack:**
- Framework: ESP-IDF (C)
- RTOS: FreeRTOS (included)
- Wireless: ESP-NOW for device sync
- Audio: I2S driver + ESP-DSP for FFT
- LEDs: led_strip component (RMT-based) or custom RMT driver
- Storage: NVS for configuration
- Update: OTA with dual partition rollback

**Performance Requirements:**
- LED refresh rate: 60+ FPS
- Audio latency: <20ms (mic to beat detection)
- Sync latency: <20ms (leader to followers)
- Beat detection accuracy: >90%
- Battery life: 4-6 hours (target)

### Synchronization Protocol (ESP-NOW)

**Architecture: Centralized Leader Model**

```
┌─────────────────────────────────────────────────────┐
│             Leader Device (Primary)                 │
├─────────────────────────────────────────────────────┤
│ - I2S Microphone Input                              │
│ - FFT Analysis (ESP-DSP)                            │
│ - Beat Detection Algorithm                          │
│ - Effect Selection Logic                            │
│ - Timestamp Generation (esp_timer)                  │
└───────────────────┬─────────────────────────────────┘
                    │
        ESP-NOW Broadcast (every beat + periodic)
                    │
    ┌───────────────┼───────────────┬────────────────┐
    ▼               ▼               ▼                ▼
┌─────────┐   ┌─────────┐   ┌─────────┐      ┌─────────┐
│Follower │   │Follower │   │Follower │ ...  │Follower │
│Device 1 │   │Device 2 │   │Device 3 │      │Device N │
├─────────┤   ├─────────┤   ├─────────┤      ├─────────┤
│- Receive│   │- Receive│   │- Receive│      │- Receive│
│  sync   │   │  sync   │   │  sync   │      │  sync   │
│- Execute│   │- Execute│   │- Execute│      │- Execute│
│  effects│   │  effects│   │  effects│      │  effects│
└─────────┘   └─────────┘   └─────────┘      └─────────┘
```

**Packet Structure:**

```c
// Protocol v1.0
typedef struct {
    uint32_t magic;              // 0xBEEFCAFE (packet validation)
    uint8_t  version;            // Protocol version
    uint8_t  packet_type;        // BEAT | EFFECT_CHANGE | CONFIG | HEARTBEAT
    uint16_t sequence;           // Packet sequence number (detect drops)

    // Timing
    uint64_t leader_timestamp;   // Leader's esp_timer value (microseconds)
    uint32_t beat_interval_ms;   // Time since last beat (for tempo tracking)

    // Effect state
    uint8_t  effect_id;          // Current effect enum
    uint8_t  color_palette;      // Color scheme
    uint8_t  brightness;         // 0-255

    // Flags
    uint8_t  is_beat;            // 1 if this packet signals a beat
    uint8_t  force_sync;         // 1 to force re-sync all devices

    // Future expansion
    uint8_t  reserved[8];

    uint16_t checksum;           // CRC16 of packet
} __attribute__((packed)) sync_packet_t;
```

**Timing Synchronization:**

ESP-NOW doesn't guarantee delivery time, so we need:

1. **Clock Drift Compensation:**
   - Periodic heartbeat packets from leader
   - Followers adjust local clock offset
   - Algorithm: Simple moving average of (received_time - leader_timestamp)

2. **Beat Prediction:**
   - Track beat intervals (tempo)
   - Followers can predict next beat if packet drops
   - Fallback to last known effect if no sync for >2 seconds

3. **Latency Measurement:**
   - Leader sends ping, follower responds
   - Measure round-trip time, estimate one-way latency
   - Followers play effects (latency/2) before target time

**Leader Election (Future Enhancement):**

For reliability, implement Raft-like leader election:
- Devices periodically broadcast heartbeats
- If leader fails (no heartbeat for 3 seconds), election starts
- Device with lowest MAC address becomes leader
- **Phase 2 feature - not for initial MVP**

### ESP-IDF Migration Plan

See `ESP-IDF-MIGRATION-PLAN.md` for detailed phase-by-phase migration.

**Summary:**
1. Project structure setup
2. I2S audio migration
3. FFT with ESP-DSP
4. LED control with RMT/led_strip
5. ESP-NOW integration
6. OTA implementation
7. Multi-device testing

**Estimated effort:** 2-4 weeks

### Research Areas for Product 1

**1. ESP-NOW Reliability**
- Packet loss rates in crowded 2.4GHz environments
- Maximum device count (ESP-NOW supports up to 20 peers)
- Range testing (indoor/outdoor)
- Interference mitigation strategies

**Resources:**
- ESP-IDF ESP-NOW examples: `examples/wifi/espnow`
- Espressif forums: esp32.com
- Paper: "Performance Analysis of ESP-NOW for IoT Applications"

**2. Time Synchronization**
- NTP for initial sync (if WiFi available)
- Cristian's algorithm for clock synchronization
- Handling clock drift on ESP32 (oscillator accuracy)

**Resources:**
- ESP-IDF SNTP component
- "Time Synchronization in Wireless Sensor Networks" (survey paper)
- Berkeley algorithm vs Cristian's algorithm

**3. Beat Detection Optimization**
- Onset detection algorithms (spectral flux, energy-based)
- Adaptive threshold for different music genres
- Tempo tracking for beat prediction

**Resources:**
- "Onset Detection Revisited" (Dixon 2006)
- librosa onset detection (for comparison)
- ESP-DSP optimization techniques

**4. Power Management**
- LED current draw vs brightness
- ESP32 light sleep between beats
- Battery selection (mAh requirements)
- Charging circuit design (TP4056, MCP73831)

**Resources:**
- ESP32 power consumption guide (Espressif docs)
- FastLED power calculation (for reference)
- LiPo battery safety guidelines

**5. OTA Security**
- HTTPS for firmware downloads
- Signature verification
- Rollback protection
- Secure storage of WiFi credentials

**Resources:**
- ESP-IDF OTA examples: `examples/system/ota`
- "Secure Boot V2" documentation
- "Flash Encryption" guide

---

## Product 2: Professional Lighting Platform

### System Architecture

```
┌────────────────────────────────────────────────────────────┐
│                     Desktop Application                    │
│              (Rust + Tauri or egui + wgpu)                │
├────────────────────────────────────────────────────────────┤
│  ┌─────────────────┐  ┌──────────────┐  ┌──────────────┐ │
│  │  Timeline       │  │  3D Preview  │  │  Device      │ │
│  │  Editor         │  │  Renderer    │  │  Manager     │ │
│  │                 │  │  (wgpu)      │  │              │ │
│  └────────┬────────┘  └──────┬───────┘  └──────┬───────┘ │
│           │                  │                  │         │
└───────────┼──────────────────┼──────────────────┼─────────┘
            │                  │                  │
            └──────────────────┴──────────────────┘
                               │
                    WebSocket / IPC (Tauri)
                               │
┌──────────────────────────────▼─────────────────────────────┐
│                     Sync Server (Rust)                     │
├────────────────────────────────────────────────────────────┤
│  ┌──────────────┐  ┌─────────────┐  ┌──────────────────┐ │
│  │  Protocol    │  │  Timing     │  │  DJ Integration  │ │
│  │  Handler     │  │  Engine     │  │  (OSC/MIDI/API)  │ │
│  └──────┬───────┘  └──────┬──────┘  └────────┬─────────┘ │
│         │                 │                   │           │
│  ┌──────▼─────────────────▼───────────────────▼─────────┐ │
│  │         Sequence Scheduler & Renderer               │ │
│  │  - Timeline playback                                 │ │
│  │  - Latency compensation                             │ │
│  │  - Device state management                          │ │
│  └──────────────────────────┬───────────────────────────┘ │
└─────────────────────────────┼─────────────────────────────┘
                              │
        ┌─────────────────────┼─────────────────────┐
        │                     │                     │
    UDP/ESP-NOW          WebSocket            DMX (future)
        │                     │                     │
┌───────▼──────┐      ┌───────▼──────┐      ┌──────▼──────┐
│ ESP32 Devices│      │ Web Clients  │      │ DMX Devices │
│ (Wearables)  │      │ (Monitoring) │      │ (Stage)     │
└──────────────┘      └──────────────┘      └─────────────┘

           ┌────────────────────────────────────┐
           │  Music Analysis Pipeline (Python)  │
           ├────────────────────────────────────┤
           │  - librosa (feature extraction)    │
           │  - essentia (beat tracking)        │
           │  - aubio (onset detection)         │
           │  - Export: JSON timeline           │
           └────────────────────────────────────┘
```

### Technology Stack

**Frontend (Desktop GUI):**
- **Option A: Tauri** (Recommended)
  - Frontend: React/Svelte/Vue (your choice)
  - Backend: Rust
  - Cross-platform: Windows, Linux, macOS
  - Small bundle size (~3MB)
  - Native performance

- **Option B: egui** (Pure Rust)
  - Immediate-mode GUI (like ImGui)
  - Full Rust stack
  - Faster development for technical UI
  - Less polished than web-based

**Recommendation: Tauri** for better UI/UX, easier to hire frontend help later

**3D Renderer:**
- **wgpu** (Rust graphics API)
  - Cross-platform (Vulkan, Metal, DX12, WebGPU)
  - Used by Bevy game engine
  - Can render LED positions in 3D space
  - Simulate lighting effects in real-time

**Backend (Sync Server):**
- **Language:** Rust
- **Framework:** Tokio (async runtime)
- **Networking:**
  - `tokio::net::UdpSocket` for ESP32 communication
  - `tungstenite` or `tokio-tungstenite` for WebSocket (GUI communication)
- **Serialization:** `serde` + `bincode` (compact binary) or MessagePack
- **Timing:** `tokio::time` for precise scheduling

**Music Analysis:**
- **Language:** Python 3.10+
- **Libraries:**
  - `librosa` - Feature extraction, tempo, beats
  - `essentia` - Advanced music analysis
  - `aubio` - Real-time onset detection
  - `numpy` - Numerical operations
  - `scipy` - Signal processing
- **Output:** JSON or MessagePack timeline exported to Rust server

**ESP32 Firmware (Reuse from Product 1):**
- ESP-IDF (C)
- Extended protocol to receive pre-programmed sequences
- Dual mode: autonomous (Product 1) or controlled (Product 2)

### Key Features Detail

**1. Timeline Editor**

```
Time:  0s        5s        10s       15s       20s
       ├─────────┼─────────┼─────────┼─────────┼─────
Track 1: [  Wave Effect   ]
Track 2:         [Strobe]
Track 3:                   [  Rainbow Fade   ]

Devices: All | Group A | Group B | Individual
Colors:  [Palette selector]
Intensity: [Curve editor]
Triggers: Beat | Time | Manual
```

**Features:**
- Multi-track timeline (like a DAW)
- Effect assignment per time range
- Device grouping (zones)
- Intensity/color curves
- Beat-locked effects
- Copy/paste/loop sections

**Implementation:**
- Rust backend stores timeline as vector of events
- GUI renders timeline (similar to Audacity/Reaper)
- Real-time playback with audio sync

**2. 3D Preview**

**Scene Setup:**
- Import 3D space dimensions
- Place virtual LED devices in 3D
- Camera controls (orbit, pan, zoom)
- Playback animation synchronized with timeline

**Rendering:**
- Each LED is a point light source
- Simulate bloom/glow
- Record preview as video for marketing

**Libraries:**
- `wgpu` for rendering
- `glam` for 3D math
- Simple forward renderer (not PBR, just colored points)

**3. Music Analysis Integration**

**Workflow:**
1. User imports audio file (MP3, WAV, FLAC)
2. Python script analyzes:
   - Beat positions (frame-accurate)
   - Tempo changes
   - Spectral features (brightness, timbre)
   - Onsets (transients)
3. Exports timeline markers
4. User assigns effects to beat ranges
5. Saves project

**Python Script Example:**
```python
import librosa
import json

y, sr = librosa.load('track.mp3')
tempo, beats = librosa.beat.beat_track(y=y, sr=sr)
onset_frames = librosa.onset.onset_detect(y=y, sr=sr)

timeline = {
    'tempo': tempo,
    'beats': librosa.frames_to_time(beats, sr=sr).tolist(),
    'onsets': librosa.frames_to_time(onset_frames, sr=sr).tolist(),
}

with open('analysis.json', 'w') as f:
    json.dump(timeline, f)
```

**4. MIXX Integration**

**MIXX (Open-Source DJ Software):**
- Supports OSC (Open Sound Control) protocol
- Can send beat/track info in real-time
- Written in C++/Qt (could potentially patch directly)

**Integration Options:**

**Option A: OSC Protocol (Easier)**
- MIXX sends OSC messages: `/mixx/beat`, `/mixx/bpm`, `/mixx/track_position`
- Rust receives via `rosc` crate
- Sync server triggers lighting on beats

**Option B: Shared Memory (Lower latency)**
- MIXX writes state to shared memory
- Rust reads via `shared_memory` crate
- <1ms latency

**Option C: Direct Integration (Fork MIXX)**
- Add Synesthetix plugin to MIXX source
- Direct API calls
- Most control, most maintenance

**Recommendation: Start with OSC**, contribute to MIXX to add better OSC support if needed

**5. Latency Compensation**

**Problem:** Network latency between server and devices varies

**Solution:**
1. **Measure latency:**
   - Server sends ping with timestamp
   - Device echoes immediately
   - Server calculates RTT/2 = latency

2. **Compensate:**
   - Store per-device latency
   - Send commands early by (latency - safety_margin)
   - Example: If device has 15ms latency, send commands 20ms early

3. **Pre-programmed sequences:**
   - Send entire sequence with timestamps
   - Device buffers and executes at precise time
   - Uses ESP32 hardware timer (esp_timer)

**Precision achievable:** ±1ms with hardware timers

---

## Research Areas for Product 2

### 1. Real-Time Audio Processing in Rust

**Topics:**
- JACK audio server integration
- Low-latency audio callbacks
- Cross-platform audio (cpal crate)

**Libraries to explore:**
- `cpal` - Cross-platform audio library
- `rodio` - Audio playback
- `dasp` - Digital audio signal processing

**Resources:**
- "Real-Time Audio Programming in Rust" (blog series)
- JACK API documentation
- Rust Audio Discord community

### 2. GUI Development with Tauri

**Topics:**
- Tauri IPC (Inter-Process Communication)
- State management (Rust backend ↔ JS frontend)
- Native system integration
- Custom window decorations

**Resources:**
- Tauri documentation: tauri.app
- Example apps: github.com/tauri-apps/awesome-tauri
- "Building Desktop Apps with Tauri" course

### 3. WebGPU/wgpu Graphics

**Topics:**
- 3D scene rendering
- Point light sources
- Particle systems (for LED glow effects)
- Compute shaders (for effect previews)

**Resources:**
- "Learn Wgpu" tutorial: sotrh.github.io/learn-wgpu
- `three-d` crate (higher-level 3D rendering)
- Bevy engine source code (reference implementation)

### 4. Network Protocol Design

**Topics:**
- Binary protocol design (vs JSON/XML)
- Reliability over UDP (sequence numbers, ACKs)
- Congestion control for multicast
- Time synchronization (PTP protocol)

**Resources:**
- "Designing Data-Intensive Applications" (Martin Kleppmann)
- "The Realm of Racket" (protocol design chapter)
- RFC 5905 (NTP protocol)

### 5. Music Information Retrieval (MIR)

**Topics:**
- Beat tracking algorithms
- Onset detection (spectral flux, HFC)
- Tempo estimation
- Downbeat detection (measure boundaries)
- Harmonic analysis (key, chord progressions)

**Resources:**
- "Fundamentals of Music Processing" (Meinard Müller) - textbook
- ISMIR conference papers: ismir.net
- Librosa tutorials
- Essentia documentation

### 6. OSC Protocol & DJ Software Integration

**Topics:**
- OSC message format
- MIDI vs OSC comparison
- Real-time synchronization with audio
- MIXX plugin development

**Resources:**
- OSC specification: opensoundcontrol.stanford.edu
- MIXX developer docs: github.com/mixxxdj/mixxx/wiki
- TouchOSC (for testing OSC)

### 7. Distributed Systems & Consistency

**Topics:**
- Leader election algorithms (Raft, Paxos)
- Clock synchronization (Lamport timestamps)
- Byzantine fault tolerance (for multiple leaders)

**Resources:**
- "Designing Data-Intensive Applications" chapters 8-9
- "Distributed Systems" (Maarten van Steen)
- Raft paper: raft.github.io

---

## Libraries & Tools

### ESP32 / Embedded (C)

**Core ESP-IDF Components:**
- `esp-idf` (v5.x latest stable)
- `esp_now` - Wireless peer-to-peer
- `esp_dsp` - Optimized DSP library (FFT, filters)
- `led_strip` - WS2812 LED control (RMT-based)
- `esp_ota_ops` - OTA update API
- `nvs_flash` - Non-volatile storage
- `esp_timer` - High-resolution timers

**Development Tools:**
- PlatformIO - Build system (recommended: familiar)
- ESP-IDF native - Alternative if need full control
- `idf.py` - ESP-IDF CLI tool
- `esptool.py` - Flash utility
- OpenOCD - On-chip debugging
- ESP-IDF Monitor - Serial debugging

**Testing:**
- Unity - Unit testing framework (included in ESP-IDF)
- ESP32 QEMU - Emulation for testing (limited)

### Rust Ecosystem

**Core Language:**
- `rustc` 1.70+ (stable)
- `cargo` - Package manager
- `rustfmt` - Code formatting
- `clippy` - Linting

**Async Runtime:**
- `tokio` - Async runtime (de facto standard)
- `async-std` - Alternative (simpler API)

**Networking:**
- `tokio::net` - TCP/UDP async sockets
- `tungstenite` - WebSocket library
- `rosc` - OSC (Open Sound Control) protocol
- `quinn` - QUIC protocol (low-latency alternative to TCP)

**Serialization:**
- `serde` - Serialization framework
- `bincode` - Binary serialization (compact)
- `rmp-serde` - MessagePack (more standard)
- `serde_json` - JSON (human-readable, debugging)

**GUI (Desktop):**

**Option A: Tauri Stack**
- `tauri` - Rust backend for web-based UI
- Frontend: React, Svelte, Vue, or vanilla JS
- `tauri-plugin-*` - Various system integrations

**Option B: Pure Rust GUI**
- `egui` - Immediate-mode GUI (recommended for technical UI)
- `iced` - Retained-mode GUI (Elm-inspired)
- `druid` - Data-oriented GUI

**3D Graphics:**
- `wgpu` - WebGPU API (cross-platform graphics)
- `glam` - Linear algebra (vectors, matrices)
- `three-d` - 3D rendering library (built on wgpu)
- `bevy_render` - Can use just the renderer from Bevy

**Audio:**
- `cpal` - Cross-platform audio I/O
- `rodio` - Audio playback library
- `dasp` - Digital audio signal processing

**Utilities:**
- `clap` - Command-line argument parsing
- `tracing` - Structured logging
- `anyhow` - Error handling
- `thiserror` - Custom error types
- `crossbeam` - Advanced concurrency primitives

### Python Ecosystem (Music Analysis)

**Music Analysis:**
- `librosa` - Music/audio analysis
- `essentia` - Advanced MIR algorithms
- `aubio` - Real-time audio analysis
- `madmom` - Beat/tempo tracking
- `pyAudioAnalysis` - Feature extraction

**Scientific Computing:**
- `numpy` - Numerical arrays
- `scipy` - Signal processing
- `matplotlib` - Visualization (for debugging)
- `pandas` - Data manipulation

**Audio I/O:**
- `soundfile` - Read/write audio files
- `pydub` - Audio manipulation
- `ffmpeg-python` - Format conversion

**Export:**
- `json` - Standard library
- `msgpack` - MessagePack binary format

### Development Tools

**Version Control:**
- `git` - Source control
- `git-lfs` - Large file storage (for audio samples)

**Build/Automation:**
- `just` - Command runner (better Make)
- `cargo-make` - Rust task runner
- `pio` - PlatformIO CLI

**Debugging:**
- `gdb` - GDB debugger (ESP32)
- `lldb` - LLDB (for Rust)
- `valgrind` - Memory leak detection
- `heaptrack` - Heap profiler

**Performance:**
- `perf` - Linux profiler
- `flamegraph` - Visualization
- `cargo-flamegraph` - Rust profiling
- ESP-IDF profiling tools

**Testing:**
- `pytest` - Python testing
- `cargo test` - Rust testing
- `cargo-nextest` - Faster test runner

**Documentation:**
- `mdbook` - Documentation book (Rust)
- `cargo doc` - API documentation
- `sphinx` - Python documentation

---

## Learning Resources

### ESP32 / ESP-IDF

**Official Documentation:**
- ESP-IDF Programming Guide: docs.espressif.com/projects/esp-idf/
- ESP-IDF API Reference
- ESP32 Technical Reference Manual (hardware details)

**Books:**
- "Kolban's Book on ESP32" (free PDF, comprehensive)
- "FreeRTOS for ESP32" (embedded RTOS concepts)

**Video Courses:**
- "ESP32 for IoT" (Udemy - various instructors)
- Espressif YouTube channel (official tutorials)

**Community:**
- ESP32 Forum: esp32.com
- Reddit: r/esp32
- Discord: ESP32 Community

**Example Projects:**
- ESP-IDF examples directory (gold mine)
- GitHub: awesome-esp (curated list)

### Rust

**Official:**
- "The Rust Programming Language" (The Book): doc.rust-lang.org/book/
- Rust by Example: doc.rust-lang.org/rust-by-example/
- Rustlings (interactive exercises): github.com/rust-lang/rustlings

**Books:**
- "Programming Rust" (O'Reilly) - comprehensive
- "Rust in Action" (Manning) - systems programming focus
- "Zero To Production In Rust" (backend development)

**Async Programming:**
- "Asynchronous Programming in Rust" (official book)
- "Tokio Tutorial": tokio.rs/tokio/tutorial

**GUI Development:**
- Tauri documentation: tauri.app/v1/guides/
- "Are We GUI Yet?" (ecosystem overview)

**Graphics:**
- "Learn Wgpu": sotrh.github.io/learn-wgpu/
- "Learn OpenGL" (concepts transfer to wgpu)

**Community:**
- Rust Users Forum: users.rust-lang.org
- r/rust
- Rust Discord
- "This Week in Rust" (newsletter)

### Music Information Retrieval

**Courses:**
- "Audio Signal Processing for Music Applications" (Coursera)
- "Fundamentals of Music Processing" (Stanford online)

**Books:**
- "Fundamentals of Music Processing" (Meinard Müller)
- "Digital Audio Signal Processing" (Udo Zölzer)
- "The Audio Programming Book" (MIT Press)

**Libraries Tutorials:**
- Librosa tutorials: librosa.org/doc/latest/tutorial.html
- Essentia tutorials: essentia.upf.edu/tutorial.html
- "Music Information Retrieval in Python" (Jupyter notebooks)

**Papers:**
- ISMIR proceedings (conference papers)
- "Onset Detection Revisited" (Dixon 2006)
- "Beat Tracking with Particle Filtering" (Hainsworth 2004)

**Community:**
- Music-IR mailing list
- ISMIR community
- r/AudioProgramming

### Distributed Systems

**Books:**
- "Designing Data-Intensive Applications" (Martin Kleppmann) - essential
- "Distributed Systems" (Maarten van Steen)
- "Time, Clocks, and the Ordering of Events" (Lamport paper)

**Courses:**
- MIT 6.824 Distributed Systems (free lectures)
- "Cloud Computing Concepts" (Coursera)

**Specific Topics:**
- Raft consensus: raft.github.io
- Cristian's algorithm (time sync)
- Network Time Protocol (RFC 5905)

### DJ Software / Live Performance

**MIXX:**
- MIXX Manual: mixxx.org/manual/latest/
- Developer Wiki: github.com/mixxxdj/mixxx/wiki
- Source code: github.com/mixxxdj/mixxx

**OSC Protocol:**
- OSC Specification: opensoundcontrol.stanford.edu
- `rosc` documentation (Rust)

**DMX Lighting (Future):**
- DMX512 protocol specification
- Open Lighting Architecture (OLA)
- ArtNet protocol (DMX over Ethernet)

### Power Electronics (Wearables)

**Battery Management:**
- "Battery University" (batteryuniversity.com)
- "LiPo Battery Safety Guide"
- Texas Instruments battery management ICs

**LED Power:**
- "FastLED Power Calculation" guide
- "WS2812B Power Requirements" datasheets
- Voltage drop calculations

---

## Risk Assessment & Mitigation

### Product 1 Risks

| Risk | Likelihood | Impact | Mitigation |
|------|------------|--------|------------|
| **ESP-IDF migration takes longer than expected** | Medium | High | Keep Arduino version in separate branch, can ship with that if needed |
| **ESP-NOW range insufficient in crowded venues** | Medium | High | Test in real-world conditions early, fallback to WiFi mesh if needed |
| **Beat detection accuracy varies by genre** | Medium | Medium | Implement adaptive algorithms, allow manual sensitivity adjustment |
| **Power consumption exceeds battery capacity** | Low | High | Early power profiling, optimize LED usage, light sleep between beats |
| **Synchronization latency too high (>50ms)** | Low | Medium | Benchmark early, optimize protocol, use hardware timers |
| **Device cost too high for market** | Medium | High | Design for manufacturability, bulk component sourcing, modular design |

### Product 2 Risks

| Risk | Likelihood | Impact | Mitigation |
|------|------------|--------|------------|
| **Rust learning curve delays development** | Medium | Medium | Allocate dedicated learning time (Month 3), start with simple projects |
| **3D renderer performance insufficient** | Low | Medium | Use optimized libraries (wgpu), LOD system, degrade gracefully |
| **MIXX integration more complex than expected** | Medium | Low | Start with OSC (standard protocol), community is helpful |
| **Music analysis inaccurate** | Medium | Medium | Use proven libraries (librosa), allow manual correction in GUI |
| **Timeline editor too complex to implement** | Medium | High | Start with simple version, iterate based on user feedback |
| **Market already saturated (commercial DMX systems)** | Low | High | Differentiate on price, ease of use, wearable integration, open ecosystem |
| **Feature creep delays launch** | High | High | Strict MVP definition, track features in backlog, resist scope expansion |

### Technical Debt Risks

| Risk | Mitigation |
|------|------------|
| **Arduino→ESP-IDF migration leaves cruft** | Code review, clean commit history, document decisions |
| **Protocol changes break backward compatibility** | Version protocol, maintain compatibility layer |
| **Rust/C integration becomes messy** | Clear FFI boundaries, use cbindgen for C headers |
| **GUI becomes spaghetti code** | Use state management pattern, separate concerns |

---

## Success Metrics

### Product 1 (Wearable System)

**Technical:**
- ✅ Synchronization latency: <20ms (p95)
- ✅ Beat detection accuracy: >90%
- ✅ LED refresh rate: >60 FPS
- ✅ Battery life: >4 hours continuous
- ✅ Supports: 10+ devices simultaneously
- ✅ OTA update success rate: >99%

**Business:**
- ✅ 3+ devices working in demo
- ✅ 5+ beta testers provide positive feedback
- ✅ 1+ sale or paid demo gig
- ✅ Video demonstration for portfolio

### Product 2 (Lighting Platform)

**Technical:**
- ✅ Timeline editor supports basic operations (add/remove/move effects)
- ✅ 3D preview renders >30 FPS with 100+ LEDs
- ✅ Music analysis extracts beats within 5% accuracy
- ✅ MIXX integration responds to beats <10ms latency
- ✅ Supports 50+ devices simultaneously

**Business:**
- ✅ 1+ DJ tests system live
- ✅ Open-source components gain 100+ GitHub stars
- ✅ Featured on Hacker News or r/rust
- ✅ Clear path to monetization (SaaS, hardware sales, or consulting)

### Resume/Portfolio

- ✅ GitHub repo with clear README and demo videos
- ✅ Blog post series documenting development
- ✅ Can demo live in interviews
- ✅ "Built with Rust" and "Real-time embedded" both featured
- ✅ Shows: DSP, networking, 3D graphics, embedded systems

---

## Next Actions

### Immediate (This Week)

1. **Review this plan** - Adjust timeline/scope as needed
2. **Verify tools installed** - Rust, ESP-IDF, PlatformIO, dependencies
3. **Create project board** - Track tasks (GitHub Projects or Trello)
4. **Start ESP-IDF migration** - Follow Phase 1 of migration plan

### Month 1

- Complete ESP-IDF migration
- Implement basic sync protocol
- Test with 2 devices

### Month 2

- OTA implementation
- Multi-device testing (4+ devices)
- Product 1 demo video

### Month 3

- Learn Rust (The Book + exercises)
- Design Product 2 architecture
- Proof-of-concept: Rust server controls ESP32

### Months 4-8

- Build Product 2 incrementally
- Blog about progress
- Gather feedback from DJ community

---

## Open Questions

- [ ] **Product 1:** Follower devices - microphone or no? (Cost vs redundancy)
- [ ] **Product 1:** Enclosure design - 3D printed, off-the-shelf, or sewable?
- [ ] **Product 1:** Battery choice - 18650, LiPo pouch, or AA with boost?
- [ ] **Product 2:** GUI framework - Tauri or egui? (UX vs dev speed)
- [ ] **Product 2:** Licensing - Open source, freemium SaaS, or proprietary?
- [ ] **Product 2:** DMX support - necessary for v1.0 or future enhancement?
- [ ] **Business:** Target market - B2C (consumers) or B2B (venues/performers)?
- [ ] **Business:** Manufacturing - DIY kits, assembled units, or license design?

---

## Appendix A: Component Costs (Estimated)

### Per Wearable Device

| Component | Quantity | Unit Cost | Total |
|-----------|----------|-----------|-------|
| ESP32 DevKit | 1 | $5-8 | $7 |
| WS2812B LED strip (5m, 60/m) | 0.6m | $15/5m | $2 |
| I2S MEMS Mic (SPH0645) | 1 | $3 | $3 |
| LiPo 3.7V 2000mAh | 1 | $6 | $6 |
| TP4056 Charging Module | 1 | $1 | $1 |
| Voltage Regulator (AMS1117) | 1 | $0.50 | $0.50 |
| Misc (wires, connectors, PCB) | - | - | $3 |
| Enclosure/mounting | - | - | $5 |
| **Total per device** | | | **~$27.50** |

**At scale (100+ units):** ~$20/device
**Retail price target:** $60-100 (2-3x margin)

### Product 2 Infrastructure

| Component | Cost | Notes |
|-----------|------|-------|
| Development PC | $0 | Already owned |
| ESP32 test devices | $50 | 5-10 units for testing |
| Audio interface (optional) | $100 | For low-latency audio testing |
| **Total upfront** | **$150** | Very low startup cost |

---

## Appendix B: Timeline Gantt Chart

```
Month 1: ESP-IDF Migration & Core Sync
Week 1: ████████ Foundation (app_main, logging, GPIO)
Week 2: ████████ I2S + Audio (ESP-DSP FFT)
Week 3: ████████ LED Control (RMT driver)
Week 4: ████████ ESP-NOW Protocol

Month 2: Product 1 Completion
Week 5: ████████ Multi-device testing
Week 6: ████████ OTA implementation
Week 7: ████████ Power optimization
Week 8: ████████ Demo + documentation

Month 3: Rust Learning & Product 2 Design
Week 9:  ████████ Rust basics (The Book)
Week 10: ████████ Async/Tokio
Week 11: ████████ Network programming
Week 12: ████████ Product 2 architecture spec

Month 4-5: Backend Development
Week 13-14: ████████ Sync server (Tokio + UDP)
Week 15-16: ████████ Protocol implementation
Week 17-18: ████████ Python music analysis
Week 19-20: ████████ Integration testing

Month 6-7: Frontend Development
Week 21-22: ████████ Tauri setup + basic UI
Week 23-24: ████████ Timeline editor
Week 25-26: ████████ Device management
Week 27-28: ████████ Settings + polish

Month 8: 3D Preview & Integration
Week 29-30: ████████ 3D renderer (wgpu)
Week 31:    ████████ MIXX/OSC integration
Week 32:    ████████ End-to-end testing

Month 9+: Polish & Launch
████████ Bug fixes, documentation, marketing
```

---

*End of Product Roadmap*

**Last Updated:** 2026-01-25
**Next Review:** After Product 1 MVP completion