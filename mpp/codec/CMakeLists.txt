# vim: syntax=cmake

# ----------------------------------------------------------------------------
# add mpp_dec implement
# ----------------------------------------------------------------------------
add_library(mpp_codec STATIC
    mpp_enc.cpp
    mpp_dec.cpp
    )

add_subdirectory(dec)

add_subdirectory(enc)
