# vim: syntax=cmake
include_directories(.)

add_library(mpp STATIC
    mpp_info.cpp
    )

target_link_libraries(mpp osal)

add_subdirectory(test)
