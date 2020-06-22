# vim: syntax=cmake
# ----------------------------------------------------------------------------
# rate control built-in unit test case
# ----------------------------------------------------------------------------

include_directories(..)

# macro for adding osal sub-module unit test
macro(add_mpp_rc_test module)
    set(test_name ${module}_test)
    string(TOUPPER ${test_name} test_tag)

    option(${test_tag} "Build rc ${module} unit test" ${BUILD_TEST})
    if(${test_tag})
        add_executable(${test_name} ${test_name}.c)
        target_link_libraries(${test_name} enc_rc mpp_base ${ASAN_LIB})
        set_target_properties(${test_name} PROPERTIES FOLDER "osal/test")
        add_test(NAME ${test_name} COMMAND ${test_name})
    endif()
endmacro()

# mpp rc unit test
add_mpp_rc_test(rc_base)

# mpp rc api test
add_mpp_rc_test(rc_api)
