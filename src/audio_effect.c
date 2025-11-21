/**
 * @file audio_effect.c
 * @brief Beat-Repeat Audio Effect Implementation
 *
 * Kammerl Beat-Repeatスタイルのオーディオエフェクト
 * ステレオ入力・ステレオ出力対応
 */

#include "audio_effect.h"
#include "config.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

// ============================================================================
// 定数定義
// ============================================================================

// スライスバッファの最大長（サンプル数）
// 44100Hz * 1秒 = 44100サンプル（ステレオペア）
#define MAX_SLICE_LENGTH  (AUDIO_SAMPLE_RATE * 1)

// パラメータ検証の定数
#define MIN_SLICE_LENGTH       128     // 最小スライス長（約3ms @ 44.1kHz）
#define MIN_REPEAT_COUNT       1       // 最小リピート回数
#define MAX_REPEAT_COUNT       16      // 最大リピート回数
#define MAX_WET_MIX            100     // 最大ウェットミックス（%）
#define MIN_PITCH_SHIFT        0.25f   // 最小ピッチシフト（2オクターブ下）
#define MAX_PITCH_SHIFT        4.0f    // 最大ピッチシフト（2オクターブ上）
#define MIN_STUTTER_LENGTH     64      // 最小スタッタースライス長
#define MIN_WINDOW_SHAPE       0.0f    // 最小ウィンドウシェイプ
#define MAX_WINDOW_SHAPE       1.0f    // 最大ウィンドウシェイプ

// Kammerl Beat-Repeat オリジナル機能の検証定数
#define MIN_LOOP_START         0.0f    // 最小ループ開始位置
#define MAX_LOOP_START         1.0f    // 最大ループ開始位置
#define MIN_LOOP_SIZE_DECAY    0.0f    // 最小ループサイズ減衰
#define MAX_LOOP_SIZE_DECAY    1.0f    // 最大ループサイズ減衰
#define MAX_SLICE_SELECT       1       // 最大スライス選択（0-1）※RAM制限
#define MIN_SLICE_PROBABILITY  0.0f    // 最小スライス確率
#define MAX_SLICE_PROBABILITY  1.0f    // 最大スライス確率
#define MAX_CLOCK_DIVIDER      8       // 最大クロック分周

// オーディオ処理の定数
#define STEREO_CHANNELS        2       // ステレオチャンネル数
#define LEFT_CHANNEL           0       // 左チャンネルオフセット
#define RIGHT_CHANNEL          1       // 右チャンネルオフセット
#define SAMPLE_MAX             32767   // 16ビットPCM最大値
#define SAMPLE_MIN             -32768  // 16ビットPCM最小値

// マルチスライスバッファの数（RP2350のRAM=520KBに制限される）
#define NUM_SLICES             2       // 2個のスライスを保持（RAM制限）

// ============================================================================
// 内部変数
// ============================================================================

// エフェクトパラメータ（現在の設定）
static beat_repeat_params_t current_params;

// スライスバッファ（ステレオインターリーブ形式）
// メモリ使用量: 44100 * 2 * 2 = 176,400 バイト (約172 KB)
static int16_t slice_buffer[MAX_SLICE_LENGTH * 2];  // ステレオなので2倍

// マルチスライスバッファ（2個のスライスを保持、RAM制限）
// メモリ使用量: 44100 * 2 * 2 * 2 = 352,800 バイト (約344 KB)
// 合計メモリ使用量: 約516 KB（RP2350 RAM=520KB以内）
static int16_t multi_slice_buffer[NUM_SLICES][MAX_SLICE_LENGTH * 2];
static uint32_t multi_slice_lengths[NUM_SLICES];  // 各スライスの長さ
static uint8_t current_slice_index = 0;            // 現在書き込み中のスライスインデックス

// スライス状態管理
static uint32_t slice_write_pos = 0;   // 書き込み位置
static float slice_read_pos_f = 0.0f;  // 読み取り位置（ピッチシフト用、float）
static uint32_t repeat_counter = 0;    // 現在のリピートカウント
static bool is_repeating = false;      // リピート中フラグ
static uint32_t pitch_mod_phase = 0;   // ピッチ変調用位相カウンタ

