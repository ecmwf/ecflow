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

#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/test/unit_test.hpp>

#include "ecflow/core/Converter.hpp"

/*
 * The following dummy classes are defined to support test `can_use_custom_conversion_traits` and confirms that
 * customization point ::ecf::converter_traits<From, To>::convert(...) can be provided.
 */
struct Widget
{
};
struct Gizmo
{
};

template <>
struct ecf::converter_traits<Widget, Gizmo>
{
    inline static Gizmo convert(const Widget&) { return Gizmo{}; }
};

BOOST_AUTO_TEST_SUITE(U_Core)

BOOST_AUTO_TEST_SUITE(T_Converter)

BOOST_AUTO_TEST_CASE(can_convert_from_numeric_to_string) {
    BOOST_CHECK_EQUAL(ecf::convert_to<std::string>(0), "0");
    BOOST_CHECK_EQUAL(ecf::convert_to<std::string>(123), "123");
    BOOST_CHECK_EQUAL(ecf::convert_to<std::string>(123L), "123");
    BOOST_CHECK_EQUAL(ecf::convert_to<std::string>('c'), "c");
    BOOST_CHECK_EQUAL(ecf::convert_to<std::string>("s"), "s");

    BOOST_CHECK_EQUAL(ecf::convert_to<std::string>(0.0), "0");
    BOOST_CHECK_EQUAL(ecf::convert_to<std::string>(0.00), "0");
    BOOST_CHECK_EQUAL(ecf::convert_to<std::string>(1.0), "1");
    BOOST_CHECK_EQUAL(ecf::convert_to<std::string>(-3.5), "-3.5");
}

BOOST_AUTO_TEST_CASE(can_convert_from_string_to_numeric) {
    BOOST_CHECK_EQUAL(ecf::convert_to<int>("-0"), 0);
    BOOST_CHECK_EQUAL(ecf::convert_to<int>("-123"), -123);
    BOOST_CHECK_EXCEPTION(ecf::convert_to<int>("s"), ecf::bad_conversion, [](const auto& e) { return true; });
    BOOST_CHECK_EXCEPTION(ecf::convert_to<int>('c'), ecf::bad_conversion, [](const auto& e) { return true; });

    BOOST_CHECK_EQUAL(ecf::convert_to<unsigned int>("-0"), 0U);
    BOOST_CHECK_EQUAL(ecf::convert_to<unsigned int>("-123"), static_cast<unsigned int>(-123));
    BOOST_CHECK_EXCEPTION(ecf::convert_to<unsigned int>("s"), ecf::bad_conversion, [](const auto& e) { return true; });
    BOOST_CHECK_EXCEPTION(ecf::convert_to<unsigned int>('c'), ecf::bad_conversion, [](const auto& e) { return true; });

    BOOST_CHECK_EQUAL(ecf::convert_to<long>("-0"), 0);
    BOOST_CHECK_EQUAL(ecf::convert_to<long>(-123), -123);
    BOOST_CHECK_EXCEPTION(ecf::convert_to<long>("s"), ecf::bad_conversion, [](const auto& e) { return true; });
    BOOST_CHECK_EXCEPTION(ecf::convert_to<long>('c'), ecf::bad_conversion, [](const auto& e) { return true; });

    BOOST_CHECK_EQUAL(ecf::convert_to<double>("0.0"), 0);
    BOOST_CHECK_EQUAL(ecf::convert_to<double>("0.00"), 0);
    BOOST_CHECK_EQUAL(ecf::convert_to<double>("1.0"), 1);
    BOOST_CHECK_EQUAL(ecf::convert_to<double>("-3.5"), -3.5);
}

BOOST_AUTO_TEST_CASE(can_convert_from_boost_object_to_string) {
    BOOST_CHECK_EQUAL(ecf::convert_to<std::string>(boost::gregorian::greg_day{23}), "23");
    BOOST_CHECK_EQUAL(ecf::convert_to<std::string>(boost::gregorian::greg_month{2}), "Feb");
    BOOST_CHECK_EQUAL(ecf::convert_to<std::string>(boost::gregorian::greg_year{2000}), "2000");
}

BOOST_AUTO_TEST_CASE(can_use_custom_conversion_traits) {
    // By compiling, the following expression confirms that a Widget can be converted to a Gizmo.
    ecf::convert_to<Gizmo>(Widget{});
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
