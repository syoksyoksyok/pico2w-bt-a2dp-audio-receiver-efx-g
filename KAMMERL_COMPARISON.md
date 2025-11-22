# Kammerl Beat-Repeat 機能比較と可変パラメータ一覧

**作成日**: 2025-11-22
**対象**: Pico 2W Bluetooth A2DP Audio Receiver with Beat-Repeat Effects

---

## 📊 オリジナルとの機能比較

### ✅ 実装済み機能（コア機能）

| 機能 | オリジナル | 現在の実装 | 状態 |
|------|----------|----------|------|
| **Freeze** | FREEZEボタン | `freeze` パラメータ | ✅ 完全実装 |
| **Position** | ループ開始位置（量子化） | `loop_start` (0.0-1.0) | ⚠️ 量子化なし |
| **Size** | ループサイズ + 交互モード | `loop_size_decay` (0.0-1.0) | ⚠️ 減衰のみ |
| **Pitch** | ピッチシフト（0.0-1.0） | `pitch_shift` (0.25-4.0) | ✅ 完全実装 |
| **Density** | ループサイズ減衰 | `loop_size_decay` | ✅ 完全実装 |
| **Dry/Wet Mix** | ドライ/ウェットミックス | `wet_mix` (0-100%) | ✅ 完全実装 |

### ✅ 実装済み機能（Blendモード）

| Blendモード | オリジナル | 現在の実装 | 状態 |
|------------|----------|----------|------|
| **Slice Probability** | 確率処理（0-100%） | `slice_probability` (0.0-1.0) | ✅ 完全実装 |
| **Clock Divider** | 1, 1/2, 1/4, 1/8 | `clock_divider` (1,2,4,8) | ✅ 完全実装 |
| **Pitch Modulation** | 4ピッチモード | `pitch_mode` (4モード) | ✅ 完全実装 |
| **Warm Distortion** | ディストーション | - | ❌ **未実装** |

### ❌ 未実装/削除された機能

| 機能 | オリジナル | 現在の実装 | 理由 |
|------|----------|----------|------|
| **Texture CV + Slice Step** | 8スライスバッファ選択 | - | ❌ **RAM制約** (344 KB消費) |
| **Warm Distortion** | 暖かいディストーション | - | ❌ **未実装** |
| **Stereo Spread** | ステレオ幅調整 | - | ❌ **未実装** |
| **Feedback/Reverb** | フィードバック | - | ❌ **未実装** |
| **Loop Alternating Mode** | 交互ループモード | - | ❌ **未実装** |
| **Position/Size Quantization** | リズム同期量子化 | - | ❌ **未実装** |
| **CV/Gate入力** | 外部CV制御 | - | ❌ **ハードウェア制約** |

---

## 🎛️ 全パラメータ一覧（現在値と可変範囲）

### 1. Beat-Repeat コアパラメータ

#### `slice_length` - スライス長
- **説明**: 録音・リピートするスライスの長さ（サンプル数）
- **現在の固定値**: `11025` サンプル（0.25秒 @ 44.1kHz = 16分音符 @ 120 BPM）
- **可変時の下限**: `2205` サンプル（0.05秒 = 50ms）
- **可変時の上限**: `88200` サンプル（2.0秒）
- **単位**: サンプル数
- **ファイル**: `src/audio_effect.c:79`
- **BPM対応表**:
  | BPM | 16分音符 | 8分音符 | 4分音符 |
  |-----|---------|--------|--------|
  | 60  | 11025   | 22050  | 44100  |
  | 120 | 5513    | 11025  | 22050  |
  | 140 | 4725    | 9450   | 18900  |
  | 180 | 3675    | 7350   | 14700  |

#### `repeat_count` - リピート回数
- **説明**: スライスを何回繰り返すか
- **現在の固定値**: `4` 回
- **可変時の下限**: `1` 回
- **可変時の上限**: `16` 回
- **単位**: 回
- **ファイル**: `src/audio_effect.c:80`

#### `wet_mix` - ドライ/ウェットミックス
- **説明**: エフェクト音の混合比率
- **現在の固定値**: `70` %
- **可変時の下限**: `0` % （完全ドライ、エフェクトなし）
- **可変時の上限**: `100` % （完全ウェット、エフェクトのみ）
- **単位**: パーセント
- **ファイル**: `src/audio_effect.c:81`

