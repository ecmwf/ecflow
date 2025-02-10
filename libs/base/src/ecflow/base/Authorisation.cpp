/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
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
