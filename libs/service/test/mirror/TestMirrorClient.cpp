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

#include "ecflow/service/mirror/MirrorClient.hpp"

BOOST_AUTO_TEST_SUITE(U_Mirror)

BOOST_AUTO_TEST_SUITE(T_MirrorClient)

BOOST_AUTO_TEST_CASE(can_create_default_mirror_client) {
    using namespace ecf::service::mirror;
    MirrorClient client;
    BOOST_CHECK(true);
}

BOOST_AUTO_TEST_CASE(can_handle_getting_status_with_invalid_credentials) {
    using namespace ecf::service::mirror;
    MirrorClient client;

    BOOST_CHECK_EXCEPTION(client.get_node_status("invalid_host", "1234", "/s1/f1/p1", true, "username", "password"),
                          std::runtime_error,
                          [](const std::runtime_error& e) {
                              return std::string(e.what()).find("failure to sync remote defs") != std::string::npos;
                          });
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
