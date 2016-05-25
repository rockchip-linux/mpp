# vim: syntax=cmake
# hal h265 reg

set(HAL_H265D_HDR
    hal_h265d_reg.h
    cabac.h
    )

set(HAL_H265D_SRC
    hal_h265d_reg.c
    )

add_library(hal_h265d STATIC
    ${HAL_H265D_SRC} ${HAL_H265D_HDR}
    )

set_target_properties(hal_h265d PROPERTIES FOLDER "mpp/hal")

if(WIN32)
    target_link_libraries(hal_h265d mpp_base)
else()
    target_link_libraries(hal_h265d vpu mpp_base)
endif()

#add_subdirectory(test)