// サンプリングレート
static uint32_t sample_rate = AUDIO_SAMPLE_RATE;

// 初期化フラグ
static bool is_initialized = false;

// ============================================================================
// デフォルトパラメータ（固定値、将来的にロータリーエンコーダで調整）
// ============================================================================

#define DEFAULT_SLICE_LENGTH         (AUDIO_SAMPLE_RATE / 4)  // 0.25秒（16分音符 @ 120 BPM）
#define DEFAULT_REPEAT_COUNT         4                        // 4回リピート
#define DEFAULT_WET_MIX              70                       // 70% ウェット
#define DEFAULT_ENABLED              true                     // エフェクト有効
#define DEFAULT_PITCH_SHIFT          1.0f                     // ピッチシフトなし
#define DEFAULT_REVERSE              false                    // 逆再生OFF
#define DEFAULT_STUTTER_ENABLED      false                    // スタッターOFF
#define DEFAULT_STUTTER_SLICE_LENGTH (AUDIO_SAMPLE_RATE / 100) // 0.01秒（441サンプル）
#define DEFAULT_WINDOW_SHAPE         0.05f                    // 5%フェード

// Kammerl Beat-Repeat オリジナル機能のデフォルト値
#define DEFAULT_LOOP_START           0.0f                     // ループ開始位置（先頭から）
#define DEFAULT_LOOP_SIZE_DECAY      0.0f                     // ループサイズ減衰なし
#define DEFAULT_SLICE_SELECT         0                        // 最新スライスを使用
#define DEFAULT_SLICE_PROBABILITY    1.0f                     // 常に処理（100%）
#define DEFAULT_CLOCK_DIVIDER        1                        // クロック分周なし
#define DEFAULT_PITCH_MODE           PITCH_MODE_FIXED_REVERSE // 固定ピッチ
#define DEFAULT_FREEZE               false                    // フリーズOFF

// ============================================================================
// エフェクト初期化
// ============================================================================

bool audio_effect_init(uint32_t sr) {
    printf("\n========================================\n");
    printf("Audio Effect Module: Beat-Repeat\n");
    printf("========================================\n");

    sample_rate = sr;

    // デフォルトパラメータ設定
    current_params.slice_length = DEFAULT_SLICE_LENGTH;
    current_params.repeat_count = DEFAULT_REPEAT_COUNT;
    current_params.wet_mix = DEFAULT_WET_MIX;
    current_params.enabled = DEFAULT_ENABLED;
    current_params.pitch_shift = DEFAULT_PITCH_SHIFT;
    current_params.reverse = DEFAULT_REVERSE;
    current_params.stutter_enabled = DEFAULT_STUTTER_ENABLED;
    current_params.stutter_slice_length = DEFAULT_STUTTER_SLICE_LENGTH;
    current_params.window_shape = DEFAULT_WINDOW_SHAPE;

    // Kammerl Beat-Repeat オリジナル機能のデフォルト設定
    current_params.loop_start = DEFAULT_LOOP_START;
    current_params.loop_size_decay = DEFAULT_LOOP_SIZE_DECAY;
    current_params.slice_select = DEFAULT_SLICE_SELECT;
    current_params.slice_probability = DEFAULT_SLICE_PROBABILITY;
    current_params.clock_divider = DEFAULT_CLOCK_DIVIDER;
    current_params.pitch_mode = DEFAULT_PITCH_MODE;
    current_params.freeze = DEFAULT_FREEZE;

    // バッファのクリア
    memset(slice_buffer, 0, sizeof(slice_buffer));
    memset(multi_slice_buffer, 0, sizeof(multi_slice_buffer));
    memset(multi_slice_lengths, 0, sizeof(multi_slice_lengths));
    slice_write_pos = 0;
    slice_read_pos_f = 0.0f;
    repeat_counter = 0;
    is_repeating = false;
    current_slice_index = 0;
    pitch_mod_phase = 0;

    is_initialized = true;

    printf("Sample Rate: %lu Hz\n", sample_rate);
    printf("Slice Length: %lu samples (%.2f ms)\n",
           current_params.slice_length,
           (float)current_params.slice_length * 1000.0f / sample_rate);
    printf("Repeat Count: %u\n", current_params.repeat_count);
    printf("Wet Mix: %u%%\n", current_params.wet_mix);
    printf("Pitch Shift: %.2f\n", current_params.pitch_shift);
    printf("Reverse: %s\n", current_params.reverse ? "ON" : "OFF");
    printf("Stutter: %s", current_params.stutter_enabled ? "ON" : "OFF");
    if (current_params.stutter_enabled) {
        printf(" (%lu samples = %.2f ms)\n",
               current_params.stutter_slice_length,
               (float)current_params.stutter_slice_length * 1000.0f / sample_rate);
    } else {
        printf("\n");
    }
    printf("Window Shape: %.2f\n", current_params.window_shape);
    printf("Effect: %s\n", current_params.enabled ? "ENABLED" : "DISABLED");
    printf("Buffer Size: %lu samples (%lu bytes)\n",
           (unsigned long)MAX_SLICE_LENGTH, (unsigned long)sizeof(slice_buffer));
    printf("========================================\n\n");

    return true;
}

