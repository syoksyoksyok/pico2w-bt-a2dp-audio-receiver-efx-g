# ビルドスクリプト説明書

このプロジェクトには、Windows環境でのビルドを簡単にするための4つのスクリプトがあります。

---

## 📋 スクリプト一覧

| ファイル名 | 説明 | 環境 |
|-----------|------|------|
| **rebuild.bat** | 完全自動リビルド（推奨） | Visual Studio |
| **build.bat** | クリーンビルドのみ | Visual Studio |
| **rebuild-mingw.bat** | 完全自動リビルド | MinGW/MSYS2 |
| **build-mingw.bat** | クリーンビルドのみ | MinGW/MSYS2 |

---

## 🔍 詳細比較

| 機能 | rebuild.bat | build.bat | rebuild-mingw.bat | build-mingw.bat |
|------|------------|-----------|------------------|----------------|
| **Git Pull** | ✅ | ❌ | ✅ | ❌ |
| **ビルドフォルダ削除** | ✅ | ✅ | ✅ | ✅ |
| **CMake設定** | ✅ | ✅ | ✅ | ✅ |
| **ビルド実行** | ✅ | ✅ | ✅ | ✅ |
| **RAM使用量チェック** | ✅ | ❌ | ❌ | ❌ |
| **エラー詳細表示** | ✅ | ✅ | ✅ | ✅ |
| **書き込み手順表示** | ✅ | ✅ | ❌ | ❌ |
| **並列ビルド** | ❌ (nmake) | ❌ (nmake) | ✅ (-j4) | ✅ (-j4) |

---

## 🎯 どのスクリプトを使うべきか？

### 🥇 rebuild.bat（最推奨）

**こんな人におすすめ：**
- 初めてビルドする人
- 常に最新のコードでビルドしたい人
- RAM使用量が心配な人

**特徴：**
- Git pullから書き込み手順まで全自動
- RAM使用量を自動チェック（520 KB制限）
- エラー時に詳細な解決方法を表示

**使い方：**
```cmd
# Developer Command Prompt for VS 2022
rebuild.bat
```

---

### 🥈 build.bat

**こんな人におすすめ：**
- コードを変更した直後
- Git履歴を変更したくない人
- 速くビルドしたい人

**特徴：**
- Git pullをスキップ
- ローカルのコードでビルド
- rebuild.batより少し速い

**使い方：**
```cmd
# Developer Command Prompt for VS 2022
build.bat
```

---

### 🥉 rebuild-mingw.bat

**こんな人におすすめ：**
- Visual Studioを使いたくない人
- MSYS2/MinGW環境を使っている人

**特徴：**
- MinGW Makefilesを使用
- 並列ビルド（-j4）で高速
- Git pullから自動実行

**使い方：**
```bash
# MSYS2 MinGW64 シェル
./rebuild-mingw.bat
```

---

### build-mingw.bat

**こんな人におすすめ：**
- MinGW環境でクリーンビルドしたい人

**特徴：**
- Git pullなし
- 並列ビルド対応

**使い方：**
```bash
# MSYS2 MinGW64 シェル
./build-mingw.bat
```

---

## 📊 処理フロー

### rebuild.bat の処理フロー

```
[開始]
  ↓
[1/5] Git Pull
  ├─ 成功 → 次へ
  └─ 失敗 → エラー表示 → 終了
  ↓
[2/5] Clean (buildフォルダ削除)
  ├─ 存在する → 削除
  └─ 存在しない → スキップ
  ↓
[3/5] Create (buildフォルダ作成)
  ↓
[4/5] CMake設定
  ├─ NMake Makefiles
  ├─ Release ビルド
  └─ 失敗時 → エラー表示 → 終了
  ↓
[5/5] ビルド (nmake)
  ├─ 成功 → RAM使用量チェック
  └─ 失敗 → エラー表示 → 終了
  ↓
[ビルド成功]
  ├─ 生成ファイル表示
  ├─ RAM使用量表示
  └─ 書き込み手順表示
  ↓
[終了]
```

