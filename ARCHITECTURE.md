# Pico 2W Bluetooth A2DP Audio Receiver - Architecture Analysis

## 1. OVERALL AUDIO PROCESSING PIPELINE

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                    AUDIO DATA FLOW (Bluetooth → DAC Output)                 │
└─────────────────────────────────────────────────────────────────────────────┘

  Smartphone (iPhone/Android)
         ↓
    A2DP Bluetooth Stream (SBC Compressed)
         ↓
┌────────────────────────────────────────────────────┐
│  CYW43 Bluetooth Module (Hardware)                 │
│  - Receives A2DP data packets                      │
└────────────────────────────────────────────────────┘
         ↓
┌────────────────────────────────────────────────────┐
│  BTstack A2DP Sink Layer (bt_audio.c)              │
│  - Packet handler: a2dp_sink_media_packet_handler()│
│  - Extracts SBC frames from A2DP packets           │
│  - Strips 13-byte header (SBC_MEDIA_PACKET_HEADER) │
└────────────────────────────────────────────────────┘
         ↓
┌────────────────────────────────────────────────────┐
│  SBC Decoder (BTstack)                             │
│  - btstack_sbc_decoder_process_data()              │
│  - Decodes SBC → PCM samples                       │
│  - Callback: handle_pcm_data()                     │
└────────────────────────────────────────────────────┘
         ↓
┌────────────────────────────────────────────────────┐
│  Software Volume Control (bt_audio.c)              │
│  - Scale samples by SOFTWARE_VOLUME_PERCENT        │
│  - Prevents audio clipping                         │
│  - Applied at line 191-203 in bt_audio.c           │
└────────────────────────────────────────────────────┘
         ↓ [PCM 16-bit stereo samples]
┌────────────────────────────────────────────────────┐
│  PCM DATA CALLBACK (main.c)                        │
│  - pcm_data_handler() in main.c                    │
│  - Receives: int16_t *pcm_data array (interleaved)│
│  - Samples: num_samples (stereo pairs)             │
└────────────────────────────────────────────────────┘
         ↓
┌────────────────────────────────────────────────────┐
│  I2S AUDIO OUTPUT (audio_out_i2s.c)                │
│  ┌──────────────────────────────────────────────┐  │
│  │ Ring Buffer (44100 samples = 1 second)       │  │
│  │ write_pos → [L1,R1,L2,R2,...] ← read_pos     │  │
│  └──────────────────────────────────────────────┘  │
│                      ↓                             │
│  ┌──────────────────────────────────────────────┐  │
│  │ DMA (Direct Memory Access)                   │  │
│  │ Ping-pong buffers (2 × 512 samples each)     │  │
│  │ Continuously fills while hardware transfers  │  │
│  └──────────────────────────────────────────────┘  │
│                      ↓                             │
│  ┌──────────────────────────────────────────────┐  │
│  │ PIO State Machine (Programmable I/O)         │  │
│  │ Generates I2S timing signals                 │  │
│  │ - DATA (GPIO 26): Audio samples              │  │
│  │ - BCLK (GPIO 27): Bit clock                  │  │
│  │ - LRCLK (GPIO 28): Channel select            │  │
│  └──────────────────────────────────────────────┘  │
└────────────────────────────────────────────────────┘
         ↓
┌────────────────────────────────────────────────────┐
│  I2S DAC Module (PCM5102A)                         │
│  - External hardware                              │
│  - Converts digital audio to analog                │
└────────────────────────────────────────────────────┘
         ↓
      🔊 Audio Output (Speaker/Headphones)
