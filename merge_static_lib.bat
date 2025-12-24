@echo off
REM Windows version of merge_static_lib.sh
REM Merges multiple static library files into one
REM Usage: merge_static_lib.bat <build_dir> <lib_name>

setlocal enabledelayedexpansion

set BUILD_DIR=%~1
set LIB_NAME=%~2

echo merge_static_lib.bat: BUILD_DIR=%BUILD_DIR% LIB_NAME=%LIB_NAME%

cd /d "%BUILD_DIR%"
if errorlevel 1 exit /b 1

REM Remove old merged library
if exist "mpp\lib%LIB_NAME%.a" (
    echo Removing old library: mpp\lib%LIB_NAME%.a
    del /F /Q "mpp\lib%LIB_NAME%.a"
)

REM Find ar.exe in Android NDK
set AR_CMD=

REM Check environment variable first
if defined ANDROID_NDK (
    set "TEST_PATH=%ANDROID_NDK%\toolchains\llvm\prebuilt\windows-x86_64\bin\llvm-ar.exe"
    if exist "!TEST_PATH!" (
        set "AR_CMD=!TEST_PATH!"
        echo Found ar via ANDROID_NDK
    )
)

REM If not found, try common paths
if "%AR_CMD%"=="" (
    echo Searching for ar.exe in D:\android-sdk-windows\ndk\...
    for /d %%d in (D:\android-sdk-windows\ndk\*) do (
        set "TEST_PATH=%%d\toolchains\llvm\prebuilt\windows-x86_64\bin\llvm-ar.exe"
        if exist "!TEST_PATH!" (
            set "AR_CMD=!TEST_PATH!"
            echo Found ar: !AR_CMD!
        )
    )
)

REM Fallback to ar in PATH
if "%AR_CMD%"=="" (
    set AR_CMD=ar
    echo Using ar from PATH
)

echo Using ar: %AR_CMD%

REM Create MRI script for ar command
set MRI_SCRIPT=%TEMP%\merge_lib_%RANDOM%.mri

echo CREATE mpp\lib%LIB_NAME%.a > "%MRI_SCRIPT%"

REM Find all .a files and add them to the script
REM Skip files in mpp subdirectory
for /r %%f in (*.a) do (
    set "filepath=%%f"
    set "skip=0"

    REM Check if file is in mpp directory
    echo !filepath! | findstr /C:"\mpp\\" >nul
    if not errorlevel 1 (
        set "skip=1"
    )

    if "!skip!"=="0" (
        echo ADDLIB %%f >> "%MRI_SCRIPT%"
    )
)

echo SAVE >> "%MRI_SCRIPT%"
echo END >> "%MRI_SCRIPT%"

REM Display script for debugging
echo MRI Script contents:
type "%MRI_SCRIPT%"
echo.

REM Execute ar with the MRI script
type "%MRI_SCRIPT%" | "%AR_CMD%" -M

if errorlevel 1 (
    echo [ERROR] ar command failed!
    del /F /Q "%MRI_SCRIPT%" 2>nul
    exit /b 1
)

REM Clean up
del /F /Q "%MRI_SCRIPT%" 2>nul

echo Successfully merged libraries into mpp\lib%LIB_NAME%.a

endlocal
exit /b 0
