# vim: syntax=cmake

# hal dummy decoder api
set(HAL_DUMMY_API
    ../inc/hal_dummy_dec_api.h
    ../inc/hal_dummy_enc_api.h
    )

# hal dummy header
set(HAL_DUMMY_HDR
    )

# hal dummy decoder sourse
set(HAL_DUMMY_SRC
    hal_dummy_dec_api.c
    hal_dummy_enc_api.c
    )

add_library(hal_dummy STATIC
            ${HAL_DUMMY_API}
            ${HAL_DUMMY_HDR}
            ${HAL_DUMMY_SRC}
            )

set_target_properties(hal_dummy PROPERTIES FOLDER "mpp/hal")

