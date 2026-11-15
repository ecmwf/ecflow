// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <stdexcept>
#include <string>

namespace ecf {

///
/// @brief Reports a request denied by the authorisation of the server.
///
/// It derives from std::runtime_error, which clients raise for every other refused request.
///
class PermissionDenied : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

} // namespace ecf
