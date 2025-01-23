#!/bin/bash
# Run this from within a bash shell

set +e

MPP_PWD=`pwd`
MPP_TOP=${MPP_PWD}/../../..
DEFAULT_TOOLCHAIN=arm-linux-gnueabi-

# toolchain detection
check_cmd(){
    "$@" >> /dev/null 2>&1
}
check_gcc(){
    check_cmd ${TOOLCHAIN}gcc -v
}

source ../opt_proc.sh

if [ -z "${TOOLCHAIN}" ]; then
    echo "Using system ${DEFAULT_TOOLCHAIN} as toolchain."
    TOOLCHAIN=$DEFAULT_TOOLCHAIN
fi

check_gcc

if [ $? -eq 127 ];then
    echo -e "\e[31m${TOOLCHAIN}gcc is not found!\e[0m"
    echo -e "Please specify valid toolchain path and it's prefix to variable 'TOOLCHAIN' with argument --toolchain."
    echo -e "For example:\n \t./make-Makefiles.bin --toolchain /path-to-toolchain/arm-linux-gnueabi-"
    exit 1
fi

# generate Makefile
cmake -DCMAKE_BUILD_TYPE=Release \
      -DTOOLCHAIN=${TOOLCHAIN} \
      -DCMAKE_TOOLCHAIN_FILE=./arm.linux.cross.cmake \
      -DHAVE_DRM=ON \
      -G "Unix Makefiles" \
      ${MPP_TOP}

if [ "${CMAKE_PARALLEL_ENABLE}" = "0" ]; then
    cmake --build .
else
    cmake --build . -j
fi
