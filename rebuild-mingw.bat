@echo off
REM ============================================================================
REM Pico 2W Bluetooth A2DP Audio Receiver - Full Rebuild Script (MinGW版)
REM ============================================================================
REM
REM MSYS2/MinGW環境で実行してください
REM

setlocal enabledelayedexpansion

echo ========================================
echo Pico 2W A2DP Receiver - Full Rebuild (MinGW)
echo ========================================
echo.

REM ============================================================================
REM [1/5] Git Pull
REM ============================================================================
echo [1/5] Pulling latest code from remote repository...

git pull origin claude/add-stereo-effect-01V4gAyTQ5QMrjH6FMZ3aqBm
if errorlevel 1 (
    echo ERROR: Git pull failed
    pause
    exit /b 1
)

echo       Done!
echo.

REM ============================================================================
REM [2/5] Clean
REM ============================================================================
if exist build (
    echo [2/5] Removing old build directory...
    rmdir /s /q build
    if errorlevel 1 (
        echo ERROR: Failed to remove build directory
        pause
        exit /b 1
    )
    echo       Done!
) else (
    echo [2/5] No old build directory found, skipping...
)

echo.

REM ============================================================================
REM [3/5] Create
REM ============================================================================
echo [3/5] Creating new build directory...

mkdir build
if errorlevel 1 (
    echo ERROR: Failed to create build directory
    pause
    exit /b 1
)

echo       Done!
echo.

REM ============================================================================
REM [4/5] CMake
REM ============================================================================
echo [4/5] Running CMake configuration...

cd build
cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release ..
if errorlevel 1 (
    echo ERROR: CMake configuration failed
    cd ..
    pause
    exit /b 1
)

echo       Done!
echo.

REM ============================================================================
REM [5/5] Build
REM ============================================================================
echo [5/5] Building project with MinGW Make...
echo.

mingw32-make -j4
set BUILD_RESULT=%errorlevel%

cd ..

if %BUILD_RESULT% neq 0 (
    echo.
    echo ========================================
    echo BUILD FAILED
    echo ========================================
    pause
    exit /b %BUILD_RESULT%
)

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
