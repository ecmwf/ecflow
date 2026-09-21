// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <string>

namespace ecf::http {

class BasicAuth {
public:
    static std::pair<std::string, std::string> get_credentials(const std::string& token);
};

} // namespace ecf::http
