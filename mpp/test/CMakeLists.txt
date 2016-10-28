# vim: syntax=cmake
# ----------------------------------------------------------------------------
# mpp built-in unit test case
# ----------------------------------------------------------------------------
# macro for adding mpp sub-module unit test
macro(add_mpp_test module)
    set(test_name ${module}_test)
    string(TOUPPER ${test_name} test_tag)
    #message(STATUS "moduule   : ${module}")
    #message(STATUS "test_name : ${test_name}")
    #message(STATUS "test_tag  : ${test_tag}")

    option(${test_tag} "Build mpp ${module} unit test" ON)
    if(${test_tag})
        add_executable(${test_name} ${test_name}.c)
        target_link_libraries(${test_name} rockchip_mpp)
        set_target_properties(${test_name} PROPERTIES FOLDER "mpp/test")
        #add_test(NAME ${test_name} COMMAND ${test_name})
    endif()
endmacro()

# info system unit test
add_mpp_test(mpp_info)

# h264 decoder test
if( HAVE_H264D )
    include_directories(../codec/dec/h264)
    add_mpp_test(h264d)
endif()

# vp9 decoder test
if( HAVE_VP9D )
    include_directories(../codec/dec/vp9)
    add_mpp_test(vp9d)
endif()

if( HAVE_AVSD )
    # avs decoder test
    include_directories(../codec/dec/avs)
    add_mpp_test(avsd)
endif()

# jpeg decoder test
if( HAVE_JPEGD )
    include_directories(../codec/dec/jpeg)
    add_mpp_test(jpegd)
endif()
