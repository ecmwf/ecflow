// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "ecflow/core/Identity.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

namespace {

// Returns the UTF-8 encoding of a code point; no check is made, so that invalid sequences can be built too
std::string utf8(char32_t c) {
    std::string s;
    if (c < 0x80) {
        s += static_cast<char>(c);
    }
    else if (c < 0x800) {
        s += static_cast<char>(0xC0 | (c >> 6));
        s += static_cast<char>(0x80 | (c & 0x3F));
    }
    else if (c < 0x10000) {
        s += static_cast<char>(0xE0 | (c >> 12));
        s += static_cast<char>(0x80 | ((c >> 6) & 0x3F));
        s += static_cast<char>(0x80 | (c & 0x3F));
    }
    else {
        s += static_cast<char>(0xF0 | (c >> 18));
        s += static_cast<char>(0x80 | ((c >> 12) & 0x3F));
        s += static_cast<char>(0x80 | ((c >> 6) & 0x3F));
        s += static_cast<char>(0x80 | (c & 0x3F));
    }
    return s;
}

std::string with(char32_t c) {
    return "a" + utf8(c) + "b";
}

struct Invalid
{
    std::string name;
    std::string reason; // a part of the expected reason
};

} // namespace

BOOST_AUTO_TEST_SUITE(U_Core)

BOOST_AUTO_TEST_SUITE(T_UserNameModel)

///
/// @brief Verifies that the names allowed by the model are accepted, at the boundaries of every excluded class.
///
BOOST_AUTO_TEST_CASE(test_user_name_model_accepts_valid_names) {
    ECF_NAME_THIS_TEST();

    const std::vector<std::string> valid{
        "alice",
        "a",
        "bob.smith",
        "user_1",
        "x-y",          // inner '-'
        "x-",           // trailing '-'
        "a*b",          // inner '*'
        "*bob",         // '*' with other characters
        "bob*",         //
        "**",           //
        "o'neil",       //
        "o\"neil",      //
        "a\\b",         //
        "a/b",          //
        "a@b",          //
        "~",            // U+007E, last printable ASCII character
        "!",            // U+0021, first printable ASCII character
        with(0x00A1),   // after U+00A0
        with(0x00E9),   // 2-byte character
        with(0x20AC),   // 3-byte character
        with(0x1F600),  // 4-byte character
        with(0x10FFFF), // last code point
        with(0x1FFF),   // before U+2000
        with(0x200B),   // after U+200A, not White_Space
        with(0x2027),   // before U+2028
        with(0x202A),   // after U+2029
        with(0x202E),   // before U+202F
        with(0x2030),   // after U+202F
        with(0x205E),   // before U+205F
        with(0x2060),   // after U+205F
        with(0x2FFF),   // before U+3000
        with(0x3001),   // after U+3000
        with(0x167F),   // before U+1680
        with(0x1681),   // after U+1680
    };
    for (const auto& name : valid) {
        std::string error;
        BOOST_CHECK_MESSAGE(ecf::is_valid_user_name(name, error),
                            ecf::printable_user_name(name) << " refused: " << error);
        BOOST_CHECK_MESSAGE(error.empty(), ecf::printable_user_name(name) << ": " << error);
    }
}

