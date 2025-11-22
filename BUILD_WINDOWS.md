# Windows ビルドガイド

Raspberry Pi Pico 2W Bluetooth A2DP Audio Receiver プロジェクトをWindows環境でビルドする方法を説明します。

---

## 📋 必要な環境

### 必須
- **Visual Studio 2022** (Community版でOK)
  - 「C++によるデスクトップ開発」ワークロードをインストール
- **CMake** (v3.13以降)
- **Pico SDK** (v1.5.0以降)
- **Git**

### オプション
- **MSYS2/MinGW** (Visual Studioが使えない場合)

---

## 🛠️ Pico SDK セットアップ

### 1. Pico SDK のインストール

```cmd
cd C:\
git clone https://github.com/raspberrypi/pico-sdk.git pico\pico-sdk
cd pico\pico-sdk
git submodule update --init
```

### 2. 環境変数の設定

**システム環境変数に追加：**

| 変数名 | 値 |
|--------|-----|
| `PICO_SDK_PATH` | `C:\pico\pico-sdk` |

**設定方法：**
1. 「Windows設定」→「システム」→「詳細情報」→「システムの詳細設定」
2. 「環境変数」ボタンをクリック
3. 「システム環境変数」→「新規」
4. 変数名: `PICO_SDK_PATH`、値: `C:\pico\pico-sdk`
5. 「OK」で保存

---

## 🚀 ビルドスクリプト一覧

このプロジェクトには4つのビルドスクリプトがあります：

| ファイル名 | 環境 | Git Pull | クリーンビルド | RAM検証 | 推奨度 |
|-----------|------|----------|--------------|---------|--------|
| **rebuild.bat** | Visual Studio | ✅ | ✅ | ✅ | ⭐⭐⭐ 最推奨 |
| **build.bat** | Visual Studio | ❌ | ✅ | ❌ | ⭐⭐ |
| **rebuild-mingw.bat** | MinGW | ✅ | ✅ | ❌ | ⭐ |
| **build-mingw.bat** | MinGW | ❌ | ✅ | ❌ | ⭐ |

---

## 📦 方法1: rebuild.bat（推奨）

### 特徴
- **最新コードを自動取得** (git pull)
- **完全クリーンビルド**
- **RAM使用量自動検証**
- **エラー詳細表示**
- **書き込み手順自動表示**

### 使い方

1. **「Developer Command Prompt for VS 2022」を開く**
   - スタートメニュー → Visual Studio 2022 → Developer Command Prompt for VS 2022

2. **プロジェクトフォルダに移動**
   ```cmd
   cd C:\Users\Administrator\Documents\Arduino\Raspberry_Pi_Pico_2_W\pico2w-bt-a2dp-audio-receiver-efx-g
   ```

3. **スクリプト実行**
   ```cmd
   rebuild.bat
   ```

4. **完了を待つ**
   - [1/5] Git Pull (最新コード取得)
   - [2/5] Clean (古いビルド削除)
   - [3/5] Create (ビルドフォルダ作成)
   - [4/5] CMake (プロジェクト設定)
   - [5/5] Build (ビルド実行)

5. **ビルド成功**
   ```
   ========================================
   BUILD SUCCESSFUL!
   ========================================

   Generated files:
     [UF2] build\pico2w_bt_a2dp_receiver.uf2  <-- Upload to Pico 2W
     [ELF] build\pico2w_bt_a2dp_receiver.elf  (Debug file)
     [BIN] build\pico2w_bt_a2dp_receiver.bin  (Binary)
   ```

---

## 📦 方法2: build.bat

### 特徴
- **Git Pull なし**（既存コードでビルド）
- **クリーンビルドのみ**

### 使い方

```cmd
# Developer Command Prompt for VS 2022 で実行
cd C:\Users\Administrator\Documents\Arduino\Raspberry_Pi_Pico_2_W\pico2w-bt-a2dp-audio-receiver-efx-g
build.bat
```

**使用場面：**
- コード変更後、すぐにビルドしたい時
- Git履歴を変更せずにビルドしたい時

---

## 📦 方法3: rebuild-mingw.bat（MinGW環境）

### 前提条件
MSYS2がインストールされていること

### 使い方

1. **MSYS2 MinGW64 シェルを開く**
2. **プロジェクトフォルダに移動**
   ```bash
   cd /c/Users/Administrator/Documents/Arduino/Raspberry_Pi_Pico_2_W/pico2w-bt-a2dp-audio-receiver-efx-g
   ```
