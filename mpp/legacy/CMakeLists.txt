# vim: syntax=cmake
include_directories(.)

# ----------------------------------------------------------------------------
# add mpp implement
# ----------------------------------------------------------------------------
add_library(mpp_legacy SHARED
    vpu_api.cpp
    vpu_api_legacy.cpp
    )
set_target_properties(mpp_legacy PROPERTIES FOLDER "mpp/legacy")
set_target_properties(mpp_legacy PROPERTIES OUTPUT_NAME "vpu") 
target_link_libraries(mpp_legacy mpp_shared)