#### `enabled` - エフェクト有効/無効
- **説明**: エフェクト全体のON/OFF
- **現在の固定値**: `true` （有効）
- **可変時の値**: `true` / `false`
- **単位**: bool
- **ファイル**: `src/audio_effect.c:82`

---

### 2. ピッチ・再生方向パラメータ

#### `pitch_shift` - ピッチシフト倍率
- **説明**: 再生速度（テープ/レコード速度変更）
- **現在の固定値**: `1.0` （オリジナルピッチ）
- **可変時の下限**: `0.25` （2オクターブ下、1/4速度）
- **可変時の上限**: `4.0` （2オクターブ上、4倍速）
- **単位**: 倍率
- **ファイル**: `src/audio_effect.c:83`
- **参考値**:
  | 倍率 | 音程変化 | 速度 |
  |------|---------|------|
  | 0.25 | -2オクターブ | 1/4倍速 |
  | 0.5  | -1オクターブ | 半速 |
  | 1.0  | オリジナル | 等速 |
  | 2.0  | +1オクターブ | 2倍速 |
  | 4.0  | +2オクターブ | 4倍速 |

#### `reverse` - 逆再生
- **説明**: スライスを逆方向に再生
- **現在の固定値**: `false` （通常再生）
- **可変時の値**: `true` / `false`
- **単位**: bool
- **ファイル**: `src/audio_effect.c:84`

#### `pitch_mode` - ピッチモード
- **説明**: ピッチ変調の種類
- **現在の固定値**: `PITCH_MODE_FIXED_REVERSE` (0)
- **可変時の値**:
  | 値 | モード | 説明 |
  |----|--------|------|
  | `0` | `PITCH_MODE_FIXED_REVERSE` | 固定ピッチ + 逆再生対応 |
  | `1` | `PITCH_MODE_DECREASING` | 線形減少（1.0 → 0.5） |
  | `2` | `PITCH_MODE_INCREASING` | 線形増加（0.5 → 1.0） |
  | `3` | `PITCH_MODE_SCRATCH` | ビニールスクラッチ（正弦波変調 0.7-1.3） |
- **単位**: enum
- **ファイル**: `src/audio_effect.h:25-30`, `src/audio_effect.c:94`

---

### 3. スタッター機能

#### `stutter_enabled` - スタッター有効
- **説明**: 短いスライスで高速リピート
- **現在の固定値**: `false` （無効）
- **可変時の値**: `true` / `false`
- **単位**: bool
- **ファイル**: `src/audio_effect.c:85`

#### `stutter_slice_length` - スタッタースライス長
- **説明**: スタッターON時の短いスライス長
- **現在の固定値**: `441` サンプル（0.01秒 = 10ms @ 44.1kHz）
- **可変時の下限**: `64` サンプル（約1.5ms）
- **可変時の上限**: `22050` サンプル（0.5秒）
- **単位**: サンプル数
- **ファイル**: `src/audio_effect.c:86`

---

### 4. ウィンドウシェイプ

#### `window_shape` - フェード長
- **説明**: スライス開始/終了のフェード長
- **現在の固定値**: `0.05` （5%フェード）
- **可変時の下限**: `0.0` （フェードなし、クリック音が出る可能性）
- **可変時の上限**: `1.0` （全体をフェード）
- **単位**: 0.0-1.0（スライス長の比率）
- **ファイル**: `src/audio_effect.c:87`

---

### 5. Kammerl Beat-Repeat 拡張パラメータ

#### `loop_start` - ループ開始位置
- **説明**: スライス内のどこからループを開始するか
- **現在の固定値**: `0.0` （スライス先頭から）
- **可変時の下限**: `0.0` （先頭）
- **可変時の上限**: `1.0` （末尾）
- **単位**: 0.0-1.0（スライス長の比率）
- **ファイル**: `src/audio_effect.c:90`
- **参考値**:
  | 値 | 位置 |
  |----|------|
  | 0.0 | 先頭 |
  | 0.25 | 1/4地点 |
  | 0.5 | 中央 |
  | 0.75 | 3/4地点 |
  | 1.0 | 末尾 |
