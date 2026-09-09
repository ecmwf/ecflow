/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#include <boost/test/unit_test.hpp>

#include "ecflow/service/mirror/MirrorService.hpp"

BOOST_AUTO_TEST_SUITE(U_Mirror)

BOOST_AUTO_TEST_SUITE(T_MirrorController)

BOOST_AUTO_TEST_CASE(can_create_start_and_stop_mirror_controller) {
    using namespace ecf::service::mirror;

    MirrorController controller;

    controller.subscribe(MirrorRequest{"/path/to/node:mirror", "path", "host", "1234", 60, true, "auth"});

    controller.start();

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    controller.stop();
    controller.terminate();
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
