#!/bin/bash
# Run this from within a bash shell
HOST_IP=`hostname --all-ip-addresses`
if [[ ${HOST_IP} == *"10.10.10.65"* ]] || [[ ${HOST_IP} == *"10.10.10.67"* ]];
then
    ANDROID_NDK=/home/pub/ndk/android-ndk-r10d/
else
	ANDROID_SDK=~/programs/android/sdk
    ANDROID_NDK=${ANDROID_SDK}/ndk-bundle/
fi

rm -rf build lib

CMAKE_PATH=${ANDROID_SDK}/cmake/3.6.3155560/bin

CMAKE=${CMAKE_PATH}/cmake
MAKE=make
ANDROID_PLATFORM="android-24"

${CMAKE} \
	-DCMAKE_BUILD_TYPE=Release \
	-H../../../ \
	-B${PWD}/build \
    -DANDROID_ABI=arm64-v8a \
	-DANDROID_ARM_MODE=arm \
	-DANDROID_PLATFORM=${ANDROID_PLATFORM} \
	-DANDROID_NDK=${ANDROID_NDK} \
	-DCMAKE_TOOLCHAIN_FILE=${ANDROID_NDK}/build/cmake/android.toolchain.cmake \
	-DCMAKE_LIBRARY_OUTPUT_DIRECTORY=${PWD}/lib \
	-G"Unix Makefiles" \
	-DCMAKE_MAKE_PROGRAM=${MAKE} \
	-DANDROID_ALLOW_UNDEFINED_SYMBOLS=TRUE

# ----------------------------------------------------------------------------
# usefull cmake debug flag
# ----------------------------------------------------------------------------
      #-DCMAKE_BUILD_TYPE=Debug                                              \
      #-DCMAKE_VERBOSE_MAKEFILE=true                                         \
      #--trace                                                               \
      #--debug-output                                                        \

#cmake --build . --clean-first -- V=1
cmake --build build

#rename
mv ${PWD}/lib/librockchip_vpu.so ${PWD}/lib/libvpu.so
mv ${PWD}/lib/librockchip_mpp.so ${PWD}/lib/libmpp.so
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


