# Audio Architecture Analysis - Complete Documentation Index

This directory contains a comprehensive analysis of the Bluetooth A2DP audio receiver architecture and audio processing pipeline. Below is a guide to all documentation files created during this analysis.

## Documentation Files Overview

### 1. EXECUTIVE_SUMMARY.md (14 KB)
**Start here for a high-level overview**

Comprehensive summary covering:
- Project overview and key achievements
- End-to-end audio processing pipeline
- Audio format specifications (44.1kHz, 16-bit stereo)
- Complete buffer architecture details
- Data flow with line numbers
- Performance characteristics (CPU/memory budgets)
- Effect processor insertion point and requirements
- Hardware wiring reference
- Summary statistics table
- Key implementation highlights

**Best for:** Developers getting started, project managers, architecture overview

---

### 2. ARCHITECTURE.md (24 KB)
**Detailed technical deep-dive**

Complete architecture documentation including:
- Overall audio processing pipeline (detailed flow diagram)
- File structure and main audio processing files
- Audio format and buffer specifications
- Data flow details (5 phases from Bluetooth to DAC)
- Buffer management and auto-start logic
- Key interrupt handlers and priorities
- 8 insertion points for audio effects (with pros/cons)
- Proposed effect module location and requirements
- Performance metrics and constraints analysis
- Latency budget breakdown
- Summary table of key metrics

**Best for:** DSP engineers, system architects, detailed implementation planning

---

### 3. QUICK_REFERENCE.md (8.3 KB)
**Quick lookup guide for code locations and implementations**

Practical reference including:
- Key code locations with line numbers
- Function call hierarchy with data flow
- Buffer management flow
- Timing analysis (callback intervals, DMA rates)
- Critical sections and re-entrancy concerns
- Effect processor integration checklist
- Hardware I2S configuration details

**Best for:** Developers implementing features, debugging, code navigation

---

### 4. ARCHITECTURE_DIAGRAM.txt (24 KB)
**Visual ASCII architecture diagrams**

Complete visual representation of:
- Hardware layer (CYW43, RP2350, PCM5102A)
- Application layer (main.c, bt_audio.c, audio_out_i2s.c, i2s.pio)
- Data flow timing and buffer management
- Interrupt priority and timing details
- Effect processor insertion point with detailed requirements

**Best for:** Visual learners, presentations, system understanding

---

### 5. FILE_REFERENCE.txt (11 KB)
**Complete file listing with absolute paths and line references**

Includes:
- Core source files with function locations
- Configuration files and their critical values
- Hardware interface files
- Build configuration
- Key function call hierarchy
- Buffer data flow with exact sizes
- Critical configuration values with line numbers
- Performance metrics calculations
- Effect processor insertion instructions

**Best for:** Code navigation, CI/CD pipelines, build system configuration

---

## Key Project Statistics

| Aspect | Value |
|--------|-------|
| **Repository** | pico2w-bt-a2dp-audio-receiver-efx-g |
| **Main Language** | C (embedded systems) |
| **Platform** | Raspberry Pi Pico 2W (RP2350) |
| **Total Source Lines** | ~1,200 LOC across 5 files |
| **Audio Format** | 44.1kHz, 16-bit, stereo |
| **Buffer Size** | 1 second (44,100 samples) |
| **Memory Usage** | ~202 KB / 264 KB SRAM |
| **Available for Effects** | 50-100 KB |
| **CPU Budget per Effect Call** | 500-1000µs |
| **Total System Latency** | 40-70ms |

---

## Audio Processing Pipeline Summary

```
Smartphone (Bluetooth A2DP)
    ↓ (Compressed SBC)
CYW43 Bluetooth Module
    ↓
A2DP Sink (BTstack)
    ↓
SBC Decoder
    ↓
Software Volume Control (85%)
    ↓
★ EFFECT PROCESSOR INSERTION POINT ★
    ↓
PCM Callback (main.c:29)
    ↓
Ring Buffer (44,100 samples)
    ↓
DMA (Ping-pong buffers, 512 samples each)
    ↓
PIO State Machine (I2S timing)
    ↓
PCM5102A DAC
    ↓
Speaker/Headphones
```

---

## Core Files & Responsibilities

### /src/main.c (189 lines)
- Application entry point
- PCM data callback handler
- Buffer status monitoring
- Bluetooth integration

### /src/bt_audio.c (376 lines)
- Bluetooth A2DP profile management
- SBC decoder interface
- Software volume control
- **EFFECT PROCESSOR INSERTION POINT: Line 209**

### /src/bt_audio.h
- Bluetooth module API definitions
- PCM callback type definition
- External interfaces

