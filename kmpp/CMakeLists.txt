# vim: syntax=cmake

# ----------------------------------------------------------------------------
# add include directory
# ----------------------------------------------------------------------------
include_directories(${PROJECT_SOURCE_DIR}/mpp/codec/inc)
include_directories(${PROJECT_SOURCE_DIR}/osal/driver/inc)

# ----------------------------------------------------------------------------
# add kmpp base component
# ----------------------------------------------------------------------------
add_subdirectory(base)

# ----------------------------------------------------------------------------
# add kmpp implement
# ----------------------------------------------------------------------------
set (KMPP_SRC
    kmpp.c
    kmpp_venc.c
    )

add_library(kmpp OBJECT ${KMPP_SRC})
set_target_properties(kmpp PROPERTIES FOLDER "kmpp")

# ----------------------------------------------------------------------------
# add kmpp test case
# ----------------------------------------------------------------------------
add_subdirectory(test)

