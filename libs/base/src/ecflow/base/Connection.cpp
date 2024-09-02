/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/base/Connection.hpp"

#include "ecflow/core/Ecf.hpp"
#include "ecflow/core/Log.hpp"

connection::~connection() {
#ifdef DEBUG_CONNECTION
    if (Ecf::server())
        std::cout << "SERVER: Connection::~connection  socket_.is_open() = " << socket_.is_open() << "\n\n";
    else
        std::cout << "CLIENT: Connection::~connection  socket_.is_open() = " << socket_.is_open() << "\n\n";
#endif
}

connection::connection(boost::asio::io_context& io) : socket_(io) {
#ifdef DEBUG_CONNECTION
    if (Ecf::server())
        std::cout << "SERVER: Connection::connection\n";
    else
        std::cout << "CLIENT: Connection::connection\n";
#endif
}

void connection::log_error(const char* msg) {
    const char* in_context = ", in client";
    if (Ecf::server())
        in_context = ", in server";
    ecf::LogToCout logToCout;
    LOG(ecf::Log::ERR, msg << in_context);
}

void connection::log_archive_error(const char* msg, const std::exception& ae, const std::string& data) {
    const char* in_context = ", in client";
    if (Ecf::server())
        in_context = ", in server";
    ecf::LogToCout logToCout;
    LOG(ecf::Log::ERR, msg << ae.what() << in_context << " data:\n" << data);
}
