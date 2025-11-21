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
 * @brief ピッチモード（Kammerl Beat-Repeat互換）
 */
typedef enum {
    PITCH_MODE_FIXED_REVERSE = 0,  // 固定ピッチ + 逆再生対応
    PITCH_MODE_DECREASING,         // 線形減少ピッチ
    PITCH_MODE_INCREASING,         // 線形増加ピッチ
    PITCH_MODE_SCRATCH,            // ビニールスクラッチ（正弦波変調）
} pitch_mode_t;

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

    // ============================================================================
    // Kammerl Beat-Repeat オリジナル機能
    // ============================================================================

    // ループ開始位置（Loop Start）
    // スライス内のどこからループを開始するか（0.0-1.0）
    // 0.0 = スライス先頭、0.5 = 中央、1.0 = 末尾
    float loop_start;

    // ループサイズ減衰（Loop Size）
    // ループサイズを徐々に減らす量（0.0-1.0）
    // 0.0 = 減衰なし、1.0 = 最大減衰（バウンシングボール効果）
    float loop_size_decay;

    // スライス選択（Slice Select）
    // マルチスライスバッファ（0-1）のどのスライスを使うか
    // 0 = 最新スライス、1 = 1つ前のスライス（RAM制限により2個のみ）
    uint8_t slice_select;

    // スライス処理確率（Slice Probability）
    // スライスを処理する確率（0.0-1.0）
    // 0.0 = 常にバイパス、1.0 = 常に処理、0.5 = 50%の確率
    float slice_probability;

    // クロック分周（Clock Divider）
    // 1, 2, 4, 8 のいずれか
    // スライス長を自動的に変更（分周比に応じて）
    uint8_t clock_divider;

    // ピッチモード（Pitch Mode）
    // ピッチ変調の種類を選択
    pitch_mode_t pitch_mode;

    // フリーズ（Freeze）
    // true = 現在のスライスを凍結して無限ループ
    bool freeze;

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