```

---

## 2. FILE STRUCTURE & MAIN AUDIO PROCESSING FILES

```
pico2w-bt-a2dp-audio-receiver-efx-g/
├── src/
│   ├── main.c                      ★ APPLICATION ENTRY POINT
│   │   └── PCM callback handling & buffer monitoring
│   │
│   ├── bt_audio.c / bt_audio.h     ★ BLUETOOTH LAYER
│   │   ├── A2DP Sink profile management
│   │   ├── SBC decoder interface (BTstack)
│   │   ├── Software volume control
│   │   └── PCM callback interface
│   │
│   ├── audio_out_i2s.c / audio_out_i2s.h  ★ I2S OUTPUT LAYER
│   │   ├── Ring buffer (1 second capacity)
│   │   ├── DMA configuration & management
│   │   ├── PIO state machine control
│   │   ├── Buffer fill level monitoring
│   │   └── Underrun/Overrun detection
│   │
│   ├── i2s.pio                     ★ PIO PROGRAM (Hardware timing)
│   │   └── Generates I2S clock signals
│   │       BCLK (GPIO 27) & LRCLK (GPIO 28)
│   │
│   ├── config.h                    ★ CONFIGURATION
│   │   ├── Sample rate: 44100 Hz
│   │   ├── Bits per sample: 16
│   │   ├── Channels: 2 (stereo)
│   │   ├── Buffer sizes
│   │   ├── Pin definitions
│   │   └── Debug settings
│   │
│   └── btstack_config.h            (BTstack configuration)
│
├── boards/
│   └── pico2_w.h                   (Board-specific definitions)
│
├── CMakeLists.txt                  (Build configuration)
├── README.md                       (Project documentation)
└── WIRING.md                       (Hardware wiring guide)
```

---

## 3. AUDIO FORMAT & BUFFER SPECIFICATIONS

### Audio Format Parameters

| Parameter | Value | Notes |
|-----------|-------|-------|
| **Sample Rate** | 44,100 Hz | Standard CD quality; no rate conversion |
| **Bit Depth** | 16 bits (signed int16_t) | Standard PCM format |
| **Channels** | 2 (Stereo) | Interleaved: [L1, R1, L2, R2, ...] |
| **Channel Format** | Interleaved stereo | Both channels in same array |
| **Codec** | SBC (Bluetooth Standard) | Decoded by BTstack library |

### Buffer Architecture

```
Ring Buffer (audio_out_i2s.c):
├── Size: 44,100 samples (1 second @ 44.1kHz)
├── Storage: int16_t ring_buffer[88,200 bytes] (stereo pairs)
├── Write Position: write_pos (updated by PCM callback)
├── Read Position: read_pos (updated by DMA handler)
├── Buffered Count: buffered_samples (stereo pairs)
└── Free Space: calculated dynamically

DMA Ping-Pong Buffers:
├── Buffer[0]: int32_t[512] = 512 stereo samples
├── Buffer[1]: int32_t[512] = 512 stereo samples
├── Format: 32-bit words (Upper 16-bit=Left, Lower 16-bit=Right)
└── Interval: ~11.6ms @ 44.1kHz per buffer

Auto-Start Threshold:
├── Trigger: 20% buffer fill (8,820 samples)
├── Purpose: Prevent immediate underruns
└── Location: audio_out_i2s.c, line 172
```

### Buffer Size Constants (from config.h)

```c
#define AUDIO_SAMPLE_RATE       44100       // Hz
#define AUDIO_BITS_PER_SAMPLE   16          // bits
#define AUDIO_CHANNELS          2           // stereo
#define AUDIO_BUFFER_SIZE       44100       // samples (1 second)
#define DMA_BUFFER_SIZE         512         // samples (~11.6ms)

// Threshold levels
#define BUFFER_LOW_THRESHOLD    (AUDIO_BUFFER_SIZE / 4)      // 25%
#define BUFFER_HIGH_THRESHOLD   (AUDIO_BUFFER_SIZE * 3 / 4)  // 75%

// Software volume
#define SOFTWARE_VOLUME_PERCENT 85          // 85% to prevent clipping
```

---

## 4. DATA FLOW DETAILS

### Phase 1: Bluetooth Reception → SBC Decoding

**Entry Point:** `a2dp_sink_media_packet_handler()` in bt_audio.c (line 342)

```c
// Receives compressed A2DP packet:
static void a2dp_sink_media_packet_handler(uint8_t seid, uint8_t *packet, uint16_t size)
  ↓
// Skip 13-byte RTP header
btstack_sbc_decoder_process_data(&sbc_decoder_state, 0,
                                  packet + 13,                // ← Skip header
                                  size - 13)                   // ← SBC data only
  ↓
// SBC Decoder calls handle_pcm_data()
static void handle_pcm_data(int16_t *data, int num_samples, 
                           int num_channels, int sample_rate, void *context)
  ↓
// Apply software volume scaling (lines 191-203)
for (int i = 0; i < total_samples; i++) {
    int32_t sample = data[i];
    sample = (sample * SOFTWARE_VOLUME_PERCENT) / 100;
    data[i] = (int16_t)sample;
}
```

### Phase 2: PCM Callback → Ring Buffer

**Entry Point:** `pcm_data_handler()` in main.c (line 29)

```c
void pcm_data_handler(const int16_t *pcm_data, uint32_t num_samples,
                      uint8_t channels, uint32_t sample_rate)
  ↓
