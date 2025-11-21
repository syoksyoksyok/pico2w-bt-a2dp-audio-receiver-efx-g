/**
 * @file audio_effect.h
 * @brief Beat-Repeat Audio Effect Module (Kammerl style)
 *
 * Mutable Instruments CloudsのKammerl Beat-Repeatファームウェアを参考にした
 * ビートリピート/スライシングエフェクト
 *
 * ステレオ入力・ステレオ出力対応
 * 将来的にロータリーエンコーダでパラメータ調整可能
 */

#ifndef AUDIO_EFFECT_H
#define AUDIO_EFFECT_H

#include <stdint.h>
#include <stdbool.h>

// ============================================================================
// エフェクトパラメータ（将来的にロータリーエンコーダで調整予定）
// ============================================================================

/**
 * @brief Beat-Repeatエフェクトのパラメータ
 */
typedef struct {
    // スライス長（サンプル数）
    // 44100Hz * 0.25秒 = 11025サンプル（16分音符 @ 120 BPM）
    // 44100Hz * 0.5秒 = 22050サンプル（8分音符 @ 120 BPM）
    uint32_t slice_length;

    // リピート回数（1-16回）
    uint8_t repeat_count;

    // ドライ/ウェットミックス（0-100%）
    // 0% = 完全ドライ（エフェクトなし）
    // 100% = 完全ウェット（エフェクトのみ）
    uint8_t wet_mix;

    // エフェクトの有効/無効
    bool enabled;

    // ピッチシフト（実装済み）
    // 1.0 = 元のピッチ、2.0 = 1オクターブ上、0.5 = 1オクターブ下
    // 範囲: 0.25 - 4.0
    float pitch_shift;

    // 逆再生（Reverse Playback）
    // true = スライスを逆方向に再生
    bool reverse;

    // スタッター（Stutter Effect）
    // true = 短いスライスを高速リピート
    bool stutter_enabled;

    // スタッタースライス長（サンプル数）
    // スタッターON時の短いスライス長（例: 0.01秒 = 441サンプル）
    uint32_t stutter_slice_length;

    // ウィンドウシェイプ（Window Shape）
    // スライス開始/終了のフェード長（0.0-1.0）
    // 0.0 = フェードなし、0.1 = 10%をフェード、1.0 = 全体をフェード
    float window_shape;

} beat_repeat_params_t;

// ============================================================================
// 関数プロトタイプ
// ============================================================================

/**
 * @brief エフェクトモジュールの初期化
 *
 * @param sample_rate サンプリングレート（Hz）
 * @return true 成功
 * @return false 失敗
 */
bool audio_effect_init(uint32_t sample_rate);

/**
 * @brief エフェクトのパラメータを設定
 *
 * @param params エフェクトパラメータ
 */
void audio_effect_set_params(const beat_repeat_params_t *params);

/**
 * @brief 現在のパラメータを取得
 *
 * @param params パラメータ格納先
 */
void audio_effect_get_params(beat_repeat_params_t *params);

/**
 * @brief オーディオデータにエフェクトを適用
 *
 * ステレオインターリーブ形式（LRLRLR...）のデータを処理
 *
 * @param data PCMデータバッファ（int16_t配列、ステレオインターリーブ）
 * @param num_samples ステレオペア数（total samples / 2）
 * @param num_channels チャンネル数（通常2 = ステレオ）
 */
void audio_effect_process(int16_t *data, uint32_t num_samples, uint8_t num_channels);

/**
 * @brief エフェクトのリセット（バッファクリア）
 */
void audio_effect_reset(void);

#endif // AUDIO_EFFECT_H
