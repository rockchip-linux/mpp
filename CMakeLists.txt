cmake_minimum_required(VERSION 2.6.3)
find_package(PkgConfig)

PROJECT(rockchip_media_process_platform C CXX)
INCLUDE(GNUInstallDirs)

pkg_search_module(PTHREAD pthread)

ADD_DEFINITIONS(-Wmissing-declarations -Wall -Wextra -Wshadow -ffast-math)
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -Wall -O0 -g3")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -Wall -O0 -g3 -std=c++11")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -Os")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -Os -std=c++11")

# putting those into config.h would be better
set(RKPLATFORM "" CACHE BOOL "Enable RK HW CONFIG")
if(RKPLATFORM)
ADD_DEFINITIONS(-DRKPLATFORM)
endif(RKPLATFORM)

# ----------------------------------------------------------------------------
# Create git version information
# ----------------------------------------------------------------------------
if(EXISTS "${PROJECT_SOURCE_DIR}/.git")
    find_package(Git)
    if(GIT_FOUND)
        # get author
        execute_process(COMMAND ${GIT_EXECUTABLE} log -1 --format=%an
            WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
            OUTPUT_VARIABLE EXEC_OUT
            ERROR_VARIABLE EXEC_ERROR
            RESULT_VARIABLE EXEC_RET
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_STRIP_TRAILING_WHITESPACE)

        if (NOT EXEC_RET)
            message(STATUS "author: ${EXEC_OUT}")
            set(VERSION_LAST_AUTHOR ${EXEC_OUT})
        endif()

        # get date
        execute_process(COMMAND ${GIT_EXECUTABLE} log -1 --format=%ad
            WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
            OUTPUT_VARIABLE EXEC_OUT
            ERROR_VARIABLE EXEC_ERROR
            RESULT_VARIABLE EXEC_RET
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_STRIP_TRAILING_WHITESPACE)

        if (NOT EXEC_RET)
            message(STATUS "author: ${EXEC_OUT}")
            set(VERSION_LAST_DATA ${EXEC_OUT})
        endif()

        # get version hash
        execute_process(COMMAND ${GIT_EXECUTABLE} log -1 --format=%H
            WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
            OUTPUT_VARIABLE EXEC_OUT
            ERROR_VARIABLE EXEC_ERROR
            RESULT_VARIABLE EXEC_RET
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_STRIP_TRAILING_WHITESPACE)

        if (NOT EXEC_RET)
            set(VERSION_REVISION ${EXEC_OUT})
        endif()

        # get one line version information
        execute_process(COMMAND ${GIT_EXECUTABLE} log -1 --pretty=format:"%h author: %an %s"
            WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
            OUTPUT_VARIABLE EXEC_OUT
            ERROR_VARIABLE EXEC_ERROR
            RESULT_VARIABLE EXEC_RET
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_STRIP_TRAILING_WHITESPACE)

        if (NOT EXEC_RET)
            message(STATUS "git version: ${EXEC_OUT}")
            set(VERSION_ONE_LINE ${EXEC_OUT})
            string(REPLACE "\"" "" VERSION_ONE_LINE ${VERSION_ONE_LINE})
        endif()

        set(VERSION_VER_NUM -1)
    else(GIT_FOUND)
            set(VERSION_ONE_LINE "Missing VCS info, under bleeding")
            set(VERSION_LAST_AUTHOR "Unknown")
    endif()

    # add git hooks
    if (EXISTS "${PROJECT_SOURCE_DIR}/tools/hooks/")
        set(GIT_HOOK_SRC "${PROJECT_SOURCE_DIR}/tools/hooks/")
        if(EXISTS "${PROJECT_SOURCE_DIR}/.git/hooks")
            set(GIT_HOOK_DST "${PROJECT_SOURCE_DIR}/.git/hooks/")
            file(COPY ${GIT_HOOK_SRC} DESTINATION ${GIT_HOOK_DST})
            message(STATUS "Install git hooks done")
        endif(EXISTS "${PROJECT_SOURCE_DIR}/.git/hooks")
    endif(EXISTS "${PROJECT_SOURCE_DIR}/tools/hooks/")

else(EXISTS "${PROJECT_SOURCE_DIR}/.git")
    set(VERSION_ONE_LINE "Without VCS, under bleeding")
    set(VERSION_LAST_AUTHOR "Unknown")
endif(EXISTS "${PROJECT_SOURCE_DIR}/.git")

# ----------------------------------------------------------------------------
#  Start module definition
# ----------------------------------------------------------------------------
# project overall include file
include_directories(inc)
# small utile functions for test case
include_directories(utils)

# ----------------------------------------------------------------------------
#  osal library
# ----------------------------------------------------------------------------
# Operation System Abstract Layer (OSAL) include
include_directories(osal/inc)
# OSAL is needed on all platform, do not need option
add_subdirectory(osal)

# ----------------------------------------------------------------------------
#  utils for test case
# ----------------------------------------------------------------------------
add_subdirectory(utils)

# ----------------------------------------------------------------------------
#  Media Process Platform library
# ----------------------------------------------------------------------------
# Media Process Platform include
add_subdirectory(mpp)

# ----------------------------------------------------------------------------
#  test / demo
# ----------------------------------------------------------------------------
add_subdirectory(test)

CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/inc/config.h.cmake"
"${CMAKE_CURRENT_SOURCE_DIR}/inc/config.h")

CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/pkgconfig/rockchip_mpp.pc.cmake"
"${CMAKE_CURRENT_BINARY_DIR}/rockchip_mpp.pc" @ONLY)

CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/pkgconfig/rockchip_vpu.pc.cmake"
"${CMAKE_CURRENT_BINARY_DIR}/rockchip_vpu.pc" @ONLY)

INSTALL(FILES "${CMAKE_CURRENT_BINARY_DIR}/rockchip_mpp.pc"
DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig/")

INSTALL(FILES "${CMAKE_CURRENT_BINARY_DIR}/rockchip_vpu.pc"
DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig/")

INSTALL(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/inc/"
DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/rockchip"
PATTERN "config.h" EXCLUDE
REGEX "cmake$" EXCLUDE
)

# End generate the build configure file