### /src/audio_out_i2s.c (324 lines)
- Ring buffer implementation (44,100 samples)
- DMA controller and ping-pong buffering
- I2S output management
- Underrun/overrun detection

### /src/audio_out_i2s.h
- I2S module public API
- Buffer status functions

### /src/i2s.pio (101 lines)
- PIO state machine program
- I2S clock generation (BCLK, LRCLK)
- Audio data serialization

### /src/config.h (120 lines)
- All configuration constants
- Sample rate: 44100 Hz
- Buffer sizes
- GPIO pin definitions
- Software volume: 85%

---

## Where to Insert Audio Effects

### Optimal Location
**File:** `/src/bt_audio.c`
**Function:** `handle_pcm_data()` (lines 174-213)
**Insertion Point:** Between lines 203 and 209

### Function Signature
```c
void audio_effect_process(int16_t *samples, uint32_t num_samples, uint8_t channels);
```

### Parameters
- `samples`: Interleaved stereo array [L1, R1, L2, R2, ..., Ln, Rn]
- `num_samples`: Total int16_t count (stereo pairs × 2)
- `channels`: 2 (stereo)

### Execution Context
- Called every 2.9ms
- 128 stereo samples (256 int16_t values) per call
- Budget: 500-1000µs per call
- No dynamic allocation allowed
- Fixed-point math recommended

### Files to Create
- `/src/audio_effect.h` (function declarations)
- `/src/audio_effect.c` (implementation)

