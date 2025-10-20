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

#include "ecflow/core/Extract.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

BOOST_AUTO_TEST_SUITE(U_Core)

BOOST_AUTO_TEST_SUITE(T_Extract)

BOOST_AUTO_TEST_CASE(can_extract_path_and_name) {
    ECF_NAME_THIS_TEST();

    using namespace std::string_literals;
    {
        auto token = "/suite/family:obj"s;
        std::string path;
        std::string name;
        BOOST_CHECK(Extract::pathAndName(token, path, name));
        BOOST_CHECK_EQUAL(path, "/suite/family"s);
        BOOST_CHECK_EQUAL(name, "obj"s);
    }
    {
        auto token = "/suite/family"s;
        std::string path;
        std::string name;
        BOOST_CHECK(Extract::pathAndName(token, path, name));
        BOOST_CHECK_EQUAL(path, "/suite/family"s);
        BOOST_CHECK_EQUAL(name, ""s);
    }
    {
        auto token = "obj"s;
        std::string path;
        std::string name;
        BOOST_CHECK(Extract::pathAndName(token, path, name));
        BOOST_CHECK_EQUAL(path, ""s);
        BOOST_CHECK_EQUAL(name, "obj"s);
    }
}

BOOST_AUTO_TEST_CASE(can_extract_second_token) {
    ECF_NAME_THIS_TEST();

    using namespace std::string_literals;
    {
        auto token = "First:Second"s;
        std::string second;
        BOOST_CHECK(Extract::split_get_second(token, second));
        BOOST_CHECK_EQUAL(second, "Second"s);
    }
    {
        auto token = "/suite/family:obj"s;
        std::string second;
        BOOST_CHECK(Extract::split_get_second(token, second));
        BOOST_CHECK_EQUAL(second, "obj"s);
    }
}

BOOST_AUTO_TEST_CASE(can_extract_integer_throw_on_failure, *boost::unit_test_framework::tolerance(0.00000001)) {
    ECF_NAME_THIS_TEST();

    using namespace std::string_literals;
    { BOOST_CHECK_THROW(Extract::value<int>("", "error message"s), std::runtime_error); }
    { BOOST_CHECK_THROW(Extract::value<int>("a", "error message"s), std::runtime_error); }
    { BOOST_CHECK_THROW(Extract::value<int>("abc", "error message"s), std::runtime_error); }
    { BOOST_CHECK_THROW(Extract::value<int>("#", "error message"s), std::runtime_error); }
    { BOOST_CHECK_THROW(Extract::value<int>(":", "error message"s), std::runtime_error); }
    { BOOST_CHECK_EQUAL(Extract::value<int>("42", "error message"s), 42); }
    { BOOST_CHECK_EQUAL(Extract::value<int>("-42", "error message"s), -42); }
    { BOOST_CHECK_THROW(Extract::value<int>("42 ", "error message"s), std::runtime_error); }
    { BOOST_CHECK_THROW(Extract::value<int>("42 42", "error message"s), std::runtime_error); }
    { BOOST_CHECK_THROW(Extract::value<int>("42.42", "error message"s), std::runtime_error); }
    {
        auto value = Extract::value<double>("42.42", "error message"s);
        BOOST_CHECK_CLOSE(value, 42.42, 1.0e-9);
    }
}

BOOST_AUTO_TEST_CASE(can_extract_integer_return_result) {
    ECF_NAME_THIS_TEST();

    {
        auto result = Extract::value<int>("");
        BOOST_CHECK(!result.ok());
    }
    {
        auto result = Extract::value<int>("a");
        BOOST_CHECK(!result.ok());
    }
    {
        auto result = Extract::value<int>("abc");
        BOOST_CHECK(!result.ok());
    }
    {
        auto result = Extract::value<int>("#");
        BOOST_CHECK(!result.ok());
    }
    {
        auto result = Extract::value<int>(":");
        BOOST_CHECK(!result.ok());
    }
    {
        auto result = Extract::value<int>("42");
        BOOST_CHECK(result.ok());
        BOOST_CHECK_EQUAL(result.value(), 42);
    }
    {
        auto result = Extract::value<int>("-42");
        BOOST_CHECK(result.ok());
        BOOST_CHECK_EQUAL(result.value(), -42);
    }
    {
        auto result = Extract::value<int>("42 ");
        BOOST_CHECK(!result.ok());
    }
    {
        auto result = Extract::value<int>("42 42");
        BOOST_CHECK(!result.ok());
    }
    {
        auto result = Extract::value<int>("42.42");
        BOOST_CHECK(!result.ok());
    }
    {
        auto result = Extract::value<double>("42.42");
        BOOST_CHECK(result.ok());
        BOOST_CHECK_CLOSE(result.value(), 42.42, 1.0e-9);
    }
}

BOOST_AUTO_TEST_CASE(can_extract_optional_integer) {
    ECF_NAME_THIS_TEST();

    using namespace std::string_literals;
    {
        auto tokens = std::vector{"repeat"s, "integer"s, "variable"s, "1"s, "2"s, "#a"s, "comment"s};
        { BOOST_CHECK_THROW(Extract::optional_value<int>(tokens, 0, 42, "error message"s), std::runtime_error); }
        { BOOST_CHECK_THROW(Extract::optional_value<int>(tokens, 1, 42, "error message"s), std::runtime_error); }
        { BOOST_CHECK_THROW(Extract::optional_value<int>(tokens, 2, 42, "error message"s), std::runtime_error); }
        { BOOST_CHECK_EQUAL(Extract::optional_value<int>(tokens, 3, 42, "error message"), 1); }
        { BOOST_CHECK_EQUAL(Extract::optional_value<int>(tokens, 4, 42, "error message"), 2); }
        { BOOST_CHECK_EQUAL(Extract::optional_value<int>(tokens, 5, 42, "error message"), 42); }
        { BOOST_CHECK_THROW(Extract::optional_value<int>(tokens, 6, 42, "error message"s), std::runtime_error); }
    }
    {
        auto tokens = std::vector{"repeat"s, "integer"s, "variable"s, "1"s, "2"s, "#"s, "a"s, "comment"s};
        { BOOST_CHECK_THROW(Extract::optional_value<int>(tokens, 0, 42, "error message"s), std::runtime_error); }
        { BOOST_CHECK_THROW(Extract::optional_value<int>(tokens, 1, 42, "error message"s), std::runtime_error); }
        { BOOST_CHECK_THROW(Extract::optional_value<int>(tokens, 2, 42, "error message"s), std::runtime_error); }
        { BOOST_CHECK_EQUAL(Extract::optional_value<int>(tokens, 3, 42, "error message"s), 1); }
        { BOOST_CHECK_EQUAL(Extract::optional_value<int>(tokens, 4, 42, "error message"s), 2); }
        { BOOST_CHECK_EQUAL(Extract::optional_value<int>(tokens, 5, 42, "error message"s), 42); }
        { BOOST_CHECK_THROW(Extract::optional_value<int>(tokens, 6, 42, "error message"s), std::runtime_error); }
        { BOOST_CHECK_THROW(Extract::optional_value<int>(tokens, 7, 42, "error message"s), std::runtime_error); }
    }
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
