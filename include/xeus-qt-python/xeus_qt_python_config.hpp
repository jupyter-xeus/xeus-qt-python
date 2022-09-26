/***************************************************************************
* Copyright (c) 2018, Martin Renou, Johan Mabille, Sylvain Corlay, and     *
* Wolf Vollprecht                                                          *
* Copyright (c) 2018, QuantStack                                           *
*                                                                          *
* Distributed under the terms of the BSD 3-Clause License.                 *
*                                                                          *
* The full license is in the file LICENSE, distributed with this software. *
****************************************************************************/

#ifndef XQTPY_CONFIG_HPP
#define XQTPY_CONFIG_HPP

// Project version
#define XQTPY_VERSION_MAJOR 0
#define XQTPY_VERSION_MINOR 14
#define XQTPY_VERSION_PATCH 3

// Composing the version string from major, minor and patch
#define XQTPY_CONCATENATE(A, B) XQTPY_CONCATENATE_IMPL(A, B)
#define XQTPY_CONCATENATE_IMPL(A, B) A##B
#define XQTPY_STRINGIFY(a) XQTPY_STRINGIFY_IMPL(a)
#define XQTPY_STRINGIFY_IMPL(a) #a

#define XQTPY_VERSION XQTPY_STRINGIFY(XQTPY_CONCATENATE(XQTPY_VERSION_MAJOR,   \
                 XQTPY_CONCATENATE(.,XQTPY_CONCATENATE(XQTPY_VERSION_MINOR,   \
                                  XQTPY_CONCATENATE(.,XQTPY_VERSION_PATCH)))))

#ifdef _WIN32
    #ifdef XEUS_PYTHON_STATIC_LIB
        #define XEUS_QT_PYTHON_API
    #else
        #ifdef XEUS_PYTHON_EXPORTS
            #define XEUS_QT_PYTHON_API __declspec(dllexport)
        #else
            #define XEUS_QT_PYTHON_API __declspec(dllimport)
        #endif
    #endif
#else
    #define XEUS_QT_PYTHON_API
#endif

#ifdef _MSC_VER
    #define XQTPY_FORCE_PYBIND11_EXPORT
#else
    #define XQTPY_FORCE_PYBIND11_EXPORT __attribute__ ((visibility ("default")))
#endif
#endif
