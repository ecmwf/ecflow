/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/server/Server.hpp"

Server::Server(boost::asio::io_context& io, ServerEnvironment& serverEnv)
    : BaseServer(io, serverEnv),
      tcp_server_(this, io, serverEnv) {
}
