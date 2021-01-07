#!/bin/bash

BUILD_TYPE="Release"
ANDROID_ABI="armeabi-v7a with NEON"

#Specify Android NDK path if needed
#ANDROID_NDK=

#Specify cmake if needed
#CMAKE_PROGRAM=

source ../env_setup.sh

${CMAKE_PROGRAM} -DCMAKE_TOOLCHAIN_FILE=${TOOLCHAIN_FILE}                   \
      -DCMAKE_BUILD_TYPE=${BUILD_TYPE}                                      \
      -DCMAKE_MAKE_PROGRAM=${MAKE_PROGRAM}                                  \
      -DANDROID_FORCE_ARM_BUILD=ON                                          \
      -DANDROID_NDK=${ANDROID_NDK}                                          \
      -DANDROID_SYSROOT=${PLATFORM}                                         \
      -DANDROID_ABI=${ANDROID_ABI}                                          \
      -DANDROID_TOOLCHAIN_NAME=${TOOLCHAIN_NAME}                            \
      -DANDROID_NATIVE_API_LEVEL=${NATIVE_API_LEVEL}                        \
      -DANDROID_STL=system                                                  \
      -DMPP_PROJECT_NAME=mpp                                                \
      -DVPU_PROJECT_NAME=vpu                                                \
      -DHAVE_DRM=ON                                                         \
      ../../../

if [ "${CMAKE_PARALLEL_ENABLE}" = "0" ]; then
    ${CMAKE_PROGRAM} --build .
else
    ${CMAKE_PROGRAM} --build . -j
fi

# ----------------------------------------------------------------------------
# usefull cmake debug flag
# ----------------------------------------------------------------------------
      #-DMPP_NAME="rockchip_mpp"                                             \
      #-DVPU_NAME="rockchip_vpu"                                             \
      #-DHAVE_DRM                                                            \
      #-DCMAKE_BUILD_TYPE=Debug                                              \
      #-DCMAKE_VERBOSE_MAKEFILE=true                                         \
      #--trace                                                               \
      #--debug-output                                                        \

#cmake --build . --clean-first -- V=1

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
