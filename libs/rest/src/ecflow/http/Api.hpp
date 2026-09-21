// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "ecflow/core/HttpLibrary.hpp"

namespace ecf::http {

void setup(httplib::Server& server);
void teardown();

} // namespace ecf::http
