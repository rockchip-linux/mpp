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

set(MPP_VERSION "0")
set(MPP_ABI_VERSION "1")

add_library(${MPP_STATIC} STATIC ${MPP_SRC})
set_target_properties(${MPP_STATIC} PROPERTIES FOLDER "mpp")
set_target_properties(${MPP_STATIC} PROPERTIES CLEAN_DIRECT_OUTPUT 1)
target_link_libraries(${MPP_STATIC} mpp_base mpp_codec mpp_hal)
set_target_properties(${MPP_STATIC} PROPERTIES C_VISIBILITY_PRESET default)
set_target_properties(${MPP_STATIC} PROPERTIES CXX_VISIBILITY_PRESET default)

add_library(${MPP_SHARED} SHARED ${MPP_SRC})
set_target_properties(${MPP_SHARED} PROPERTIES FOLDER "mpp")
set_target_properties(${MPP_SHARED} PROPERTIES CLEAN_DIRECT_OUTPUT 1)
target_link_libraries(${MPP_SHARED} ${MPP_STATIC})
set_target_properties(${MPP_SHARED} PROPERTIES C_VISIBILITY_PRESET default)
set_target_properties(${MPP_SHARED} PROPERTIES CXX_VISIBILITY_PRESET default)

# NOTE: due to legacy libray naming issue we can not support version on Android
if (NOT ANDROID)
set_target_properties(${MPP_SHARED} PROPERTIES VERSION ${MPP_VERSION})
set_target_properties(${MPP_SHARED} PROPERTIES SOVERSION ${MPP_ABI_VERSION})
endif()


add_subdirectory(legacy)

add_subdirectory(test)

install(TARGETS ${MPP_STATIC} ${MPP_SHARED}
        ARCHIVE DESTINATION "${LIB_INSTALL_DIR}"
        LIBRARY DESTINATION "${LIB_INSTALL_DIR}"
        )
