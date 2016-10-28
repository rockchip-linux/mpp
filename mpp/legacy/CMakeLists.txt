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

set(LIBROCKCHIP_VPU_VERSION "0")
set(LIBROCKCHIP_VPU_ABI_VERSION "1")

add_library(rockchip_vpu_static STATIC ${MPP_LEGACY_SRC})
set_target_properties(rockchip_vpu_static PROPERTIES FOLDER "mpp/legacy")
target_link_libraries(rockchip_vpu_static rockchip_mpp_static)

add_library(rockchip_vpu SHARED ${MPP_LEGACY_SRC})
set_target_properties(rockchip_vpu PROPERTIES FOLDER "mpp/legacy")
set_target_properties(rockchip_vpu PROPERTIES CLEAN_DIRECT_OUTPUT 1)
set_target_properties(rockchip_vpu PROPERTIES C_VISIBILITY_PRESET default)
set_target_properties(rockchip_vpu PROPERTIES CXX_VISIBILITY_PRESET default)

if(RKPLATFORM)
    target_link_libraries(rockchip_vpu rockchip_mpp dl)
else()
    target_link_libraries(rockchip_vpu rockchip_mpp dl)
endif()

target_include_directories(rockchip_vpu PUBLIC
${CMAKE_CURRENT_SOURCE_DIR}
)

target_include_directories(rockchip_vpu_static PUBLIC
${CMAKE_CURRENT_SOURCE_DIR}
)

set_target_properties(rockchip_vpu PROPERTIES VERSION
${LIBROCKCHIP_VPU_VERSION}
SOVERSION ${LIBROCKCHIP_VPU_ABI_VERSION})

install(TARGETS rockchip_vpu LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}")
install(TARGETS rockchip_vpu_static ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}")
