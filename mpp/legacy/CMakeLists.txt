# vim: syntax=cmake
include_directories(.)

# ----------------------------------------------------------------------------
# add mpp implement
# ----------------------------------------------------------------------------
set (MPP_LEGACY_SRC
    vpu.c
    vpu_api.cpp
    vpu_api_legacy.cpp
    vpu_mem_legacy.c
    rk_list.cpp
    ppOp.cpp
    ../mpp_info.cpp
    )

set(VPU_VERSION "0")
set(VPU_ABI_VERSION "1")

add_library(${VPU_STATIC} STATIC ${MPP_LEGACY_SRC})
set_target_properties(${VPU_STATIC} PROPERTIES FOLDER "mpp/legacy")
target_link_libraries(${VPU_STATIC} ${MPP_STATIC})

add_library(${VPU_SHARED} SHARED ${MPP_LEGACY_SRC})
set_target_properties(${VPU_SHARED} PROPERTIES FOLDER "mpp/legacy")
set_target_properties(${VPU_SHARED} PROPERTIES CLEAN_DIRECT_OUTPUT 1)
set_target_properties(${VPU_SHARED} PROPERTIES C_VISIBILITY_PRESET default)
set_target_properties(${VPU_SHARED} PROPERTIES CXX_VISIBILITY_PRESET default)

# NOTE: due to legacy libray naming issue we can not support version on Android
if (NOT ANDROID)
set_target_properties(${VPU_SHARED} PROPERTIES VERSION ${MPP_VERSION})
set_target_properties(${VPU_SHARED} PROPERTIES SOVERSION ${MPP_ABI_VERSION})
endif()

target_link_libraries(${VPU_SHARED} dl ${MPP_SHARED})

install(TARGETS ${VPU_STATIC} ${VPU_SHARED}
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        )
