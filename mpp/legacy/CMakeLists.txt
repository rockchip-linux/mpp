# vim: syntax=cmake
include_directories(.)

# ----------------------------------------------------------------------------
# add mpp implement
# ----------------------------------------------------------------------------
set (MPP_LEGACY_SRC
    vpu.c
    vpu_api.cpp
    vpu_api_legacy.cpp
    vpu_api_mlvec.cpp
    vpu_mem_legacy.c
    rk_list.cpp
    ppOp.cpp
    ../mpp_info.c
    )

set(VPU_VERSION "0")
set(VPU_ABI_VERSION "1")

# NOTE: link_directories should be before add_library or add_executable
link_directories(${CMAKE_BINARY_DIR}/mpp)

add_library(${VPU_SHARED} SHARED ${MPP_LEGACY_SRC})
add_dependencies(${VPU_SHARED} ${MPP_SHARED})
set_target_properties(${VPU_SHARED} PROPERTIES FOLDER "mpp/legacy")
set_target_properties(${VPU_SHARED} PROPERTIES CLEAN_DIRECT_OUTPUT 1)
set_target_properties(${VPU_SHARED} PROPERTIES C_VISIBILITY_PRESET default)
set_target_properties(${VPU_SHARED} PROPERTIES CXX_VISIBILITY_PRESET default)

# NOTE: due to legacy libray naming issue we can not support version on Android
if (NOT ANDROID)
set_target_properties(${VPU_SHARED} PROPERTIES VERSION ${MPP_VERSION})
set_target_properties(${VPU_SHARED} PROPERTIES SOVERSION ${MPP_ABI_VERSION})
endif()

# force dynamic link
target_link_libraries(${VPU_SHARED} ${ASAN_LIB}
                      -Wl,-Bdynamic dl lib${MPP_SHARED}.so)

install(TARGETS ${VPU_SHARED} LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}")
