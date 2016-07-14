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

option(RKPLATFORM "Enable RK HW CONFIG" OFF)
if(${CMAKE_RKPLATFORM_ENABLE} MATCHES "ON")
    option(RKPLATFORM "Enable RK HW CONFIG" ON)
    set(RKPLATFORM True)
    add_definitions(-DRKPLATFORM)
endif()

project (rk_mpp)

cmake_minimum_required (VERSION 2.8.8) # OBJECT libraries require 2.8.8
include(CheckIncludeFiles)
include(CheckFunctionExists)
include(CheckSymbolExists)
include(CheckCXXCompilerFlag)

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
# add debug define in project
# ----------------------------------------------------------------------------
if(NOT $(CMAKE_BUILD_TYPE) MATCHES "Release")
    option(RK_DEBUG "Enable run-time debug mode(debugging)" ON)
    if(RK_DEBUG)
        add_definitions(-DRK_DEBUG)
        message(STATUS "rk_mpp debug mode is enabled")
    endif()
endif()

# ----------------------------------------------------------------------------
# System architecture detection
# ----------------------------------------------------------------------------
string(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" SYSPROC)
set(X86_ALIASES x86 i386 i686 x86_64 amd64)
list(FIND X86_ALIASES "${SYSPROC}" X86MATCH)
if("${SYSPROC}" STREQUAL "" OR X86MATCH GREATER "-1")
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
    add_definitions(-DARCH_ARM=1 -DDHAVE_ARMV6=1)
elseif(${SYSPROC} STREQUAL "armv7-a")
    message(STATUS "Detected ARMv7 system processor")
    set(ARM true)
    set(ARMEABI_V7A true)
    add_definitions(-DARCH_ARM=1 -DDHAVE_ARMV7=1)
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
if(CMAKE_GENERATOR STREQUAL "Xcode")
  set(XCODE true)
endif()

if (APPLE)
  add_definitions(-DMACOS)
endif()

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    set(CLANG true)
endif()
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
    set(INTEL_CXX true)
endif()
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    set(GCC true)
endif()

if(INTEL_CXX AND WIN32)
    # treat icl roughly like MSVC
    set(MSVC true)
endif()
if(MSVC)
    option(STATIC_LINK_CRT "Statically link C runtime for release builds" OFF)
    if (STATIC_LINK_CRT)
        set(CompilerFlags CMAKE_CXX_FLAGS_RELEASE CMAKE_C_FLAGS_RELEASE)
        foreach(CompilerFlag ${CompilerFlags})
            string(REPLACE "/MD" "/MT" ${CompilerFlag} "${${CompilerFlag}}")
        endforeach()
    endif (STATIC_LINK_CRT)
    add_definitions(/W3)  # Full warnings
    add_definitions(/Ob2) # always inline
    add_definitions(/MP)  # multithreaded build
    # disable Microsofts suggestions for proprietary secure APIs
    add_definitions(/D_CRT_SECURE_NO_WARNINGS)
endif(MSVC)

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
    if(X64 AND NOT WIN32)
        add_definitions(-fPIC)
    endif(X64 AND NOT WIN32)
    if(X86 AND NOT X64)
        add_definitions(-march=i686)
    endif()
    if(ARM)
        if(ARMEABI_V6)
            add_definitions(-march=armv6 -mfloat-abi=hard -mfpu=vfp)
        elseif(ARMEABI_V7A)
            add_definitions(-march=armv7-a -mfloat-abi=softfp -mfpu=neon)
        endif()
    endif()
endif(GCC)

# ----------------------------------------------------------------------------
# Create svn version information
# ----------------------------------------------------------------------------
set(HAVE_SVN 0)
if(EXISTS "${PROJECT_SOURCE_DIR}/.svn/")
    find_host_package(Subversion)
    if(Subversion_FOUND)
        set(HAVE_SVN true)
    else()
        if(WIN32)
            # The official subversion client is not available, so fall back to
            # tortoise if it is installed.
            find_program(TORTOISE_WCREV_EXECUTABLE
                NAMES SubWCRev.exe
                PATHS "[HKEY_LOCAL_MACHINE\\SOFTWARE\\TortoiseSVN;Directory]/bin"
            )

            if (NOT TORTOISE_WCREV_EXECUTABLE)
                message(STATUS "TortoiseSVN was not found.")
            else(NOT TORTOISE_WCREV_EXECUTABLE)
                message(STATUS "TortoiseSVN was Found.")
                set(HAVE_SVN true)

                macro(Subversion_WC_INFO dir prefix)
                    # the subversion commands should be executed with the C locale, otherwise
                    # the message (which are parsed) may be translated, Alex
                    set(_Subversion_SAVED_LC_ALL "$ENV{LC_ALL}")
                    set(ENV{LC_ALL} C)

                    execute_process(COMMAND ${TORTOISE_WCREV_EXECUTABLE} ${dir}
                    OUTPUT_VARIABLE ${prefix}_WC_INFO
                    ERROR_VARIABLE Subversion_svn_info_error
                    RESULT_VARIABLE Subversion_svn_info_result
                    OUTPUT_STRIP_TRAILING_WHITESPACE
                    ERROR_STRIP_TRAILING_WHITESPACE)

                    if(NOT ${Subversion_svn_info_result} EQUAL 0)
                        message(SEND_ERROR "Command \"${TORTOISE_WCREV_EXECUTABLE} ${dir}\" failed with output:\n${Subversion_svn_info_error}")
                    else()
                        string(REGEX REPLACE "^(.*\n)?Last committed at revision ([0-9]*)\n.*"
                        "\\2" ${prefix}_WC_REVISION "${${prefix}_WC_INFO}")
                    endif()
                    set(Project_WC_LAST_CHANGED_AUTHOR "unknown")
                    set(Project_WC_LAST_CHANGED_DATE   "unknown")

                    # restore the previous LC_ALL
                    set(ENV{LC_ALL} ${_Subversion_SAVED_LC_ALL})
                endmacro()
            endif (NOT TORTOISE_WCREV_EXECUTABLE)
        endif(WIN32)
    endif(Subversion_FOUND)

endif(EXISTS "${PROJECT_SOURCE_DIR}/.svn/")

if(${HAVE_SVN})
    Subversion_WC_INFO(${PROJECT_SOURCE_DIR} Project)

    message(STATUS "Current svn revision is ${Project_WC_REVISION}")
    set(VERSION_REVISION        ${Project_WC_REVISION})
    set(VERSION_LAST_AUTHOR     ${Project_WC_LAST_CHANGED_AUTHOR})
    set(VERSION_LAST_DATA       ${Project_WC_LAST_CHANGED_DATE})
else()
    set(VERSION_REVISION -1)
endif()

configure_file(
    "${PROJECT_SOURCE_DIR}/build/cmake/svn_info.in"
    "${PROJECT_SOURCE_DIR}/mpp/svn_info.h"
)


# ----------------------------------------------------------------------------
# Build options
# ----------------------------------------------------------------------------
set(CMAKE_INSTALL_PREFIX "")
set(LIB_INSTALL_DIR  "${CMAKE_SOURCE_DIR}/out/lib" CACHE STRING "Install location of libraries")
set(BIN_INSTALL_DIR  "${CMAKE_SOURCE_DIR}/out/bin" CACHE STRING "Install location of executables")
set(TEST_INSTALL_DIR "${CMAKE_BINARY_DIR}/test" CACHE STRING "Install location of unit test")


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
# disable multichar warning
add_definitions(-Wno-multichar)

# ----------------------------------------------------------------------------
# Visual leak detector
# ----------------------------------------------------------------------------
if (WIN32)
    find_host_package(VLD QUIET)
    if(VLD_FOUND)
        add_definitions(-DHAVE_VLD)
        include_directories(${VLD_INCLUDE_DIRS})
        set(PLATFORM_LIBS ${PLATFORM_LIBS} ${VLD_LIBRARIES})
        link_directories(${VLD_LIBRARY_DIRS})
    endif()
    option(WINXP_SUPPORT "Make binaries compatible with Windows XP" OFF)
    if(WINXP_SUPPORT)
        # force use of workarounds for CONDITION_VARIABLE and atomic
        # intrinsics introduced after XP
        add_definitions(-D_WIN32_WINNT=_WIN32_WINNT_WINXP)
    endif()
endif()

# ----------------------------------------------------------------------------
# look for stdint.h
# ----------------------------------------------------------------------------
if(MSVC)
    check_include_files(stdint.h HAVE_STDINT_H)
    if(NOT HAVE_STDINT_H)
        include_directories(osal/window)
    endif(NOT HAVE_STDINT_H)
endif(MSVC)

# ----------------------------------------------------------------------------
# On window import win32 pthread library
# ----------------------------------------------------------------------------
if(MSVC)
    set(WIN32_PTHREAD_INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/osal/window/pthread/inc")
    include_directories(${WIN32_PTHREAD_INCLUDE_DIRS})
    add_library(pthread SHARED IMPORTED)
    message(STATUS "platform X86 ${X86} X64 ${X64}")
    set(WIN32_PTHREAD_LIB_DIRS "${CMAKE_SOURCE_DIR}/osal/window/pthread/lib")
    set(WIN32_PTHREAD_DLL_DIRS "${CMAKE_SOURCE_DIR}/osal/window/pthread/dll")
    if(X64)
		set(WIN32_ARCH "x64")
	else()
		set(WIN32_ARCH "x86")
    endif()
    include_directories("${WIN32_PTHREAD_LIB_DIRS}/${WIN32_ARCH}")
    include_directories("${WIN32_PTHREAD_DLL_DIRS}/${WIN32_ARCH}")
    set_target_properties(pthread PROPERTIES
                          IMPORTED_LOCATION
                          "${WIN32_PTHREAD_LIB_DIRS}/${WIN32_ARCH}/pthreadVC2.dll")
    set_target_properties(pthread PROPERTIES
                          IMPORTED_IMPLIB
                          "${WIN32_PTHREAD_LIB_DIRS}/${WIN32_ARCH}/pthreadVC2.lib")
endif()

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
