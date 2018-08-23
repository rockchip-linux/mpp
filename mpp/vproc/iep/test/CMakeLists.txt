# vim: syntax=cmake
# ----------------------------------------------------------------------------
# mpp/vproc/iep built-in unit test case
# ----------------------------------------------------------------------------
# iep unit test
option(IEP_TEST "Build base iep unit test" ON)
add_executable(iep_test iep_test.cpp)
target_link_libraries(iep_test ${MPP_SHARED} utils)
set_target_properties(iep_test PROPERTIES FOLDER "mpp/vproc/iep")
add_test(NAME iep_test COMMAND iep_test)
