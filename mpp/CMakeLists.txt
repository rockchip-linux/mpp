# vim: syntax=cmake
# ----------------------------------------------------------------------------
# setup mpp codec config first
# ----------------------------------------------------------------------------
include(codecs.cmake)
include(vproc.cmake)

# ----------------------------------------------------------------------------
# add include directory
# ----------------------------------------------------------------------------
include_directories(inc)
include_directories(common)
include_directories(base/inc)
include_directories(codec/inc)
include_directories(hal/inc)
include_directories(hal/common)
include_directories(vproc/inc)

# ----------------------------------------------------------------------------
# add mpp base component
# ----------------------------------------------------------------------------
add_subdirectory(base)

# ----------------------------------------------------------------------------
# add codec parsers
# ----------------------------------------------------------------------------
add_subdirectory(codec)

# ----------------------------------------------------------------------------
# add video processor
# ----------------------------------------------------------------------------
add_subdirectory(vproc)

# ----------------------------------------------------------------------------
# add register generation hal
# ----------------------------------------------------------------------------
add_subdirectory(hal)

# ----------------------------------------------------------------------------
# add mpp implement
# ----------------------------------------------------------------------------
set (MPP_SRC
    mpp_info.c
    mpp.c
    mpp_impl.c
    mpi.c
    )

set(MPP_VERSION "0")
set(MPP_ABI_VERSION "1")

add_library(${MPP_SHARED} SHARED ${MPP_SRC})
target_link_libraries(${MPP_SHARED} ${ASAN_LIB})
set_target_properties(${MPP_SHARED} PROPERTIES C_VISIBILITY_PRESET default)
set_target_properties(${MPP_SHARED} PROPERTIES CXX_VISIBILITY_PRESET default)

# NOTE: due to legacy libray naming issue we can not support version on Android
if (NOT ANDROID)
set_target_properties(${MPP_SHARED} PROPERTIES VERSION ${MPP_VERSION})
set_target_properties(${MPP_SHARED} PROPERTIES SOVERSION ${MPP_ABI_VERSION})
endif()

set_target_properties(${MPP_SHARED} PROPERTIES FOLDER "mpp")
set_target_properties(${MPP_SHARED} PROPERTIES CLEAN_DIRECT_OUTPUT 1)
target_link_libraries(${MPP_SHARED} mpp_codec mpp_hal mpp_vproc kmpp
    ${BEGIN_WHOLE_ARCHIVE} mpp_base kmpp_base osal ${END_WHOLE_ARCHIVE} ${LIBM})

# build static library
add_library(${MPP_STATIC} STATIC ${MPP_SRC})
set_target_properties(${MPP_STATIC} PROPERTIES FOLDER "mpp" OUTPUT_NAME "${MPP_SHARED}")
set_target_properties(${MPP_STATIC} PROPERTIES CLEAN_DIRECT_OUTPUT 1)
target_link_libraries(${MPP_STATIC} mpp_codec mpp_hal mpp_vproc kmpp
                      ${BEGIN_WHOLE_ARCHIVE} mpp_base kmpp_base osal ${END_WHOLE_ARCHIVE})

add_custom_command(TARGET ${MPP_STATIC} POST_BUILD
    COMMAND ${CMAKE_AR} x $<TARGET_FILE:${MPP_STATIC}>
    COMMAND ${CMAKE_AR} rcs lib${MPP_SHARED}.a *.o
    COMMAND ${CMAKE_SOURCE_DIR}/merge_static_lib.sh ${CMAKE_BINARY_DIR} ${MPP_SHARED}
    COMMAND ${CMAKE_STRIP} --strip-debug lib${MPP_SHARED}.a
    COMMAND pwd
    COMMAND rm *.o
    )

add_subdirectory(legacy)

install(TARGETS ${MPP_SHARED} LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}")
install(TARGETS ${MPP_STATIC} ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}")