// Call I2S write function
uint32_t written = audio_out_i2s_write(pcm_data, num_samples);
  ↓
// Check for buffer overflow
if (written < num_samples) {
    uint32_t dropped = num_samples - written;  // Audio loss!
}
```

### Phase 3: Ring Buffer → DMA Buffers

**Entry Point:** `fill_dma_buffer()` in audio_out_i2s.c (line 280)

```c
static void fill_dma_buffer(int32_t *buffer, uint32_t num_samples)
  ↓
for (uint32_t i = 0; i < num_samples; i++) {
    if (buffered_samples > 0) {
        // Read stereo pair from ring buffer
        int16_t left = ring_buffer[read_pos * 2];      // L sample
        int16_t right = ring_buffer[read_pos * 2 + 1]; // R sample
        
        // Pack into 32-bit word: [Left (16-bit) | Right (16-bit)]
        buffer[i] = ((uint32_t)(uint16_t)left << 16) | (uint16_t)right;
        
        read_pos = (read_pos + 1) % (I2S_BUFFER_SIZE / 2);
        buffered_samples--;
    } else {
        buffer[i] = 0;  // Silence (underrun)
        underrun_count++;
    }
}
```

### Phase 4: DMA → I2S Hardware

**Entry Point:** `dma_handler()` in audio_out_i2s.c (line 304)

```c
// DMA interrupt fires every 512 samples (~11.6ms)
static void dma_handler(void)
  ↓
// Switch to next DMA buffer (pre-filled)
dma_channel_set_read_addr(dma_channel, dma_buffer[next_buffer], true);
  ↓
// Re-fill finished buffer for next cycle
fill_dma_buffer(dma_buffer[finished_buffer], I2S_DMA_BUFFER_SIZE);
  ↓
// Hardware DMA continuously transfers to PIO TX FIFO
// PIO SM generates I2S signals in real-time
```

### Phase 5: PIO → I2S Signals

**Hardware:** i2s.pio State Machine

```
PIO outputs 32-bit samples to I2S DAC at precisely timed intervals:
- DATA (GPIO 26): Serial audio data bits
- BCLK (GPIO 27): Bit clock (64 × sample rate = 2.82 MHz @ 44.1kHz)
- LRCLK (GPIO 28): Left/Right channel select (44.1 kHz)

Timing (66 cycles per stereo sample):
- Left channel:  33 cycles (left LRCLK=0)
- Right channel: 33 cycles (right LRCLK=1)
```

---

## 5. DATA SAMPLE JOURNEY

```
Example: 128 stereo samples from Bluetooth
─────────────────────────────────────────────

1. SBC Decoder Output:
   array[256] = {L1, R1, L2, R2, ..., L128, R128}  // 128 stereo pairs
   
2. Software Volume (85%):
   array[i] = (array[i] * 85) / 100  // For all 256 int16 values

3. PCM Callback:
   pcm_data_handler(array, 128, 2, 44100)
   // Note: num_samples=128 (stereo pairs), not 256

4. Ring Buffer Write:
   for i in 0..127:
       ring_buffer[write_pos*2] = array[i*2]       // Left
       ring_buffer[write_pos*2+1] = array[i*2+1]   // Right
       write_pos = (write_pos+1) % 44100

5. DMA Fill (every 512 samples):
   for i in 0..511:
       left = ring_buffer[read_pos*2]
       right = ring_buffer[read_pos*2+1]
       dma_buffer[i] = (left << 16) | right  // 32-bit word

6. DMA Transfer → I2S:
   dma_channel transfers 512 × 32-bit words to PIO TX FIFO
   
7. PIO → I2S Output:
   PIO SM serializes bits with proper BCLK/LRCLK timing
   
8. PCM5102A DAC:
   Converts digital I2S → analog audio signals
```

---

## 6. BUFFER MANAGEMENT & AUTO-START

### Buffer Fill Level Monitoring

```
Main Loop (main.c line 176-181):
├── Calls log_buffer_status() every 5 seconds
├── Checks: buffered_samples count
├── Reports buffer level as percentage
└── Warns if Level < 25% or > 75%

Auto-Start Mechanism:
├── Trigger: buffered_samples >= 8,820 (20%)
├── Purpose: Wait for sufficient data before starting playback
├── Prevents immediate underruns on stream start
├── Code: audio_out_i2s.c, line 173
└── Message: "[I2S] Auto-starting DMA (buffer: X/44100, Y%)"
```

### Underrun/Overrun Handling

```
Underrun (Buffer Empty):
├── Cause: DMA wants data but ring buffer is empty
├── Effect: Outputs silence (0x0000)
├── Counter: underrun_count++
├── Detection: audio_out_i2s_get_stats()

