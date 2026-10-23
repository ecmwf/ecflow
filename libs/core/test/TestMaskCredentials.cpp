// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#include <string>

#include <boost/test/unit_test.hpp>

#include "ecflow/core/MaskCredentials.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

using ecf::mask_credentials;

BOOST_AUTO_TEST_SUITE(U_Core)

BOOST_AUTO_TEST_SUITE(T_MaskCredentials)

BOOST_AUTO_TEST_CASE(test_mask_credentials_keeps_values_without_credentials) {
    ECF_NAME_THIS_TEST();

    BOOST_CHECK_EQUAL(mask_credentials(""), "");
    BOOST_CHECK_EQUAL(mask_credentials("%ECF_JOB% 1> %ECF_JOBOUT% 2>&1"), "%ECF_JOB% 1> %ECF_JOBOUT% 2>&1");
    BOOST_CHECK_EQUAL(mask_credentials("https://host:8080/path?x=1"), "https://host:8080/path?x=1");
    BOOST_CHECK_EQUAL(mask_credentials("https://user@host/path"), "https://user@host/path");
    BOOST_CHECK_EQUAL(mask_credentials("ssh -i ~/.ssh/key host"), "ssh -i ~/.ssh/key host");
}

BOOST_AUTO_TEST_CASE(test_mask_credentials_masks_the_password_of_a_url) {
    ECF_NAME_THIS_TEST();

    BOOST_CHECK_EQUAL(mask_credentials("https://user:s3cr3t@host:8080/path"), "https://user:***@host:8080/path");
    BOOST_CHECK_EQUAL(mask_credentials("curl ftp://a:b@h/x && curl http://c:d@h/y"),
                      "curl ftp://a:***@h/x && curl http://c:***@h/y");
}

BOOST_AUTO_TEST_CASE(test_mask_credentials_masks_the_value_of_a_named_key) {
    ECF_NAME_THIS_TEST();

    BOOST_CHECK_EQUAL(mask_credentials("API_KEY=abc123 submit %ECF_JOB%"), "API_KEY=*** submit %ECF_JOB%");
    BOOST_CHECK_EQUAL(mask_credentials("MY_SECRET=s3cr3t"), "MY_SECRET=***");
    BOOST_CHECK_EQUAL(mask_credentials("https://host/x?access_token=abc&y=1"), "https://host/x?access_token=***&y=1");
    BOOST_CHECK_EQUAL(mask_credentials("db_passwd='p w'"), "db_passwd=***");
}

BOOST_AUTO_TEST_CASE(test_mask_credentials_masks_the_value_of_a_named_option) {
    ECF_NAME_THIS_TEST();

    BOOST_CHECK_EQUAL(mask_credentials("submit --password=abc %ECF_JOB%"), "submit --password=*** %ECF_JOB%");
    BOOST_CHECK_EQUAL(mask_credentials("submit --token abc %ECF_JOB%"), "submit --token *** %ECF_JOB%");
    BOOST_CHECK_EQUAL(mask_credentials("submit -secret \"a b\" x"), "submit -secret *** x");
}

BOOST_AUTO_TEST_CASE(test_mask_credentials_ignores_the_case_of_the_name) {
    ECF_NAME_THIS_TEST();

    BOOST_CHECK_EQUAL(mask_credentials("--Passwd=abc"), "--Passwd=***");
    BOOST_CHECK_EQUAL(mask_credentials("Token=abc"), "Token=***");
}

BOOST_AUTO_TEST_CASE(test_mask_credentials_masks_the_whole_shell_word) {
    ECF_NAME_THIS_TEST();

    // A value is a complete shell word: adjacent quoted segments, escaped spaces and escaped quotes included
    BOOST_CHECK_EQUAL(mask_credentials("PASSWORD=ab'cd' submit"), "PASSWORD=*** submit");
    BOOST_CHECK_EQUAL(mask_credentials(R"(PASSWORD=abc\ def submit)"), "PASSWORD=*** submit");
    BOOST_CHECK_EQUAL(mask_credentials(R"(TOKEN="a b"'c d'e submit)"), "TOKEN=*** submit");
    BOOST_CHECK_EQUAL(mask_credentials(R"(submit --password "a\" b secret-tail" job)"), "submit --password *** job");
    BOOST_CHECK_EQUAL(mask_credentials(R"(submit --token a\ b job)"), "submit --token *** job");
}

BOOST_AUTO_TEST_CASE(test_mask_credentials_masks_the_rest_of_an_unterminated_quote) {
    ECF_NAME_THIS_TEST();

    BOOST_CHECK_EQUAL(mask_credentials("submit --token 'abc def job"), "submit --token ***");
    BOOST_CHECK_EQUAL(mask_credentials(R"(PASSWORD="abc def job)"), "PASSWORD=***");
}

BOOST_AUTO_TEST_CASE(test_mask_credentials_leaves_no_fragment_of_the_secret) {
    ECF_NAME_THIS_TEST();

    for (const auto* value : {"PASSWORD=ab'SECRET' submit",
                              R"(PASSWORD=abc\ SECRET submit)",
                              R"(submit --password "a\" b SECRET" job)",
                              R"(TOKEN="a b"'SECRET'SECRET submit)",
                              "https://user:SECRET@host/?api_key=SECRET&x=1"}) {
        auto masked = mask_credentials(value);
        BOOST_CHECK_MESSAGE(masked.find("SECRET") == std::string::npos, "Unmasked fragment in: " << masked);
    }
}

BOOST_AUTO_TEST_CASE(test_mask_credentials_masks_each_named_key_of_a_query) {
    ECF_NAME_THIS_TEST();

    BOOST_CHECK_EQUAL(mask_credentials("https://host/x?a=1&token=abc&b=2&secret=def"),
                      "https://host/x?a=1&token=***&b=2&secret=***");
}

BOOST_AUTO_TEST_CASE(test_mask_credentials_matches_whole_parts_of_the_name) {
    ECF_NAME_THIS_TEST();

    // A name is a credential name when one of its parts, split at '_', '-' or '.', is a credential word
    BOOST_CHECK_EQUAL(mask_credentials("keyfile=/etc/x"), "keyfile=/etc/x");
    BOOST_CHECK_EQUAL(mask_credentials("MONKEY=3"), "MONKEY=3");
    BOOST_CHECK_EQUAL(mask_credentials("ssh_keys_dir=/x"), "ssh_keys_dir=/x");
    BOOST_CHECK_EQUAL(mask_credentials("submit --keyfile /etc/x"), "submit --keyfile /etc/x");
    BOOST_CHECK_EQUAL(mask_credentials("api.key=abc"), "api.key=***");
    BOOST_CHECK_EQUAL(mask_credentials("--api-token abc"), "--api-token ***");
}

BOOST_AUTO_TEST_CASE(test_mask_credentials_handles_long_values) {
    ECF_NAME_THIS_TEST();

    // A huge command is about 4096 characters long, as one word or as many
    std::string word(4096, 'a');
    BOOST_CHECK_EQUAL(mask_credentials(word), word);

    std::string words;
    while (words.size() < 4096) {
        words += "argument ";
    }
    BOOST_CHECK_EQUAL(mask_credentials(words + "--token abc"), words + "--token ***");
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
