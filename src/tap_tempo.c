/**
 * @file tap_tempo.c
 * @brief Tap Tempo BPM Detection Implementation
 */

#include "tap_tempo.h"
#include "config.h"
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

// ============================================================================
// 内部変数
// ============================================================================

static tap_tempo_state_t state;
static uint8_t button_pin = 0;
static bool button_last_state = false;
static bool is_initialized = false;

// LED点滅管理
static uint32_t last_beat_time = 0;
static bool led_is_on = false;
static uint32_t led_on_time = 0;

// LED点滅の長さ（ミリ秒）
#define LED_BLINK_DURATION_MS  50

// ============================================================================
// 内部関数（前方宣言）
// ============================================================================

static void update_bpm(void);
static void update_led_blink(void);

// ============================================================================
// 初期化
// ============================================================================

bool tap_tempo_init(uint8_t button_gpio) {
    printf("\n========================================\n");
    printf("Tap Tempo Module\n");
    printf("========================================\n");

    button_pin = button_gpio;

    // GPIO初期化（プルアップ、押すとGND接続）
    gpio_init(button_pin);
    gpio_set_dir(button_pin, GPIO_IN);
    gpio_pull_up(button_pin);

    // 状態の初期化
    memset(&state, 0, sizeof(state));
    state.current_bpm = DEFAULT_BPM;
    state.note_division = NOTE_SIXTEENTH;  // デフォルト：16分音符
    state.bpm_detected = false;

    button_last_state = gpio_get(button_pin);
    last_beat_time = 0;
    led_is_on = false;

    is_initialized = true;

    printf("Button GPIO: %d\n", button_pin);
    printf("Default BPM: %.1f\n", state.current_bpm);
    printf("Note Division: 16th note\n");
    printf("Tap the button to set tempo!\n");
    printf("========================================\n\n");

    return true;
}

// ============================================================================
// タップテンポのメイン処理
// ============================================================================

void tap_tempo_process(void) {
    if (!is_initialized) return;

    uint32_t now = to_ms_since_boot(get_absolute_time());

    // ボタン状態の読み取り（ボタン押下 = LOW）
    bool button_state = !gpio_get(button_pin);  // 反転（押下=true）

    // ボタン押下検出（立ち上がりエッジ）
    if (button_state && !button_last_state) {
        // タップ検出！
        printf("[TAP] Button pressed at %lu ms\n", now);

        // タイムアウトチェック
        if (state.tap_count > 0 && (now - state.last_tap_time) > TAP_TIMEOUT_MS) {
            printf("[TAP] Timeout - resetting tap history\n");
            state.tap_count = 0;
        }

        // タップ時刻を記録
        if (state.tap_count < TAP_HISTORY_SIZE) {
            state.tap_times[state.tap_count] = now;
            state.tap_count++;
        } else {
            // 履歴がいっぱいの場合はシフト
            for (int i = 0; i < TAP_HISTORY_SIZE - 1; i++) {
                state.tap_times[i] = state.tap_times[i + 1];
            }
            state.tap_times[TAP_HISTORY_SIZE - 1] = now;
        }

        state.last_tap_time = now;

        // BPM計算（2タップ以上必要）
        if (state.tap_count >= 2) {
            update_bpm();
        }

        // LED即座に点灯
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
        led_is_on = true;
        led_on_time = now;
    }

    button_last_state = button_state;

    // LED点滅更新
    update_led_blink();
}

// ============================================================================
// BPM計算
// ============================================================================

