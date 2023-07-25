#!/bin/bash

#################################################
# Arguments
#################################################
while [ $# -gt 0 ]; do
    case $1 in
        --help | -h)
            echo "Execute make-Android.sh in *arm/* or *aarch64/* with some args."
            echo "  use --ndk to set ANDROID_NDK"
            echo "  use --cmake to specify which cmake to use"
            echo "  use --debug to enable debug build"
            exit 1
            ;;
        --debug)
            BUILD_TYPE="Debug"
            ;;
        -B)
            if [ -f "CMakeCache.txt" ]; then
                rm CMakeCache.txt
            fi
            ;;
        --ndk)
            ANDROID_NDK=$2
            shift
            ;;
        --cmake)
            CMAKE_PROGRAM=$2
            shift
            ;;
    esac
    shift
done

CMAKE_PARALLEL_ENABLE=0

# Run this from within a bash shell
MAKE_PROGRAM=`which make`

#################################################
# Detect cmake version
#################################################
if [ -z $CMAKE_PROGRAM ]; then
    CMAKE_PROGRAM=`which cmake`
fi

CMAKE_VERSION=$(${CMAKE_PROGRAM} --version | grep "version" | cut -d " " -f 3)
CMAKE_MAJOR_VERSION=`echo ${CMAKE_VERSION} | cut -d "." -f 1`
CMAKE_MINOR_VERSION=`echo ${CMAKE_VERSION} | cut -d "." -f 2`

if [ -z ${CMAKE_VERSION} ]; then
    echo -e "\e[1;31m cmake in ${CMAKE_PROGRAM} is invalid, please check!\e[0m"
    exit 1
else
    echo "Found cmake in ${CMAKE_PROGRAM}, version: ${CMAKE_VERSION}"
fi

if [ ${CMAKE_MAJOR_VERSION} -ge 3 ] && [ ${CMAKE_MINOR_VERSION} -ge 12 ]; then
    CMAKE_PARALLEL_ENABLE=1
    echo "use cmake parallel build."
fi

#################################################
# Detect ndk path and version
#################################################

NDK_SEARCH_PATH=(
    /home/pub/ndk/
    ~/work/android/ndk/
)

FOUND_NDK=0

if [ -z "$ANDROID_NDK" ]; then
    # try find ndk path in CMakeCache.txt
    if [ -f "CMakeCache.txt" ]; then
        ANDROID_NDK=`grep ANDROID_NDK\: CMakeCache.txt | awk -F '=' '{ print $2 }'`

        if [ -d "${ANDROID_NDK}" ]; then
            echo "use android ndk from CMakeCache.txt : ${ANDROID_NDK}"
            FOUND_NDK=1
        fi
    fi
else
    FOUND_NDK=1
fi

#################################################
# search possible path to get ndk with higher version
#################################################
NDK_OPTION=""
NDK_COUNT=0

if [ "${FOUND_NDK}" = "0" ]; then
    echo "trying to find android ndk in the following paths:"
    for NDK_BASE in ${NDK_SEARCH_PATH[@]};
    do
        echo "${NDK_BASE}"
    done

    echo "find valid android ndk:"

    for NDK_BASE in ${NDK_SEARCH_PATH[@]};
    do
        if [ -d ${NDK_BASE} ]; then
            NDKS=`ls -r -d ${NDK_BASE}android-ndk-r*/`

            for NDK_PATH in ${NDKS[@]};
            do
                if [ -d ${NDK_PATH} ]; then
                    NDK_COUNT=$[${NDK_COUNT}+1]
                    NDK_OPT="${NDK_COUNT} - ${NDK_PATH}"

                    echo ${NDK_OPT}

                    NDK_OPTION+="${NDK_PATH} "
                fi
            done
        fi

        if [ "${FOUND_NDK}" = "1" ]; then
            break
        fi
    done
fi

