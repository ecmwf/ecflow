/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
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
