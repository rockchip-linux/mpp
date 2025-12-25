# vim: syntax=cmake

# hal mpeg2 decoder sourse
add_library(hal_mpeg2d_api OBJECT
   hal_m2vd_api.c
   hal_m2vd_vdpu1.c
   hal_m2vd_vdpu2.c
   )
