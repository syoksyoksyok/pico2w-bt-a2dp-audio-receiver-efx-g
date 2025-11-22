@echo off
REM ============================================================================
REM Pico 2W Bluetooth A2DP Audio Receiver - Clean Build Script (MinGW版)
REM ============================================================================
REM
REM このスクリプトはMSYS2/MinGW環境で実行してください
REM

echo ========================================
echo Pico 2W A2DP Receiver - Clean Build (MinGW)
echo ========================================
echo.

REM 古いbuildディレクトリを削除
if exist build (
    echo [1/4] Removing old build directory...
    rmdir /s /q build
    echo       Done!
) else (
    echo [1/4] No old build directory found, skipping...
)

echo.
echo [2/4] Creating new build directory...
mkdir build
cd build

echo.
echo [3/4] Running CMake configuration...
cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release ..
if errorlevel 1 (
    echo ERROR: CMake configuration failed
    cd ..
    pause
    exit /b 1
)

echo.
echo [4/4] Building project with MinGW Make...
mingw32-make -j4
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
echo   - build\pico2w_bt_a2dp_receiver.uf2
echo   - build\pico2w_bt_a2dp_receiver.elf
echo   - build\pico2w_bt_a2dp_receiver.bin
echo.

pause
