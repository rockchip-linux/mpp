#!/bin/bash
# Run this from within a bash shell
HOST_IP=`hostname --all-ip-addresses`
if [ ${HOST_IP} == "10.10.10.65" ] || [ ${HOST_IP} == "10.10.10.67" ]; then
    ANDROID_NDK=/home/pub/ndk/android-ndk-r10d/
else
    ANDROID_NDK=~/work/android/ndk/android-ndk-r10d/
fi

PLATFORM=$ANDROID_NDK/platforms/android-21/arch-arm64

cmake -DCMAKE_TOOLCHAIN_FILE=../android.toolchain.cmake                     \
      -DCMAKE_BUILD_TYPE=Release                                            \
      -DANDROID_FORCE_ARM_BUILD=ON                                          \
      -DANDROID_NDK=${ANDROID_NDK}                                          \
      -DANDROID_SYSROOT=${PLATFORM}                                         \
      -DANDROID_ABI="arm64-v8a"                                             \
      -DANDROID_TOOLCHAIN_NAME="aarch64-linux-android-4.9"                  \
      -DANDROID_NATIVE_API_LEVEL=android-21                                 \
      -DANDROID_STL=system                                                  \
      -DRKPLATFORM=ON                                                       \
      -DHAVE_DRM=ON                                                         \
      ../../../

# ----------------------------------------------------------------------------
# usefull cmake debug flag
# ----------------------------------------------------------------------------
      #-DCMAKE_BUILD_TYPE=Debug                                              \
      #-DCMAKE_VERBOSE_MAKEFILE=true                                         \
      #--trace                                                               \
      #--debug-output                                                        \

#cmake --build . --clean-first -- V=1
cmake --build .

# ----------------------------------------------------------------------------
# test script
# ----------------------------------------------------------------------------
#adb push osal/test/rk_log_test /system/bin/
#adb push osal/test/rk_thread_test /system/bin/
#adb shell sync
#adb shell logcat -c
#adb shell rk_log_test
#adb shell rk_thread_test
#adb logcat -d|tail -30


