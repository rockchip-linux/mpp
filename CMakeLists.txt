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
option(BUILD_TEST "enable test binary building)" ON)

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
elseif(${SYSPROC} STREQUAL "armv7-a_hardfp" OR ${SYSPROC} STREQUAL "armv7l")
    message(STATUS "Detected ARMv7 system processor")
    set(ARM true)
    set(ARMEABI_V7A_HARDFP true)
elseif(${SYSPROC} STREQUAL "aarch64")
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
    # disable exception for C++
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-exceptions -fno-rtti")

    # for libary linking
    set(BEGIN_WHOLE_ARCHIVE -Wl,--whole-archive)
    set(END_WHOLE_ARCHIVE -Wl,--no-whole-archive)

    option(ASAN_CHECK "enable Address Sanitizer (Asan)" OFF)
    if(ASAN_CHECK)
        add_definitions(-fsanitize=address -static-libasan -g)
        set(ASAN_LIB libasan.a dl rt m)
        set(ASAN_BIN dl rt m)
    endif(ASAN_CHECK)
endif(GCC)

# ----------------------------------------------------------------------------
# Create git version information
# ----------------------------------------------------------------------------
set(VERSION_CNT         0)
set(VERSION_MAX_CNT     9)
set(VERSION_INFO        "\"unknown mpp version for missing VCS info\"")
foreach (CNT RANGE ${VERSION_MAX_CNT})
    set(VERSION_HISTORY_${CNT} "NULL")
endforeach(CNT)

if(EXISTS "${PROJECT_SOURCE_DIR}/.git")
    find_host_package(Git)
    if(GIT_FOUND)
        # get current version info
        set(GIT_LOG_FORMAT "%h author: %<|(30)%an %cd %s")

        execute_process(COMMAND ${GIT_EXECUTABLE} log -1 --oneline --date=short --pretty=format:${GIT_LOG_FORMAT}
            WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
            OUTPUT_VARIABLE EXEC_OUT
            ERROR_VARIABLE EXEC_ERROR
            RESULT_VARIABLE EXEC_RET
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_STRIP_TRAILING_WHITESPACE)

        if (NOT EXEC_RET)
            set(VERSION_INFO ${EXEC_OUT})
            message(STATUS "current version:")
            message(STATUS "${VERSION_INFO}")
            string(REPLACE "\"" "\\\"" VERSION_INFO ${VERSION_INFO})
            set(VERSION_INFO "\"${VERSION_INFO}\"")
        else()
            message(STATUS "git ret ${EXEC_RET}")
            message(STATUS "${EXEC_ERROR}")
        endif()

        set(GIT_LOG_FORMAT "%h author: %<|(30)%an %cd %s %d")

        # get history version information
        # setup logs
        message(STATUS "git version history:")
        foreach (CNT RANGE ${VERSION_MAX_CNT})
            execute_process(COMMAND ${GIT_EXECUTABLE} log HEAD~${CNT} -1 --oneline --date=short --pretty=format:${GIT_LOG_FORMAT}
                WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
                OUTPUT_VARIABLE EXEC_OUT
                ERROR_VARIABLE EXEC_ERROR
                RESULT_VARIABLE EXEC_RET
                OUTPUT_STRIP_TRAILING_WHITESPACE
                ERROR_STRIP_TRAILING_WHITESPACE)

            if (NOT EXEC_RET)
                set(VERSION_LOG ${EXEC_OUT})
                string(REPLACE "\"" "\\\"" VERSION_LOG ${VERSION_LOG})
                message(STATUS ${VERSION_LOG})
                set(VERSION_HISTORY_${CNT} "\"${VERSION_LOG}\"")
                math(EXPR VERSION_CNT "${VERSION_CNT}+1")
            endif()
        endforeach(CNT)
        message(STATUS "total ${VERSION_CNT} git version recorded")
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