- **注**: オリジナルは量子化あり（1/64, 1/32, 1/16, 1/8, 1/4, 1/3, 1/2, 1）

#### `loop_size_decay` - ループサイズ減衰
- **説明**: ループサイズを徐々に減らす量（バウンシングボール効果）
- **現在の固定値**: `0.0` （減衰なし）
- **可変時の下限**: `0.0` （減衰なし）
- **可変時の上限**: `1.0` （最大減衰、最小10%まで減衰）
- **単位**: 0.0-1.0
- **ファイル**: `src/audio_effect.c:91`
- **効果**:
  - `0.0` = 減衰なし（一定ループサイズ）
  - `0.5` = 中程度減衰（最終的に55%まで減少）
  - `1.0` = 最大減衰（最終的に10%まで減少）

#### `slice_probability` - スライス処理確率
- **説明**: スライスを処理する確率
- **現在の固定値**: `1.0` （100%、常に処理）
- **可変時の下限**: `0.0` （0%、常にバイパス）
- **可変時の上限**: `1.0` （100%、常に処理）
- **単位**: 0.0-1.0
- **ファイル**: `src/audio_effect.c:92`
- **効果**:
  - `1.0` = 全スライスを処理
  - `0.5` = 50%の確率で処理（ランダムに半分）
  - `0.1` = 10%の確率で処理（たまに）
  - `0.0` = 処理しない（エフェクトバイパス）

#### `clock_divider` - クロック分周
- **説明**: スライス長を自動的に変更（分周比に応じて）
- **現在の固定値**: `1` （分周なし）
- **可変時の値**: `1`, `2`, `4`, `8` のいずれか
- **単位**: 分周比
- **ファイル**: `src/audio_effect.c:93`
- **効果**:
  | 値 | 効果 | スライス長（11025サンプル基準） |
  |----|------|-------------------------------|
  | `1` | 分周なし | 11025サンプル（0.25秒） |
  | `2` | 1/2に分周 | 5513サンプル（0.125秒 = 32分音符） |
  | `4` | 1/4に分周 | 2756サンプル（0.0625秒 = 64分音符） |
  | `8` | 1/8に分周 | 1378サンプル（0.03125秒） |

#### `freeze` - フリーズモード
- **説明**: 現在のスライスを凍結して無限ループ
- **現在の固定値**: `false` （無効）
- **可変時の値**: `true` / `false`
- **単位**: bool
- **ファイル**: `src/audio_effect.c:95`
- **効果**:
  - `false` = 通常動作（repeat_count回リピート後に停止）
  - `true` = フリーズ（現在のスライスを無限ループ、新規スライス録音なし）

---

## 📋 パラメータ一覧表（全15パラメータ）

| # | パラメータ | 現在値 | 下限 | 上限 | 単位 | カテゴリ |
|---|-----------|-------|------|------|------|---------|
| 1 | `slice_length` | 11025 | 2205 | 88200 | samples | コア |
| 2 | `repeat_count` | 4 | 1 | 16 | 回 | コア |
| 3 | `wet_mix` | 70 | 0 | 100 | % | コア |
| 4 | `enabled` | true | false | true | bool | コア |
| 5 | `pitch_shift` | 1.0 | 0.25 | 4.0 | 倍率 | ピッチ |
| 6 | `reverse` | false | false | true | bool | ピッチ |
| 7 | `pitch_mode` | 0 | 0 | 3 | enum | ピッチ |
| 8 | `stutter_enabled` | false | false | true | bool | スタッター |
| 9 | `stutter_slice_length` | 441 | 64 | 22050 | samples | スタッター |
| 10 | `window_shape` | 0.05 | 0.0 | 1.0 | 0-1 | ウィンドウ |
| 11 | `loop_start` | 0.0 | 0.0 | 1.0 | 0-1 | Kammerl |
| 12 | `loop_size_decay` | 0.0 | 0.0 | 1.0 | 0-1 | Kammerl |
| 13 | `slice_probability` | 1.0 | 0.0 | 1.0 | 0-1 | Kammerl |
| 14 | `clock_divider` | 1 | 1 | 8 | 1,2,4,8 | Kammerl |
| 15 | `freeze` | false | false | true | bool | Kammerl |

