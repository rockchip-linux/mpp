#!/bin/bash
adb remount
adb push ./mpp/libmpp.so /system/lib/
adb push ./mpp/legacy/libvpu.so /system/lib/
#adb push librk_vpuapi.so /system/lib/

adb shell sync

adb shell busybox pkill mediaserver

