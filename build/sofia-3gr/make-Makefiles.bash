#!/bin/bash
# Run this from within a bash shell
cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=debug -DCMAKE_RKPLATFORM_ENABLE=ON -DCMAKE_SOFIA_3GR_LINUX_ENABLE=ON ../../  && ccmake  ../../
