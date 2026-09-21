// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <string>

namespace ecf {

///
/// @brief Return the login name of the current user.
///
/// @return The login name of the current user as a string.
///
std::string get_login_name();

} // namespace ecf