Overrun (Buffer Full):
├── Cause: PCM callback writes but buffer is full
├── Effect: Samples are dropped (lost audio)
├── Counter: overrun_count++
├── Prevention: Dynamic monitoring & threshold adjustment
```

---

## 7. KEY INTERRUPT HANDLERS

```
CYW43 Bluetooth Interrupt (Hardware):
  └─→ cyw43_arch_poll()
      └─→ async_context_poll()
          └─→ BTstack A2DP packet processing
              └─→ SBC decoder
                  └─→ PCM callback

DMA Interrupt (DMA_IRQ_0):
  └─→ dma_handler() [Priority: 0xFF - Absolute Lowest]
      └─→ Switch ping-pong buffers
          └─→ Re-fill used buffer
              └─→ Continue seamless I2S output

PIO State Machine (Continuous):
  └─→ i2s.pio generates timing signals
      └─→ Pushes samples to PCM5102A DAC
```

---

## 8. WHERE TO INSERT AUDIO EFFECT PROCESSOR

### Current Data Flow Insertion Points

```
┌─────────────────────────────────────────────────────────────────┐
│                    EFFECT INSERTION POINTS                      │
└─────────────────────────────────────────────────────────────────┘

OPTION A: AFTER SBC DECODING (Best)
  ┌─────────────────┐
  │  SBC Decoder    │
  │  handle_pcm_    │
  │  data()         │
  └────────┬────────┘
           ↓
  ╔═════════════════════════╗  ← INSERT HERE: EFFECT PROCESSOR
  ║  🎵 EFFECT MODULE 🎵    ║     (After decoding, before ring buffer)
  ║ - EQ, Reverb, Delay    ║     • Access: Full PCM sample stream
  ║ - Peak limiter         ║     • Frequency: Every 128 samples
  ║ - Stereo effects       ║     • CPU Budget: ~11.6ms per batch
  ║ - Amp simulation       ║     • Latency: ~5.7ms (half DMA buffer)
  ╚═════════════════════════╝
           ↓
  ┌─────────────────┐
  │  Ring Buffer    │
  │  Write          │
  └─────────────────┘


OPTION B: IN RING BUFFER READING
           (fill_dma_buffer)
  ┌─────────────────────┐
  │ Ring Buffer Read    │
  │ fill_dma_buffer()   │
  └────────┬────────────┘
           ↓
  ╔═════════════════════════╗  ← INSERT HERE: Lightweight effects
  ║  🎵 EFFECT PROCESSOR    ║     (Simple, per-buffer processing)
  ║ - Gain/limiting        ║     • Access: 512 samples at a time
  ║ - Compressor           ║     • Frequency: Every ~11.6ms
  ║ - Soft clipping        ║     • CPU Budget: Very tight!
  ║ - Simple EQ            ║     • Latency: ~5.8ms (half DMA buffer)
  ╚═════════════════════════╝
           ↓
  ┌─────────────────┐
  │  DMA Buffer     │
  │  Transfer       │
  └─────────────────┘


OPTION C: BEFORE RING BUFFER (Current Software Volume)
  ┌──────────────────────┐
  │  Software Volume     │
  │  (bt_audio.c:191)    │  ← ALREADY EXISTS HERE for volume
  └──────────────────────┘


RECOMMENDATION: OPTION A (After SBC Decoding)
  ✓ Larger sample batches (128 stereo pairs)
  ✓ More CPU time per batch (~500µs available)
  ✓ Better for complex DSP (filters, convolution)
  ✓ Lower overall interrupt frequency
  ✗ Slightly higher latency (5.7ms estimated)
```

### Proposed Effect Module Location

```c
// FILE: src/audio_effect.h
typedef void (*audio_effect_fn_t)(int16_t *data, uint32_t num_samples, 
                                  uint8_t channels);

bool audio_effect_init(void);
void audio_effect_process(int16_t *data, uint32_t num_samples, 
                         uint8_t channels);
void audio_effect_shutdown(void);

// FILE: src/audio_effect.c
// Implement effect processing here
// Called from bt_audio.c handle_pcm_data() at line 205
// BEFORE pcm_callback() call


