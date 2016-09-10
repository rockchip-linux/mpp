# vim: syntax=cmake
# hal h265 reg

set(HAL_H265D_HDR
    hal_h265d_reg.h
    cabac.h
    )

set(HAL_H265D_SRC
    hal_h265d_reg.c
    )

add_library(${HAL_H265D} STATIC
    ${HAL_H265D_SRC}
    ${HAL_H265D_HDR}
    )

set_target_properties(${HAL_H265D} PROPERTIES FOLDER "mpp/hal")
target_link_libraries(${HAL_H265D} mpp_base)

#add_subdirectory(test)
