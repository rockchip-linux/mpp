#!/bin/bash
adb root
adb remount

#BASE_PATH=/system
BASE_PATH=/vendor
#BASE_PATH=/oem/usr

BIN_PATH=${BASE_PATH}/bin/
LIB_PATH=${BASE_PATH}/lib/

push_file() {
    if [ -f $1 ]; then
        adb push $1 $2
    fi
}

push_file ./mpp/libmpp.so ${LIB_PATH}
push_file ./mpp/legacy/libvpu.so ${LIB_PATH}
push_file ./test/mpi_dec_test ${BIN_PATH}
push_file ./test/mpi_enc_test ${BIN_PATH}
push_file ./test/vpu_api_test ${BIN_PATH}
push_file ./test/mpp_info_test ${BIN_PATH}
