# vim: syntax=cmake
# ----------------------------------------------------------------------------
# codec h264 built-in unit test case
# ----------------------------------------------------------------------------
# log system unit test
option(H264E_HAL_TEST "Build codec h264e_hal unit test" ON)
if(H264E_HAL_TEST)
    add_executable(h264e_hal_test h264e_hal_test.c)
    include_directories(.)
    link_directories(.)
    if(ANDROID)
        target_link_libraries(h264e_hal_test mpp_shared utils)   
    else()
        target_link_libraries(h264e_hal_test mpp_shared utils)
    endif()
endif()


