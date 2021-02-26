# vim: syntax=cmake
# ----------------------------------------------------------------------------
# mpp/vproc/iep2 built-in unit test case
# ----------------------------------------------------------------------------
# iep2 unit test
option(IEP2_TEST "Build base iep2 unit test" ON)
add_executable(iep2_test iep2_test.c)
target_link_libraries(iep2_test ${MPP_SHARED} utils)
set_target_properties(iep2_test PROPERTIES FOLDER "mpp/vproc/iep2")
add_test(NAME iep2_test COMMAND iep2_test)
