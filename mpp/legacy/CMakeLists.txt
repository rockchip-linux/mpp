# vim: syntax=cmake
include_directories(.)

# ----------------------------------------------------------------------------
# add mpp implement
# ----------------------------------------------------------------------------
add_library(mpp_legacy STATIC
    vpu_api.cpp
    vpu_api_legacy.cpp
    )

target_link_libraries(mpp_legacy osal)
