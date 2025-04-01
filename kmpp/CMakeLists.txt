# vim: syntax=cmake

# ----------------------------------------------------------------------------
# add include directory
# ----------------------------------------------------------------------------
include_directories(${PROJECT_SOURCE_DIR}/mpp/codec/inc)

# ----------------------------------------------------------------------------
# add kmpp base component
# ----------------------------------------------------------------------------
add_subdirectory(base)

# ----------------------------------------------------------------------------
# add kmpp implement
# ----------------------------------------------------------------------------
set (KMPP_SRC
    kmpp.c
    )

add_library(kmpp STATIC ${KMPP_SRC})
target_link_libraries(kmpp kmpp_base)
set_target_properties(kmpp PROPERTIES FOLDER "kmpp")