// ============================================================================
// パラメータ検証ヘルパー関数
// ============================================================================

/**
 * @brief スライス長を検証して範囲内にクランプ
 */
static inline uint32_t validate_slice_length(uint32_t slice_len) {
    if (slice_len > MAX_SLICE_LENGTH) {
        printf("WARNING: Slice length clamped to %lu\n", (unsigned long)MAX_SLICE_LENGTH);
        return MAX_SLICE_LENGTH;
    }
    if (slice_len < MIN_SLICE_LENGTH) {
        printf("WARNING: Slice length clamped to minimum %d\n", MIN_SLICE_LENGTH);
        return MIN_SLICE_LENGTH;
    }
    return slice_len;
}

/**
 * @brief リピート回数を検証して範囲内にクランプ
 */
static inline uint8_t validate_repeat_count(uint8_t repeat) {
    if (repeat < MIN_REPEAT_COUNT) return MIN_REPEAT_COUNT;
    if (repeat > MAX_REPEAT_COUNT) return MAX_REPEAT_COUNT;
    return repeat;
}

/**
 * @brief ウェットミックスを検証して範囲内にクランプ
 */
static inline uint8_t validate_wet_mix(uint8_t wet) {
    return (wet > MAX_WET_MIX) ? MAX_WET_MIX : wet;
}

/**
 * @brief ピッチシフトを検証して範囲内にクランプ
 */
static inline float validate_pitch_shift(float pitch) {
    if (pitch < MIN_PITCH_SHIFT) return MIN_PITCH_SHIFT;
    if (pitch > MAX_PITCH_SHIFT) return MAX_PITCH_SHIFT;
    return pitch;
}

/**
 * @brief スタッタースライス長を検証して範囲内にクランプ
 */
static inline uint32_t validate_stutter_length(uint32_t stutter_len) {
    if (stutter_len < MIN_STUTTER_LENGTH) return MIN_STUTTER_LENGTH;
    if (stutter_len > MAX_SLICE_LENGTH) return MAX_SLICE_LENGTH;
    return stutter_len;
}

/**
 * @brief ウィンドウシェイプを検証して範囲内にクランプ
 */
static inline float validate_window_shape(float window) {
    if (window < MIN_WINDOW_SHAPE) return MIN_WINDOW_SHAPE;
    if (window > MAX_WINDOW_SHAPE) return MAX_WINDOW_SHAPE;
    return window;
}

/**
 * @brief ループ開始位置を検証して範囲内にクランプ
 */
static inline float validate_loop_start(float loop_start) {
    if (loop_start < MIN_LOOP_START) return MIN_LOOP_START;
    if (loop_start > MAX_LOOP_START) return MAX_LOOP_START;
    return loop_start;
}

