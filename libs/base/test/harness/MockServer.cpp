/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#include "MockServer.hpp"

void MockServer::set_server_state(SState::State ss) {
    serverState_    = ss;
    stats().status_ = static_cast<int>(serverState_);
    defs_->server_state().set_state(serverState_);
}
