/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ecflow/base/Authorisation.hpp"

#include "ecflow/base/cts/ClientToServerCmd.hpp"
#include "ecflow/server/BaseServer.hpp"

namespace ecf {

authorisation_t is_authorised(const ClientToServerCmd& command, AbstractServer& server) {
    if (server.authorisation().good()) {
        return command.authorise(server);
    }

    // By default, i.e. when authorisations are not set, a command is always allowed
    return authorisation_t::success(true);
}

} // namespace ecf
