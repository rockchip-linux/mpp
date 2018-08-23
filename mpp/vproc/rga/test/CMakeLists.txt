# vim: syntax=cmake
# ----------------------------------------------------------------------------
# mpp/vproc/rga built-in unit test case
# ----------------------------------------------------------------------------
# rga unit test
option(RGA_TEST "Build base rga unit test" ON)
add_executable(rga_test rga_test.cpp)
target_link_libraries(rga_test ${MPP_SHARED} utils)
set_target_properties(rga_test PROPERTIES FOLDER "mpp/vproc/rga")
add_test(NAME rga_test COMMAND rga_test)
