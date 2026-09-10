/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
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
    ((expr) ? ((void)0) : UIDebug::uiAssert(#expr, __FILE__, __LINE__, MESSAGE(EXPRESSION)))

#endif /* ecflow_viewer_UIDebug_HPP */
