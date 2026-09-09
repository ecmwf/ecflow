/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ecflow/base/Authentication.hpp"

#include "ecflow/base/cts/ClientToServerCmd.hpp"
#include "ecflow/server/BaseServer.hpp"

namespace ecf {

authentication_t is_authentic(const ClientToServerCmd& command, AbstractServer& server) {
    return command.authenticate(server);
}

} // namespace ecf
