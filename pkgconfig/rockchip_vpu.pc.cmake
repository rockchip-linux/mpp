prefix=/usr
exec_prefix=${prefix}
libdir=${prefix}/@CMAKE_INSTALL_LIBDIR@
includedir=${prefix}/@CMAKE_INSTALL_INCLUDEDIR@

Name: rockchip_vpu
Description: Rockchip Media Process Platform (Legacy)
Requires.private:
Version: 0.3.0
Libs: -L${libdir} -lrockchip_vpu
Libs.private:
Cflags: -I${includedir}
