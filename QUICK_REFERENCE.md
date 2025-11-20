# Quick Reference - Code Locations & Data Flow

## KEY CODE LOCATIONS

### 1. Bluetooth Reception & SBC Decoding
```
File: src/bt_audio.c
───────────────────────────────────────────────────────────

Line  59-131:   bt_audio_init()
                - Initialize Bluetooth A2DP Sink
                - Register SBC decoder
                - Setup event handlers

Line 342-376:   a2dp_sink_media_packet_handler()
                - Receive compressed A2DP media packets
                - Extract SBC frames (skip 13-byte header)
                - Feed to SBC decoder
                - Entry point for audio data!

Line 174-213:   handle_pcm_data()
                - SBC decoder callback
                - Receives decoded PCM samples
                - LOCATION FOR EFFECT PROCESSOR (line 205)
                - Software volume scaling (line 191-203)
                - Call pcm_callback()

Line 166-168:   bt_audio_set_pcm_callback()
                - Register callback from main.c
```

### 2. PCM Callback (Application Layer)
```
File: src/main.c
───────────────────────────────────────────────────────────

Line  29-58:    pcm_data_handler()
                - PCM callback from bt_audio.c
                - Call audio_out_i2s_write()
                - Monitor for dropped samples
                - Log statistics

Line  64-89:    log_buffer_status()
                - Monitor ring buffer levels
                - Report underruns/overruns
                - Called every 5 seconds

Line 137-138:   bt_audio_set_pcm_callback(pcm_data_handler)
                - Register callback in main()
```

### 3. I2S Output & Ring Buffer
```
File: src/audio_out_i2s.c
───────────────────────────────────────────────────────────

Line  36-42:    Ring buffer variables
                int16_t ring_buffer[88200]
                write_pos, read_pos, buffered_samples

Line  69-133:   audio_out_i2s_init()
                - Initialize DMA channel
                - Configure PIO state machine
                - Setup DMA interrupt handler

Line 139-187:   audio_out_i2s_write()
                - Write PCM data to ring buffer
                - Check buffer fill level
                - Auto-start at 20% threshold (line 172)
                - Returns samples written

Line 172:       AUTO_START_THRESHOLD = 8,820 samples (20%)

Line 280-298:   fill_dma_buffer()
                - Read from ring buffer
                - Pack stereo pairs into 32-bit words
                - Output silence on underrun

Line 304-323:   dma_handler()
                - DMA interrupt handler (priority 0xFF)
                - Switch ping-pong buffers
                - Refill finished buffer
                - Called every ~11.6ms
```

### 4. Configuration
```
File: src/config.h
───────────────────────────────────────────────────────────

Line  36:       AUDIO_SAMPLE_RATE = 44100 Hz

Line  39:       AUDIO_BITS_PER_SAMPLE = 16

Line  42:       AUDIO_CHANNELS = 2 (Stereo)

Line  52:       AUDIO_BUFFER_SIZE = 44100 samples (1 second)

Line  55:       DMA_BUFFER_SIZE = 512 samples (~11.6ms)

Line  58-59:    BUFFER_LOW_THRESHOLD = 11025 (25%)
                BUFFER_HIGH_THRESHOLD = 33075 (75%)

Line  97:       SBC_MEDIA_PACKET_HEADER_OFFSET = 13 bytes

Line 117:       SOFTWARE_VOLUME_PERCENT = 85%
```

### 5. I2S Hardware Interface
```
File: src/i2s.pio
───────────────────────────────────────────────────────────

Line 62-99:     i2s_output_program_init()
                - Configure GPIO pins
                - Set up clocking
                - Initialize PIO state machine

Timing Details:
├── 66 cycles per stereo sample
├── BCLK = 64 × sample rate = 2.82 MHz @ 44.1kHz
└── LRCLK = sample rate = 44.1 kHz

GPIO Mapping:
├── GPIO 26: DATA (audio samples)
├── GPIO 27: BCLK (bit clock)
└── GPIO 28: LRCLK (left/right select)
```

---

## DATA FLOW WITH LINE NUMBERS

