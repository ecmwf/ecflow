/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ecflow/base/Connection.hpp"

#include "ecflow/core/Ecf.hpp"
#include "ecflow/core/Log.hpp"

connection::~connection() {
#ifdef DEBUG_CONNECTION
    auto is_socket_open = socket_.is_open();
    auto location       = Ecf::server() ? "SERVER" : "CLIENT";
    std::cout << location << ": Connection::~connection socket_.is_open() = " << is_socket_open << "\n\n";
#endif
}

connection::connection(boost::asio::io_context& io)
    : socket_(io) {
#ifdef DEBUG_CONNECTION
    auto location = Ecf::server() ? "SERVER" : "CLIENT";
    std::cout << location << ": Connection::connection\n";
#endif
}

void connection::log_error(const char* msg) {
    const char* in_context = ", in client";
    if (Ecf::server()) {
        in_context = ", in server";
    }
    ecf::LogToCout logToCout;
    LOG(ecf::Log::ERR, msg << in_context);
}

void connection::log_archive_error(const char* msg, const std::exception& ae, const std::string& data) {
    const char* in_context = ", in client";
    if (Ecf::server()) {
        in_context = ", in server";
    }
    ecf::LogToCout logToCout;
    LOG(ecf::Log::ERR, msg << ae.what() << in_context << " data:\n" << data);
}