/**
 * @brief ループサイズ減衰を検証して範囲内にクランプ
 */
static inline float validate_loop_size_decay(float decay) {
    if (decay < MIN_LOOP_SIZE_DECAY) return MIN_LOOP_SIZE_DECAY;
    if (decay > MAX_LOOP_SIZE_DECAY) return MAX_LOOP_SIZE_DECAY;
    return decay;
}

/**
 * @brief スライス選択を検証して範囲内にクランプ
 */
static inline uint8_t validate_slice_select(uint8_t select) {
    return (select > MAX_SLICE_SELECT) ? MAX_SLICE_SELECT : select;
}

/**
 * @brief スライス確率を検証して範囲内にクランプ
 */
static inline float validate_slice_probability(float prob) {
    if (prob < MIN_SLICE_PROBABILITY) return MIN_SLICE_PROBABILITY;
    if (prob > MAX_SLICE_PROBABILITY) return MAX_SLICE_PROBABILITY;
    return prob;
}

/**
 * @brief クロック分周を検証（1, 2, 4, 8のいずれか）
 */
static inline uint8_t validate_clock_divider(uint8_t divider) {
    if (divider == 1 || divider == 2 || divider == 4 || divider == 8) {
        return divider;
    }
    return 1;  // デフォルトは1
}

/**
 * @brief ピッチモードを検証
 */
static inline pitch_mode_t validate_pitch_mode(pitch_mode_t mode) {
    if (mode >= PITCH_MODE_FIXED_REVERSE && mode <= PITCH_MODE_SCRATCH) {
        return mode;
    }
    return PITCH_MODE_FIXED_REVERSE;  // デフォルト
}

// ============================================================================
// パラメータ設定・取得
// ============================================================================

void audio_effect_set_params(const beat_repeat_params_t *params) {
    if (!params) return;

    // 各パラメータを検証
    current_params.slice_length = validate_slice_length(params->slice_length);
    current_params.repeat_count = validate_repeat_count(params->repeat_count);
    current_params.wet_mix = validate_wet_mix(params->wet_mix);
    current_params.pitch_shift = validate_pitch_shift(params->pitch_shift);
    current_params.stutter_slice_length = validate_stutter_length(params->stutter_slice_length);
    current_params.window_shape = validate_window_shape(params->window_shape);

    // Kammerl Beat-Repeat オリジナル機能のパラメータを検証
    current_params.loop_start = validate_loop_start(params->loop_start);
    current_params.loop_size_decay = validate_loop_size_decay(params->loop_size_decay);
    current_params.slice_select = validate_slice_select(params->slice_select);
    current_params.slice_probability = validate_slice_probability(params->slice_probability);
    current_params.clock_divider = validate_clock_divider(params->clock_divider);
    current_params.pitch_mode = validate_pitch_mode(params->pitch_mode);

    // ブール値はそのまま設定
    current_params.enabled = params->enabled;
    current_params.reverse = params->reverse;
    current_params.stutter_enabled = params->stutter_enabled;
    current_params.freeze = params->freeze;

    printf("Effect params updated: slice=%lu, repeat=%u, wet=%u%%, enabled=%d\n",
           current_params.slice_length, current_params.repeat_count,
           current_params.wet_mix, current_params.enabled);
    printf("  pitch=%.2f, reverse=%d, stutter=%d, window=%.2f\n",
           current_params.pitch_shift, current_params.reverse,
           current_params.stutter_enabled, current_params.window_shape);
    printf("  loop_start=%.2f, loop_decay=%.2f, slice_sel=%u, probability=%.2f\n",
           current_params.loop_start, current_params.loop_size_decay,
           current_params.slice_select, current_params.slice_probability);
    printf("  clock_div=%u, pitch_mode=%d, freeze=%d\n",
           current_params.clock_divider, current_params.pitch_mode, current_params.freeze);
}

void audio_effect_get_params(beat_repeat_params_t *params) {
    if (!params) return;
    *params = current_params;
}

