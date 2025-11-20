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
static uint32_t slice_read_pos = 0;    // 読み取り位置
static uint32_t repeat_counter = 0;    // 現在のリピートカウント
static bool is_repeating = false;      // リピート中フラグ

// サンプリングレート
static uint32_t sample_rate = AUDIO_SAMPLE_RATE;

// 初期化フラグ
static bool is_initialized = false;

// ============================================================================
// デフォルトパラメータ（固定値、将来的にロータリーエンコーダで調整）
// ============================================================================

#define DEFAULT_SLICE_LENGTH   (AUDIO_SAMPLE_RATE / 4)  // 0.25秒（16分音符 @ 120 BPM）
#define DEFAULT_REPEAT_COUNT   4                        // 4回リピート
#define DEFAULT_WET_MIX        70                       // 70% ウェット
#define DEFAULT_ENABLED        true                     // エフェクト有効

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

    current_params.slice_length = slice_len;
    current_params.repeat_count = repeat;
    current_params.wet_mix = wet;
    current_params.enabled = params->enabled;

    printf("Effect params updated: slice=%lu, repeat=%u, wet=%u%%, enabled=%d\n",
           current_params.slice_length, current_params.repeat_count,
           current_params.wet_mix, current_params.enabled);
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
    slice_read_pos = 0;
    repeat_counter = 0;
    is_repeating = false;
    printf("Effect reset\n");
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

    // num_samples はステレオペア数
    // 実際のサンプル数は num_samples * 2 (L + R)
    uint32_t total_samples = num_samples * num_channels;

    // Beat-Repeatアルゴリズム
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
        if (slice_write_pos >= current_params.slice_length) {
            slice_write_pos = 0;
            if (!is_repeating) {
                is_repeating = true;
                slice_read_pos = 0;
                repeat_counter = 0;
            }
        }

        // リピート処理
        if (is_repeating) {
            // スライスバッファから読み取り
            uint32_t read_idx = slice_read_pos * 2;
            int16_t repeat_l = slice_buffer[read_idx];
            int16_t repeat_r = slice_buffer[read_idx + 1];

            // ドライ/ウェットミックス
            output_l = mix_samples(input_l, repeat_l, current_params.wet_mix);
            output_r = mix_samples(input_r, repeat_r, current_params.wet_mix);

            slice_read_pos++;

            // リピート終了判定
            if (slice_read_pos >= current_params.slice_length) {
                slice_read_pos = 0;
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
