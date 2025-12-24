@echo off
REM Windows script to push compiled MPP files to Android device

set BASE_PATH=/vendor
REM Other options:
REM set BASE_PATH=/system
REM set BASE_PATH=/oem/usr

set BIN_PATH=%BASE_PATH%/bin/
set LIB_PATH=%BASE_PATH%/lib/

echo.
echo ========================================
echo Pushing MPP files to Android device...
echo ========================================
echo.

REM Check if adb is available
where adb >nul 2>&1
if %errorlevel% neq 0 (
    echo [ERROR] adb not found in PATH!
    echo Please install Android SDK Platform-tools.
    exit /b 1
)

REM Restart adbd with root permissions
echo Getting root permissions...
adb root
adb remount

REM Function to push file if exists
:push_file
if exist "%~1" (
    echo Pushing %~1 to %~2
    adb push "%~1" %~2
) else (
    echo [WARNING] File not found: %~1
    exit /b 0
)

REM Push library files
echo.
echo Pushing libraries...
call :push_file ".\mpp\libmpp.so" %LIB_PATH%
call :push_file ".\mpp\legacy\libvpu.so" %LIB_PATH%

REM Push test executables
echo.
echo Pushing test executables...
call :push_file ".\test\mpi_dec_test" %BIN_PATH%
call :push_file ".\test\mpi_enc_test" %BIN_PATH%
call :push_file ".\test\vpu_api_test" %BIN_PATH%
call :push_file ".\test\mpp_info_test" %BIN_PATH%

echo.
echo ========================================
echo Push completed!
echo ========================================
echo.
