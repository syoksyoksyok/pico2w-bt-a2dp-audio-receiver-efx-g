@echo off
REM ============================================================================
REM Pico 2W Bluetooth A2DP Audio Receiver - Full Rebuild Script
REM ============================================================================
REM
REM このスクリプトは以下の処理を行います：
REM   1. Git pull で最新のコードを取得
REM   2. 古いビルドディレクトリを削除
REM   3. CMake設定
REM   4. ビルド実行
REM
REM 「Developer Command Prompt for VS 2022」から実行してください
REM

setlocal enabledelayedexpansion

echo ========================================
echo Pico 2W A2DP Receiver - Full Rebuild
echo ========================================
echo.

REM ============================================================================
REM [1/5] Git Pull - 最新のコードを取得
REM ============================================================================
echo [1/5] Pulling latest code from remote repository...

git pull origin claude/add-stereo-effect-01V4gAyTQ5QMrjH6FMZ3aqBm
if errorlevel 1 (
    echo.
    echo ========================================
    echo ERROR: Git pull failed
    echo ========================================
    echo.
    echo Please check:
    echo   - Network connection
    echo   - Git repository status
    echo   - Branch name
    echo.
    pause
    exit /b 1
)

echo       Done! Latest code pulled successfully.
echo.

REM ============================================================================
REM [2/5] Clean - 古いビルドディレクトリを削除
REM ============================================================================
if exist build (
    echo [2/5] Removing old build directory...
    rmdir /s /q build
    if errorlevel 1 (
        echo.
        echo ========================================
        echo ERROR: Failed to remove build directory
        echo ========================================
        echo.
        echo Please check:
        echo   - Build directory is not locked
        echo   - No programs are using files in build/
        echo.
        pause
        exit /b 1
    )
    echo       Done! Old build directory removed.
) else (
    echo [2/5] No old build directory found, skipping...
)

echo.

REM ============================================================================
REM [3/5] Create - 新しいビルドディレクトリを作成
REM ============================================================================
echo [3/5] Creating new build directory...

mkdir build
if errorlevel 1 (
    echo.
    echo ========================================
    echo ERROR: Failed to create build directory
    echo ========================================
    echo.
    pause
    exit /b 1
)

echo       Done! Build directory created.
echo.

REM ============================================================================
REM [4/5] CMake - プロジェクト設定
REM ============================================================================
echo [4/5] Running CMake configuration...

cd build
cmake -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release ..
if errorlevel 1 (
    echo.
    echo ========================================
    echo ERROR: CMake configuration failed
    echo ========================================
    echo.
    echo Please check:
    echo   - PICO_SDK_PATH environment variable
    echo   - CMakeLists.txt syntax
    echo   - CMake is installed correctly
    echo.
    cd ..
    pause
    exit /b 1
)

echo       Done! CMake configuration completed.
echo.

REM ============================================================================
REM [5/5] Build - プロジェクトをビルド
REM ============================================================================
echo [5/5] Building project with NMake...
echo.

nmake
set BUILD_RESULT=%errorlevel%

cd ..

if %BUILD_RESULT% neq 0 (
    echo.
    echo ========================================
    echo BUILD FAILED (Exit Code: %BUILD_RESULT%)
    echo ========================================
    echo.
    echo Please check:
    echo   - Compiler errors in the output above
    echo   - Source code syntax
    echo   - Missing dependencies
    echo.
    pause
    exit /b %BUILD_RESULT%
)

REM ============================================================================
REM ビルド成功
REM ============================================================================
echo.
echo ========================================
echo BUILD SUCCESSFUL!
echo ========================================
echo.
echo Generated files:
echo   [UF2] build\pico2w_bt_a2dp_receiver.uf2  ^<-- Upload to Pico 2W
echo   [ELF] build\pico2w_bt_a2dp_receiver.elf  (Debug file)
echo   [BIN] build\pico2w_bt_a2dp_receiver.bin  (Binary)
echo   [MAP] build\pico2w_bt_a2dp_receiver.elf.map (Memory map)
echo.

REM ============================================================================
REM メモリ使用量チェック
REM ============================================================================
echo ========================================
echo Memory Usage Check
echo ========================================

if exist build\pico2w_bt_a2dp_receiver.elf.map (
    echo Checking RAM usage...
    findstr /C:"region 'RAM' overflowed" build\pico2w_bt_a2dp_receiver.elf.map >nul 2>&1
    if errorlevel 1 (
        echo [OK] RAM usage is within RP2350 limit (520 KB)
    ) else (
        echo [WARNING] RAM overflow detected! Check build\pico2w_bt_a2dp_receiver.elf.map
    )
) else (
    echo Memory map file not found, skipping RAM check...
)

echo.

REM ============================================================================
REM 書き込み手順の表示
REM ============================================================================
echo ========================================
echo Upload to Pico 2W:
echo ========================================
echo.
echo 1. Hold BOOTSEL button on Pico 2W
echo 2. Connect USB cable while holding BOOTSEL
echo 3. Release BOOTSEL button
echo 4. Pico 2W appears as "RPI-RP2" drive
echo 5. Copy: build\pico2w_bt_a2dp_receiver.uf2
echo    To:   RPI-RP2 drive
echo 6. Pico 2W will automatically reboot
echo.

pause
