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

add_library(hal_h265d STATIC
	${HAL_H265D_SRC} ${HAL_H265D_HDR}
    )

if(ANDROID)
	target_link_libraries(hal_h265d vpu osal)
else()
target_link_libraries(hal_h265d osal)
endif()

#add_subdirectory(test)
