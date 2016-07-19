#!/bin/bash
adb remount
adb push ./mpp/libmpp.so /system/lib/
adb push ./mpp/legacy/libvpu.so /system/lib/
adb push ./test/mpi_dec_test /system/bin/
#adb push librk_vpuapi.so /system/lib/

adb shell sync

adb shell busybox pkill mediaserver

adb shell logcat -c
#adb shell mpi_dec_test -i /sdcard/beauty.mpg -t 4
adb shell logcat -v time > log.txt
#adb shell logcat | grep vpu > log.txt