### Files to Modify
- `/src/bt_audio.c` (add #include, call effect function)
- `CMakeLists.txt` (add audio_effect.c to sources)

---

## Performance Characteristics

### CPU Budget
```
Per PCM Callback (every 2.9ms):
├── Bluetooth processing: ~1-2ms
├── SBC decoding: ~1-2ms (in BTstack)
├── Software volume: ~100µs
├── Ring buffer write: ~100µs
└── Available for effects: 500-1000µs
```

### Memory Budget
```
RP2350 SRAM: 264 KB total
├── Ring buffer: 88 KB
├── DMA buffers: 4 KB
├── BTstack + BSS: ~100 KB
├── Stack: ~10 KB
└── Available for effects: 50-100 KB
```

### Feasible Effects
- IIR filters (1-2 pole): Very fast
- Simple EQ (3-band): Feasible
- Peak limiter: Fast
- Soft clipping: Fast
- 16-tap FIR: Tight but possible
- Reverb (< 0.5s tail): Possible

### Not Feasible
- Real-time FFT convolution (too slow)
- Heavy reverb (> 1s tail) (memory limited)
- High-order filters (>32 taps) (CPU limited)

---

## Key Configuration Values

All in `/src/config.h`:

```c
#define AUDIO_SAMPLE_RATE           44100       // Hz
#define AUDIO_BITS_PER_SAMPLE       16          // bits
#define AUDIO_CHANNELS              2           // stereo
#define AUDIO_BUFFER_SIZE           44100       // samples (1 second)
#define DMA_BUFFER_SIZE             512         // samples
#define BUFFER_LOW_THRESHOLD        11025       // 25%
#define BUFFER_HIGH_THRESHOLD       33075       // 75%
#define AUTO_START_THRESHOLD        8820        // 20%
#define SOFTWARE_VOLUME_PERCENT     85          // %
#define DMA_IRQ_PRIORITY            0xFF        // Lowest (protect Bluetooth)

// GPIO Pins
#define I2S_DATA_PIN                26
#define I2S_BCLK_PIN                27
#define I2S_LRCLK_PIN               28

// Timing
#define BUFFER_STATUS_LOG_INTERVAL_MS  5000     // 5 seconds
#define STATS_LOG_FREQUENCY            100      // Every 100 callbacks
```

---

## Callback Rates & Timing

### PCM Callback
- **Rate:** 344 times per second
- **Interval:** Every 2.9ms
- **Samples per call:** 128 stereo pairs (256 int16_t values)
- **Total time per second:** 344 × 128 = 44,032 samples

### DMA Interrupt
- **Rate:** 86 times per second
- **Interval:** Every 11.6ms
- **Samples per interrupt:** 512 samples
- **Total time per second:** 86 × 512 = 44,032 samples

### Ring Buffer Latency
- **Capacity:** 44,100 samples (1 second at 44.1kHz)
- **Auto-start threshold:** 8,820 samples (20%, 200ms)
- **Safe operating range:** 11,025 - 33,075 samples (25-75%)
- **Typical operation:** 50% fill = 500ms buffered

---

## Monitoring & Debugging

### Console Output
The application logs real-time status to USB serial:
```
[I2S] Buffer: 22050/44100 samples | Free: 22050 | Underruns: 0 | Overruns: 0
[PCM Stats] Callbacks: 100, Total samples: 12800, Dropped: 0
[I2S] Auto-starting DMA (buffer: 8820/44100, 20.0%)
```

### Key Metrics
- **Underruns:** Should be 0 (indicates Bluetooth/effect too slow)
- **Overruns:** Should be 0 (indicates input overflow)
- **Buffered samples:** Should stay 25-75% for healthy operation
- **Dropped samples:** Should be 0 (audio loss indicator)

### Monitoring Functions
```c
uint32_t buffered = audio_out_i2s_get_buffered_samples();
uint32_t free = audio_out_i2s_get_free_space();
audio_out_i2s_get_stats(&underruns, &overruns);
bool connected = bt_audio_is_connected();
uint32_t rate = bt_audio_get_sample_rate();
```

---

## Hardware Wiring Summary

### I2S DAC Connection (PCM5102A)
```
Pico 2W Pin    →    PCM5102A Pin
GPIO 26 (DATA) →    DIN
GPIO 27 (BCLK) →    BCK
GPIO 28 (LRCLK)→    LCK
3.3V (Pin 36)  →    VCC/VIN
GND (Pin 38)   →    GND
```

### PCM5102A Configuration
```
XSMT  → 3.3V    (Unmute)
FLT   → GND     (Normal filter)
DEMP  → GND     (No de-emphasis)
FMT   → GND     (I2S format)
SCK   → GND     (CRITICAL: Clock source config)
```

---

## Quick Start for Effect Implementation

1. **Understand the pipeline:** Read EXECUTIVE_SUMMARY.md
2. **Review architecture:** Read ARCHITECTURE.md
3. **Find the insertion point:** See QUICK_REFERENCE.md and FILE_REFERENCE.txt
4. **Implement effect module:**
   - Create src/audio_effect.h (function declarations)
   - Create src/audio_effect.c (implementation)
   - Update src/bt_audio.c (add #include and call)
   - Update CMakeLists.txt (add source file)

5. **Test carefully:**
   - Monitor console output for underruns
   - Verify buffer levels stay 25-75%
   - Check execution time with oscilloscope/profiler

---

## Document Cross-References

| Question | Answer Document | Section |
|----------|-----------------|---------|
| How does audio flow through the system? | EXECUTIVE_SUMMARY | Data Flow Details |
| Where are the audio buffers? | ARCHITECTURE | Buffer Architecture |
| How do I add an effect? | QUICK_REFERENCE | Effect Processor Integration |
| What are the CPU constraints? | ARCHITECTURE | Performance Metrics |
| Where is a specific function? | FILE_REFERENCE | Key Code Locations |
| What is the buffer latency? | ARCHITECTURE | Latency Budget |
| How fast is the callback? | QUICK_REFERENCE | Timing Analysis |
| What are the file paths? | FILE_REFERENCE | Complete File Reference |
| Show me a diagram | ARCHITECTURE_DIAGRAM | Full ASCII diagrams |
| What is the config format? | FILE_REFERENCE | Critical Values |

---

## Technical Resources

### Related Documentation in Repository
- `README.md` - Project overview and setup
- `WIRING.md` - Hardware connection details
- `src/config.h` - Configuration constants
- `CMakeLists.txt` - Build configuration

### Key Libraries Used
- **BTstack** - Bluetooth A2DP implementation
- **Pico SDK** - Raspberry Pi SDK
- **PIO** - Programmable I/O for I2S timing
- **DMA** - Direct Memory Access for audio streaming

### Hardware References
- **RP2350** - Raspberry Pi Pico 2W MCU
- **CYW43** - Bluetooth/WiFi module
- **PCM5102A** - I2S DAC module
- **I2S** - Inter-IC Sound protocol

---

## Summary

This analysis documents a **production-quality Bluetooth audio receiver** with:

✓ Clean layer separation (Bluetooth, Buffering, I2S)
✓ Professional buffer management
✓ Comprehensive error detection
✓ Clear effect processor insertion point
✓ Adequate CPU and memory budgets
✓ Detailed documentation and code references
✓ Real-time debugging capabilities

The architecture is **ready for audio DSP effect processor integration** with full documentation of insertion points, constraints, and integration requirements.

---

**Generated:** November 20, 2025
**Analysis Scope:** Complete audio processing pipeline architecture
**Documentation Files:** 5 comprehensive guides + this index
**Total Documentation:** ~95 KB of detailed technical analysis
