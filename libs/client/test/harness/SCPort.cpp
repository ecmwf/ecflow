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
    return ClientInvoker::find_free_port(thePort_, debug);
}

} // namespace ecf
