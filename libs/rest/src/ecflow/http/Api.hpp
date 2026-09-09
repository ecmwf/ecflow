/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_http_Api_HPP
#define ecflow_http_Api_HPP

#include "ecflow/core/HttpLibrary.hpp"

namespace ecf::http {

void setup(httplib::Server& server);
void teardown();

} // namespace ecf::http

#endif /* ecflow_http_Api_HPP */
