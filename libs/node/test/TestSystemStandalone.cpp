/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unistd.h>

#include "ecflow/node/Signal.hpp"
#include "ecflow/node/System.hpp"

using namespace ecf;

// leap42 boost_1_64_0 gcc-5.3.0 release mode
//    time bin/gcc-5.3.0/release/perf_aparser_only ${ECF_TEST_DEFS_DIR}/vsms2.31415.def
//    real    0m2.79s
//    user    0m2.59s
//    sys     0m0.18s

int main(int argc, char* argv[]) {
    std::cout << "argc = " << argc << "\n";
    for (int i = 0; i < argc; i++) {
        std::cout << "arg " << i << ":" << argv[i] << "\n";
    }

    if (argc != 2) {
        std::cout << "Expect single argument \n";
        return 1;
    }

    std::cout << "Invoke command: " << argv[1] << "\n";
    std::string errorMsg;
    if (!System::instance()->spawn(System::ECF_STATUS_CMD, argv[1], "", errorMsg)) {
        throw std::runtime_error(errorMsg);
    }

    while (System::instance()->process() != 0) {
        // cout << "no of System::instance()->process() " << System::instance()->process() << "\n";

        // Capture child process termination. Child sends SIGNAl SIGCHLD, caught by parent
        Signal unblock_on_desctruction_then_reblock;
        sleep(1); // Need to wait for child termination()
        System::instance()->processTerminatedChildren();
    }

    return 0;
}
