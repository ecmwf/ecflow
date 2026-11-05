// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <boost/test/unit_test.hpp>

#include "ecflow/core/Filesystem.hpp"
#include "ecflow/test/scaffold/Naming.hpp"
#include "ecflow/test/scaffold/Provisioning.hpp"

using ecf::test::scaffold::MakeDirectory;

BOOST_AUTO_TEST_SUITE(U_Core)

BOOST_AUTO_TEST_SUITE(T_ScaffoldProvisioning)

BOOST_AUTO_TEST_CASE(test_make_directory_gives_distinct_paths_within_a_test) {
    ECF_NAME_THIS_TEST();

    const auto first  = MakeDirectory{}.create();
    const auto second = MakeDirectory{}.create();

    BOOST_CHECK_NE(first.path(), second.path());
}

BOOST_AUTO_TEST_CASE(test_make_directory_gives_distinct_paths_after_removal) {
    ECF_NAME_THIS_TEST();

    fs::path removed;
    {
        const auto directory = MakeDirectory{}.create();
        removed              = directory.path();
    }
    const auto directory = MakeDirectory{}.create();

    BOOST_CHECK_NE(directory.path(), removed);
}

BOOST_AUTO_TEST_CASE(test_make_directory_removal_keeps_other_directories) {
    ECF_NAME_THIS_TEST();

    const auto outer = MakeDirectory{}.create();
    { const auto inner = MakeDirectory{}.create(); }

    BOOST_CHECK(fs::is_directory(outer.path()));
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
