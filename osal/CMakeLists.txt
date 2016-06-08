# vim: syntax=cmake
if(${ANDROID})
set(OS_DIR android)
set(MPP_ALLOCATOR allocator/allocator_ion.c)
elseif(${UNIX})
set(OS_DIR linux)
set(MPP_ALLOCATOR allocator/allocator_ion.c allocator/allocator_drm.c)
elseif(${WIN32})
set(OS_DIR window)
set(MPP_ALLOCATOR )
else()
message(SEND_ERROR "Can not found platform definistion ${CMAKE_SYSTEM}")
endif()

include_directories(.)
include_directories(./allocator)

add_library(osal STATIC
    mpp_allocator.cpp
    mpp_thread.cpp
    mpp_common.cpp
    mpp_time.cpp
    mpp_list.cpp
    mpp_mem.cpp
    mpp_env.cpp
    mpp_log.cpp
    ${OS_DIR}/os_allocator.c
    ${OS_DIR}/os_mem.c
    ${OS_DIR}/os_env.c
    ${OS_DIR}/os_log.c
    ${MPP_ALLOCATOR}
    )

if(ANDROID)
    add_definitions(-static)
    # in Android pthread is in libc, also need liblog
    target_link_libraries(osal log stdc++ m c )
else()
    target_link_libraries(osal pthread)
endif()

set_target_properties(osal PROPERTIES FOLDER "osal")
add_subdirectory(test)