case ${NDK_COUNT} in
    0)
        ;;
    1)
        ANDROID_NDK=${NDK_PATH[0]}
        FOUND_NDK=1

        echo "use ndk: ${ANDROID_NDK}"
        ;;
    *)
        read -p "select [1-${NDK_COUNT}] ndk used for compiling: " -ra NDK_INTPUT

        NDK_INDEX=0

        for NDK_PATH in ${NDK_OPTION[@]};
        do
            NDK_INDEX=$[${NDK_INDEX}+1]

            if [ "${NDK_INDEX}" -eq "${NDK_INTPUT}" ]; then
                echo "${NDK_INTPUT} - ${NDK_PATH} selected as ANDROID_NDK"
                ANDROID_NDK=${NDK_PATH}
                FOUND_NDK=1
                break
            fi
        done

        if [ $FOUND_NDK -eq 0 ]; then
            echo "invalid input option ${NDK_INTPUT}"
        fi
esac

if [ $FOUND_NDK -eq 0 ]; then
    echo "can not found any valid android ndk"
    exit 1
fi

#################################################
# try to detect NDK version
# for ndk > 10, ndk version is presented at $ANDROID_NDK/source.properties
# for ndk <=10, ndk version is presented at $ANDROID_NDK/RELEASE.TXT
# parameter: ndk path
# return: ndk version, 0 if not found
#################################################
detect_ndk_version()
{
    RET=0

    if [ -f "$1/source.properties" ]; then
        # NDK version is greater than 10
        RET=$(grep -o '^Pkg.Revision.*[0-9]*.*' $ANDROID_NDK/source.properties |cut -d " " -f 3 | cut -d "." -f 1)
    elif [ -f "$1/RELEASE.TXT" ]; then
        # NDK version is less than 11
        RET=$(grep -o '^r[0-9]*.*' $ANDROID_NDK/RELEASE.TXT | cut -d " " -f 1 | cut -b2- | sed 's/[a-z]//g')
    else
        # A correct NDK directory must be pointed
        RET=0
    fi

    echo $RET
}

NDK_VERSION=$(detect_ndk_version ${ANDROID_NDK})

echo "NDK: ${ANDROID_NDK} version: ${NDK_VERSION}"

if [ $NDK_VERSION -eq 0 ]; then
    echo "NDK version isn't detected, please check $ANDROID_NDK"
    FOUND_NDK=0
else
    FOUND_NDK=1

    if [ $NDK_VERSION -ge 16 ]; then
        TOOLCHAIN_FILE=${ANDROID_NDK}/build/cmake/android.toolchain.cmake
        NATIVE_API_LEVEL="android-27"
    elif [ $NDK_VERSION -gt 12 ]; then
        TOOLCHAIN_FILE=${ANDROID_NDK}/build/cmake/android.toolchain.cmake
        NATIVE_API_LEVEL="android-21"
    elif [ $NDK_VERSION -le 12 ]; then
        TOOLCHAIN_FILE=../android.toolchain.cmake
        NATIVE_API_LEVEL="android-21"
    fi

    if [ $NDK_VERSION -lt 18 ]; then
        #################################################
        # Set platform tools
        #################################################
        if [ "${ANDROID_ABI}" = "armeabi-v7a" ] || [ "${ANDROID_ABI}" = "armeabi-v7a with NEON" ]; then
            TOOLCHAIN_NAME="arm-linux-androideabi-4.9"
            PLATFORM=$ANDROID_NDK/platforms/${NATIVE_API_LEVEL}/arch-arm
        elif [ "${ANDROID_ABI}" = "arm64-v8a" ]; then
            TOOLCHAIN_NAME="aarch64-linux-android-4.9"
            PLATFORM=$ANDROID_NDK/platforms/${NATIVE_API_LEVEL}/arch-arm64
        fi
        ANDROID_STL="system"
    else
        # From NDK 18, GCC is deprecated
        TOOLCHAIN_NAME=""
        PLATFORM=""
        ANDROID_STL="c++_static"
    fi
fi

if [ "${FOUND_NDK}" = "0" ]; then
    echo -e "\e[1;31m No ndk path found. You should add your ndk path\e[0m"
    exit 1
else
    echo "ndk path: $ANDROID_NDK"
    echo "toolchain file: $TOOLCHAIN_FILE"
    echo "toolchain name: $TOOLCHAIN_NAME"
    echo "api level: $NATIVE_API_LEVEL"
fi
