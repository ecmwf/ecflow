// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <iostream>

#include "ecflow/base/PermissionDenied.hpp"
#include "ecflow/client/ClientInvoker.hpp"
#include "ecflow/core/CommandLine.hpp"

int main(int argc, char* argv[]) {

    /// By default, error condition will throw exception.
    try {
        ClientInvoker client;
        client.set_cli(true); // output log and file commands to standard out
        (void)client.invoke(CommandLine(argc, argv));
    }
    catch (ecf::PermissionDenied& e) {
        // A request denied by the authorisation of the server has an exit status of its own
        std::cerr << e.what() << std::endl;
        return 3;
    }
    catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    return 0;
}
