# vim: syntax=cmake
cmake_minimum_required(VERSION 2.6.3)
PROJECT(osal C CXX)
INCLUDE(GNUInstallDirs)

find_package(Threads)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_REENTRANT -D_GNU_SOURCE")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")

if (HAVE_DRM)
    add_definitions(-DHAVE_DRM)
    set(DRM_FILES allocator/allocator_drm.c)
    message(STATUS "compile with drm support")
else()
    message(STATUS "compile without drm support")
endif()

set(MPP_ALLOCATOR
    allocator/allocator_std.c
    allocator/allocator_ion.c
    allocator/allocator_ext_dma.c
    ${DRM_FILES}
)

add_library(osal STATIC
    mpp_platform.cpp
    mpp_runtime.cpp
    mpp_allocator.cpp
    mpp_thread.cpp
    mpp_common.cpp
    mpp_queue.cpp
    mpp_time.cpp
    mpp_list.cpp
    mpp_mem.cpp
    mpp_env.cpp
    mpp_log.cpp
    # Those files have a compiler marco protection, so only target
    # OS will be built
    android/os_allocator.c
    android/os_mem.c
    android/os_env.c
    android/os_log.c
    linux/os_allocator.c
    linux/os_mem.c
    linux/os_env.c
    linux/os_log.c
    windows/os_allocator.c
    windows/os_mem.c
    windows/os_env.c
    windows/os_log.c
    ${MPP_ALLOCATOR}
)

target_link_libraries(osal ${CMAKE_THREAD_LIBS_INIT})

target_include_directories(osal PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/inc"
    "${CMAKE_CURRENT_SOURCE_DIR}/allocator"
)

set_target_properties(osal PROPERTIES FOLDER "osal")

# leave those special platform here
if(ANDROID)
    add_definitions(-static)
    # in Android pthread is in libc, also need liblog
    target_link_libraries(osal log stdc++ m)
endif(ANDROID)

# unit test
add_subdirectory(test)
