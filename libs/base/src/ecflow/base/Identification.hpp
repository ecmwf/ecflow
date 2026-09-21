// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "ecflow/base/Cmd.hpp"
#include "ecflow/core/Identity.hpp"

namespace ecf {

Identity identify(const Cmd_ptr& cmd);

} // namespace ecf
