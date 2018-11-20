# ----------------------------------------------------------------------------
#  Root CMake file for Rockchip Media Process Platform (MPP)
#
#   - 10:34 2015/7/27: Initial version <herman.chen@rock-chips.com>
#
# ----------------------------------------------------------------------------

# vim: syntax=cmake
if(NOT CMAKE_BUILD_TYPE)
    # default to Release build for GCC builds
    set(CMAKE_BUILD_TYPE Debug CACHE STRING
        "Choose the type of build, options are: None(CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel."
        FORCE)
endif()
message(STATUS "cmake version ${CMAKE_VERSION}")
if(NOT CMAKE_VERSION VERSION_LESS "2.8.12.20131121")
    cmake_policy(SET CMP0025 OLD) # report Apple's Clang as just Clang
    cmake_policy(SET CMP0042 OLD) # do not require MACOSX_RPATH
endif()

# Search packages for host system instead of packages for target system
# in case of cross compilation these macro should be defined by toolchain file
if(NOT COMMAND find_host_package)
    macro(find_host_package)
        find_package(${ARGN})
    endmacro()
endif()
if(NOT COMMAND find_host_program)
    macro(find_host_program)
        find_program(${ARGN})
    endmacro()
endif()

project (rk_mpp)

cmake_minimum_required (VERSION 2.8.8) # OBJECT libraries require 2.8.8
include(CheckIncludeFiles)
include(CheckFunctionExists)
include(CheckSymbolExists)
include(CheckCXXCompilerFlag)

# setup output library name
# Linux   default name - rockchip_mpp and rockchip_vpu
# Android default name - mpp and vpu
# For historical reason libraries on Android is named as mpp and vpu. But for
# better naming rule on Linux it should add vendor prefix.
# So we use this ugly method to avoid massive maintain issue.
if (NOT MPP_PROJECT_NAME)
    set(MPP_PROJECT_NAME rockchip_mpp)
endif()
set(MPP_STATIC ${MPP_PROJECT_NAME}_static)
set(MPP_SHARED ${MPP_PROJECT_NAME})

if (NOT VPU_PROJECT_NAME)
    set(VPU_PROJECT_NAME rockchip_vpu)
endif()
set(VPU_STATIC ${VPU_PROJECT_NAME}_static)
set(VPU_SHARED ${VPU_PROJECT_NAME})

# ----------------------------------------------------------------------------
# set property to classify library kinds
# ----------------------------------------------------------------------------
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "CMakeTargets")
# ----------------------------------------------------------------------------
# enable test in this project
# ----------------------------------------------------------------------------
enable_testing()

