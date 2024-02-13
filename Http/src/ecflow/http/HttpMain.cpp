/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include <csignal>
#include <cstring>
#include <iostream>

#include "ecflow/http/HttpServer.hpp"

using ecf::http::HttpServer;

int main(int argc, char* argv[]) {
    // Sometimes we get SIGPIPE through openssl, when server is trying
    // to write to a socket which client has already closed.
    // Ignore this signal.
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGPIPE);

    if (pthread_sigmask(SIG_BLOCK, &set, nullptr) != 0) {
        throw std::runtime_error("Failed to set signal mask");
    }

    try {
        HttpServer server(argc, argv);
        server.run();
    }
    catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    return 0;
}