### build.bat の処理フロー

```
[開始]
  ↓
[1/4] Clean
  ↓
[2/4] Create
  ↓
[3/4] CMake設定
  ↓
[4/4] ビルド
  ↓
[ビルド成功]
  ↓
[終了]
```

---

## 🛠️ カスタマイズ

### 並列ビルド数を変更（MinGW版）

`rebuild-mingw.bat` または `build-mingw.bat` を編集：

```bat
# 変更前
mingw32-make -j4

# 変更後（8並列）
mingw32-make -j8
```

### ビルドタイプを変更

デフォルトは `Release` ですが、デバッグビルドに変更可能：

```bat
# 変更前
cmake -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release ..

# 変更後（Debugビルド）
cmake -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Debug ..
```

---

## ⚙️ 内部で使用されているコマンド

### rebuild.bat が実行するコマンド

```bat
# Git Pull
git pull origin claude/add-stereo-effect-01V4gAyTQ5QMrjH6FMZ3aqBm

# Clean
rmdir /s /q build

# Create
mkdir build

# CMake
cd build
cmake -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release ..

# Build
nmake

# RAM Check
findstr /C:"region 'RAM' overflowed" build\pico2w_bt_a2dp_receiver.elf.map
```

### rebuild-mingw.bat が実行するコマンド

```bash
# Git Pull
git pull origin claude/add-stereo-effect-01V4gAyTQ5QMrjH6FMZ3aqBm

# Clean
rmdir /s /q build

# Create
mkdir build

# CMake
cd build
cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release ..

# Build (並列)
mingw32-make -j4
```

---

## 🔧 トラブルシューティング

### スクリプトが見つからない

**症状：**
```
'rebuild.bat' is not recognized as an internal or external command
```

**解決方法：**
プロジェクトのルートディレクトリにいることを確認
```cmd
cd C:\Users\...\pico2w-bt-a2dp-audio-receiver-efx-g
dir *.bat
```

### Git Pullでエラー

**症状：**
```
ERROR: Git pull failed
```

**解決方法：**
1. インターネット接続を確認
2. 手動で `git pull` を試す
3. `build.bat` を使用（Git pullなし）

### CMakeでエラー

**症状：**
```
ERROR: CMake configuration failed
```

**解決方法：**
1. `PICO_SDK_PATH` 環境変数を確認
2. コマンドプロンプトを再起動
3. Pico SDKが正しくインストールされているか確認

### ビルドでエラー

**症状：**
```
BUILD FAILED (Exit Code: 2)
```

**解決方法：**
1. コンパイルエラーメッセージを確認
2. ソースコードの構文エラーをチェック
3. GitHub Issuesに報告

---

## 📝 よくある質問

### Q1: rebuild.batとbuild.batの違いは？

**A:** rebuild.batは最新コードを取得（git pull）してからビルド、build.batは既存コードでビルドします。

### Q2: MinGW版とVisual Studio版、どちらが速い？

**A:** MinGW版の方が並列ビルド（-j4）により高速ですが、Visual Studio版の方がエラーメッセージが分かりやすいです。

### Q3: スクリプトを途中で止めるには？

**A:** `Ctrl + C` を押してください。

### Q4: ビルド成功後、もう一度ビルドできる？

**A:** はい、何度でも実行できます。古いビルドは自動的に削除されます。

### Q5: カスタムブランチでビルドしたい

**A:** スクリプト内の以下の行を編集：
```bat
git pull origin claude/add-stereo-effect-01V4gAyTQ5QMrjH6FMZ3aqBm
```
→
```bat
git pull origin YOUR_BRANCH_NAME
```

---

## 📚 関連ドキュメント

- [BUILD_WINDOWS.md](BUILD_WINDOWS.md) - 詳細ビルドガイド
- [QUICKSTART_WINDOWS.md](QUICKSTART_WINDOWS.md) - クイックスタート
- [README.md](README.md) - プロジェクト概要

---

**最終更新:** 2025-11-22
**スクリプトバージョン:** 1.0
