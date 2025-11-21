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

// オーディオ処理の定数
#define STEREO_CHANNELS        2       // ステレオチャンネル数
#define LEFT_CHANNEL           0       // 左チャンネルオフセット
#define RIGHT_CHANNEL          1       // 右チャンネルオフセット
#define SAMPLE_MAX             32767   // 16ビットPCM最大値
#define SAMPLE_MIN             -32768  // 16ビットPCM最小値

// ============================================================================
// 内部変数
// ============================================================================

// エフェクトパラメータ（現在の設定）
static beat_repeat_params_t current_params;

// スライスバッファ（ステレオインターリーブ形式）
// メモリ使用量: 44100 * 2 * 2 = 176,400 バイト (約172 KB)
static int16_t slice_buffer[MAX_SLICE_LENGTH * 2];  // ステレオなので2倍

// スライス状態管理
static uint32_t slice_write_pos = 0;   // 書き込み位置
static float slice_read_pos_f = 0.0f;  // 読み取り位置（ピッチシフト用、float）
static uint32_t repeat_counter = 0;    // 現在のリピートカウント
static bool is_repeating = false;      // リピート中フラグ

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

    // バッファのクリア
    memset(slice_buffer, 0, sizeof(slice_buffer));
    slice_write_pos = 0;
    slice_read_pos_f = 0.0f;
    repeat_counter = 0;
    is_repeating = false;

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

    // ブール値はそのまま設定
    current_params.enabled = params->enabled;
    current_params.reverse = params->reverse;
    current_params.stutter_enabled = params->stutter_enabled;

    printf("Effect params updated: slice=%lu, repeat=%u, wet=%u%%, enabled=%d\n",
           current_params.slice_length, current_params.repeat_count,
           current_params.wet_mix, current_params.enabled);
    printf("  pitch=%.2f, reverse=%d, stutter=%d, window=%.2f\n",
           current_params.pitch_shift, current_params.reverse,
           current_params.stutter_enabled, current_params.window_shape);
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
    slice_write_pos = 0;
    slice_read_pos_f = 0.0f;
    repeat_counter = 0;
    is_repeating = false;
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

    // スタッターモード時のスライス長
    uint32_t active_slice_length = current_params.stutter_enabled ?
        current_params.stutter_slice_length : current_params.slice_length;

    // Beat-Repeatアルゴリズム（リファクタリング版）
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
            if (!is_repeating) {
                is_repeating = true;
                slice_read_pos_f = 0.0f;
                repeat_counter = 0;
            }
        }

        // リピート処理
        if (is_repeating) {
            int16_t repeat_l, repeat_r;

            // ピッチシフト・逆再生対応の読み取り
            if (current_params.pitch_shift != 1.0f || current_params.reverse) {
                read_slice_with_pitch(&repeat_l, &repeat_r, active_slice_length);
            } else {
                read_slice_normal(&repeat_l, &repeat_r);
            }

            // ウィンドウシェイプ（フェードイン/アウト）を適用
            apply_window_shape(&repeat_l, &repeat_r, active_slice_length);

            // ドライ/ウェットミックス
            output_l = mix_samples(input_l, repeat_l, current_params.wet_mix);
            output_r = mix_samples(input_r, repeat_r, current_params.wet_mix);

            // リピート終了判定
            check_repeat_end(active_slice_length);
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
