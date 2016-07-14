# vim: syntax=cmake
# ----------------------------------------------------------------------------
# codec h264 built-in unit test case
# ----------------------------------------------------------------------------
# log system unit test
option(H264E_TEST "Build codec h264e_parse unit test" ON)
if(H264E_TEST)
    add_executable(h264e_test H264TestBench.c)
    include_directories(.)
    link_directories(.)
    if(ANDROID)
        target_link_libraries(h264e_test mpp_shared utils)   
    else()
        target_link_libraries(h264e_test mpp_shared utils)
    endif()
endif()


