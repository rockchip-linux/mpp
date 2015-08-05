# vim: syntax=cmake
# ----------------------------------------------------------------------------
# OSAL built-in unit test case
# ----------------------------------------------------------------------------
# log system unit test
option(OSAL_LOG_TEST "Build osal rk_log unit test" ON)
if(OSAL_LOG_TEST)
    add_executable(rk_log_test rk_log_test.c)
    target_link_libraries(rk_log_test osal)
endif()

# env system unit test
option(OSAL_ENV_TEST "Build osal rk_env unit test" ON)
if(OSAL_ENV_TEST)
    add_executable(rk_env_test rk_env_test.c)
    target_link_libraries(rk_env_test osal)
endif()

# malloc system unit test
option(OSAL_MALLOC_TEST "Build osal rk_malloc unit test" ON)
if(OSAL_MALLOC_TEST)
    add_executable(rk_malloc_test rk_malloc_test.c)
    target_link_libraries(rk_malloc_test osal)
endif()

# thread implement unit test
option(OSAL_THREAD_TEST "Build osal rk_thread unit test" ON)
if(OSAL_THREAD_TEST)
    add_executable(rk_thread_test rk_thread_test.c)
    target_link_libraries(rk_thread_test osal)
endif()