// ============================================================================
// エフェクトリセット
// ============================================================================

void audio_effect_reset(void) {
    memset(slice_buffer, 0, sizeof(slice_buffer));
    memset(multi_slice_buffer, 0, sizeof(multi_slice_buffer));
    memset(multi_slice_lengths, 0, sizeof(multi_slice_lengths));
    slice_write_pos = 0;
    slice_read_pos_f = 0.0f;
    repeat_counter = 0;
    is_repeating = false;
    current_slice_index = 0;
    pitch_mod_phase = 0;
    printf("Effect reset\n");
}

// ============================================================================
// ヘルパー関数：線形補間（ピッチシフト用）
// ============================================================================

static inline int16_t interpolate_sample(float pos, uint32_t max_length, bool is_left) {
    // pos が範囲外の場合は0を返す
    if (pos < 0.0f || pos >= (float)max_length) {
        return 0;
    }

    uint32_t pos_int = (uint32_t)pos;
    float frac = pos - (float)pos_int;

    // チャンネルオフセット（LEFT_CHANNEL = 0, RIGHT_CHANNEL = 1）
    uint32_t ch_offset = is_left ? LEFT_CHANNEL : RIGHT_CHANNEL;

    // 現在のサンプル
    int16_t sample1 = slice_buffer[pos_int * STEREO_CHANNELS + ch_offset];

    // 次のサンプル
    int16_t sample2;
    if (pos_int + 1 < max_length) {
        sample2 = slice_buffer[(pos_int + 1) * STEREO_CHANNELS + ch_offset];
    } else {
        sample2 = sample1;  // バッファ端では同じ値を使う
    }

    // 線形補間
    float result = (float)sample1 * (1.0f - frac) + (float)sample2 * frac;

    // クリッピング
    if (result > (float)SAMPLE_MAX) result = (float)SAMPLE_MAX;
    if (result < (float)SAMPLE_MIN) result = (float)SAMPLE_MIN;

    return (int16_t)result;
}

// ============================================================================
// ヘルパー関数：ウィンドウシェイプ（フェードイン/アウト）
// ============================================================================

static inline float get_window_envelope(uint32_t pos, uint32_t length, float window_shape) {
    if (window_shape <= 0.0f) {
        return 1.0f;  // ウィンドウなし
    }

    // フェード長（サンプル数）
    uint32_t fade_len = (uint32_t)((float)length * window_shape);
    if (fade_len == 0) {
        return 1.0f;
    }

    // フェードイン（開始部分）
    if (pos < fade_len) {
        return (float)pos / (float)fade_len;
    }

    // フェードアウト（終了部分）
    if (pos >= length - fade_len) {
        return (float)(length - pos) / (float)fade_len;
    }

    // 中間部分
    return 1.0f;
}

// ============================================================================
// ヘルパー関数：ドライ/ウェットミックス
// ============================================================================

static inline int16_t mix_samples(int16_t dry, int16_t wet, uint8_t wet_percent) {
    // 32ビット整数で計算してオーバーフロー防止
    int32_t dry_part = (int32_t)dry * (MAX_WET_MIX - wet_percent) / MAX_WET_MIX;
    int32_t wet_part = (int32_t)wet * wet_percent / MAX_WET_MIX;
    int32_t result = dry_part + wet_part;

    // クリッピング
    if (result > SAMPLE_MAX) result = SAMPLE_MAX;
    if (result < SAMPLE_MIN) result = SAMPLE_MIN;

    return (int16_t)result;
}

// ============================================================================
// エフェクト処理ヘルパー関数
// ============================================================================

/**
 * @brief スライスバッファに入力サンプルを書き込み
 */
static inline void write_to_slice_buffer(int16_t input_l, int16_t input_r) {
    uint32_t buf_idx = slice_write_pos * STEREO_CHANNELS;
    slice_buffer[buf_idx + LEFT_CHANNEL] = input_l;
    slice_buffer[buf_idx + RIGHT_CHANNEL] = input_r;
    slice_write_pos++;
}

