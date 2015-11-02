# vim: syntax=cmake
include_directories(.)

# ----------------------------------------------------------------------------
# add mpp implement
# ----------------------------------------------------------------------------
set (MPP_LEGACY_SRC
    vpu_api.cpp
    vpu_api_legacy.cpp
    vpu_mem_legacy.c
    rk_list.cpp
    )

add_library(mpp_legacy STATIC ${MPP_LEGACY_SRC})
set_target_properties(mpp_legacy PROPERTIES FOLDER "mpp/legacy")

add_library(mpp_legacy_shared SHARED ${MPP_LEGACY_SRC})
set_target_properties(mpp_legacy_shared PROPERTIES FOLDER "mpp/legacy")
set_target_properties(mpp_legacy_shared PROPERTIES OUTPUT_NAME "vpu") 
set_target_properties(mpp_legacy_shared PROPERTIES CLEAN_DIRECT_OUTPUT 1)

target_link_libraries(mpp_legacy mpp)
target_link_libraries(mpp_legacy_shared mpp_shared)
