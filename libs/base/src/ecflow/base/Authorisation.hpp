// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "ecflow/base/AbstractServer.hpp"
#include "ecflow/core/Result.hpp"

namespace ecf {

using authorisation_t = Result<bool>;

authorisation_t is_authorised(const ClientToServerCmd& command, AbstractServer& server);

} // namespace ecf
