/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ecflow/base/stc/ServerToClientCmd.hpp"

#include "ecflow/core/Str.hpp"

using namespace ecf;

ServerToClientCmd::~ServerToClientCmd() = default;

const std::string& ServerToClientCmd::get_string() const {
    return ecf::string_constants::empty;
}
