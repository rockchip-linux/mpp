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
        target_link_libraries(${test_name} mpp_shared)
        set_target_properties(${test_name} PROPERTIES FOLDER "mpp/test") 
        install(TARGETS ${test_name} RUNTIME DESTINATION ${TEST_INSTALL_DIR})
    endif()
endmacro()

# info system unit test
add_mpp_test(mpp_info)

# h264 decoder test
option(H264D_TEST "Build mpp h264 decoder unit test" ON)
if(H264D_TEST)
	set(test_name h264d_test)
    add_executable(${test_name} ${test_name}.c h264d_rwfile.h h264d_rwfile.c)
    target_link_libraries(${test_name} mpp_shared)
    set_target_properties(${test_name} PROPERTIES FOLDER "mpp/test") 
    install(TARGETS ${test_name} RUNTIME DESTINATION ${TEST_INSTALL_DIR})
endif()

# vp9 decoder test
option(VP9D_TEST "Build mpp vp9 decoder unit test" ON)
if(VP9D_TEST)
	set(test_name vp9d_test)
    add_executable(${test_name} ${test_name}.c)
    target_link_libraries(${test_name} mpp_shared)
    set_target_properties(${test_name} PROPERTIES FOLDER "mpp/test") 
    install(TARGETS ${test_name} RUNTIME DESTINATION ${TEST_INSTALL_DIR})
endif()
