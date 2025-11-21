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
    slice_read_pos = 0;
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
    printf("Buffer Size: %u samples (%u bytes)\n",
           MAX_SLICE_LENGTH, (uint32_t)sizeof(slice_buffer));
    printf("========================================\n\n");

    return true;
}

// ============================================================================
// パラメータ設定・取得
// ============================================================================

void audio_effect_set_params(const beat_repeat_params_t *params) {
    if (!params) return;

    // スライス長の検証
    uint32_t slice_len = params->slice_length;
    if (slice_len > MAX_SLICE_LENGTH) {
        slice_len = MAX_SLICE_LENGTH;
        printf("WARNING: Slice length clamped to %lu\n", MAX_SLICE_LENGTH);
    }
    if (slice_len < 128) {  // 最小128サンプル（約3ms @ 44.1kHz）
        slice_len = 128;
        printf("WARNING: Slice length clamped to minimum 128\n");
    }

    // リピート回数の検証
    uint8_t repeat = params->repeat_count;
    if (repeat < 1) repeat = 1;
    if (repeat > 16) repeat = 16;

    // ウェットミックスの検証
    uint8_t wet = params->wet_mix;
    if (wet > 100) wet = 100;

    // ピッチシフトの検証（0.25 - 4.0）
    float pitch = params->pitch_shift;
    if (pitch < 0.25f) pitch = 0.25f;
    if (pitch > 4.0f) pitch = 4.0f;

    // スタッタースライス長の検証
    uint32_t stutter_len = params->stutter_slice_length;
    if (stutter_len < 64) stutter_len = 64;  // 最小64サンプル
    if (stutter_len > MAX_SLICE_LENGTH) stutter_len = MAX_SLICE_LENGTH;

    // ウィンドウシェイプの検証（0.0 - 1.0）
    float window = params->window_shape;
    if (window < 0.0f) window = 0.0f;
    if (window > 1.0f) window = 1.0f;

    current_params.slice_length = slice_len;
    current_params.repeat_count = repeat;
    current_params.wet_mix = wet;
    current_params.enabled = params->enabled;
    current_params.pitch_shift = pitch;
    current_params.reverse = params->reverse;
    current_params.stutter_enabled = params->stutter_enabled;
    current_params.stutter_slice_length = stutter_len;
    current_params.window_shape = window;

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

    // チャンネルオフセット（0 = 左、1 = 右）
    uint32_t ch_offset = is_left ? 0 : 1;

    // 現在のサンプル
    int16_t sample1 = slice_buffer[pos_int * 2 + ch_offset];

    // 次のサンプル
    int16_t sample2;
    if (pos_int + 1 < max_length) {
        sample2 = slice_buffer[(pos_int + 1) * 2 + ch_offset];
    } else {
        sample2 = sample1;  // バッファ端では同じ値を使う
    }

    // 線形補間
    float result = (float)sample1 * (1.0f - frac) + (float)sample2 * frac;

    // クリッピング
    if (result > 32767.0f) result = 32767.0f;
    if (result < -32768.0f) result = -32768.0f;

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
    int32_t dry_part = (int32_t)dry * (100 - wet_percent) / 100;
    int32_t wet_part = (int32_t)wet * wet_percent / 100;
    int32_t result = dry_part + wet_part;

    // クリッピング
    if (result > 32767) result = 32767;
    if (result < -32768) result = -32768;

    return (int16_t)result;
}

// ============================================================================
// メインエフェクト処理
// ============================================================================

void audio_effect_process(int16_t *data, uint32_t num_samples, uint8_t num_channels) {
    if (!is_initialized || !data || num_channels != 2) {
        return;  // ステレオ以外は未対応
    }

    // エフェクトが無効な場合はスルー
    if (!current_params.enabled) {
        return;
    }

    // スタッターモード時のスライス長
    uint32_t active_slice_length = current_params.stutter_enabled ?
        current_params.stutter_slice_length : current_params.slice_length;

    // Beat-Repeatアルゴリズム（新機能統合版）
    for (uint32_t i = 0; i < num_samples; i++) {
        // ステレオペアのインデックス
        uint32_t l_idx = i * 2;      // 左チャンネル
        uint32_t r_idx = i * 2 + 1;  // 右チャンネル

        int16_t input_l = data[l_idx];
        int16_t input_r = data[r_idx];
        int16_t output_l, output_r;

        // スライスバッファに書き込み（常に最新の音を記録）
        uint32_t buf_idx = slice_write_pos * 2;
        slice_buffer[buf_idx] = input_l;
        slice_buffer[buf_idx + 1] = input_r;

        slice_write_pos++;

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

            // ピッチシフト＋逆再生の処理
            if (current_params.pitch_shift != 1.0f || current_params.reverse) {
                // 読み取り位置の計算
                float read_pos;
                if (current_params.reverse) {
                    // 逆再生：末尾から開始
                    read_pos = (float)active_slice_length - 1.0f - slice_read_pos_f;
                } else {
                    // 通常再生
                    read_pos = slice_read_pos_f;
                }

                // 線形補間でサンプルを読み取り
                repeat_l = interpolate_sample(read_pos, active_slice_length, true);
                repeat_r = interpolate_sample(read_pos, active_slice_length, false);

                // 読み取り位置を進める（ピッチシフト倍率分）
                slice_read_pos_f += current_params.pitch_shift;
            } else {
                // ピッチシフトなし＋通常再生：整数インデックスで読み取り
                uint32_t read_idx = (uint32_t)slice_read_pos_f * 2;
                repeat_l = slice_buffer[read_idx];
                repeat_r = slice_buffer[read_idx + 1];

                slice_read_pos_f += 1.0f;
            }

            // ウィンドウシェイプ（フェードイン/アウト）を適用
            if (current_params.window_shape > 0.0f) {
                float envelope = get_window_envelope(
                    (uint32_t)slice_read_pos_f, active_slice_length, current_params.window_shape
                );
                repeat_l = (int16_t)((float)repeat_l * envelope);
                repeat_r = (int16_t)((float)repeat_r * envelope);
            }

            // ドライ/ウェットミックス
            output_l = mix_samples(input_l, repeat_l, current_params.wet_mix);
            output_r = mix_samples(input_r, repeat_r, current_params.wet_mix);

            // リピート終了判定
            if (slice_read_pos_f >= (float)active_slice_length) {
                slice_read_pos_f = 0.0f;
                repeat_counter++;

                if (repeat_counter >= current_params.repeat_count) {
                    is_repeating = false;
                    repeat_counter = 0;
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
