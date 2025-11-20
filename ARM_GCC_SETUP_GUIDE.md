# ARM GCC ツールチェーン セットアップガイド

## 問題

Arduino の ARM GCC ツールチェーン（14.3.0）を使用すると、newlib のロック関数に関するリンカーエラーが発生します：

```
undefined reference to `__retarget_lock_acquire_recursive'
undefined reference to `__lock___malloc_recursive_mutex'
undefined reference to `__retarget_lock_release_recursive'
```

## 解決策：公式 ARM GCC のインストール

### 1. ARM GCC ツールチェーンのダウンロード

公式サイトから最新の ARM GCC をダウンロードします：

**ダウンロードURL：**
https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads

**推奨バージョン：**
- `arm-gnu-toolchain-13.2.rel1-mingw-w64-i686-arm-none-eabi.zip` (Windows用)

または

- ARM GCC 12.3.1 (Pico SDK 公式推奨バージョン)

### 2. 解凍とインストール

1. ダウンロードした ZIP ファイルを解凍
2. 例えば `C:\Program Files\ARM_GCC` に配置
3. 完全パス例：
   ```
   C:\Program Files\ARM_GCC\arm-gnu-toolchain-13.2.Rel1-mingw-w64-i686-arm-none-eabi\bin
   ```

### 3. 環境変数 PATH に追加

1. **Windowsの設定** → **システム** → **システムの詳細設定**
2. **環境変数** ボタンをクリック
3. **システム環境変数** の **Path** を選択して **編集**
4. **新規** をクリックして ARM GCC の `bin` フォルダのパスを追加：
   ```
   C:\Program Files\ARM_GCC\arm-gnu-toolchain-13.2.Rel1-mingw-w64-i686-arm-none-eabi\bin
   ```
5. **OK** で閉じる

### 4. 確認

コマンドプロンプトを **新しく開いて** 以下を実行：

```cmd
arm-none-eabi-gcc --version
```

以下のような出力が表示されればOK：

```
arm-none-eabi-gcc (Arm GNU Toolchain 13.2.Rel1 ...) 13.2.1 20231009
```

### 5. CMake キャッシュをクリア

古いツールチェーン情報をクリアします：

```cmd
cd C:\pico2w-bt-a2dp-audio-receiver-efx-g
rmdir /s /q build
mkdir build
```

### 6. 再ビルド

```cmd
cd build
cmake -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release ..
nmake
```

## Arduino ARM GCC を使い続ける場合の回避策（非推奨）

もし Arduino の GCC を使い続けたい場合は、ロック関数のスタブ実装を追加する必要がありますが、**公式 ARM GCC の使用を強く推奨します**。

## トラブルシューティング

### エラー: arm-none-eabi-gcc が見つからない

- PATH が正しく設定されているか確認
- コマンドプロンプトを **再起動** する
- `where arm-none-eabi-gcc` で実際に検出されるパスを確認

### 複数の arm-none-eabi-gcc が見つかる場合

```cmd
where arm-none-eabi-gcc
```

出力例：
```
C:\Program Files\ARM_GCC\...\bin\arm-none-eabi-gcc.exe
C:\Users\Administrator\AppData\Local\Arduino15\...\arm-none-eabi-gcc.exe
```

この場合、Arduino のパスを PATH から削除するか、公式 ARM GCC のパスを Arduino より上に配置してください。

### CMake が古い GCC を検出する場合

CMake キャッシュを完全削除：

```cmd
cd C:\pico2w-bt-a2dp-audio-receiver-efx-g
rmdir /s /q build
```

または CMake に明示的に指定：

```cmd
cmake -G "NMake Makefiles" ^
  -DCMAKE_C_COMPILER="C:/Program Files/ARM_GCC/arm-gnu-toolchain-13.2.Rel1-mingw-w64-i686-arm-none-eabi/bin/arm-none-eabi-gcc.exe" ^
  -DCMAKE_CXX_COMPILER="C:/Program Files/ARM_GCC/arm-gnu-toolchain-13.2.Rel1-mingw-w64-i686-arm-none-eabi/bin/arm-none-eabi-g++.exe" ^
  -DCMAKE_BUILD_TYPE=Release ..
```

## 参考

- [ARM GCC 公式ダウンロード](https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads)
- [Pico SDK ドキュメント](https://www.raspberrypi.com/documentation/microcontrollers/c_sdk.html)
