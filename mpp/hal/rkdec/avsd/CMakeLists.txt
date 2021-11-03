# vim: syntax=cmake

# hal avs decoder api
set(HAL_AVSD_API
    )

# hal avs header
set(HAL_AVSD_HDR
    hal_avsd_base.h
    hal_avsd_vdpu1.h
    hal_avsd_vdpu2.h
    hal_avsd_plus.h
    hal_avsd_vdpu1_reg.h
    hal_avsd_vdpu2_reg.h
    hal_avsd_plus_reg.h
    )

# hal avs decoder sourse
set(HAL_AVSD_SRC
    hal_avsd_api.c
    hal_avsd_base.c
    hal_avsd_vdpu1.c
    hal_avsd_vdpu2.c
    hal_avsd_plus.c
    )

add_library(hal_avsd STATIC
            ${HAL_AVSD_API}
            ${HAL_AVSD_HDR}
            ${HAL_AVSD_SRC}
            )

target_link_libraries(hal_avsd mpp_base)
set_target_properties(hal_avsd PROPERTIES FOLDER "mpp/hal")

