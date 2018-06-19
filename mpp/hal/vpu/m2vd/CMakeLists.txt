#vim: syntax=cmake
include_directories(.)

#hal m2v decoder api
set(HAL_M2VD_API
   ../../inc/hal_m2vd_api.h
   ../../../common/m2vd_syntax.h
   )

#hal m2v header
set(HAL_M2VD_HDR
   hal_m2vd_base.h
   hal_m2vd_vdpu1_reg.h
   hal_m2vd_vdpu2_reg.h
   hal_m2vd_vpu1.h
   hal_m2vd_vpu2.h
   )

#hal m2v decoder sourse
set(HAL_M2VD_SRC
   hal_m2vd_api.c
   hal_m2vd_vdpu1.c
   hal_m2vd_vdpu2.c
   )

add_library(${HAL_MPEG2D} STATIC
   ${HAL_M2VD_API}
   ${HAL_M2VD_HDR}
   ${HAL_M2VD_SRC}
   )

target_link_libraries(${HAL_MPEG2D} mpp_base)
set_target_properties(${HAL_MPEG2D} PROPERTIES FOLDER "mpp/hal")

