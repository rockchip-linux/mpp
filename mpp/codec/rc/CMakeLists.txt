# vim: syntax=cmake

include_directories(.)

# ----------------------------------------------------------------------------
# add mpp rate control function
# ----------------------------------------------------------------------------
add_library(enc_rc STATIC
    h264e_rc.c
    h265e_rc.c
    jpege_rc.c
    vp8e_rc.c
    rc_model_v2_smt.c
    rc_model_v2.c
    rc_data_base.cpp
    rc_data_impl.cpp
    rc_data.cpp
    rc_impl.cpp
    rc.cpp
    rc_base.cpp
    )

target_link_libraries(enc_rc mpp_rc)

add_subdirectory(test)
