/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include <iterator> // std::ostream_iterator
#include <random>
#include <string>

#include <boost/test/unit_test.hpp>

#include "ecflow/core/NodePath.hpp"
#include "ecflow/core/Stl.hpp"
#include "ecflow/core/Str.hpp"
#include "ecflow/core/Timer.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

//
// Important:
//
// The following implementation was the one used by ecFlow to check if a node name is valid.
// It is now replaced by the implementation in Str.hpp, but it was kept in the scope of this test
// to check that the new implementation behaves as expected.
//

namespace ecf {
namespace prototype {

const char* VALID_NODE_CHARS = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_.";

bool Str_valid_name(const std::string& name, std::string& msg) {
    // valid names are alphabetic (alphanumeric | underscore | .)
    // however we can't have a leading '.' as that can interfere with trigger expressions

    // verify that the string is not empty
    if (name.empty()) {
        msg = "Invalid name. Empty string.";
        return false;
    }

    // verify that the first character is alphanumeric or is an underscore
    bool result = ecf::string_constants::alphanumeric_underscore_chars.find(name[0], 0) != std::string::npos;
    if (!result) {
        msg = "Valid names can only consist of alphanumeric characters, "
              "underscores and dots (The first character cannot be a dot). "
              "The first character is not valid (only alphanumeric or an underscore is allowed): ";
        msg += name;
        return false;
    }

    // verify that any other characters are alphanumeric or underscore
    if (name.size() > 1) {
        result = name.find_first_not_of(VALID_NODE_CHARS, 1) == std::string::npos;
        if (!result) {
            msg = "Valid names can only consist of alphanumeric characters, "
                  "underscores and dots (The first character cannot be a dot). ";
            if (name.find('\r') != std::string::npos) {
                msg += "Windows line ending ? ";
            }
            msg += "'";
            msg += name;
            msg += "'"; // use '<name>' to show if PC format, i.e. carriage return
        }
    }

    return result;
}

bool Str_valid_name(const std::string& name) {
    // valid names are alphabetic (alphanumeric | underscore | .)
    // however we can't have a leading '.' as that can interfere with trigger expressions

    // verify that the string is not empty
    if (name.empty()) {
        return false;
    }

    // verify that the first character is alphabetic or has underscore
    bool result = ecf::string_constants::alphanumeric_underscore_chars.find(name[0], 0) != std::string::npos;
    if (!result) {
        return false;
    }

    // verify that any other characters are alphanumeric or underscore
    if (name.size() > 1) {
        result = name.find_first_not_of(VALID_NODE_CHARS, 1) == std::string::npos;
    }

    return result;
}

} // namespace prototype
} // namespace ecf

BOOST_AUTO_TEST_SUITE(U_Core)

BOOST_AUTO_TEST_SUITE(T_NodeNameValidity)

