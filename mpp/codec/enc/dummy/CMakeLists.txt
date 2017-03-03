# vim: syntax=cmake

# dummy decoder api
set(DUMMY_API
    ../../inc/dummy_enc_api.h
    )

# dummy decoder header
set(DUMMY_HDR
    )

# dummy decoder sourse
set(DUMMY_SRC
    dummy_enc_api.c
    )

add_library(codec_dummy_enc STATIC
            ${DUMMY_API}
            ${DUMMY_HDR}
            ${DUMMY_SRC}
            )

set_target_properties(codec_dummy_enc PROPERTIES FOLDER "mpp/codec")

