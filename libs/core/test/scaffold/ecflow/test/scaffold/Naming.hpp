/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_test_scaffold_Naming_HPP
#define ecflow_test_scaffold_Naming_HPP

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <string>

#include <boost/test/unit_test.hpp>

namespace ecf::test {

inline std::string name_this_test() {
    std::string fullname = boost::unit_test::framework::current_test_case().p_name;
    long parent_id       = boost::unit_test::framework::current_test_case().p_parent_id;
    long master_id       = boost::unit_test::framework::master_test_suite().p_id;

    while (parent_id != master_id) {
        const auto& parent = boost::unit_test::framework::get<boost::unit_test::test_suite>(parent_id);
        fullname           = std::string(parent.p_name) + std::string(" / ") + fullname;
        parent_id          = parent.p_parent_id;
    }
    return fullname;
}

} // namespace ecf::test

#define ECF_NAME_THIS_TEST(ARGS)                                             \
    do {                                                                     \
        std::cout << " * " << ecf::test::name_this_test() ARGS << std::endl; \
    } while (0)

#define ECF_TEST_DBG(ARGS)                      \
    do {                                        \
        std::cout << " +++ " ARGS << std::endl; \
    } while (0)

#define ECF_TEST_ERR(ARGS)                      \
    do {                                        \
        std::cerr << " +++ " ARGS << std::endl; \
    } while (0)

#endif /* ecflow_test_scaffold_Naming_HPP */
