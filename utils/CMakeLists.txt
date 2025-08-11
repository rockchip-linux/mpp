# vim: syntax=cmake
# ----------------------------------------------------------------------------
# add libvpu implement
# ----------------------------------------------------------------------------
include_directories(${PROJECT_SOURCE_DIR}/mpp/base/inc)

add_library(utils STATIC
    mpp_enc_roi_utils.c
    mpi_enc_utils.c
    mpi_dec_utils.c
    mpp_opt.c
    utils.c
    iniparser.c
    dictionary.c
    camera_source.c
    osd3_test.c
    )

target_link_libraries(utils mpp_base)
