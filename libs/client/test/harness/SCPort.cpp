/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "SCPort.hpp"

#include <iostream>

#include "ecflow/client/ClientEnvironment.hpp"
#include "ecflow/client/ClientInvoker.hpp"
#include "ecflow/core/EcfPortLock.hpp"
#include "ecflow/core/Environment.hpp"
#include "ecflow/core/Str.hpp"

namespace ecf {

// init the globals. Note we dont use 3141, so that in the case where we already
// have a remote/local server started external to the test, it does not clash
// Also debug and release version use different port numbers to avoid clashes, if both tests run at same time

#ifdef DEBUG
int SCPort::thePort_ = 3161;
#else
int SCPort::thePort_ = 3144;
#endif

std::string SCPort::next() {
    bool debug = false;
    ecf::environment::get("ECF_DEBUG_TEST", debug);

    if (debug) {
        std::cout << "\nSCPort::next() : ";
    }

    // Allow parallel tests

    if (auto port = ecf::environment::fetch("ECF_FREE_PORT"); port) {
        if (debug) {
            std::cout << " seed_port=ECF_FREE_PORT=(" << port.value() << ")";
        }
        try {
            thePort_ = ecf::convert_to<int>(port.value());
        }
        catch (...) {
            std::cout << "SCPort::next()  ECF_FREE_PORT(" << port.value() << ") not convertible to an integer\n";
        }
    }

    // This is used to test remote servers(or legacy server with new client). Here ECF_HOST=localhost in the test
    // scripts
    std::string host = ClientEnvironment::hostSpecified();
    if (debug) {
        std::cout << " ECF_HOST('" << host << "')";
    }

    if (host == Str::LOCALHOST()) {
        if (auto port = ecf::environment::fetch(ecf::environment::ECF_PORT); port) {
            if (!port.value().empty()) {
                if (debug) {
                    std::cout << " ECF_PORT('" << port.value() << "')\n";
                }
                return port.value();
            }
        }
        if (debug) {
            std::cout << " !!!!!! ERROR when ECF_HOST=localhost  EXPECTED ECF_PORT to be set !!!!!! ";
        }
    }

    if (debug) {
        std::cout << "\n";
    }
    std::string the_port = next_only(debug);
    if (debug) {
        std::cout << " SCPort::next() returning free port=" << the_port << "\n";
    }
    return the_port;
}

std::string SCPort::next_only(bool debug) {
    if (debug) {
        std::cout << " SCPort::next_only : starting seed_port(" << thePort_ << ")\n";
    }

    // Use a combination of local lock file, and pinging the server
    while (!EcfPortLock::is_free(thePort_, debug)) {
        thePort_++;
    }

    if (debug) {
        std::cout << " SCPort::next_only() seed_port(" << thePort_ << ")\n";
    }
    return SCPort::find_free_port(thePort_, debug);
}

bool SCPort::is_free_port(int port, bool debug) {
    // Ping failed, We need to distinguish between:
    //    a/ Server does not exist : <FREE> port
    //    b/ Address in use        : <BUSY> port on existing server
    // Using server_version() but then get error messages
    // ******** Until this is done we can't implement port hopping **********

    if (debug) {
        std::cout << "  ClientInvoker::is_free_port: checking port " << port << "\n";
    }

    ClientInvoker client;
    client.set_retry_connection_period(1); // avoid long wait
    client.set_connection_attempts(1);     // avoid long wait

    const auto the_port = ecf::convert_to<std::string>(port);
    try {
        if (debug) {
            std::cout << "   Trying to connect to server on '" << Str::LOCALHOST() << ":" << the_port << "'\n";
        }
        client.set_host_port(Str::LOCALHOST(), the_port);
        client.pingServer();
        if (debug) {
            std::cout << "   Connected to server on port " << the_port << ". Returning FALSE\n";
        }
        return false;
    }
    catch (std::runtime_error& e) {
        std::string msg = e.what();
        if (debug) {
            std::cout << "   " << msg;
        }
        if (msg.find("authentication failed") != std::string::npos) {
            if (debug) {
                std::cout << "   Could not connect, due to authentication failure, hence port " << the_port
                          << " is used. Returning FALSE\n";
            }
            return false;
        }
        if (msg.find("invalid_argument") != std::string::npos) {
            if (debug) {
                std::cout << "   Mixing 4 and 5 series ?, hence port " << the_port << " is used. Returning FALSE\n";
            }
            return false;
        }
        else {
            if (debug) {
                std::cout << "   Found free port " << the_port << "\n";
            }
        }
    }
    return true;
}

std::string SCPort::find_free_port(int seed_port_number, bool debug) {
    // Ping failed, We need to distinguish between:
    //    a/ Server does not exist : <FREE> port
    //    b/ Address in use        : <BUSY> port on existing server
    // Using server_version() but then get error messages
    // ******** Until this is done we can't implement port hopping **********

    if (debug) {
        std::cout << "  ClientInvoker::find_free_port: starting with port " << seed_port_number << "\n";
    }
    int the_port = seed_port_number;
    std::string free_port;
    ClientInvoker client;
    client.set_retry_connection_period(1); // avoid long wait
    client.set_connection_attempts(1);     // avoid long wait
    while (true) {
        free_port = ecf::convert_to<std::string>(the_port);
        try {
            if (debug) {
                std::cout << "   Trying to connect to server on '" << Str::LOCALHOST() << ":" << free_port << "'\n";
            }
            client.set_host_port(Str::LOCALHOST(), free_port);
            client.pingServer();
            if (debug) {
                std::cout << "   Connected to server on port " << free_port << " trying next port\n";
            }
            the_port++;
        }
        catch (std::runtime_error& e) {
            std::string error_msg = e.what();
            if (debug) {
                std::cout << "   " << error_msg;
            }
            if (error_msg.find("authentication failed") != std::string::npos) {
                if (debug) {
                    std::cout << "   Could not connect, due to authentication failure, hence port " << the_port
                              << " is used, trying next port\n";
                }
                the_port++;
                continue;
            }
            if (error_msg.find("invalid_argument") != std::string::npos) {
                if (debug) {
                    std::cout << "   Mixing 4 and 5 series ?, hence port " << the_port
                              << " is used, trying next port\n";
                }
                the_port++;
                continue;
            }
            else {
                if (debug) {
                    std::cout << "   Found free port " << free_port << "\n";
                }
                break;
            }
        }
    }
    return free_port;
}

std::string SCPort::find_available_port(const std::string& port) {
    auto current_port = ecf::convert_to<int>(port);

    // The goal is to create a unique port number, allowing debug and release tests to run at the same time
    // Since several serves might run on same machine, on different workspaces, checking lock file is not sufficient.
    // We try to ensure the port is available by attempting to contact the server to confirm that the lock has been
    // performed.

    // (1) Search for port that isn't locked (i.e. for which there is no `.lock` file)
    for (int selected_port = EcfPortLock::try_next_port_lock(current_port, true); /* always advance */;
         selected_port     = EcfPortLock::try_next_port_lock(selected_port, true)) {
        if (SCPort::is_free_port(selected_port, true)) {
            // We found the free port that we wanted!
            return ecf::convert_to<std::string>(selected_port);
        }
        else {
            // This port is in use (maybe by a server running on another workspace)
            EcfPortLock::try_port_lock(selected_port);
        }
    }

    throw std::runtime_error("Unable to find an available port");
}

} // namespace ecf
