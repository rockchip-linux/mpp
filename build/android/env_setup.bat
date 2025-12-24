@echo off
REM Windows environment setup for Android MPP build
REM This script reads configuration from environment variables

setlocal enabledelayedexpansion

echo ========================================
echo Environment Setup Script
echo ========================================

REM Default build type
if not defined BUILD_TYPE set BUILD_TYPE=Release

REM Detect cmake
echo Checking cmake...
if not defined CMAKE_PROGRAM (
    where cmake >nul 2>&1
    if %errorlevel% equ 0 (
        set CMAKE_PROGRAM=cmake
        echo Found cmake in PATH
    ) else (
        REM Search in Android SDK cmake directory
        echo Searching cmake in Android SDK...

        REM Extract SDK root from ANDROID_NDK if possible
        set SDK_ROOT=
        if defined ANDROID_NDK (
            for %%A in ("%ANDROID_NDK%") do set SDK_ROOT=%%~dpA
            if "!SDK_ROOT:~-1!"=="\" set SDK_ROOT=!SDK_ROOT:~0,-1!

            echo SDK root candidate: !SDK_ROOT!
        )

        REM Look for cmake in SDK
        set CMAKE_PROGRAM=
        if defined SDK_ROOT (
            if exist "!SDK_ROOT!\cmake" (
                for /d %%D in ("!SDK_ROOT!\cmake\*") do (
                    if not defined CMAKE_PROGRAM (
                        if exist "%%D\bin\cmake.exe" (
                            set CMAKE_PROGRAM=%%D\bin\cmake.exe
                            echo Found cmake at !CMAKE_PROGRAM!
                        )
                    )
                )
            )
        )

        REM Common cmake paths in Android SDK
        if not defined CMAKE_PROGRAM (
            for %%P in (D:\android-sdk-windows\cmake C:\android-sdk\cmake %LOCALAPPDATA%\Android\Sdk\cmake) do (
                if not defined CMAKE_PROGRAM (
                    if exist "%%P" (
                        for /d %%D in ("%%P\*") do (
                            if not defined CMAKE_PROGRAM (
                                if exist "%%D\bin\cmake.exe" (
                                    set CMAKE_PROGRAM=%%D\bin\cmake.exe
                                    echo Found cmake at !CMAKE_PROGRAM!
                                )
                            )
                        )
                    )
                )
            )
        )

        if not defined CMAKE_PROGRAM (
            echo [ERROR] cmake not found in PATH or Android SDK!
            echo Please set CMAKE_PROGRAM environment variable, e.g.:
            echo   set CMAKE_PROGRAM=D:\android-sdk-windows\cmake\3.22.1\bin\cmake.exe
            echo Or add cmake to PATH
            echo Or install cmake from: https://cmake.org/download/
            exit /b 1
        )
    )
)

REM Verify cmake exists
echo Verifying cmake at: %CMAKE_PROGRAM%
if not exist "%CMAKE_PROGRAM%" (
    echo [ERROR] cmake not found at %CMAKE_PROGRAM%
    exit /b 1
)

echo Getting cmake version...
set CMAKE_VERSION=
"%CMAKE_PROGRAM%" --version > "%TEMP%\cmake_version.tmp" 2>&1
for /f "tokens=3" %%i in ('type "%TEMP%\cmake_version.tmp" ^| findstr /i "version"') do set CMAKE_VERSION=%%i
del "%TEMP%\cmake_version.tmp" >nul 2>&1

if not defined CMAKE_VERSION (
    echo [ERROR] Failed to get cmake version from %CMAKE_PROGRAM%
    exit /b 1
)

echo Found cmake version: %CMAKE_VERSION%

REM Parse cmake version
for /f "tokens=1,2 delims=." %%a in ("%CMAKE_VERSION%") do (
    set CMAKE_MAJOR_VERSION=%%a
    set CMAKE_MINOR_VERSION=%%b
)

echo CMake major version: %CMAKE_MAJOR_VERSION%, minor version: %CMAKE_MINOR_VERSION%

set CMAKE_PARALLEL_ENABLE=0
if defined CMAKE_MAJOR_VERSION (
    if %CMAKE_MAJOR_VERSION% geq 3 (
        if defined CMAKE_MINOR_VERSION (
            if %CMAKE_MINOR_VERSION% geq 12 (
                set CMAKE_PARALLEL_ENABLE=1
                echo use cmake parallel build.
            )
        )
    )
)