static void update_bpm(void) {
    if (state.tap_count < 2) return;

    // タップ間隔の平均を計算
    uint32_t total_interval = 0;
    uint32_t interval_count = 0;

    for (int i = 1; i < state.tap_count; i++) {
        uint32_t interval = state.tap_times[i] - state.tap_times[i - 1];
        total_interval += interval;
        interval_count++;
    }

    float avg_interval_ms = (float)total_interval / interval_count;
    float avg_interval_sec = avg_interval_ms / 1000.0f;

    // BPM計算（1タップ = 1拍 = 4分音符）
    float bpm = 60.0f / avg_interval_sec;

    // BPM範囲チェック
    if (bpm < MIN_BPM) bpm = MIN_BPM;
    if (bpm > MAX_BPM) bpm = MAX_BPM;

    state.current_bpm = bpm;
    state.bpm_detected = true;

    printf("[TAP] BPM detected: %.1f (from %lu taps, avg interval: %.1f ms)\n",
           bpm, state.tap_count, avg_interval_ms);
}

// ============================================================================
// LED点滅更新
// ============================================================================

static void update_led_blink(void) {
    if (!state.bpm_detected) {
        return;
    }

    uint32_t now = to_ms_since_boot(get_absolute_time());

    // LED消灯処理（短い点滅）
    if (led_is_on && (now - led_on_time) >= LED_BLINK_DURATION_MS) {
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
        led_is_on = false;
    }

    // 自動点滅（BPMに同期、4分音符ごと）
    if (!led_is_on) {
        // 4分音符の間隔（ミリ秒）
        uint32_t beat_interval_ms = (uint32_t)(60000.0f / state.current_bpm);

        if (last_beat_time == 0 || (now - last_beat_time) >= beat_interval_ms) {
            // LED点灯
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
            led_is_on = true;
            led_on_time = now;
            last_beat_time = now;
        }
    }
}

// ============================================================================
// BPM取得
// ============================================================================

float tap_tempo_get_bpm(void) {
    return state.current_bpm;
}

bool tap_tempo_is_detected(void) {
    return state.bpm_detected;
}

// ============================================================================
// 音符分解能の設定・取得
// ============================================================================

void tap_tempo_set_note_division(note_division_t division) {
    state.note_division = division;
    printf("[TAP] Note division changed: ");
    switch (division) {
        case NOTE_WHOLE:         printf("Whole note\n"); break;
        case NOTE_HALF:          printf("Half note\n"); break;
        case NOTE_QUARTER:       printf("Quarter note\n"); break;
        case NOTE_EIGHTH:        printf("8th note\n"); break;
        case NOTE_SIXTEENTH:     printf("16th note\n"); break;
        case NOTE_THIRTY_SECOND: printf("32nd note\n"); break;
    }
}

note_division_t tap_tempo_get_note_division(void) {
    return state.note_division;
}

// ============================================================================
// BPMからスライス長への変換
// ============================================================================

uint32_t tap_tempo_bpm_to_slice_length(float bpm, note_division_t division, uint32_t sample_rate) {
    if (bpm <= 0) bpm = DEFAULT_BPM;

    // 4分音符の長さ（秒）
    float quarter_note_sec = 60.0f / bpm;

    // 音符分解能に応じて調整
    float note_sec = quarter_note_sec;
    switch (division) {
        case NOTE_WHOLE:
            note_sec = quarter_note_sec * 4.0f;
            break;
        case NOTE_HALF:
            note_sec = quarter_note_sec * 2.0f;
            break;
        case NOTE_QUARTER:
            note_sec = quarter_note_sec;
            break;
        case NOTE_EIGHTH:
            note_sec = quarter_note_sec / 2.0f;
            break;
        case NOTE_SIXTEENTH:
            note_sec = quarter_note_sec / 4.0f;
            break;
        case NOTE_THIRTY_SECOND:
            note_sec = quarter_note_sec / 8.0f;
            break;
    }

    // サンプル数に変換
    uint32_t slice_length = (uint32_t)(note_sec * sample_rate);

    return slice_length;
}

// ============================================================================
// リセット
// ============================================================================

void tap_tempo_reset(void) {
    memset(&state, 0, sizeof(state));
    state.current_bpm = DEFAULT_BPM;
    state.note_division = NOTE_SIXTEENTH;
    state.bpm_detected = false;
    last_beat_time = 0;
    led_is_on = false;
    printf("[TAP] Reset\n");
}