# ----------------------------------------------------------------------------
# System architecture detection
# ----------------------------------------------------------------------------
string(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" SYSPROC)
set(X86_ALIASES x86 i386 i686 x86_64 amd64)
list(FIND X86_ALIASES "${SYSPROC}" X86MATCH)
if("${CMAKE_C_COMPILER}" MATCHES "-buildroot-[^/]+$")
    message(STATUS "Detected Buildroot toolchain")
    # Use buildroot toolchain's default architecture settings
elseif("${SYSPROC}" STREQUAL "" OR X86MATCH GREATER "-1")
    message(STATUS "Detected x86 system processor")
    set(X86 true)
    add_definitions(-DARCH_X86=1)
    if("${CMAKE_SIZEOF_VOID_P}" MATCHES 8)
        set(X64 true)
        add_definitions(-DARCH_X64=1)
        message(STATUS "Define X86_64 to 1")
    endif()
elseif(${SYSPROC} STREQUAL "armv6l")
    message(STATUS "Detected ARMv6 system processor")
    set(ARM true)
    set(ARMEABI_V6 true)
elseif(${SYSPROC} STREQUAL "armv7-a")
    message(STATUS "Detected ARMv7 system processor")
    set(ARM true)
    set(ARMEABI_V7A true)
elseif(${SYSPROC} STREQUAL "armv7-a_hardfp")
    message(STATUS "Detected ARMv7 system processor")
    set(ARM true)
    set(ARMEABI_V7A_HARDFP true)
elseif(${SYSPROC} STREQUAL "armv8-a")
    message(STATUS "Detected ARMv8 system processor")
    set(ARM true)
    set(ARMEABI_V8 true)
else()
    message(STATUS "CMAKE_SYSTEM_PROCESSOR value `${CMAKE_SYSTEM_PROCESSOR}` is unknown")
    message(STATUS "Please add this value near ${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_LIST_LINE}")
endif()

if(UNIX)
    SET(PLATFORM_LIBS pthread)
    if(NOT ${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
        SET(PLATFORM_LIBS ${PLATFORM_LIBS} rt)
    endif()
endif(UNIX)

# ----------------------------------------------------------------------------
# Compiler detection
# ----------------------------------------------------------------------------
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    set(CLANG true)
endif()
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
    set(INTEL_CXX true)
endif()
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    set(GCC true)
endif()

if(INTEL_CXX AND UNIX)
    # treat icpc roughly like gcc
    set(GCC true)
    add_definitions(-Wall -Wextra -Wshadow)
elseif(CLANG)
    # treat clang roughly like gcc
    set(GCC true)
    add_definitions(-Wall -Wextra -Wshadow -ffast-math)
elseif(CMAKE_COMPILER_IS_GNUCXX)
    add_definitions(-Wall -Wextra -Wshadow -ffast-math)
    check_cxx_compiler_flag(-Wno-narrowing GCC_HAS_NO_NARROWING)
    check_cxx_compiler_flag(-mstackrealign GCC_HAS_STACK_REALIGN)
    if (GCC_HAS_STACK_REALIGN)
        add_definitions(-mstackrealign)
    endif()
    execute_process(COMMAND ${CMAKE_CXX_COMPILER} -dumpversion OUTPUT_VARIABLE GCC_VERSION)
endif()

if(GCC)
    if(ARM)
        if(ARMEABI_V6)
            add_definitions(-march=armv6 -mfloat-abi=hard -mfpu=vfp)
        elseif(ARMEABI_V7A)
            add_definitions(-march=armv7-a -mfloat-abi=softfp -mfpu=neon)
        elseif(ARMEABI_V7A_HARDFP)
            add_definitions(-march=armv7-a -mfloat-abi=hard -mfpu=neon)
        elseif(ARMEABI_V8)
            add_definitions(-march=armv8-a)
        endif()
    else()
        if(X86 AND NOT X64)
            add_definitions(-march=i686)
        endif()
    endif()

    if($(CMAKE_BUILD_TYPE) MATCHES "Release")
        set(CMAKE_C_VISIBILITY_PRESET hidden)
        set(CMAKE_CXX_VISIBILITY_PRESET hidden)
    endif()

    # disable multichar warning
    add_definitions(-Wno-multichar)
    # add PIC flag
    add_definitions(-fPIC)
endif(GCC)

# ----------------------------------------------------------------------------
# Create svn version information
# ----------------------------------------------------------------------------
set(HAVE_SVN 0)
if(EXISTS "${PROJECT_SOURCE_DIR}/.svn/")
    find_host_package(Subversion)
    if(Subversion_FOUND)
        set(HAVE_SVN true)
    endif(Subversion_FOUND)
endif(EXISTS "${PROJECT_SOURCE_DIR}/.svn/")

if(${HAVE_SVN})
    Subversion_WC_INFO(${PROJECT_SOURCE_DIR} Project)

    message(STATUS "Current svn revision is ${Project_WC_REVISION}")
    set(VERSION_REVISION        ${Project_WC_REVISION})
    set(VERSION_LAST_AUTHOR     ${Project_WC_LAST_CHANGED_AUTHOR})
    set(VERSION_LAST_DATE       ${Project_WC_LAST_CHANGED_DATE})
    set(VERSION_ONE_LINE        "${VERSION_REVISION} author: ${VERSION_LAST_AUTHOR} date: ${VERSION_LAST_DATE}")
    set(VERSION_VER_NUM         ${VERSION_REVISION})
else()
    set(VERSION_REVISION -1)
endif()

# ----------------------------------------------------------------------------
# Create git version information
# ----------------------------------------------------------------------------
if(EXISTS "${PROJECT_SOURCE_DIR}/.git")
    find_host_package(Git)
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
            message(STATUS "date  : ${EXEC_OUT}")
            set(VERSION_LAST_DATE ${EXEC_OUT})
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
        set(VERSION_ONE_LINE "Missing VCS info")
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
    set(VERSION_ONE_LINE "Without VCS info")
    set(VERSION_LAST_AUTHOR "Unknown")
endif(EXISTS "${PROJECT_SOURCE_DIR}/.git")

configure_file(
    "${PROJECT_SOURCE_DIR}/build/cmake/version.in"
    "${PROJECT_SOURCE_DIR}/mpp/version.h"
)

# ----------------------------------------------------------------------------
# Build options
# ----------------------------------------------------------------------------
find_package(PkgConfig)
INCLUDE(GNUInstallDirs)
pkg_search_module(PTHREAD pthread)

# ----------------------------------------------------------------------------
# Set Warning as Error
# ----------------------------------------------------------------------------
option(WARNINGS_AS_ERRORS "Stop compiles on first warning" OFF)
if(WARNINGS_AS_ERRORS)
    if(GCC)
        add_definitions(-Werror)
    elseif(MSVC)
        add_definitions(/WX)
    endif()
endif(WARNINGS_AS_ERRORS)

# ----------------------------------------------------------------------------
# look for stdint.h
# ----------------------------------------------------------------------------
if(MSVC)
    check_include_files(stdint.h HAVE_STDINT_H)
    if(NOT HAVE_STDINT_H)
        include_directories(osal/windows)
    endif(NOT HAVE_STDINT_H)
endif(MSVC)

# ----------------------------------------------------------------------------
# Share library option
# ----------------------------------------------------------------------------
option(ENABLE_STATIC "Build shared library" ON)
option(ENABLE_SHARED "Build shared library" OFF)

# ----------------------------------------------------------------------------
# scan all LOG_TAG for log information and generate module header file
# ----------------------------------------------------------------------------
set( module_list "" )
file ( GLOB_RECURSE ALL_SRC . *.c;*.cpp )
foreach( files ${ALL_SRC} )
    file( STRINGS ${files} module_tag_line REGEX "MODULE_TAG( )+\".+\"" )
    if(module_tag_line)
        string( REGEX REPLACE "^(.)* MODULE_TAG( )+\"(.*)\"" \\3 module_tag ${module_tag_line} )
        list( APPEND module_list ${module_tag} )
    endif()
endforeach()
list( SORT module_list )
list( LENGTH module_list module_size )
#message(STATUS "module_list: ${module_list}")
#message(STATUS "module_size: ${module_size}")

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
include_directories(mpp/inc)
add_subdirectory(mpp)

# ----------------------------------------------------------------------------
#  test / demo
# ----------------------------------------------------------------------------
add_subdirectory(test)

# ----------------------------------------------------------------------------
#  install headers
# ----------------------------------------------------------------------------
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/inc/
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/rockchip"
        FILES_MATCHING PATTERN "*.h"
        )

# ----------------------------------------------------------------------------
#  pkgconfig
# ----------------------------------------------------------------------------
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/pkgconfig/rockchip_mpp.pc.cmake"
                "${CMAKE_BINARY_DIR}/rockchip_mpp.pc" @ONLY)
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/pkgconfig/rockchip_vpu.pc.cmake"
                "${CMAKE_BINARY_DIR}/rockchip_vpu.pc" @ONLY)
install(FILES "${CMAKE_BINARY_DIR}/rockchip_mpp.pc"
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig/")
install(FILES "${CMAKE_BINARY_DIR}/rockchip_vpu.pc"
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig/")
