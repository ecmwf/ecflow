/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/base/Authentication.hpp"

#include "ecflow/base/cts/ClientToServerCmd.hpp"
#include "ecflow/server/BaseServer.hpp"

namespace ecf {

authentication_t is_authentic(const ClientToServerCmd& command, AbstractServer& server) {
    return command.authenticate(server);
}

} // namespace ecf
