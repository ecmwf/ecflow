/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/server/SslServer.hpp"

#include "ecflow/server/ServerEnvironment.hpp"

SslServer::SslServer(boost::asio::io_context& io, ServerEnvironment& serverEnv)
    : BaseServer(io, serverEnv),
      server_(this, io, serverEnv) {
}

const std::string& SslServer::ssl() const {
    return serverEnv_.openssl().ssl();
}
