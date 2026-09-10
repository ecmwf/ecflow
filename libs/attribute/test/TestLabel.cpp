/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#include <iostream>
#include <string>

#include <boost/test/unit_test.hpp>

#include "ecflow/attribute/NodeAttr.hpp"
#include "ecflow/core/Str.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

using namespace ecf;

BOOST_AUTO_TEST_SUITE(U_Attributes)

BOOST_AUTO_TEST_SUITE(T_Label)

BOOST_AUTO_TEST_CASE(test_label_parsing) {
    ECF_NAME_THIS_TEST();

    {
        std::string line = "label name \"value\"";
        std::vector<std::string> linetokens;
        ecf::algorithm::split_at(linetokens, line);

        Label label;
        label.parse(line, linetokens, false);

        Label expected("name", "value");
        BOOST_CHECK_MESSAGE(label == expected, "Expected " << expected.toString() << " but found " << label.toString());
        BOOST_CHECK_MESSAGE(label.dump() == expected.dump(),
                            "Expected " << expected.dump() << " but found " << label.dump());
    }
    {
        std::string line = R"(label name "value\nvalue")";
        std::vector<std::string> linetokens;
        ecf::algorithm::split_at(linetokens, line);

        Label label;
        label.parse(line, linetokens, false);

        Label expected("name", "value\nvalue");
        BOOST_CHECK_MESSAGE(label == expected, "Expected " << expected.toString() << " but found " << label.toString());
        BOOST_CHECK_MESSAGE(label.dump() == expected.dump(),
                            "Expected " << expected.dump() << " but found " << label.dump());
    }
    {
        std::string line = "label name \"value that is multiple token !!!! 23445 !^ & * ( )\"";
        std::vector<std::string> linetokens;
        ecf::algorithm::split_at(linetokens, line);

        Label label;
        label.parse(line, linetokens, false);

        Label expected("name", "value that is multiple token !!!! 23445 !^ & * ( )");
        BOOST_CHECK_MESSAGE(label == expected, "Expected " << expected.toString() << " but found " << label.toString());
        BOOST_CHECK_MESSAGE(label.dump() == expected.dump(),
                            "Expected " << expected.dump() << " but found " << label.dump());
    }
    {
        std::string line = R"(label name "value\n that\n is\n multiple\n token\n and\n new\n \nlines")";
        std::vector<std::string> linetokens;
        ecf::algorithm::split_at(linetokens, line);

        Label label;
        label.parse(line, linetokens, false);

        Label expected("name", "value\n that\n is\n multiple\n token\n and\n new\n \nlines");
        BOOST_CHECK_MESSAGE(label == expected, "Expected " << expected.toString() << " but found " << label.toString());
        BOOST_CHECK_MESSAGE(label.dump() == expected.dump(),
                            "Expected " << expected.dump() << " but found " << label.dump());
    }
}

BOOST_AUTO_TEST_CASE(test_label_equality) {
    ECF_NAME_THIS_TEST();

    Label a("name", "value", "new_value");
    Label same("name", "value", "new_value");
    Label different_name("other", "value", "new_value");
    Label different_value("name", "other", "new_value");
    Label different_new_value("name", "value", "other");

    BOOST_CHECK(a == same);
    BOOST_CHECK(!(a != same));

    BOOST_CHECK(a != different_name);
    BOOST_CHECK(!(a == different_name));

    BOOST_CHECK(a != different_value);
    BOOST_CHECK(!(a == different_value));

    BOOST_CHECK(a != different_new_value);
    BOOST_CHECK(!(a == different_new_value));
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
