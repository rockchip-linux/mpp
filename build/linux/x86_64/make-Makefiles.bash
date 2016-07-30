#!/bin/bash
# Run this from within a bash shell
cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=debug -DCMAKE_RKPLATFORM_ENABLE=ON ../../../  && ccmake  ../../../