BOOST_AUTO_TEST_CASE(test_node_name_validity) {
    ECF_NAME_THIS_TEST();

    struct tc
    {
        std::string name;
        bool expected;
    };

    std::vector<tc> test_cases = {
        {"", false},
        {".", false},
        {".a.", false},
        {".A", false},
        {".1", false},
        {"_", true},
        {"_a.", true},
        {"_A", true},
        {"_1", true},
        {"a.", true},
        {"A", true},
        {"1", true},
        {"aa.", true},
        {"AA", true},
        {"11", true},
        {"a.", true},
        {"ab.", true},
        {"a.b", true},
        {"a.b.", true},
        {"a._", true},
        {"ab._", true},
        {"a.b_", true},
        {"a.b._", true},
        {"a1", true},
        {"1a", true},
        {"a1.", true},
        {"1a.", true},
        {"a1_", true},
        {"1a_", true},
        {"a", true},
        {"a122345", true},
        {"_a122345", true},
        {"0", true},
        {"1", true},
        {"2", true},
        {"3", true},
        {"4", true},
        {"5", true},
        {"6", true},
        {"7", true},
        {"8", true},
        {"9", true},
        {"11", true},
        {"111", true},
        {"abcdefghABCDEFGH", true},
        {"1234567890abcdefghABCDEFGH", true},
        {"abcdefgh1234567890ABCDEFGH", true},
        {"abcdefghABCDEFGH1234567890", true},
        {"a.b.c.d.e.f.g.h.i.j.k.l.m.n.o.p.q.r.s.t.u.v.w.x.y.z", true},
        {"a.b.c.d.e.f.g.h.i.j.k.l.m.n.o.p.q.r.s.t.u.v.w.x.y.z.", true},
        {"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_.", true},
        {"_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_.", true},
        {".abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_.", false},
        {"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_.\r", false},
        {"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_.\n", false},
        {"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_.\t", false},
        {"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_. ", false},
        {"\t", false},
        {"\n", false},
        {"\r\n", false},
        {"?", false},
        {"!", false},
        {"\"", false},
        {"$", false},
        {"%", false},
        {"^", false},
        {"*", false},
        {"(", false},
        {")", false},
        {"-", false},
        {"+", false},
        {":", false},
        {";", false},
        {"@", false},
        {"~", false},
        {"<", false},
        {">", false},
        {"!", false},
        {" invalid", false},
        {"\tinvalid", false},
        {"\ninvalid", false},
        {"invalid space", false},
        {"invalid ", false},
        {"inva lid", false},
        {"inva\tlid", false},
        {"inva\nlid", false},
        {"inva\rlid", false},
    };

    for (const auto& tc : test_cases) {

        {
            bool actual   = ecf::algorithm::is_valid_name(tc.name);
            bool original = ecf::prototype::Str_valid_name(tc.name);

            BOOST_CHECK_MESSAGE(actual == tc.expected,
                                "For name '" << tc.name << "' expected " << tc.expected << ", found " << actual);
            BOOST_CHECK_MESSAGE(actual == original,
                                "For name '" << tc.name << "' both original and new algorithms provide same result");
        }
        {
            std::string actual_error;
            bool actual = ecf::algorithm::is_valid_name(tc.name, actual_error);

            std::string original_error;
            bool original = ecf::prototype::Str_valid_name(tc.name, original_error);

            BOOST_CHECK_MESSAGE(actual == tc.expected,
                                "For name '" << tc.name << "' expected " << tc.expected << ", found " << actual);
            BOOST_CHECK_MESSAGE(actual == original,
                                "For name '" << tc.name << "' both original and new algorithms provide same result");

            if (!tc.expected) {
                auto expected_prefix = "Invalid name '" + tc.name + "': ";
                auto actual_prefix   = actual_error.substr(0, expected_prefix.size());

                BOOST_CHECK_MESSAGE(actual_prefix == expected_prefix,
                                    "The error message for '" << tc.name << "' expected prefix <" << expected_prefix
                                                              << ">, found <" << actual_prefix << ">");
            }
        }
    }
}

BOOST_AUTO_TEST_CASE(test_node_name_validity_random) {
    ECF_NAME_THIS_TEST();

    struct timer
    {
        timer()  = default;
        ~timer() = default;

        void begin() { start = std::chrono::steady_clock::now(); }
        void end() { finish = std::chrono::steady_clock::now(); }

        std::chrono::nanoseconds duration() const {
            return std::chrono::duration_cast<std::chrono::nanoseconds>(finish - start);
        }

        std::chrono::time_point<std::chrono::steady_clock> start;
        std::chrono::time_point<std::chrono::steady_clock> finish;
        std::string_view name;
        std::string_view type;
    };

    auto generate_random_valid_name = []() {
        std::string_view valid = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_.";

        static std::random_device rd;
        static std::mt19937_64 gen(rd());
        std::uniform_int_distribution<int> length_dist(1, 20);

        std::uniform_int_distribution<int> first_chars_dist(0, 62);     // abc...ABC...0123456789_ (63 chars, no '.'!)
        std::uniform_int_distribution<int> following_chars_dist(0, 63); // abc...ABC...0123456789_. (64 chars)

        int length = length_dist(gen);
        std::string name;
        name.reserve(length);
        for (int i = 0; i < length; ++i) {
            if (i == 0) {
                name.push_back(valid[first_chars_dist(gen)]);
            }
            else {
                name.push_back(valid[following_chars_dist(gen)]);
            }
        }
        return name;
    };

    double n_time = 0.;
    double o_time = 0.;

    int iterations = 1000;

    for (int i = 0; i < iterations; ++i) {

        auto name = generate_random_valid_name();

        bool original;
        {
            timer t;

            t.begin();
            original = ecf::prototype::Str_valid_name(name);
            t.end();

            auto ns = t.duration().count();
            o_time += ns;
            ECF_TEST_DBG(<< "Original approach, name: '" << name << "', duration: " << ns << " ns");
        }

        bool actual;
        {
            timer t;

            t.begin();
            actual = ecf::algorithm::is_valid_name(name);
            t.end();

            auto ns = t.duration().count();
            n_time += ns;
            ECF_TEST_DBG(<< "    New approach, name: " << name << ", duration: " << ns << " ns");
        }

        BOOST_CHECK_MESSAGE(actual == original,
                            "For name '" << name << "' both original and new approaches provide same result");
    }

    double old_time_per_iteration = o_time / iterations;
    ECF_TEST_DBG(<< "Original approach time: " << old_time_per_iteration << " ns");
    double new_time_per_iteration = n_time / iterations;
    ECF_TEST_DBG(<< "     New approach time: " << new_time_per_iteration << " ns");

    BOOST_CHECK_MESSAGE(new_time_per_iteration < old_time_per_iteration, "New algorithm is faster than original");
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
