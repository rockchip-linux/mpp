# vim: syntax=cmake
# ----------------------------------------------------------------------------
# codec h265 built-in unit test case
# ----------------------------------------------------------------------------
# log system unit test
option(H265D_TEST "Build codec h265d_parse unit test" ON)
if(H265D_TEST)
    add_executable(h265d_parser_test h265d_parser_test.c)
    include_directories(.)
    link_directories(.)
    if(LINUX)
        set(OPENHEVCSO "${CMAKE_SOURCE_DIR}/mpp/codec/dec/h265/test")
        add_library(openhevcwrapper SHARED IMPORTED)
        set_target_properties(openhevcwrapper PROPERTIES
                                IMPORTED_LOCATION
                        "${OPENHEVCSO}/libLibOpenHevcWrapper.so")
        target_link_libraries(h265d_parser_test openhevcwrapper m h265d_parse osal h265d_hal mpp_codec mpp utils)
    else()
        target_link_libraries(h265d_parser_test mpp utils)
    endif()
endif()