/**
 * @brief スライスバッファから読み取り位置を計算（逆再生対応）
 */
static inline float calculate_read_position(float slice_read_pos, uint32_t slice_length, bool reverse) {
    if (reverse) {
        return (float)slice_length - 1.0f - slice_read_pos;
    }
    return slice_read_pos;
}

/**
 * @brief ピッチシフトありでスライスバッファから読み取り
 */
static inline void read_slice_with_pitch(int16_t *repeat_l, int16_t *repeat_r,
                                         uint32_t slice_length) {
    float read_pos = calculate_read_position(slice_read_pos_f, slice_length, current_params.reverse);
    *repeat_l = interpolate_sample(read_pos, slice_length, true);
    *repeat_r = interpolate_sample(read_pos, slice_length, false);
    slice_read_pos_f += current_params.pitch_shift;
}

/**
 * @brief ピッチシフトなしでスライスバッファから読み取り
 */
static inline void read_slice_normal(int16_t *repeat_l, int16_t *repeat_r) {
    uint32_t read_idx = (uint32_t)slice_read_pos_f * STEREO_CHANNELS;
    *repeat_l = slice_buffer[read_idx + LEFT_CHANNEL];
    *repeat_r = slice_buffer[read_idx + RIGHT_CHANNEL];
    slice_read_pos_f += 1.0f;
}

/**
 * @brief ウィンドウシェイプを適用
 */
static inline void apply_window_shape(int16_t *sample_l, int16_t *sample_r, uint32_t slice_length) {
    if (current_params.window_shape > 0.0f) {
        float envelope = get_window_envelope((uint32_t)slice_read_pos_f, slice_length,
                                            current_params.window_shape);
        *sample_l = (int16_t)((float)*sample_l * envelope);
        *sample_r = (int16_t)((float)*sample_r * envelope);
    }
}

/**
 * @brief リピート終了判定と状態更新
 */
static inline bool check_repeat_end(uint32_t slice_length) {
    if (slice_read_pos_f >= (float)slice_length) {
        slice_read_pos_f = 0.0f;
        repeat_counter++;

        if (repeat_counter >= current_params.repeat_count) {
            is_repeating = false;
            repeat_counter = 0;
            return true;
        }
    }
    return false;
}

// ============================================================================
// Kammerl Beat-Repeat オリジナル機能ヘルパー
// ============================================================================

/**
 * @brief マルチスライスバッファに現在のスライスをコピー
 */
static inline void save_slice_to_multi_buffer(uint32_t slice_length) {
    // リングバッファとして次のインデックスに保存
    current_slice_index = (current_slice_index + 1) % NUM_SLICES;

    // スライスをコピー
    uint32_t copy_size = slice_length * STEREO_CHANNELS * sizeof(int16_t);
    memcpy(multi_slice_buffer[current_slice_index], slice_buffer, copy_size);
    multi_slice_lengths[current_slice_index] = slice_length;
}

/**
 * @brief 選択されたスライスバッファから読み取り
 * @param slice_idx スライス選択インデックス（0 = 最新、7 = 最古）
 */
static inline int16_t read_from_multi_slice(uint8_t slice_idx, uint32_t pos, bool is_left) {
    // インデックスを逆順に変換（0 = 最新）
    uint8_t actual_idx = (current_slice_index - slice_idx + NUM_SLICES) % NUM_SLICES;

    uint32_t slice_len = multi_slice_lengths[actual_idx];
    if (slice_len == 0 || pos >= slice_len) {
        return 0;  // 無効なスライス
    }

    uint32_t ch_offset = is_left ? LEFT_CHANNEL : RIGHT_CHANNEL;
    return multi_slice_buffer[actual_idx][pos * STEREO_CHANNELS + ch_offset];
}

/**
 * @brief スライス処理確率を判定（0.0-1.0）
 * @return true処理する, false バイパス
 */
