#!/bin/bash
# Run this from within a bash shell

set +e

MPP_PWD=`pwd`
MPP_TOP=${MPP_PWD}/../../..

# toolchain detection
check_cmd(){
    "$@" >> /dev/null 2>&1
}
check_system_arm_linux_gcc(){
    check_cmd arm-linux-gcc -v
}

check_system_arm_linux_gcc
if [ $? -eq 127 ];then
    MPP_TOOLCHAIN=${MPP_TOP}/../prebuilts/toolschain/usr/bin
    export PATH=$PATH:${MPP_TOOLCHAIN}
fi

# generate Makefile
cmake -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_TOOLCHAIN_FILE=./arm.linux.cross.cmake \
      -DHAVE_DRM=ON \
      -G "Unix Makefiles" \
      ${MPP_TOP}
