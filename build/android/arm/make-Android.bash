#!/bin/bash
# Run this from within a bash shell
MAKE_PROGRAM=`which make`

ANDROID_NDK="No ndk path found. You should add your ndk path"
TOOLCHAIN_FILE="no toolchain file found, Please fix ndk path"

FOUND_NDK=

# Search r10d ndk
# NOTE: r10d ndk do not have toolchain file
if [ ! ${FOUND_NDK} ]; then
    NDK_R10D_PATHS=(
    /home/pub/ndk/android-ndk-r10d/
    ~/work/android/ndk/android-ndk-r10d/
    )

    for NDK_PATH in ${NDK_R10D_PATHS[@]};
    do
        if [ -d ${NDK_PATH} ]; then
            echo "found r10d ndk ${NDK_PATH}"
            FOUND_NDK=true

            ANDROID_NDK=${NDK_PATH}
            TOOLCHAIN_FILE=../android.toolchain.cmake
            NATIVE_API_LEVEL="android-21"
            TOOLCHAIN_NAME="arm-linux-androideabi-4.8"
        fi
    done
fi

# Search r16b ndk
# NOTE: r16b use ndk toolchain file
if [ ! ${FOUND_NDK} ]; then
    NDK_R16B_PATHS=(
    /home/pub/ndk/android-ndk-r16b/
    ~/work/android/ndk/android-ndk-r16b/
    )

    for NDK_PATH in ${NDK_R16B_PATHS[@]};
    do
        if [ -d ${NDK_PATH} ]; then
            echo "found r16 ndk ${NDK_PATH}"
            FOUND_NDK=true

            ANDROID_NDK=${NDK_PATH}
            TOOLCHAIN_FILE=${ANDROID_NDK}/build/cmake/android.toolchain.cmake
            NATIVE_API_LEVEL="android-27"
            TOOLCHAIN_NAME="arm-linux-androideabi-4.9"
        fi
    done
fi

if [ ! ${FOUND_NDK} ]; then
    echo ${ANDROID_NDK}
    echo ${TOOLCHAIN_FILE}
    exit
fi

PLATFORM=$ANDROID_NDK/platforms/${NATIVE_API_LEVEL}/arch-arm

cmake -DCMAKE_TOOLCHAIN_FILE=${TOOLCHAIN_FILE}                              \
      -DCMAKE_BUILD_TYPE=Release                                            \
      -DCMAKE_MAKE_PROGRAM=${MAKE_PROGRAM}                                  \
      -DANDROID_FORCE_ARM_BUILD=ON                                          \
      -DANDROID_NDK=${ANDROID_NDK}                                          \
      -DANDROID_SYSROOT=${PLATFORM}                                         \
      -DANDROID_ABI="armeabi-v7a with NEON"                                 \
      -DANDROID_TOOLCHAIN_NAME=${TOOLCHAIN_NAME}                            \
      -DANDROID_NATIVE_API_LEVEL=${NATIVE_API_LEVEL}                        \
      -DANDROID_STL=system                                                  \
      -DMPP_PROJECT_NAME=mpp                                                \
      -DVPU_PROJECT_NAME=vpu                                                \
      -DHAVE_DRM=ON                                                         \
      ../../../

cmake --build .

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
