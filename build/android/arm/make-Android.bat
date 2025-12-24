@echo off
REM Windows build script for MPP Android ARMv7 (armeabi-v7a with NEON)

setlocal

set BUILD_TYPE=Release
set ANDROID_ABI=armeabi-v7a with NEON

REM Parse command line arguments by position
:parse_loop
if "%~1"=="--ndk" (
    set ANDROID_NDK=%~2
    shift
    shift
    goto parse_loop
)
if "%~1"=="--cmake" (
    set CMAKE_PROGRAM=%~2
    shift
    shift
    goto parse_loop
)
if "%~1"=="--debug" (
    set BUILD_TYPE=Debug
    shift
    goto parse_loop
)
if "%~1"=="-c" (
    cls
    shift
    goto parse_loop
)
if "%~1"=="--rebuild" (
    shift
    goto parse_loop
)
if not "%~1"=="" (
    shift
    goto parse_loop
)

echo Configuration:
echo   ANDROID_NDK=%ANDROID_NDK%
echo   CMAKE_PROGRAM=%CMAKE_PROGRAM%
echo   BUILD_TYPE=%BUILD_TYPE%
echo.

REM Get current directory
set MPP_PWD=%cd%

REM Call environment setup
call ..\env_setup.bat
if %errorlevel% neq 0 (
    echo [ERROR] Environment setup failed!
    exit /b 1
)

REM Run CMake configuration
echo.
echo ========================================
echo Running CMake configuration...
echo ========================================
echo Build Type: %BUILD_TYPE%
echo Android ABI: %ANDROID_ABI%
echo.

"%CMAKE_PROGRAM%" -G Ninja -DCMAKE_TOOLCHAIN_FILE="%TOOLCHAIN_FILE%" -DCMAKE_BUILD_TYPE=%BUILD_TYPE% -DANDROID_FORCE_ARM_BUILD=ON -DANDROID_NDK=%ANDROID_NDK% -DANDROID_ABI=%ANDROID_ABI% -DANDROID_NATIVE_API_LEVEL=%NATIVE_API_LEVEL% -DANDROID_STL=%ANDROID_STL% -DMPP_PROJECT_NAME=mpp -DVPU_PROJECT_NAME=vpu -DHAVE_DRM=ON ..\..\..

if %errorlevel% neq 0 (
    echo [ERROR] CMake configuration failed!
    exit /b 1
)

REM Run build
echo.
echo ========================================
echo Building MPP...
echo ========================================

if "%CMAKE_PARALLEL_ENABLE%"=="0" (
    "%CMAKE_PROGRAM%" --build .
) else (
    "%CMAKE_PROGRAM%" --build . -j
)

if %errorlevel% neq 0 (
    echo [ERROR] Build failed!
    exit /b 1
)

echo.
echo ========================================
echo Build completed successfully!
echo ========================================

endlocal
