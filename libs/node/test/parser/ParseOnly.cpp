/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#include <fstream>
#include <iostream>
#include <string>

#include "ecflow/node/Defs.hpp"

using namespace ecf;

// leap42 boost_1_64_0 gcc-5.3.0 release mode
//    time bin/gcc-5.3.0/release/perf_aparser_only ${ECF_TEST_DEFS_DIR}/vsms2.31415.def
//    real    0m2.79s
//    user    0m2.59s
//    sys     0m0.18s

int main(int argc, char* argv[]) {

    if (argc != 2) {
        std::cout << "Expect single argument which is path to a defs file\n";
        return 1;
    }

    std::string path = argv[1];

    Defs defs;
    std::string errorMsg, warningMsg;
    if (!defs.restore(path, errorMsg, warningMsg)) {
        std::cout << errorMsg << "\n";
        std::cout << warningMsg << "\n";
        return 1;
    }

    return 0;
}
