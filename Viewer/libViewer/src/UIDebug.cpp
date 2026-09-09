/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#include "UIDebug.hpp"

#include <iostream>

void UIDebug::uiAssert(char const* expr, char const* file, long line, const std::string& message) {
    std::stringstream ss;
    ss << "ASSERT failure: " << expr << " at " << file << ":" << line << " " << message;
    std::string assert_msg = ss.str();
    std::cerr << assert_msg << "\n";
    exit(1);
}
