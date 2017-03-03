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
    ppOp.cpp
    ../mpp_info.cpp
    )

add_library(mpp_legacy STATIC ${MPP_LEGACY_SRC})
set_target_properties(mpp_legacy PROPERTIES FOLDER "mpp/legacy")
target_link_libraries(mpp_legacy mpp)

add_library(mpp_legacy_shared SHARED ${MPP_LEGACY_SRC})
set_target_properties(mpp_legacy_shared PROPERTIES FOLDER "mpp/legacy")
set_target_properties(mpp_legacy_shared PROPERTIES OUTPUT_NAME "vpu")
set_target_properties(mpp_legacy_shared PROPERTIES CLEAN_DIRECT_OUTPUT 1)
set_target_properties(mpp_legacy_shared PROPERTIES C_VISIBILITY_PRESET default)
set_target_properties(mpp_legacy_shared PROPERTIES CXX_VISIBILITY_PRESET default)

if(RKPLATFORM)
    target_link_libraries(mpp_legacy_shared worker_vpu dl mpp_shared)
else()
    target_link_libraries(mpp_legacy_shared mpp_shared)
endif()
