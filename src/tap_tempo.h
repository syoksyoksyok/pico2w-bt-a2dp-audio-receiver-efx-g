/**
 * @file tap_tempo.h
 * @brief Tap Tempo BPM Detection Module
 *
 * ボタンタップからBPMを検出し、エフェクトパラメータとLED点滅を制御
 */

#ifndef TAP_TEMPO_H
#define TAP_TEMPO_H

#include <stdint.h>
#include <stdbool.h>

// ============================================================================
// タップテンポ設定
// ============================================================================

// タップ履歴の保存数（4回のタップで平均を計算）
#define TAP_HISTORY_SIZE  4

// タップタイムアウト（ミリ秒）
// この時間内にタップがなければリセット
#define TAP_TIMEOUT_MS    2000

// BPM範囲制限
#define MIN_BPM  30
#define MAX_BPM  303

// デフォルトBPM（タップなしの初期値）
#define DEFAULT_BPM  120

// ============================================================================
// 音符分解能（ビートリピートのスライス長）
// ============================================================================

typedef enum {
    NOTE_WHOLE = 0,      // 全音符
    NOTE_HALF,           // 2分音符
    NOTE_QUARTER,        // 4分音符
    NOTE_EIGHTH,         // 8分音符
    NOTE_SIXTEENTH,      // 16分音符
    NOTE_THIRTY_SECOND,  // 32分音符
} note_division_t;

// ============================================================================
// タップテンポ状態
// ============================================================================

typedef struct {
    uint32_t tap_times[TAP_HISTORY_SIZE];  // タップタイムスタンプ（ms）
    uint8_t tap_count;                     // 現在のタップ数
    uint32_t last_tap_time;                // 最後のタップ時刻
    float current_bpm;                     // 現在のBPM
    note_division_t note_division;         // 音符分解能
    bool bpm_detected;                     // BPM検出済みフラグ
} tap_tempo_state_t;

// ============================================================================
// 関数プロトタイプ
// ============================================================================

/**
 * @brief タップテンポモジュールの初期化
 *
 * @param button_gpio ボタンのGPIOピン番号
 * @return true 成功
 * @return false 失敗
 */
bool tap_tempo_init(uint8_t button_gpio);

/**
 * @brief タップテンポの処理（メインループから定期的に呼び出す）
 *
 * ボタン状態の監視、BPM計算、LED点滅制御を行う
 */
void tap_tempo_process(void);

/**
 * @brief 現在のBPMを取得
 *
 * @return float 現在のBPM（検出されていない場合はDEFAULT_BPM）
 */
float tap_tempo_get_bpm(void);

/**
 * @brief BPMが検出されているか確認
 *
 * @return true BPM検出済み
 * @return false 未検出
 */
bool tap_tempo_is_detected(void);

/**
 * @brief 音符分解能を設定（将来的にロータリーエンコーダで変更予定）
 *
 * @param division 音符分解能
 */
void tap_tempo_set_note_division(note_division_t division);

/**
 * @brief 現在の音符分解能を取得
 *
 * @return note_division_t 音符分解能
 */
note_division_t tap_tempo_get_note_division(void);

/**
 * @brief BPMと音符分解能からスライス長（サンプル数）を計算
 *
 * @param bpm BPM
 * @param division 音符分解能
 * @param sample_rate サンプリングレート（Hz）
 * @return uint32_t スライス長（サンプル数）
 */
uint32_t tap_tempo_bpm_to_slice_length(float bpm, note_division_t division, uint32_t sample_rate);

/**
 * @brief タップテンポをリセット
 */
void tap_tempo_reset(void);

#endif // TAP_TEMPO_H
