#!/bin/bash
# Run this from within a bash shell
cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=debug \
    -DCMAKE_TOOLCHAIN_FILE=./arm.linux.cross.cmake \
    -DCMAKE_RKPLATFORM_ENABLE=ON                   \
    ../../../  && cmake --build .
