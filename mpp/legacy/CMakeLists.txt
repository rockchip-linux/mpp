# vim: syntax=cmake
include_directories(.)

# ----------------------------------------------------------------------------
# add mpp implement
# ----------------------------------------------------------------------------
if(RKPLATFORM)
    set(RKPLAT_VPU_LEGACY vpu.c)
endif(RKPLATFORM)

set (MPP_LEGACY_SRC
    ${RKPLAT_VPU_LEGACY}
    vpu_api.cpp
    vpu_api_legacy.cpp
    vpu_mem_legacy.c
    rk_list.cpp
    ppOp.cpp
    ../mpp_info.cpp
    )

add_library(mpp_legacy STATIC ${MPP_LEGACY_SRC})
set_target_properties(mpp_legacy PROPERTIES FOLDER "mpp/legacy")
target_link_libraries(mpp_legacy rockchip_mpp)

add_library(mpp_legacy_shared SHARED ${MPP_LEGACY_SRC})
set_target_properties(mpp_legacy_shared PROPERTIES FOLDER "mpp/legacy")
set_target_properties(mpp_legacy_shared PROPERTIES OUTPUT_NAME "vpu")
set_target_properties(mpp_legacy_shared PROPERTIES CLEAN_DIRECT_OUTPUT 1)
set_target_properties(mpp_legacy_shared PROPERTIES C_VISIBILITY_PRESET default)
set_target_properties(mpp_legacy_shared PROPERTIES CXX_VISIBILITY_PRESET default)

if(RKPLATFORM)
    target_link_libraries(mpp_legacy_shared dl rockchip_mpp)
else()
    target_link_libraries(mpp_legacy_shared rockchip_mpp)
endif()
