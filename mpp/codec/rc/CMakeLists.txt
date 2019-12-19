# vim: syntax=cmake

include_directories(.)

# ----------------------------------------------------------------------------
# add mpp rate control function
# ----------------------------------------------------------------------------
add_library(enc_rc STATIC
    rc_data_base.cpp
    rc_data_impl.cpp
    )