static inline bool check_slice_probability(void) {
    if (current_params.slice_probability >= 1.0f) {
        return true;  // 常に処理
    }
    if (current_params.slice_probability <= 0.0f) {
        return false;  // 常にバイパス
    }

    // 簡易乱数生成（0-32767）
    static uint32_t random_state = 12345;
    random_state = (random_state * 1103515245 + 12345) & 0x7FFFFFFF;
    float random_val = (float)(random_state % 1000) / 1000.0f;  // 0.0-1.0

    return (random_val < current_params.slice_probability);
}

/**
 * @brief ピッチモードに応じたピッチ倍率を計算
 * @param pos 現在の読み取り位置
 * @param length スライス長
 * @return ピッチ倍率
 */
static inline float calculate_pitch_for_mode(uint32_t pos, uint32_t length) {
    float normalized_pos = (float)pos / (float)length;  // 0.0-1.0

    switch (current_params.pitch_mode) {
        case PITCH_MODE_DECREASING:
            // 線形減少（1.0 -> 0.5）
            return 1.0f - (normalized_pos * 0.5f);

        case PITCH_MODE_INCREASING:
            // 線形増加（0.5 -> 1.0）
            return 0.5f + (normalized_pos * 0.5f);

        case PITCH_MODE_SCRATCH:
            // ビニールスクラッチ（正弦波）
            pitch_mod_phase = (pitch_mod_phase + 1) % 1000;
            return 1.0f + 0.3f * sinf(2.0f * 3.14159f * (float)pitch_mod_phase / 1000.0f);

        case PITCH_MODE_FIXED_REVERSE:
        default:
            // 固定ピッチ（current_params.pitch_shift を使用）
            return current_params.pitch_shift;
    }
}

/**
 * @brief ループスタート/サイズ減衰を考慮した有効なループ範囲を計算
 * @param slice_length スライス長
 * @param loop_start_pos ループ開始位置（出力）
 * @param loop_end_pos ループ終了位置（出力）
 */
static inline void calculate_loop_range(uint32_t slice_length,
                                       uint32_t *loop_start_pos,
                                       uint32_t *loop_end_pos) {
    // ループ開始位置
    *loop_start_pos = (uint32_t)((float)slice_length * current_params.loop_start);

    // ループサイズ減衰を適用
    if (current_params.loop_size_decay > 0.0f) {
        // リピート回数に応じてループサイズを減らす
        float decay_factor = 1.0f - (current_params.loop_size_decay *
                                     ((float)repeat_counter / (float)current_params.repeat_count));
        if (decay_factor < 0.1f) decay_factor = 0.1f;  // 最小10%

        uint32_t effective_length = (uint32_t)((float)slice_length * decay_factor);
        *loop_end_pos = *loop_start_pos + effective_length;

        if (*loop_end_pos > slice_length) {
            *loop_end_pos = slice_length;
        }
    } else {
        // 減衰なし
        *loop_end_pos = slice_length;
    }
}

// ============================================================================
// メインエフェクト処理
// ============================================================================