///
/// @brief Verifies that every class excluded by the model is refused, at its boundaries, with the reason.
///
BOOST_AUTO_TEST_CASE(test_user_name_model_refuses_invalid_names) {
    ECF_NAME_THIS_TEST();

    std::vector<Invalid> invalid{
        {"", "empty"},
        {"*", "'*'"},
        {"-", "starts with '-'"},
        {"-alice", "starts with '-'"},
        {"--alice", "starts with '-'"},
        {"al#ice", "'#'"},
        {"al,ice", "','"},
        {"al:ice", "':'"},
        {"al;ice", "';'"},
        {"#", "'#'"},
        {"al\xffice", "UTF-8"},                  // byte never valid in UTF-8
        {"al\xc3", "UTF-8"},                     // truncated 2-byte sequence
        {"al\xe2\x82", "UTF-8"},                 // truncated 3-byte sequence
        {"al\xf0\x9f\x98", "UTF-8"},             // truncated 4-byte sequence
        {"\xc3\x28", "UTF-8"},                   // bad continuation byte
        {"\x80", "UTF-8"},                       // continuation byte alone
        {"\xc0\xaf", "UTF-8"},                   // overlong 2-byte form of '/'
        {"\xe0\x80\xaf", "UTF-8"},               // overlong 3-byte form
        {"\xf0\x80\x80\xaf", "UTF-8"},           // overlong 4-byte form
        {"\xed\xa0\x80", "UTF-8"},               // surrogate U+D800
        {"\xed\xbf\xbf", "UTF-8"},               // surrogate U+DFFF
        {"\xf4\x90\x80\x80", "UTF-8"},           // above U+10FFFF
        {"\xf8\x88\x80\x80\x80", "UTF-8"},       // 5-byte form
        {std::string("a\0b", 3), "control"},     // U+0000
        {with(0x0001), "control"},               //
        {with(0x001F), "control"},               // last C0 control character
        {with(0x007F), "control"},               // DEL
        {with(0x0080), "control"},               // first C1 control character
        {with(0x0084), "control"},               //
        {with(0x009F), "control"},               // last C1 control character
        {with(0x0020), "white space"},           // space
        {with(0x0009), "white space"},           // tab
        {with(0x000A), "white space"},           // line feed
        {with(0x000B), "white space"},           // vertical tab
        {with(0x000C), "white space"},           // form feed
        {with(0x000D), "white space"},           // carriage return
        {with(0x0085), "white space"},           // next line
        {with(0x00A0), "white space"},           // no-break space
        {with(0x1680), "white space"},           // ogham space mark
        {with(0x2028), "white space"},           // line separator
        {with(0x2029), "white space"},           // paragraph separator
        {with(0x202F), "white space"},           // narrow no-break space
        {with(0x205F), "white space"},           // medium mathematical space
        {with(0x3000), "white space"},           // ideographic space
        {utf8(0x00A0) + "alice", "white space"}, // leading
        {"alice" + utf8(0x3000), "white space"}, // trailing
    };
    for (char32_t c = 0x2000; c <= 0x200A; ++c) {
        invalid.push_back({with(c), "white space"});
    }

    for (const auto& [name, reason] : invalid) {
        std::string error;
        BOOST_CHECK_MESSAGE(!ecf::is_valid_user_name(name, error), "accepted: " << ecf::printable_user_name(name));
        BOOST_CHECK_MESSAGE(error.find(reason) != std::string::npos,
                            ecf::printable_user_name(name)
                                << ": reason '" << error << "' does not mention '" << reason << "'");
        // The reason can be logged and shown: it holds no control character and no raw invalid byte
        for (unsigned char c : error) {
            BOOST_CHECK_MESSAGE(c >= 0x20 && c != 0x7F, "reason holds a control character: " << error);
        }
    }
}

///
/// @brief Verifies that a name is shown with its control characters, white space and invalid bytes made visible.
///
BOOST_AUTO_TEST_CASE(test_user_name_model_printable_form) {
    ECF_NAME_THIS_TEST();

    BOOST_CHECK_EQUAL(ecf::printable_user_name("alice"), "'alice'");
    BOOST_CHECK_EQUAL(ecf::printable_user_name("jos\xc3\xa9"), "'jos\xc3\xa9'");
    BOOST_CHECK_EQUAL(ecf::printable_user_name("a b"), "'a<U+0020>b'");
    BOOST_CHECK_EQUAL(ecf::printable_user_name("a\nb"), "'a<U+000A>b'");
    BOOST_CHECK_EQUAL(ecf::printable_user_name(with(0x00A0)), "'a<U+00A0>b'");
    BOOST_CHECK_EQUAL(ecf::printable_user_name(with(0x3000)), "'a<U+3000>b'");
    BOOST_CHECK_EQUAL(ecf::printable_user_name("a\xff"), "'a<0xFF>'");
    BOOST_CHECK_EQUAL(ecf::printable_user_name(std::string("a\0", 2)), "'a<U+0000>'");
}

///
/// @brief Verifies that a text keeps its white space, and has its control characters and invalid bytes made visible.
///
BOOST_AUTO_TEST_CASE(test_user_name_model_printable_text) {
    ECF_NAME_THIS_TEST();

    BOOST_CHECK_EQUAL(ecf::printable_text("a b\tc"), "a b<U+0009>c");
    BOOST_CHECK_EQUAL(ecf::printable_text("jos\xc3\xa9 x"), "jos\xc3\xa9 x");
    BOOST_CHECK_EQUAL(ecf::printable_text(std::string("a\x01") + "b"), "a<U+0001>b");
    BOOST_CHECK_EQUAL(ecf::printable_text("a\xff"), "a<0xFF>");
    BOOST_CHECK_EQUAL(ecf::printable_text(with(0x00A0)), with(0x00A0));
}

///
/// @brief Verifies that names are compared exactly: no case folding and no Unicode normalisation.
///
BOOST_AUTO_TEST_CASE(test_user_name_model_exact_comparison) {
    ECF_NAME_THIS_TEST();

    BOOST_CHECK(ecf::Username{"alice"} != ecf::Username{"Alice"});
    // The precomposed (U+00E9) and decomposed (e + U+0301) forms of e acute are different names, both valid
    const std::string precomposed = "jos" + utf8(0x00E9);
    const std::string decomposed  = "jose" + utf8(0x0301);
    std::string error;
    BOOST_CHECK(ecf::is_valid_user_name(precomposed, error));
    BOOST_CHECK(ecf::is_valid_user_name(decomposed, error));
    BOOST_CHECK(ecf::Username{precomposed} != ecf::Username{decomposed});
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