**合計**: 15パラメータ（うち6パラメータがKammerl拡張）

---

## ❌ 現在のプログラムに欠けている機能（詳細）

### 1. マルチスライスバッファ（Multi-Slice Buffer）❌

**オリジナル機能**:
- 8個の最新スライスをバッファに保存
- Texture CV（0-8V）で任意のスライスを選択
- Slice Stepパターンでスライス間を自動遷移
- 0V = 最新スライス（リアルタイム）、8V = 8個前のスライス

**現在の実装**: 完全削除

**理由**: **RAM制約**
- 8スライス実装: 1.38 MB消費 → RAM overflow 1,272,644 bytes
- 2スライス実装: 344 KB消費 → RAM overflow 214,220 bytes
- RP2350は520 KBのRAMしかない

**削減量**: 352,800 bytes（344 KB）

**再実装の可能性**:
- RP2350B（1 MB RAM版）が必要
- または他の機能を削減してRAMを確保

---

### 2. Warm Distortion（暖かいディストーション）❌

**オリジナル機能**:
- Blendモード4番目の機能
- 暖かいサウンドのディストーション
- Dry/Wet後にポスト処理として適用

**現在の実装**: なし

**実装難易度**: ⭐⭐（中程度）

**RAM必要量**: ほぼなし（約100 bytes）

**実装方法**:
```c
// ディストーションパラメータ追加
float distortion_amount;  // 0.0-1.0

// ソフトクリッピング
static inline int16_t apply_distortion(int16_t sample, float amount) {
    if (amount <= 0.0f) return sample;

    float normalized = (float)sample / 32768.0f;
    float drive = 1.0f + (amount * 9.0f);  // 1.0-10.0
    float distorted = tanhf(normalized * drive) / tanhf(drive);
    return (int16_t)(distorted * 32767.0f);
}
```

---

### 3. Stereo Spread（ステレオ幅調整）❌

**オリジナル機能**:
- ステレオ幅を調整（モノラル ↔ 超ワイド）
- CV制御可能

**現在の実装**: なし

**実装難易度**: ⭐（簡単）

**RAM必要量**: ほぼなし（約50 bytes）

**実装方法**:
```c
// ステレオ幅パラメータ追加
float stereo_width;  // 0.0-2.0 (0=モノ, 1=通常, 2=超ワイド)

// M/S処理
static inline void apply_stereo_width(int16_t *left, int16_t *right, float width) {
    int32_t mid = ((int32_t)*left + (int32_t)*right) / 2;
    int32_t side = ((int32_t)*left - (int32_t)*right) / 2;

    side = (int32_t)((float)side * width);

    *left = (int16_t)((mid + side) > 32767 ? 32767 :
                      (mid + side) < -32768 ? -32768 : (mid + side));
    *right = (int16_t)((mid - side) > 32767 ? 32767 :
                       (mid - side) < -32768 ? -32768 : (mid - side));
}
```

---

### 4. Feedback/Reverb（フィードバック）❌

**オリジナル機能**:
- リピート音をさらにリピート
- CloudsのフィードバックパスをBeat-Repeatに統合

**現在の実装**: なし

**実装難易度**: ⭐⭐⭐⭐（高難易度）

**RAM必要量**: 大量（約200-500 KB、ディレイバッファ必要）

**理由**: RP2350のRAM制約により実装困難

---

### 5. Loop Alternating Mode（交互ループモード）❌

**オリジナル機能**:
- Sizeノブ12時以降で交互ループモード
- 量子化値: 1/2, 1/3, 1/4, 1/8, 1/16, 1/32, 1/64

**現在の実装**: なし

**実装難易度**: ⭐⭐（中程度）

**RAM必要量**: ほぼなし（約100 bytes）

**実装方法**:
```c
// 交互ループフラグ追加
bool loop_alternating;  // true = 交互ループ

// 処理ロジック修正
static bool forward = true;
if (loop_alternating) {
    if (slice_read_pos_f >= (float)effective_loop_length) {
        forward = !forward;  // 方向を反転
        slice_read_pos_f = 0.0f;
    }
    if (!forward) {
        read_idx = loop_end - 1 - (uint32_t)slice_read_pos_f;
    }
}
```

---

