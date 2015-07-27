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

project (rk_mpp)

cmake_minimum_required (VERSION 2.8.8) # OBJECT libraries require 2.8.8
include(CheckIncludeFiles)
include(CheckFunctionExists)
include(CheckSymbolExists)
include(CheckCXXCompilerFlag)

# ----------------------------------------------------------------------------
# add debug define in project
# ----------------------------------------------------------------------------
if(NOT $(CMAKE_BUILD_TYPE) MATCHES “Release”)
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
    set(X86 1)
    add_definitions(-DARCH_X86=1)
    if("${CMAKE_SIZEOF_VOID_P}" MATCHES 8)
        set(X64 1)
        add_definitions(-DARCH_X64=1)
        message(STATUS "Define X86_64 to 1")
    endif()
elseif(${SYSPROC} STREQUAL "armv6l")
    message(STATUS "Detected ARM system processor")
    set(ARM 1)
    add_definitions(-DARCH_ARM=1 -DDHAVE_ARMV6=1)
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
  set(XCODE 1)
endif()

if (APPLE)
  add_definitions(-DMACOS)
endif()

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    set(CLANG 1)
endif()
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
    set(INTEL_CXX 1)
endif()
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    set(GCC 1)
endif()

if(INTEL_CXX AND WIN32)
    # treat icl roughly like MSVC
    set(MSVC 1)
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
    set(GCC 1)
    add_definitions(-Wall -Wextra -Wshadow)
elseif(CLANG)
    # treat clang roughly like gcc
    set(GCC 1)
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
        add_definitions(-march=armv6 -mfloat-abi=hard -mfpu=vfp)
    endif()
endif(GCC)

# ----------------------------------------------------------------------------
# Build options
# ----------------------------------------------------------------------------
set(HDR_INSTALL_DIR "./out/inc" CACHE STRING "Install location of headers")
set(LIB_INSTALL_DIR "./out/lib" CACHE STRING "Install location of libraries")
set(BIN_INSTALL_DIR "./out/bin" CACHE STRING "Install location of executables")

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
# Visual leak detector
# ----------------------------------------------------------------------------
if (WIN32)
    find_package(VLD QUIET)
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
# Share library option
# ----------------------------------------------------------------------------
option(ENABLE_STATIC "Build shared library" ON)
option(ENABLE_SHARED "Build shared library" OFF)


# ----------------------------------------------------------------------------
#  Start module definition
# ----------------------------------------------------------------------------
# project overall include file
include_directories(inc)

# ----------------------------------------------------------------------------
#  osal library
# ----------------------------------------------------------------------------
# Operation System Abstract Layer (OSAL) include
include_directories(osal/inc)
# OSAL is needed on all platform, do not need option
add_subdirectory(osal)



