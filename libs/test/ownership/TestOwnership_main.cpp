// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#define BOOST_TEST_MODULE TestOwnership
#define BOOST_TEST_ALTERNATIVE_INIT_API
#define BOOST_TEST_NO_MAIN
#include <iostream>

#include <boost/test/included/unit_test.hpp>

#include "PrivilegedEnvironment.hpp"

///
/// The privileged tests only run in the privileged test image (see docker/README.md): elsewhere, they report
/// themselves skipped (exit status 77, which CTest records as skipped); in the image, recognised by its marker variable
/// or its marker file, a missing prerequisite fails, and the tests are never skipped.
///
int main(int argc, char* argv[]) {
    using namespace ecf::test::ownership;

    if (!in_privileged_image()) {
        std::cout << "s_ownership: skipped, as not running in the privileged test image (neither " << image_marker
                  << " is set, nor " << image_marker_file << " exists); see libs/test/ownership/docker/README.md\n";
        return skipped_exit_status;
    }

    if (const auto missing = missing_prerequisites(); !missing.empty()) {
        std::cerr << "s_ownership: the privileged test image lacks the following prerequisites:\n";
        for (const auto& item : missing) {
            std::cerr << "  - " << item << "\n";
        }
        return 1;
    }

    // The servers and the clients of the tests are configured by the tests only: every inherited ecFlow setting (e.g.
    // ECF_HOST_PROTOCOL, read by the clients) is removed, except those of the tests themselves (ECF_TEST_*)
    clear_inherited_ecflow_settings();

    return boost::unit_test::unit_test_main(&init_unit_test, argc, argv);
}