### 6. Position/Size Quantization（リズム同期量子化）❌

**オリジナル機能**:
- ループ開始位置とサイズを音楽的に量子化
- Position量子化: free, 1/64, 1/32, 1/16, 1/8, 1/4, 1/3, 1/2, 1
- Size量子化: free, 1/64, 1/32, 1/16, 1/8, 1/4, 1/3, 1/2

**現在の実装**: なし（連続値のみ）

**実装難易度**: ⭐⭐（中程度）

**RAM必要量**: ほぼなし（約200 bytes）

**実装方法**:
```c
// 量子化テーブル
static const float quantize_table[] = {
    0.0f,      // free (0-1/64)
    1.0f/64,   // 1/64
    1.0f/32,   // 1/32
    1.0f/16,   // 1/16
    1.0f/8,    // 1/8
    1.0f/4,    // 1/4
    1.0f/3,    // 1/3
    1.0f/2,    // 1/2
    1.0f       // 1
};

static inline float quantize_position(float value) {
    // 最も近い量子化値を返す
    float min_diff = 1.0f;
    float result = value;

    for (int i = 0; i < 9; i++) {
        float diff = fabsf(value - quantize_table[i]);
        if (diff < min_diff) {
            min_diff = diff;
            result = quantize_table[i];
        }
    }
    return result;
}
```

---

### 7. CV/Gate入力（外部制御）❌

**オリジナル機能**:
- Trigger入力: クロック信号
- Texture CV: スライス選択（0-8V）
- Position CV: ループ開始位置制御
- Size CV: ループサイズ制御
- Pitch CV: ピッチ制御（1V/Oct対応）
- その他すべてのパラメータにCV入力

**現在の実装**: なし

**理由**: **ハードウェア制約**（Pico 2WにはCV/Gate入力なし）

**代替手段**:
1. **ロータリーエンコーダ** - 手動パラメータ調整
2. **MIDI CC** - Bluetooth MIDI経由で制御
3. **OSC (Open Sound Control)** - Wi-Fi経由で制御
4. **外部ADC** - CV入力用のADCチップを追加（ハードウェア改造必要）

---

## 🚀 今後の実装可能性

### 高優先度（実装容易 + RAM余裕あり）

1. **Stereo Spread** ⭐ RAM: ~50 bytes
2. **Warm Distortion** ⭐⭐ RAM: ~100 bytes
3. **Loop Alternating Mode** ⭐⭐ RAM: ~100 bytes
4. **Position/Size Quantization** ⭐⭐ RAM: ~200 bytes

**合計RAM**: ~450 bytes（余裕で実装可能）

### 中優先度（条件付き実装可能）

5. **ロータリーエンコーダ制御** - 全15パラメータをリアルタイム調整
6. **MIDI CC対応** - Bluetooth MIDI経由でパラメータ制御
7. **プリセット保存/ロード** - Flash メモリに8プリセット

### 低優先度（RAM制約あり）

8. **マルチスライスバッファ** - RP2350B（1 MB RAM）が必要
9. **Feedback/Reverb** - 大量のRAM必要（200-500 KB）

---

## 📊 現在のメモリ使用状況

| 項目 | 使用量 | 備考 |
|------|--------|------|
| `slice_buffer` | 176,400 bytes (172 KB) | 単一スライスバッファ |
| BTstack + USB | ~300 KB (推定) | Bluetoothスタック |
| システムオーバーヘッド | ~20 KB (推定) | システム予約 |
| **合計** | **~492 KB** | - |
| **RP2350上限** | **520 KB** | - |
| **余裕** | **28 KB** | 追加機能用 |

**結論**: 約28 KBの余裕あり → Stereo Spread, Distortion, Quantizationは実装可能

---

## 📚 参考資料

- [Kammerl Beat-Repeat 公式サイト](https://www.kammerl.de/audio/clouds/)
- [Mutable Instruments Clouds](https://mutable-instruments.net/modules/clouds/)
- [ModWiggler Forum - Kammerl Beat-Repeat Discussion](https://www.modwiggler.com/forum/viewtopic.php?t=179369)

---

**最終更新**: 2025-11-22
**次のステップ**: 高優先度機能（Stereo Spread, Distortion, Quantization）の実装検討