```
Bluetooth Packet Reception (CYW43 Hardware)
           ↓
a2dp_sink_media_packet_handler() [bt_audio.c:342]
           ↓
btstack_sbc_decoder_process_data() [BTstack library]
           ↓
handle_pcm_data() [bt_audio.c:174]
  - Software volume (191-203)
  - ★ EFFECT PROCESSOR INSERTION POINT (before line 209)
           ↓
pcm_data_handler() [main.c:29]
           ↓
audio_out_i2s_write() [audio_out_i2s.c:139]
  - Check auto-start threshold (line 172)
           ↓
Ring Buffer [audio_out_i2s.c:36-41]
           ↓
fill_dma_buffer() [audio_out_i2s.c:280]
  (called from dma_handler)
           ↓
DMA → PIO → I2S DAC → Speaker
```

---

## BUFFER MANAGEMENT FLOW

```
PCM Callback (~86/sec, 128 samples each)
            ↓
write_pos advances in ring buffer (44100 sample capacity)
buffered_samples increases
            ↓
Check: if (buffered_samples >= 8820)  [line 173]
  YES → Call audio_out_i2s_start()
            ↓
DMA Handler (~86/sec, 512 samples each)
            ↓
fill_dma_buffer() reads ring_buffer[read_pos]
read_pos advances
buffered_samples decreases
            ↓
Transfer to I2S DAC via PIO
```

---

## TIMING ANALYSIS

```
PCM Callback Interval:
  44100 samples/sec ÷ 128 samples/callback = 344 callbacks/sec
  = 1 callback every 2.9ms

DMA Interrupt Interval:
  44100 samples/sec ÷ 512 samples/interrupt = 86 interrupts/sec
  = 1 interrupt every ~11.6ms

Ring Buffer Duration:
  44100 samples ÷ 44100 samples/sec = 1 second

DMA Buffer Duration:
  512 samples ÷ 44100 samples/sec = 11.6ms

Auto-Start Delay:
  8820 samples ÷ 44100 samples/sec = 0.2 seconds = 200ms
```

---

## CRITICAL SECTIONS (NO INTERRUPTS)

None explicitly disabled. However:

```
Critical Operations (not re-entrant safe):
├── write_pos/read_pos/buffered_samples updates
├── DMA_IRQ_0 handler (lowest priority: 0xFF)
└── Note: Single-core processor, but async context switching

Bluetooth handler has higher priority implicitly via
BTstack event loop in main loop.
```

---

## EFFECT PROCESSOR INTEGRATION CHECKLIST

To add an audio effect processor:

```
1. Create new files:
   ✓ src/audio_effect.h     (function declarations)
   ✓ src/audio_effect.c     (implementation)

2. Implement functions:
   ✓ audio_effect_init()           [startup initialization]
   ✓ audio_effect_process()        [effect processing]
   ✓ audio_effect_shutdown()       [cleanup]

3. Update bt_audio.c:
   ✓ Add #include "audio_effect.h"
   ✓ Call audio_effect_init() in bt_audio_init()
   ✓ Call audio_effect_process() in handle_pcm_data() 
     AT LINE 209 (before pcm_callback)
   ✓ Call audio_effect_shutdown() on error handling

4. Update CMakeLists.txt:
   ✓ Add src/audio_effect.c to add_executable()

5. Function signature:
   void audio_effect_process(int16_t *data, uint32_t num_samples, 
                            uint8_t channels)
   // num_samples = stereo pairs
   // data = interleaved stereo [L1,R1,L2,R2,...]

6. Memory constraints:
   ✓ Static allocation only (no malloc in real-time)
   ✓ Max 50-100 KB available
   ✓ Execution budget: ~500-1000µs per call

7. CPU budget:
   ✓ PCM callback every 2.9ms with 128 samples
   ✓ Available time: ~4µs per sample
   ✓ Simple IIR filters: OK
   ✓ Complex convolution: Need optimization
```

---

## HARDWARE I2S CONFIGURATION

```
Default GPIO Pins (configurable in config.h):
├── GPIO 26: DATA (output)      [I2S_DATA_PIN]
├── GPIO 27: BCLK (output)      [I2S_BCLK_PIN]  
└── GPIO 28: LRCLK (output)     [I2S_LRCLK_PIN]

DAC Module Expected (PCM5102A):
├── VCC:  3.3V
├── GND:  GND
├── DIN:  GPIO 26 (DATA)
├── BCK:  GPIO 27 (BCLK)
├── LCK:  GPIO 28 (LRCLK)
└── SCK:  GND (hardware configuration)

I2S Signal Characteristics:
├── Bit depth: 16-bit
├── Frame format: Stereo (2 channels)
├── BCLK freq: 2.82 MHz (64 × 44.1kHz)
├── LRCLK freq: 44.1 kHz
└── I2S format: Standard MSB-first
```

