/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ecflow/node/CmdContext.hpp"

namespace ecf {
bool CmdContext::in_command_ = false;

CmdContext::CmdContext() {
    in_command_ = true;
}

CmdContext::~CmdContext() {
    in_command_ = false;
}

} // namespace ecf
