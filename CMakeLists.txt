############################################################################
# Copyright (c) 2022, Dr. Thorsten Beier                                   #
# Copyright (c) 2022, QuantStack                                           #
#                                                                          #
# Distributed under the terms of the BSD 3-Clause License.                 #
#                                                                          #
# The full license is in the file LICENSE, distributed with this software. #
############################################################################

cmake_minimum_required(VERSION 3.4.3)
project(xeus-qt-python)

set(XEUS_PYTHON_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Versionning
# ===========

file(STRINGS "${XEUS_PYTHON_INCLUDE_DIR}/xeus-qt-python/xeus_qt_python_config.hpp" XQTPY_version_defines
     REGEX "#define XQTPY_VERSION_(MAJOR|MINOR|PATCH)")
foreach (ver ${XQTPY_version_defines})
    if (ver MATCHES "#define XQTPY_VERSION_(MAJOR|MINOR|PATCH) +([^ ]+)$")
        set(XQTPY_VERSION_${CMAKE_MATCH_1} "${CMAKE_MATCH_2}" CACHE INTERNAL "")
    endif ()
endforeach ()
set(${PROJECT_NAME}_VERSION
    ${XQTPY_VERSION_MAJOR}.${XQTPY_VERSION_MINOR}.${XQTPY_VERSION_PATCH})
message(STATUS "Building xeus-qt-python v${${PROJECT_NAME}_VERSION}")

# Configuration
# =============

include(GNUInstallDirs)

if (NOT DEFINED XPYTHON_KERNELSPEC_PATH)
    set(XPYTHON_KERNELSPEC_PATH "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}/")
endif ()

# Running find_package(PythonInterp) to retrieve the Python version
# which is not exported by Pybind11's cmake.
# Cf. https://github.com/pybind/pybind11/issues/2268
find_package(PythonInterp ${PythonLibsNew_FIND_VERSION} REQUIRED)

# Build options
# =============

# Compilation options

OPTION(XQTPY_DISABLE_ARCH_NATIVE "disable -march=native flag" OFF)
OPTION(XQTPY_DISABLE_TUNE_GENERIC "disable -mtune=generic flag" OFF)
OPTION(XQTPY_ENABLE_PYPI_WARNING "Enable warning on PyPI wheels" OFF)



OPTION(XQTPY_USE_SHARED_XEUS "Link xpython or xpython_extension with the xeus shared library (instead of the static library)" ON)
OPTION(XQTPY_USE_SHARED_XEUS_PYTHON "Link xpython and xpython_extension with the xeus-qt-python shared library (instead of the static library)" ON)

# Test options
OPTION(XQTPY_BUILD_TESTS "xeus-qt-python test suite" OFF)
OPTION(XQTPY_DOWNLOAD_GTEST "build gtest from downloaded sources" OFF)



# Dependencies
# ============

set(xtl_REQUIRED_VERSION 0.7.0)
set(xeus_REQUIRED_VERSION 3.0.3)
set(xeus_zmq_REQUIRED_VERSION 1.0.2)
set(xq_REQUIRED_VERSION 0.0.1)
set(xeus_python_REQUIRED_VERSION 0.15.2)

set(pybind11_REQUIRED_VERSION 2.6.1)
set(pybind11_json_REQUIRED_VERSION 0.2.8)

find_package(Qt5 COMPONENTS Core REQUIRED)

if (NOT TARGET xtl)
    find_package(xtl ${xtl_REQUIRED_VERSION} REQUIRED)
endif ()
if (NOT TARGET xeus AND NOT TARGET xeus-static)
    find_package(xeus ${xeus_REQUIRED_VERSION} REQUIRED)
endif ()
if (NOT TARGET xeus-zmq AND NOT TARGET xeus-zmq-static)
    find_package(xeus-zmq ${xeus_zmq_REQUIRED_VERSION} REQUIRED)
endif ()
if (NOT TARGET xeus-python AND NOT TARGET xeus-python-static)
    find_package(xeus-python ${xeus_python_REQUIRED_VERSION} REQUIRED)
endif ()
if (NOT TARGET xq AND NOT TARGET xeus-qt-static)
    find_package(xq ${xq_REQUIRED_VERSION} REQUIRED)
endif ()
if (NOT TARGET pybind11::headers)
    find_package(pybind11 ${pybind11_REQUIRED_VERSION} REQUIRED)
endif ()
if (NOT TARGET pybind11_json)
    find_package(pybind11_json ${pybind11_json_REQUIRED_VERSION} REQUIRED)
endif ()



# Targets and link - Macros
# =========================

include(CheckCXXCompilerFlag)

string(TOUPPER "${CMAKE_BUILD_TYPE}" U_CMAKE_BUILD_TYPE)

macro(xqtpy_set_common_options target_name)
    if (MSVC)
        target_compile_options(${target_name} PUBLIC /wd4251 /wd4141)
        target_compile_options(${target_name} PUBLIC /wd4018 /wd4267 /wd4715 /wd4146 /wd4129)
    endif ()

    if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR
        CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR
        CMAKE_CXX_COMPILER_ID MATCHES "Intel")

        target_compile_options(${target_name} PUBLIC -Wunused-parameter -Wextra -Wreorder)

        # Mtune generic/native
        if (XQTPY_DISABLE_ARCH_NATIVE AND NOT XQTPY_DISABLE_TUNE_GENERIC)
            target_compile_options(${target_name} PUBLIC -mtune=generic)
        elseif (XQTPY_DISABLE_TUNE_GENERIC)
        else ()
            target_compile_options(${target_name} PUBLIC -march=native)
        endif ()

        # C++14 flag
        CHECK_CXX_COMPILER_FLAG("-std=c++14" HAS_CPP14_FLAG)
        if (HAS_CPP14_FLAG)
            target_compile_features(${target_name} PRIVATE cxx_std_14)
        else ()
            message(FATAL_ERROR "Unsupported compiler -- xeus-python requires C++14 support!")
        endif ()
    endif ()
