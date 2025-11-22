# Windows クイックスタートガイド ⚡

最速でビルドして動かすための5ステップガイド

---

## ⚙️ 事前準備（初回のみ）

### 1. Visual Studio 2022 をインストール
- [Visual Studio 2022 Community](https://visualstudio.microsoft.com/ja/downloads/) （無料）
- インストール時に「**C++によるデスクトップ開発**」を選択

### 2. Pico SDK をインストール

```cmd
cd C:\
git clone https://github.com/raspberrypi/pico-sdk.git pico\pico-sdk
cd pico\pico-sdk
git submodule update --init
```

### 3. 環境変数を設定

1. Windowsキー + `sysdm.cpl` で「システムのプロパティ」を開く
2. 「詳細設定」→「環境変数」
3. 「システム環境変数」→「新規」
4. **変数名:** `PICO_SDK_PATH`
5. **値:** `C:\pico\pico-sdk`
6. 「OK」で保存

### 4. コマンドプロンプトを再起動
環境変数を反映させるため、開いているコマンドプロンプトを全て閉じて再起動

---

## 🚀 ビルド（5ステップ）

### Step 1: Developer Command Prompt を開く
スタートメニュー → **Visual Studio 2022** → **Developer Command Prompt for VS 2022**

### Step 2: プロジェクトフォルダに移動
```cmd
cd C:\Users\Administrator\Documents\Arduino\Raspberry_Pi_Pico_2_W\pico2w-bt-a2dp-audio-receiver-efx-g
```
（実際のパスは適宜変更してください）

### Step 3: ビルド実行
```cmd
rebuild.bat
```

### Step 4: 完了を待つ
```
[1/5] Pulling latest code...
[2/5] Removing old build...
[3/5] Creating build directory...
[4/5] Running CMake...
[5/5] Building...

BUILD SUCCESSFUL!
```

### Step 5: 確認
`build\pico2w_bt_a2dp_receiver.uf2` が生成されていればOK！

---

## 📤 Pico 2W に書き込み

### 簡単3ステップ

1. **BOOTSELボタンを押しながらUSB接続**
   ```
   Pico 2W の BOOTSEL ボタンを押したまま
   ↓
   USBケーブルを接続
   ↓
   BOOTSEL ボタンを離す
   ```

2. **RPI-RP2 ドライブが表示される**
   エクスプローラーで確認

3. **UF2ファイルをドラッグ&ドロップ**
   ```
   build\pico2w_bt_a2dp_receiver.uf2
   ↓
   RPI-RP2 ドライブにコピー
   ```

自動的に再起動して完了！

---

## 🎵 動作確認

### Bluetooth接続

1. スマホ/PCのBluetooth設定を開く
2. 「Pico BT Speaker」（または類似の名前）を探す
3. ペアリング
4. 音楽を再生

音が聞こえればOK！

---

## 📝 コマンド早見表

| やりたいこと | コマンド |
|------------|---------|
| 最新コードでビルド | `rebuild.bat` |
| 既存コードでビルド | `build.bat` |
| ビルドフォルダを削除 | `rmdir /s /q build` |
| Git状態確認 | `git status` |

---

## ⚠️ トラブルシューティング

### ❌ "CMake configuration failed"
→ `PICO_SDK_PATH` 環境変数を設定後、コマンドプロンプトを**再起動**

### ❌ "nmake is not recognized"
→ 通常のコマンドプロンプトではなく **Developer Command Prompt for VS 2022** を使用

### ❌ "Git pull failed"
→ インターネット接続を確認、または `build.bat` を使用

### ❌ "region 'RAM' overflowed"
→ RAM使用量オーバー。GitHub Issuesに報告してください

---

## 💡 Tips

### 速くビルドしたい
- 初回のみ時間がかかります（5～10分）
- 2回目以降は1～3分で完了

### コードを変更したら
`rebuild.bat` を実行するだけでOK

### 古いビルドを削除したい
```cmd
rmdir /s /q build
```

### ビルド済みファイルの場所
```
build\
├── pico2w_bt_a2dp_receiver.uf2  ← Pico 2Wに書き込むファイル
├── pico2w_bt_a2dp_receiver.elf  ← デバッグ用
├── pico2w_bt_a2dp_receiver.bin  ← バイナリ
└── pico2w_bt_a2dp_receiver.elf.map  ← メモリマップ
```

---

## 📚 詳細ドキュメント

もっと詳しく知りたい場合：
- [BUILD_WINDOWS.md](BUILD_WINDOWS.md) - 詳細ビルドガイド
- [README.md](README.md) - プロジェクト概要

---

**以上！** 何か問題があれば GitHub Issues へ報告してください。

Happy Hacking! 🎉
