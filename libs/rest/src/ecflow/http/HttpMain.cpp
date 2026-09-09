/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
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
        std::cerr << "Failed to set signal mask" << std::endl;
        return 1;
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