endmacro()

# Common macro kernels (xpython and xpython_extension)
macro(xqtpy_set_kernel_options target_name)
    if(XQTPY_ENABLE_PYPI_WARNING)
        message(STATUS "Enabling PyPI warning for target: " ${target_name})
        target_compile_definitions(${target_name} PRIVATE XEUS_PYTHON_PYPI_WARNING)
    endif()
endmacro()

# Common macro for linking targets
macro(xqtpy_target_link_libraries target_name)
    if (XQTPY_USE_SHARED_XEUS_PYTHON)
        target_link_libraries(${target_name} PRIVATE xeus-python xq)

        if(CMAKE_DL_LIBS)
            target_link_libraries(${target_name} PRIVATE ${CMAKE_DL_LIBS} util)
        endif()
    else ()
        target_compile_definitions(${target_name} PUBLIC "XEUS_PYTHON_STATIC_LIB")
        target_link_libraries(${target_name} PRIVATE xeus-python-static xq)
    endif()

    find_package(Threads) # TODO: add Threads as a dependence of xeus or xeus-static?
    target_link_libraries(${target_name} PRIVATE ${CMAKE_THREAD_LIBS_INIT})
    set_target_properties(${target_name} PROPERTIES INSTALL_RPATH_USE_LINK_PATH TRUE)
endmacro()

# Source files
# ============

set(XEUS_QT_PYTHON_EXTENSION_SRC
    src/x_qt_python_extension.cpp
)

set(XEUS_PYTHON_HEADERS
    include/xeus-qt-python/xeus_python_config.hpp
)



pybind11_add_module(xqtpython ${XEUS_QT_PYTHON_EXTENSION_SRC})

xqtpy_set_common_options(xqtpython)
xqtpy_set_kernel_options(xqtpython)
xqtpy_target_link_libraries(xqtpython)


execute_process(
  COMMAND "${PYTHON_EXECUTABLE}" -c "if True:
    from distutils import sysconfig as sc
    print(sc.get_python_lib(prefix='', plat_specific=True))"
  OUTPUT_VARIABLE PYTHON_SITE
  OUTPUT_STRIP_TRAILING_WHITESPACE)

install(TARGETS xqtpython
  COMPONENT python
  LIBRARY DESTINATION "${PYTHON_SITE}"
  )
