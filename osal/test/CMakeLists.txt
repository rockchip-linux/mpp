# vim: syntax=cmake
# ----------------------------------------------------------------------------
# OSAL built-in unit test case
# ----------------------------------------------------------------------------
# macro for adding osal sub-module unit test
macro(add_mpp_osal_test module)
    set(test_name ${module}_test)
    string(TOUPPER ${test_name} test_tag)
    #message(STATUS "moduule   : ${module}")
    #message(STATUS "test_name : ${test_name}")
    #message(STATUS "test_tag  : ${test_tag}")

    option(${test_tag} "Build osal ${module} unit test" ON)
    if(${test_tag})
        add_executable(${test_name} ${test_name}.c)
        target_link_libraries(${test_name} ${MPP_SHARED})
        set_target_properties(${test_name} PROPERTIES FOLDER "osal/test")
        add_test(NAME ${test_name} COMMAND ${test_name})
    endif()
endmacro()

# log system unit test
add_mpp_osal_test(mpp_log)

# env system unit test
add_mpp_osal_test(mpp_env)

# malloc system unit test
add_mpp_osal_test(mpp_mem)

# time system unit test
add_mpp_osal_test(mpp_time)

# hardware platform feature detection unit test
add_mpp_osal_test(mpp_platform)

# software runtime feature detection unit test
add_mpp_osal_test(mpp_runtime)

# thread implement unit test
add_mpp_osal_test(mpp_thread)

