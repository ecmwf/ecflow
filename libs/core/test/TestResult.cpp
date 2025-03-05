/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include <iostream>
#include <string>

#include <boost/test/unit_test.hpp>

#include "ecflow/core/Result.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

enum class some_error_t { io_error, unknown_error };

inline std::ostream& operator<<(std::ostream& os, const some_error_t& error) {
    os << "error_t(" << static_cast<int>(error) << ")";
    return os;
}

BOOST_AUTO_TEST_SUITE(U_Core)

BOOST_AUTO_TEST_SUITE(T_Result)

BOOST_AUTO_TEST_CASE(test_create_successful_result) {
    ECF_NAME_THIS_TEST();
    {
        const auto result = ecf::Result<int, some_error_t>::success(42);
        BOOST_CHECK(result.ok());
        BOOST_CHECK_EQUAL(result.value(), 42);
    }
    {
        const auto result = ecf::Result<std::string, some_error_t>::success("hello");
        BOOST_CHECK(result.ok());
        BOOST_CHECK_EQUAL(result.value(), std::string("hello"));
    }
}

BOOST_AUTO_TEST_CASE(test_create_unsuccessful_result) {
    ECF_NAME_THIS_TEST();
    {
        const auto result = ecf::Result<int, some_error_t>::failure(some_error_t::io_error);
        BOOST_CHECK(!result.ok());
        BOOST_CHECK_EQUAL(result.reason(), some_error_t::io_error);
    }
    {
        const auto result = ecf::Result<std::string, some_error_t>::failure(some_error_t::unknown_error);
        BOOST_CHECK(!result.ok());
        BOOST_CHECK_EQUAL(result.reason(), some_error_t::unknown_error);
    }

    {
        const auto result = ecf::Result<std::string, std::string>::failure(std::string{"description"});
        BOOST_CHECK(!result.ok());
        BOOST_CHECK_EQUAL(result.reason(), "description");
    }
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