void audio_effect_process(int16_t *data, uint32_t num_samples, uint8_t num_channels) {
    if (!is_initialized || !data || num_channels != STEREO_CHANNELS) {
        return;  // ステレオ以外は未対応
    }

    // エフェクトが無効な場合はスルー
    if (!current_params.enabled) {
        return;
    }

    // クロック分周を適用したスライス長
    uint32_t base_slice_length = current_params.stutter_enabled ?
        current_params.stutter_slice_length : current_params.slice_length;
    uint32_t active_slice_length = base_slice_length / current_params.clock_divider;

    // Beat-Repeatアルゴリズム（Kammerl オリジナル機能統合版）
    for (uint32_t i = 0; i < num_samples; i++) {
        // ステレオペアのインデックス
        uint32_t l_idx = i * STEREO_CHANNELS + LEFT_CHANNEL;
        uint32_t r_idx = i * STEREO_CHANNELS + RIGHT_CHANNEL;

        int16_t input_l = data[l_idx];
        int16_t input_r = data[r_idx];
        int16_t output_l, output_r;

        // スライスバッファに書き込み（常に最新の音を記録）
        write_to_slice_buffer(input_l, input_r);

        // スライスが満杯になったらリピート開始
        if (slice_write_pos >= active_slice_length) {
            slice_write_pos = 0;

            // マルチスライスバッファに保存
            save_slice_to_multi_buffer(active_slice_length);

            if (!is_repeating && !current_params.freeze) {
                // スライス確率チェック
                if (check_slice_probability()) {
                    is_repeating = true;
                    slice_read_pos_f = 0.0f;
                    repeat_counter = 0;
                    pitch_mod_phase = 0;
                }
            }
        }

        // リピート処理
        if (is_repeating) {
            int16_t repeat_l, repeat_r;

            // ループスタート/サイズ減衰を計算
            uint32_t loop_start, loop_end;
            calculate_loop_range(active_slice_length, &loop_start, &loop_end);
            uint32_t effective_loop_length = loop_end - loop_start;

            // 読み取り位置をループ範囲内に調整
            float adjusted_read_pos = slice_read_pos_f + (float)loop_start;
            if (adjusted_read_pos >= (float)loop_end) {
                adjusted_read_pos = (float)loop_start;
            }

            // ピッチモードに応じたピッチ計算
            float pitch_mult = calculate_pitch_for_mode((uint32_t)slice_read_pos_f,
                                                         effective_loop_length);

            // マルチスライスバッファまたは現在のスライスから読み取り
            if (current_params.slice_select == 0) {
                // 現在のスライス（slice_buffer）から読み取り
                uint32_t read_idx = (uint32_t)adjusted_read_pos;

                if (current_params.reverse) {
                    read_idx = loop_end - 1 - ((uint32_t)slice_read_pos_f % effective_loop_length);
                }

                if (read_idx < active_slice_length) {
                    uint32_t buf_idx = read_idx * STEREO_CHANNELS;
                    repeat_l = slice_buffer[buf_idx + LEFT_CHANNEL];
                    repeat_r = slice_buffer[buf_idx + RIGHT_CHANNEL];
                } else {
                    repeat_l = repeat_r = 0;
                }
            } else {
                // マルチスライスバッファから読み取り
                uint32_t read_idx = (uint32_t)adjusted_read_pos;

                if (current_params.reverse) {
                    read_idx = loop_end - 1 - ((uint32_t)slice_read_pos_f % effective_loop_length);
                }

                repeat_l = read_from_multi_slice(current_params.slice_select, read_idx, true);
                repeat_r = read_from_multi_slice(current_params.slice_select, read_idx, false);
            }

            // ウィンドウシェイプ（フェードイン/アウト）を適用
            if (current_params.window_shape > 0.0f) {
                float envelope = get_window_envelope((uint32_t)slice_read_pos_f,
                                                     effective_loop_length,
                                                     current_params.window_shape);
                repeat_l = (int16_t)((float)repeat_l * envelope);
                repeat_r = (int16_t)((float)repeat_r * envelope);
            }

            // ドライ/ウェットミックス
            output_l = mix_samples(input_l, repeat_l, current_params.wet_mix);
            output_r = mix_samples(input_r, repeat_r, current_params.wet_mix);

            // 読み取り位置を進める（ピッチ倍率適用）
            slice_read_pos_f += pitch_mult;

            // リピート終了判定（フリーズ時は終了しない）
            if (!current_params.freeze) {
                if (slice_read_pos_f >= (float)effective_loop_length) {
                    slice_read_pos_f = 0.0f;
                    repeat_counter++;

                    if (repeat_counter >= current_params.repeat_count) {
                        is_repeating = false;
                        repeat_counter = 0;
                    }
                }
            } else {
                // フリーズ時はループ範囲内で巻き戻し
                if (slice_read_pos_f >= (float)effective_loop_length) {
                    slice_read_pos_f = 0.0f;
                }
            }
        } else {
            // リピートしていない時は入力をそのまま出力
            output_l = input_l;
            output_r = input_r;
        }

        // 出力
        data[l_idx] = output_l;
        data[r_idx] = output_r;
    }
}
