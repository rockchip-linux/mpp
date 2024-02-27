# vim: syntax=cmake
# ----------------------------------------------------------------------------
# mpp/vproc/vdpp built-in unit test case
# ----------------------------------------------------------------------------
# vdpp unit test
option(VDPP_TEST "Build base vdpp unit test" ON)
add_executable(vdpp_test vdpp_test.c)
target_link_libraries(vdpp_test ${MPP_SHARED} utils vproc_vdpp)
set_target_properties(vdpp_test PROPERTIES FOLDER "mpp/vproc/vdpp")
add_test(NAME vdpp_test COMMAND vdpp_test)