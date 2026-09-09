/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_http_JSON_HPP
#define ecflow_http_JSON_HPP

#include "nlohmann/json.hpp"

namespace ecf::http {

using ojson = nlohmann::ordered_json;

std::string json_type_to_string(const ojson& j);

} // namespace ecf::http

#endif /* ecflow_http_JSON_HPP */
