# vim: syntax=cmake
set(OSAL_HDR
    inc/rk_list.h
    inc/rk_thread.h
	inc/rk_log.h
    )

set(OSAL_SRC
    rk_list.cpp
	rk_log.cpp
	)

add_library(osal STATIC
    ${OSAL_SRC}
    )

# ----------------------------------------------------------------------------
# On window import win32 pthread library
# ----------------------------------------------------------------------------
if(MSVC)
    set(WIN32_PTHREAD_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/window/pthread/inc")
    include_directories(${WIN32_PTHREAD_INCLUDE_DIRS})
    add_library(pthread STATIC IMPORTED)
    message(STATUS "platform X86 ${X86} X64 ${X64}")
    set(WIN32_PTHREAD_LIB_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/window/pthread/lib")
    set(WIN32_PTHREAD_DLL_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/window/pthread/dll")
    if(X64)
    set(WIN32_ARCH "x64")
    set_target_properties(pthread PROPERTIES
                          IMPORTED_LOCATION
                          "${CMAKE_CURRENT_SOURCE_DIR}/window/pthread/lib/x64/pthreadVC2.lib")
	else()
    set(WIN32_ARCH "x86")
    endif()
    include_directories("${WIN32_PTHREAD_LIB_DIRS}/${WIN32_ARCH}")
    include_directories("${WIN32_PTHREAD_DLL_DIRS}/${WIN32_ARCH}")
    set_target_properties(pthread PROPERTIES
                          IMPORTED_LOCATION
                          "${WIN32_PTHREAD_LIB_DIRS}/${WIN32_ARCH}/pthreadVC2.lib")
endif()
target_link_libraries(osal pthread)
install(TARGETS osal
    LIBRARY DESTINATION ${LIB_INSTALL_DIR}
    ARCHIVE DESTINATION ${LIB_INSTALL_DIR})

add_subdirectory(test)