REM Detect NDK path
echo Checking NDK...
if not defined ANDROID_NDK (
    echo [ERROR] ANDROID_NDK environment variable not set!
    echo Please set ANDROID_NDK environment variable, e.g.:
    echo   set ANDROID_NDK=D:\android-sdk-windows\ndk\26.3.11579264
    exit /b 1
)

REM Remove quotes from ANDROID_NDK if present
set ANDROID_NDK=!ANDROID_NDK:"=!

if not exist "%ANDROID_NDK%" (
    echo [ERROR] Specified NDK path does not exist: %ANDROID_NDK%
    exit /b 1
)

echo Using NDK: %ANDROID_NDK%

REM Detect NDK version
echo Detecting NDK version...
set NDK_VERSION=0
set SOURCE_PROPS=%ANDROID_NDK%\source.properties
if exist "%SOURCE_PROPS%" (
    echo Found source.properties
    REM NDK version is greater than 10
    REM Format: Pkg.Revision = 26.3.11579264
    for /f "tokens=1,2,3" %%a in ('type "%SOURCE_PROPS%" 2^>nul ^| findstr /C:"Pkg.Revision"') do (
        set NDK_VERSION_RAW=%%c
    )
    if defined NDK_VERSION_RAW (
        for /f "tokens=1 delims=." %%a in ("%NDK_VERSION_RAW%") do set NDK_VERSION=%%a
        echo Detected NDK version: %NDK_VERSION%
    )
) else if exist "%ANDROID_NDK%\RELEASE.TXT" (
    echo Found RELEASE.TXT
    REM NDK version is less than 11
    for /f "tokens=1" %%a in ('type "%ANDROID_NDK%\RELEASE.TXT" 2^>nul ^| findstr /R "^r[0-9]"') do set NDK_LINE=%%a
    set NDK_VERSION=%NDK_LINE:~1%
    set NDK_VERSION=%NDK_VERSION:a=%
    set NDK_VERSION=%NDK_VERSION:b=%
    set NDK_VERSION=%NDK_VERSION:c=%
    set NDK_VERSION=%NDK_VERSION:d=%
    set NDK_VERSION=%NDK_VERSION:e=%
    set NDK_VERSION=%NDK_VERSION:f=%
)

REM Check if NDK_VERSION is a valid number
if "%NDK_VERSION%"=="0" (
    set NDK_VERSION=26
    echo [WARNING] Could not detect NDK version, assuming 26+
)

echo NDK version: %NDK_VERSION%

REM Configure NDK settings based on version
if %NDK_VERSION% geq 16 (
    set TOOLCHAIN_FILE=%ANDROID_NDK%\build\cmake\android.toolchain.cmake
    set NATIVE_API_LEVEL=android-27
    set ANDROID_STL=c++_static
    set TOOLCHAIN_NAME=
    set PLATFORM=
) else if %NDK_VERSION% gtr 12 (
    set TOOLCHAIN_FILE=%ANDROID_NDK%\build\cmake\android.toolchain.cmake
    set NATIVE_API_LEVEL=android-21
    set ANDROID_STL=system
    set TOOLCHAIN_NAME=
    set PLATFORM=
) else (
    set TOOLCHAIN_FILE=..\..\android.toolchain.cmake
    set NATIVE_API_LEVEL=android-21
    set ANDROID_STL=system
)

echo Configuration:
echo   NDK path: %ANDROID_NDK%
echo   Toolchain file: %TOOLCHAIN_FILE%
echo   API level: %NATIVE_API_LEVEL%
echo   STL: %ANDROID_STL%
echo.

endlocal & (
    set ANDROID_NDK=%ANDROID_NDK%
    set TOOLCHAIN_FILE=%TOOLCHAIN_FILE%
    set TOOLCHAIN_NAME=%TOOLCHAIN_NAME%
    set PLATFORM=%PLATFORM%
    set NATIVE_API_LEVEL=%NATIVE_API_LEVEL%
    set ANDROID_STL=%ANDROID_STL%
    set CMAKE_PROGRAM=%CMAKE_PROGRAM%
    set BUILD_TYPE=%BUILD_TYPE%
    set CMAKE_PARALLEL_ENABLE=%CMAKE_PARALLEL_ENABLE%
)

echo Environment setup completed successfully!
echo.

exit /b 0
