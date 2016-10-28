# vim: syntax=cmake
# ----------------------------------------------------------------------------
# setup mpp codec config first
# ----------------------------------------------------------------------------
include(codecs.cmake)

# ----------------------------------------------------------------------------
# add include directory
# ----------------------------------------------------------------------------
include_directories(inc)
include_directories(common)
include_directories(base/inc)
include_directories(codec/inc)
include_directories(hal/inc)
include_directories(hal/common)

# ----------------------------------------------------------------------------
# add mpp base component
# ----------------------------------------------------------------------------
add_subdirectory(base)

# ----------------------------------------------------------------------------
# add codec parsers
# ----------------------------------------------------------------------------
add_subdirectory(codec)

# ----------------------------------------------------------------------------
# add register generation hal
# ----------------------------------------------------------------------------
add_subdirectory(hal)

# ----------------------------------------------------------------------------
# add mpp implement
# ----------------------------------------------------------------------------
set (MPP_SRC
    mpp_info.cpp
    mpp.cpp
    mpi_impl.cpp
    mpi.cpp
    )

add_library(mpp STATIC ${MPP_SRC})
set_target_properties(mpp PROPERTIES FOLDER "mpp")
set_target_properties(mpp PROPERTIES CLEAN_DIRECT_OUTPUT 1)
target_link_libraries(mpp mpp_base mpp_codec mpp_hal)
set_target_properties(mpp PROPERTIES C_VISIBILITY_PRESET default)
set_target_properties(mpp PROPERTIES CXX_VISIBILITY_PRESET default)

add_library(mpp_shared SHARED ${MPP_SRC})
set_target_properties(mpp_shared PROPERTIES FOLDER "mpp")
set_target_properties(mpp_shared PROPERTIES OUTPUT_NAME "mpp")
set_target_properties(mpp_shared PROPERTIES CLEAN_DIRECT_OUTPUT 1)
target_link_libraries(mpp_shared mpp)
set_target_properties(mpp_shared PROPERTIES C_VISIBILITY_PRESET default)
set_target_properties(mpp_shared PROPERTIES CXX_VISIBILITY_PRESET default)

add_subdirectory(legacy)

add_subdirectory(test)

install(TARGETS mpp mpp_shared
        ARCHIVE DESTINATION "${LIB_INSTALL_DIR}"
        LIBRARY DESTINATION "${LIB_INSTALL_DIR}"
        )
