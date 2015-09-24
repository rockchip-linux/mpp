# vim: syntax=cmake
# hal h265 reg

set(HAL_H265D_HDR
    hal_h265d_reg.h
    cabac.h
    ../../../common/h265d_syntax.h
    )

set(HAL_H265D_SRC
    hal_h265d_reg.c
    ../../../mpp_bitread.c
    )

set_target_properties(mpp_hal PROPERTIES FOLDER "mpp/hal")

add_library(h265d_hal STATIC
	${HAL_H265D_SRC} ${HAL_H265D_HDR}
    )

target_link_libraries(h265d_hal osal)

#add_subdirectory(test)
