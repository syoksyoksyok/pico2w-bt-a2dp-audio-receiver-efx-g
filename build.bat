@echo off
REM ============================================================================
REM Pico 2W Bluetooth A2DP Audio Receiver - Clean Build Script
REM ============================================================================
REM
REM このスクリプトは「Developer Command Prompt for VS 2022」から実行してください
REM または、Visual Studio 2022がインストールされている環境で実行してください
REM

echo ========================================
echo Pico 2W A2DP Receiver - Clean Build
echo ========================================
echo.

REM 古いbuildディレクトリを削除
if exist build (
    echo [1/4] Removing old build directory...
    rmdir /s /q build
    if errorlevel 1 (
        echo ERROR: Failed to remove build directory
        pause
        exit /b 1
    )
    echo       Done!
) else (
    echo [1/4] No old build directory found, skipping...
)

echo.
echo [2/4] Creating new build directory...
mkdir build
if errorlevel 1 (
    echo ERROR: Failed to create build directory
    pause
    exit /b 1
)
echo       Done!

echo.
echo [3/4] Running CMake configuration...
cd build
cmake -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release ..
if errorlevel 1 (
    echo ERROR: CMake configuration failed
    cd ..
    pause
    exit /b 1
)
echo       Done!

echo.
echo [4/4] Building project with NMake...
nmake
if errorlevel 1 (
    echo.
    echo ========================================
    echo BUILD FAILED
    echo ========================================
    cd ..
    pause
    exit /b 1
)

cd ..

echo.
echo ========================================
echo BUILD SUCCESSFUL!
echo ========================================
echo.
echo Generated files:
echo   - build\pico2w_bt_a2dp_receiver.uf2  (Upload to Pico 2W)
echo   - build\pico2w_bt_a2dp_receiver.elf  (Debug file)
echo   - build\pico2w_bt_a2dp_receiver.bin  (Binary)
echo.
echo To upload to Pico 2W:
echo   1. Hold BOOTSEL button while connecting USB
echo   2. Copy build\pico2w_bt_a2dp_receiver.uf2 to RPI-RP2 drive
echo.

pause
