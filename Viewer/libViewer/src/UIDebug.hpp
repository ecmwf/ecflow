/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_viewer_UIDebug_HPP
#define ecflow_viewer_UIDebug_HPP

#include "ecflow/core/Log.hpp" // from ACore

class UIDebug {
public:
    UIDebug()  = default;
    ~UIDebug() = default;

    static void uiAssert(char const* expr, char const* file, long line, const std::string& message);
};

#define UI_ASSERT(expr, EXPRESSION) \
    ((expr) ? ((void)0) : UIDebug::uiAssert(#expr, __FILE__, __LINE__, STRINGIZE(EXPRESSION)))

#endif /* ecflow_viewer_UIDebug_HPP */