// MODIFICATION TO: src/bt_audio.c (line 205-212)
// BEFORE:
if (pcm_callback) {
    pcm_callback(data, (uint32_t)num_samples, (uint8_t)num_channels, 
                (uint32_t)sample_rate);
}

// AFTER:
if (pcm_callback) {
    // Apply audio effects
    audio_effect_process(data, num_samples * num_channels, num_channels);
    
    // Then send to output buffer
    pcm_callback(data, (uint32_t)num_samples, (uint8_t)num_channels, 
                (uint32_t)sample_rate);
}
```

### Effect Processor Requirements

```c
// Sample Signature for Effect Function

void apply_effect(int16_t *samples, uint32_t num_samples, uint8_t channels) {
    // Input:  num_samples = number of stereo PAIRS (each pair = 2 int16_t values)
    //         samples[] has (num_samples * 2) int16_t elements
    //         samples[] = [L1, R1, L2, R2, ..., Ln, Rn]
    //
    // Processing constraints:
    // - Execution time: ~500 µs available per call (11.6ms callback interval)
    // - Memory: ~50-100KB available for effect state
    // - Must NOT allocate in real-time (causes jitter)
    // - Should avoid division/floating-point (use fixed-point math)
    //
    // Output: Modified samples[] in-place (same memory)
}
```

---

## 9. PERFORMANCE METRICS & CONSTRAINTS

### CPU Budget Analysis

```
Main Loop Cycle: ~5.7ms (at 44.1kHz)
├── Bluetooth processing: ~2-3ms
│   ├── CYW43 polling
│   ├── A2DP packet handling
│   └── SBC decoding (128 samples)
├── PCM callback: ~1ms
│   ├── Software volume adjustment
│   └── Ring buffer write
├── DMA interrupt (every 512 samples): ~100-200µs
│   ├── Buffer switch
│   └── DMA refill
└── Available for effects: ~500-1000µs

Per-Sample Processing Budget:
├── 128 stereo samples = 256 int16_t samples
├── Time available: ~1000µs
├── Per-sample: ~4µs
└── Example: 1-tap FIR filter = OK, 16-tap = possible
```

### Memory Constraints

```
RP2350 (Pico 2W) Resources:
├── Total SRAM: 264 KB
├── Current usage:
│   ├── Ring buffer: 88 KB (44100 samples × 2 channels × 2 bytes)
│   ├── DMA buffers: 4 KB (2 × 512 × 4 bytes)
│   ├── BSS/heap: ~50 KB
│   └── Stack: ~10 KB
├── Available for effects: ~50-100 KB
└── Note: No malloc() in real-time path!

Effect State Allocation Strategy:
├── Static/global allocation (at startup)
├── Pre-allocate delay lines, coefficients
├── Use fixed-point math (int16_t/int32_t)
├── Avoid dynamic allocation
└── Example: Reverb with 4 second tail → 44100×4×2×2 = 700KB ✗
             Reverb with 0.5 second tail → 44100×0.5×2×2 = 88KB ✓
```

### Latency Budget

```
Total Playback Latency:
├── Bluetooth reception: ~20-50ms
├── SBC decoding: ~10ms
├── Ring buffer (at 20% threshold): ~5.7ms
├── DMA/PIO processing: ~2ms
└── PCM5102A DAC: ~1ms
└─→ Total: ~40-70ms latency (acceptable for audio)

Effect Processing Latency Budget:
├── Fits within 5.7ms ring buffer margin
├── No additional latency if processed in main line
└── Safe window: Before ring buffer write
```

---

## 10. SUMMARY TABLE

| Aspect | Details |
|--------|---------|
| **Audio Format** | 16-bit, stereo, 44.1 kHz, SBC compressed input |
| **Sample Rate** | 44,100 Hz (no resampling) |
| **Ring Buffer Size** | 44,100 samples (1 second) |
| **DMA Buffers** | 2 × 512 samples (ping-pong) |
| **DMA Interval** | ~11.6ms |
| **Auto-Start Threshold** | 20% (8,820 samples) |
| **Software Volume** | 85% (clipping prevention) |
| **PCM Callback Rate** | ~86 times/sec (128 samples × 44.1kHz) |
| **DMA Interrupt Rate** | ~86 times/sec (512 samples × 44.1kHz) |
| **Available CPU for Effects** | 500-1000µs per batch |
| **Max Effect Memory** | ~50-100 KB static allocation |
| **Recommended Effect Location** | After SBC decoding (bt_audio.c line 205) |
| **Effect Execution Latency** | ~5.7ms added to playback |