3. **スクリプト実行**
   ```bash
   ./rebuild-mingw.bat
   ```

---

## 🔧 トラブルシューティング

### エラー: "CMake configuration failed"

**原因：** `PICO_SDK_PATH` 環境変数が設定されていない

**解決方法：**
1. 環境変数 `PICO_SDK_PATH` を設定
2. コマンドプロンプトを**再起動**
3. 再度ビルド実行

### エラー: "region 'RAM' overflowed"

**原因：** RAM使用量が520 KBを超えている

**解決方法：**
1. `src/audio_effect.c` のバッファサイズを確認
2. 不要な機能を無効化
3. GitHub Issuesに報告

### エラー: "Git pull failed"

**原因：** ネットワーク接続の問題

**解決方法：**
1. インターネット接続を確認
2. `git status` でリポジトリ状態を確認
3. 手動で `git pull` 実行後、`build.bat` を使用

### エラー: "nmake is not recognized"

**原因：** Developer Command Prompt を使用していない

**解決方法：**
通常のコマンドプロンプトではなく、**Developer Command Prompt for VS 2022** を使用

---

## 📤 Pico 2W への書き込み

### 手順

1. **BOOTSELボタンを押しながらUSB接続**
   - Pico 2WのBOOTSELボタンを押したまま
   - USBケーブルを接続
   - BOOTSELボタンを離す

2. **RPI-RP2 ドライブが表示される**
   - エクスプローラーで「RPI-RP2」ドライブを確認

3. **UF2ファイルをコピー**
   ```
   build\pico2w_bt_a2dp_receiver.uf2
   ```
   を RPI-RP2 ドライブにドラッグ&ドロップ

4. **自動的に再起動**
   - コピー完了後、自動的にPico 2Wが再起動
   - Bluetoothオーディオレシーバーとして動作開始

---

## 📊 メモリ使用量の確認

### rebuild.bat の場合

ビルド完了後、自動的にRAM使用量をチェック：

```
========================================
Memory Usage Check
========================================
Checking RAM usage...
[OK] RAM usage is within RP2350 limit (520 KB)
```

### 手動確認

```cmd
cd build
type pico2w_bt_a2dp_receiver.elf.map | findstr "RAM"
```

---

## 🎯 ビルド時間の目安

| 環境 | 初回ビルド | 2回目以降 |
|------|-----------|----------|
| Visual Studio (NMake) | 5～10分 | 1～3分 |
| MinGW | 3～8分 | 1～2分 |

**注：** 初回はPico SDK、BTstack、TinyUSBなど全ライブラリをビルドするため時間がかかります。

---

## 📝 よくある質問

### Q1: Visual StudioとMinGW、どちらを使うべき？

**A:** Visual Studioを推奨します。
- エラーメッセージが分かりやすい
- RAM使用量の自動検証機能
- Windowsとの互換性が高い

### Q2: ビルド済みのファイルはどこにある？

**A:** `build` フォルダ内：
- `pico2w_bt_a2dp_receiver.uf2` - Pico 2Wに書き込むファイル
- `pico2w_bt_a2dp_receiver.elf` - デバッグ用ELFファイル
- `pico2w_bt_a2dp_receiver.bin` - バイナリファイル
- `pico2w_bt_a2dp_receiver.elf.map` - メモリマップ

### Q3: コードを変更したらどうすればいい？

**A:** 変更後、`rebuild.bat` を実行するだけでOKです。

### Q4: エラーが出たらどうすればいい？

**A:** 以下を確認：
1. エラーメッセージをよく読む
2. 上記「トラブルシューティング」を確認
3. GitHub Issuesで報告

---

## 🔗 関連ドキュメント

- [README.md](README.md) - プロジェクト概要
- [Pico SDK Documentation](https://raspberrypi.github.io/pico-sdk-doxygen/)
- [CMake Documentation](https://cmake.org/documentation/)

---

## 📞 サポート

問題が解決しない場合：
- GitHub Issues: https://github.com/YOUR_USERNAME/pico2w-bt-a2dp-audio-receiver-efx-g/issues
- Pico SDK Forum: https://forums.raspberrypi.com/

---

**最終更新:** 2025-11-22
**対応バージョン:** Visual Studio 2022, Pico SDK v1.5.0+
