/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include <boost/test/unit_test.hpp>

#include "ecflow/service/mirror/MirrorService.hpp"

BOOST_AUTO_TEST_SUITE(U_Mirror)

BOOST_AUTO_TEST_SUITE(T_MirrorController)

BOOST_AUTO_TEST_CASE(can_create_start_and_stop_mirror_controller) {
    using namespace ecf::service::mirror;

    MirrorController controller;

    controller.subscribe(MirrorRequest{"path", "host", "1234", 60, true, "auth"});

    controller.start();

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    controller.stop();
    controller.terminate();
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
