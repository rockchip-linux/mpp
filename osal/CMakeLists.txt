# vim: syntax=cmake
cmake_minimum_required(VERSION 2.6.3)
PROJECT(osal C CXX)
INCLUDE(GNUInstallDirs)

find_package(Threads)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_REENTRANT -D_GNU_SOURCE")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")

set(MPP_ALLOCATOR
    allocator/allocator_std.c
    allocator/allocator_ion.c
    allocator/allocator_ext_dma.c
    allocator/allocator_dma_heap.c
    allocator/allocator_drm.c
)

set(MPP_DRIVER
    driver/mpp_server.c
    driver/mpp_device.c
    driver/mpp_service.c
    driver/vcodec_service.c
    driver/mpp_vcodec_client.c
)

add_library(osal OBJECT
    mpp_singleton.c
    mpp_soc.c
    mpp_platform.c
    mpp_runtime.c
    mpp_allocator.c
    mpp_mem_pool.c
    mpp_callback.c
    mpp_eventfd.c
    mpp_dmabuf.c
    mpp_thread.c
    mpp_compat.c
    mpp_common.c
    mpp_queue.c
    mpp_trace.c
    mpp_lock.c
    mpp_time.c
    mpp_list.c
    mpp_mem.c
    mpp_env.c
    mpp_log.c
    osal_2str.c
    # Those files have a compiler marco protection, so only target
    # OS will be built
    android/os_mem.c
    android/os_env.c
    android/os_log.c
    linux/os_mem.c
    linux/os_env.c
    linux/os_log.c
    ${MPP_ALLOCATOR}
    ${MPP_DRIVER}
)

target_link_libraries(osal ${CMAKE_THREAD_LIBS_INIT})

target_include_directories(osal PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/inc"
    "${CMAKE_CURRENT_SOURCE_DIR}/allocator"
    "${CMAKE_CURRENT_SOURCE_DIR}/driver/inc"
)

set_target_properties(osal PROPERTIES FOLDER "osal")

# leave those special platform here
if(ANDROID)
    add_definitions(-static)
    # in Android pthread is in libc, also need liblog
    # Android 14 requires libc++ not libstdc++
    if("${ANDROID_STL}" STREQUAL "c++_static")
        target_link_libraries(osal log m)
    else()
        target_link_libraries(osal log stdc++ m)
    endif()
endif(ANDROID)

# unit test
add_subdirectory(test)
