# vim: syntax=cmake
# ----------------------------------------------------------------------------
# kmpp/base built-in unit test case
# ----------------------------------------------------------------------------
# macro for adding osal sub-module unit test
macro(add_kmpp_test module)
    set(test_name ${module}_test)
    string(TOUPPER ${test_name} test_tag)
    #message(STATUS "moduule   : ${module}")
    #message(STATUS "test_name : ${test_name}")
    #message(STATUS "test_tag  : ${test_tag}")

    option(${test_tag} "Build base ${module} unit test" ${BUILD_TEST})
    if(${test_tag})
        add_executable(${test_name} ${test_name}.c)
        target_link_libraries(${test_name} ${MPP_SHARED})
        set_target_properties(${test_name} PROPERTIES FOLDER "kmpp/test")
        add_test(NAME ${test_name} COMMAND ${test_name})
    endif()
endmacro()

# kmpp venc unit test
add_kmpp_test(kmpp_venc)

