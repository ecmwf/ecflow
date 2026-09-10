/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#include <iostream>
#include <utility>

#include "ecflow/udp/UDPClient.hpp"

int main(int argc, char* argv[]) {
    if (argc != 4) {
        std::cerr << "Usage: udp_client <host> <port> <request>" << std::endl;
        return EXIT_FAILURE;
    }

    try {
        std::string host    = argv[1];
        std::string port    = argv[2];
        std::string request = argv[3];

        ecf::UDPClient client(host, port);
        client.send(request);
    }
    catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    catch (...) {
        std::cerr << "Unknown error..." << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
