/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include <fstream>
#include <initializer_list>
#include <string>
#include <utility>

#include <boost/test/unit_test.hpp>

#include "ecflow/core/Host.hpp"
#include "ecflow/core/Identity.hpp"
#include "ecflow/core/PasswordEncryption.hpp"
#include "ecflow/server/AuthenticationService.hpp"
#include "ecflow/test/scaffold/Naming.hpp"
#include "ecflow/test/scaffold/Provisioning.hpp"

using ecf::AuthenticationService;
using ecf::Host;
using ecf::Identity;
using ecf::test::scaffold::AutomaticTestFile;
using ecf::test::scaffold::WithoutTestEnvironmentVariable;
using ecf::test::scaffold::WithTestEnvironmentVariable;
using ecf::test::scaffold::WithTestFile;

namespace {

///
/// @brief The host and port used, throughout the tests, to describe the server being authenticated against.
///
const std::string test_host = "testhost";
const std::string test_port = "31415";

///
/// @brief Compute the ciphertext the server expects a client to present for the given plain text password.
///
/// The password file stores every password as ciphertext, and the client is expected to present the
/// same ciphertext; the username acts as the salt.
///
std::string encrypted(const std::string& username, const std::string& plain_password) {
    return PasswordEncryption::encrypt(plain_password, username);
}

///
/// @brief Compose the content of a version 4.5.0 password file granting the given users access to the test server.
///
std::string passwd_file_content(std::initializer_list<std::pair<std::string, std::string>> users,
                                const std::string& host = test_host,
                                const std::string& port = test_port) {
    std::string content = "4.5.0\n";
    for (const auto& [user, password] : users) {
        content += user + " " + host + " " + port + " " + password + "\n";
    }
    return content;
}

///
/// @brief Create a service already configured with the given regular password file, and confirm it is valid.
///
void configure_and_validate(AuthenticationService& service, const std::string& passwd_file) {
    service.set_passwd_file(passwd_file);
    std::string error;
    BOOST_REQUIRE_MESSAGE(service.valid(test_host, test_port, error), "unexpected validation failure: " << error);
}

} // namespace

BOOST_AUTO_TEST_SUITE(U_Server)

BOOST_AUTO_TEST_SUITE(T_Authentication)

// ===========================================================================
// Default state, accessors and constants
// ===========================================================================

BOOST_AUTO_TEST_CASE(test_authentication_default_constructed_service_has_no_files) {
    ECF_NAME_THIS_TEST();

    AuthenticationService service;

    BOOST_CHECK(service.passwd_file().empty());
    BOOST_CHECK(service.custom_passwd_file().empty());
}

BOOST_AUTO_TEST_CASE(test_authentication_default_file_names) {
    ECF_NAME_THIS_TEST();

    BOOST_CHECK_EQUAL(AuthenticationService::default_passwd_file(), "ecf.passwd");
    BOOST_CHECK_EQUAL(AuthenticationService::default_custom_passwd_file(), "ecf.custom_passwd");
}

BOOST_AUTO_TEST_CASE(test_authentication_setters_and_getters_round_trip) {
    ECF_NAME_THIS_TEST();

    AuthenticationService service;

    service.set_passwd_file("/some/where/regular.passwd");
    service.set_custom_passwd_file("/some/where/custom.passwd");

    BOOST_CHECK_EQUAL(service.passwd_file(), "/some/where/regular.passwd");
    BOOST_CHECK_EQUAL(service.custom_passwd_file(), "/some/where/custom.passwd");

    // Setting an empty value is accepted, and clears the file name
    service.set_passwd_file("");
    service.set_custom_passwd_file("");

    BOOST_CHECK(service.passwd_file().empty());
    BOOST_CHECK(service.custom_passwd_file().empty());
}

// ===========================================================================
// init(): expansion of the default file names with host and port
// ===========================================================================

BOOST_AUTO_TEST_CASE(test_authentication_init_prefixes_default_file_names_with_host_and_port) {
    ECF_NAME_THIS_TEST();

    AuthenticationService service;
    service.set_passwd_file(std::string{AuthenticationService::default_passwd_file()});
    service.set_custom_passwd_file(std::string{AuthenticationService::default_custom_passwd_file()});

    Host host(test_host);
    service.init(host, test_port);

    BOOST_CHECK_EQUAL(service.passwd_file(), test_host + "." + test_port + ".ecf.passwd");
    BOOST_CHECK_EQUAL(service.custom_passwd_file(), test_host + "." + test_port + ".ecf.custom_passwd");
}

BOOST_AUTO_TEST_CASE(test_authentication_init_with_empty_port_prefixes_with_host_only) {
    ECF_NAME_THIS_TEST();

    AuthenticationService service;
    service.set_passwd_file(std::string{AuthenticationService::default_passwd_file()});
    service.set_custom_passwd_file(std::string{AuthenticationService::default_custom_passwd_file()});

    Host host(test_host);
    service.init(host, "");

    BOOST_CHECK_EQUAL(service.passwd_file(), test_host + ".ecf.passwd");
    BOOST_CHECK_EQUAL(service.custom_passwd_file(), test_host + ".ecf.custom_passwd");
}

BOOST_AUTO_TEST_CASE(test_authentication_init_leaves_non_default_file_names_untouched) {
    ECF_NAME_THIS_TEST();

    AuthenticationService service;
    service.set_passwd_file("my.passwd");
    service.set_custom_passwd_file("/absolute/path/to/custom.passwd");

    Host host(test_host);
    service.init(host, test_port);

    BOOST_CHECK_EQUAL(service.passwd_file(), "my.passwd");
    BOOST_CHECK_EQUAL(service.custom_passwd_file(), "/absolute/path/to/custom.passwd");
}

BOOST_AUTO_TEST_CASE(test_authentication_init_leaves_empty_file_names_untouched) {
    ECF_NAME_THIS_TEST();

    AuthenticationService service;

    Host host(test_host);
    service.init(host, test_port);

    BOOST_CHECK(service.passwd_file().empty());
    BOOST_CHECK(service.custom_passwd_file().empty());
}

BOOST_AUTO_TEST_CASE(test_authentication_init_is_not_applied_twice) {
    ECF_NAME_THIS_TEST();

    AuthenticationService service;
    service.set_passwd_file(std::string{AuthenticationService::default_passwd_file()});

    Host host(test_host);
    service.init(host, test_port);
    service.init(host, test_port);

    // Once expanded, the file name no longer matches the default, and is thus not expanded again
    BOOST_CHECK_EQUAL(service.passwd_file(), test_host + "." + test_port + ".ecf.passwd");
}

BOOST_AUTO_TEST_CASE(test_authentication_init_expands_each_file_independently) {
    ECF_NAME_THIS_TEST();

    AuthenticationService service;
    service.set_passwd_file("regular.passwd");
    service.set_custom_passwd_file(std::string{AuthenticationService::default_custom_passwd_file()});

    Host host(test_host);
    service.init(host, test_port);

    BOOST_CHECK_EQUAL(service.passwd_file(), "regular.passwd");
    BOOST_CHECK_EQUAL(service.custom_passwd_file(), test_host + "." + test_port + ".ecf.custom_passwd");
}

// ===========================================================================
// retrieve_*_passwd_file(): pick up of file names from the environment
// ===========================================================================

BOOST_AUTO_TEST_CASE(test_authentication_retrieve_passwd_file_from_environment) {
    ECF_NAME_THIS_TEST();

    WithTestEnvironmentVariable passwd("ECF_PASSWD", "from_env.passwd");
    WithTestEnvironmentVariable custom("ECF_CUSTOM_PASSWD", "from_env.custom_passwd");

    AuthenticationService service;
    service.retrieve_passwd_file();
    service.retrieve_custom_passwd_file();

    BOOST_CHECK_EQUAL(service.passwd_file(), "from_env.passwd");
    BOOST_CHECK_EQUAL(service.custom_passwd_file(), "from_env.custom_passwd");
}

BOOST_AUTO_TEST_CASE(test_authentication_retrieve_passwd_file_overrides_previous_value) {
    ECF_NAME_THIS_TEST();

    WithTestEnvironmentVariable passwd("ECF_PASSWD", "from_env.passwd");
    WithTestEnvironmentVariable custom("ECF_CUSTOM_PASSWD", "from_env.custom_passwd");

    AuthenticationService service;
    service.set_passwd_file("previous.passwd");
    service.set_custom_passwd_file("previous.custom_passwd");

    service.retrieve_passwd_file();
    service.retrieve_custom_passwd_file();

    BOOST_CHECK_EQUAL(service.passwd_file(), "from_env.passwd");
    BOOST_CHECK_EQUAL(service.custom_passwd_file(), "from_env.custom_passwd");
}

BOOST_AUTO_TEST_CASE(test_authentication_retrieve_passwd_file_keeps_value_when_environment_is_unset) {
    ECF_NAME_THIS_TEST();

    WithoutTestEnvironmentVariable passwd("ECF_PASSWD");
    WithoutTestEnvironmentVariable custom("ECF_CUSTOM_PASSWD");

    AuthenticationService service;
    service.set_passwd_file("previous.passwd");
    service.set_custom_passwd_file("previous.custom_passwd");

    service.retrieve_passwd_file();
    service.retrieve_custom_passwd_file();

    BOOST_CHECK_EQUAL(service.passwd_file(), "previous.passwd");
    BOOST_CHECK_EQUAL(service.custom_passwd_file(), "previous.custom_passwd");
}

BOOST_AUTO_TEST_CASE(test_authentication_retrieve_passwd_file_accepts_empty_environment_value) {
    ECF_NAME_THIS_TEST();

    WithTestEnvironmentVariable passwd("ECF_PASSWD", "");
    WithTestEnvironmentVariable custom("ECF_CUSTOM_PASSWD", "");

    AuthenticationService service;
    service.set_passwd_file("previous.passwd");
    service.set_custom_passwd_file("previous.custom_passwd");

    service.retrieve_passwd_file();
    service.retrieve_custom_passwd_file();

    BOOST_CHECK(service.passwd_file().empty());
    BOOST_CHECK(service.custom_passwd_file().empty());
}

BOOST_AUTO_TEST_CASE(test_authentication_retrieve_is_independent_per_file) {
    ECF_NAME_THIS_TEST();

    WithTestEnvironmentVariable passwd("ECF_PASSWD", "from_env.passwd");
    WithoutTestEnvironmentVariable custom("ECF_CUSTOM_PASSWD");

    AuthenticationService service;
    service.retrieve_passwd_file();
    service.retrieve_custom_passwd_file();

    BOOST_CHECK_EQUAL(service.passwd_file(), "from_env.passwd");
    BOOST_CHECK(service.custom_passwd_file().empty());
}

// ===========================================================================
// valid(): validation and loading at server start-up
// ===========================================================================

BOOST_AUTO_TEST_CASE(test_authentication_valid_when_no_files_are_specified) {
    ECF_NAME_THIS_TEST();

    AuthenticationService service;

    std::string error;
    BOOST_CHECK(service.valid(test_host, test_port, error));
    BOOST_CHECK(error.empty());
}

BOOST_AUTO_TEST_CASE(test_authentication_valid_when_files_do_not_exist) {
    ECF_NAME_THIS_TEST();

    AuthenticationService service;
    service.set_passwd_file("this.file.does.not.exist.passwd");
    service.set_custom_passwd_file("this.file.does.not.exist.custom_passwd");

    std::string error;
    BOOST_CHECK(service.valid(test_host, test_port, error));
    BOOST_CHECK(error.empty());
}

BOOST_AUTO_TEST_CASE(test_authentication_valid_with_well_formed_files) {
    ECF_NAME_THIS_TEST();

    WithTestFile passwd(AutomaticTestFile{"ecf.passwd"}, passwd_file_content({{"alice", "secret"}}));
    WithTestFile custom(AutomaticTestFile{"ecf.custom_passwd"}, passwd_file_content({{"bob", "hidden"}}));

    AuthenticationService service;
    service.set_passwd_file(passwd.path().string());
    service.set_custom_passwd_file(custom.path().string());

    std::string error;
    BOOST_CHECK_MESSAGE(service.valid(test_host, test_port, error), error);
    BOOST_CHECK(error.empty());
}

BOOST_AUTO_TEST_CASE(test_authentication_valid_with_only_regular_file) {
    ECF_NAME_THIS_TEST();

    WithTestFile passwd(AutomaticTestFile{"ecf.passwd"}, passwd_file_content({{"alice", "secret"}}));

    AuthenticationService service;
    service.set_passwd_file(passwd.path().string());

    std::string error;
    BOOST_CHECK_MESSAGE(service.valid(test_host, test_port, error), error);
}

BOOST_AUTO_TEST_CASE(test_authentication_valid_with_only_custom_file) {
    ECF_NAME_THIS_TEST();

    WithTestFile custom(AutomaticTestFile{"ecf.custom_passwd"}, passwd_file_content({{"bob", "hidden"}}));

    AuthenticationService service;
    service.set_custom_passwd_file(custom.path().string());

    std::string error;
    BOOST_CHECK_MESSAGE(service.valid(test_host, test_port, error), error);
}

BOOST_AUTO_TEST_CASE(test_authentication_valid_tolerates_comments_and_blank_lines) {
    ECF_NAME_THIS_TEST();

    std::string content = "# leading comment\n"
                          "\n"
                          "4.5.0\n"
                          "\n"
                          "   # indented comment\n"
                          "alice " +
                          test_host + " " + test_port + " secret # trailing comment\n";
    WithTestFile passwd(AutomaticTestFile{"ecf.passwd"}, content);

    AuthenticationService service;
    service.set_passwd_file(passwd.path().string());

    std::string error;
    BOOST_CHECK_MESSAGE(service.valid(test_host, test_port, error), error);
    BOOST_CHECK(service.is_authentic(Identity::make_user("alice", encrypted("alice", "secret"))));
}

BOOST_AUTO_TEST_CASE(test_authentication_invalid_when_regular_file_cannot_be_parsed) {
    ECF_NAME_THIS_TEST();

    // The version number is mandatory, and must precede all users
    WithTestFile passwd(AutomaticTestFile{"ecf.passwd"}, "alice " + test_host + " " + test_port + " secret\n");

    AuthenticationService service;
    service.set_passwd_file(passwd.path().string());

    std::string error;
    BOOST_CHECK(!service.valid(test_host, test_port, error));
    BOOST_CHECK(!error.empty());
}

BOOST_AUTO_TEST_CASE(test_authentication_invalid_when_custom_file_cannot_be_parsed) {
    ECF_NAME_THIS_TEST();

    WithTestFile custom(AutomaticTestFile{"ecf.custom_passwd"}, "4.5.0\nbob " + test_host + " not_a_port hidden\n");

    AuthenticationService service;
    service.set_custom_passwd_file(custom.path().string());

    std::string error;
    BOOST_CHECK(!service.valid(test_host, test_port, error));
    BOOST_CHECK(!error.empty());
}

BOOST_AUTO_TEST_CASE(test_authentication_invalid_when_file_has_unsupported_version) {
    ECF_NAME_THIS_TEST();

    WithTestFile passwd(AutomaticTestFile{"ecf.passwd"}, "4.4.0\nalice " + test_host + " " + test_port + " secret\n");

    AuthenticationService service;
    service.set_passwd_file(passwd.path().string());

    std::string error;
    BOOST_CHECK(!service.valid(test_host, test_port, error));
    BOOST_CHECK(error.find("version") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(test_authentication_invalid_when_user_is_duplicated_for_host_and_port) {
    ECF_NAME_THIS_TEST();

    WithTestFile passwd(AutomaticTestFile{"ecf.passwd"},
                        passwd_file_content({{"alice", "secret"}, {"alice", "another"}}));

    AuthenticationService service;
    service.set_passwd_file(passwd.path().string());

    std::string error;
    BOOST_CHECK(!service.valid(test_host, test_port, error));
    BOOST_CHECK(error.find("can only appear once") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(test_authentication_invalid_when_regular_file_has_no_user_for_this_server) {
    ECF_NAME_THIS_TEST();

    // The file is well-formed, but every user targets another server
    WithTestFile passwd(AutomaticTestFile{"ecf.passwd"},
                        passwd_file_content({{"alice", "secret"}}, "otherhost", test_port));

    AuthenticationService service;
    service.set_passwd_file(passwd.path().string());

    std::string error;
    BOOST_CHECK(!service.valid(test_host, test_port, error));
}

BOOST_AUTO_TEST_CASE(test_authentication_invalid_when_regular_file_only_matches_another_port) {
    ECF_NAME_THIS_TEST();

    WithTestFile passwd(AutomaticTestFile{"ecf.passwd"}, passwd_file_content({{"alice", "secret"}}, test_host, "9999"));

    AuthenticationService service;
    service.set_passwd_file(passwd.path().string());

    std::string error;
    BOOST_CHECK(!service.valid(test_host, test_port, error));
}

BOOST_AUTO_TEST_CASE(test_authentication_invalid_when_custom_file_has_no_user_for_this_server) {
    ECF_NAME_THIS_TEST();

    WithTestFile custom(AutomaticTestFile{"ecf.custom_passwd"},
                        passwd_file_content({{"bob", "hidden"}}, "otherhost", "9999"));

    AuthenticationService service;
    service.set_custom_passwd_file(custom.path().string());

    std::string error;
    BOOST_CHECK(!service.valid(test_host, test_port, error));
}

BOOST_AUTO_TEST_CASE(test_authentication_valid_when_at_least_one_user_targets_this_server) {
    ECF_NAME_THIS_TEST();

    std::string content = "4.5.0\n"
                          "alice otherhost 9999 secret\n"
                          "bob " +
                          test_host + " " + test_port + " hidden\n";
    WithTestFile passwd(AutomaticTestFile{"ecf.passwd"}, content);

    AuthenticationService service;
    service.set_passwd_file(passwd.path().string());

    std::string error;
    BOOST_CHECK_MESSAGE(service.valid(test_host, test_port, error), error);
}

BOOST_AUTO_TEST_CASE(test_authentication_invalid_when_file_is_empty) {
    ECF_NAME_THIS_TEST();

    // An empty file parses successfully, but grants no user access to this server
    WithTestFile passwd(AutomaticTestFile{"ecf.passwd"}, "");

    AuthenticationService service;
    service.set_passwd_file(passwd.path().string());

    std::string error;
    BOOST_CHECK(!service.valid(test_host, test_port, error));
}

BOOST_AUTO_TEST_CASE(test_authentication_invalid_when_file_only_holds_version) {
    ECF_NAME_THIS_TEST();

    WithTestFile passwd(AutomaticTestFile{"ecf.passwd"}, "4.5.0\n");

    AuthenticationService service;
    service.set_passwd_file(passwd.path().string());

    std::string error;
    BOOST_CHECK(!service.valid(test_host, test_port, error));
}

BOOST_AUTO_TEST_CASE(test_authentication_invalid_when_regular_file_is_ok_but_custom_is_not) {
    ECF_NAME_THIS_TEST();

    WithTestFile passwd(AutomaticTestFile{"ecf.passwd"}, passwd_file_content({{"alice", "secret"}}));
    WithTestFile custom(AutomaticTestFile{"ecf.custom_passwd"}, "this is not a password file\n");

    AuthenticationService service;
    service.set_passwd_file(passwd.path().string());
    service.set_custom_passwd_file(custom.path().string());

    std::string error;
    BOOST_CHECK(!service.valid(test_host, test_port, error));
    BOOST_CHECK(!error.empty());
}

BOOST_AUTO_TEST_CASE(test_authentication_invalid_when_custom_file_is_ok_but_regular_is_not) {
    ECF_NAME_THIS_TEST();

    WithTestFile passwd(AutomaticTestFile{"ecf.passwd"}, "this is not a password file\n");
    WithTestFile custom(AutomaticTestFile{"ecf.custom_passwd"}, passwd_file_content({{"bob", "hidden"}}));

    AuthenticationService service;
    service.set_passwd_file(passwd.path().string());
    service.set_custom_passwd_file(custom.path().string());

    std::string error;
    BOOST_CHECK(!service.valid(test_host, test_port, error));
    BOOST_CHECK(!error.empty());
}

BOOST_AUTO_TEST_CASE(test_authentication_valid_loads_the_files_for_subsequent_authentication) {
    ECF_NAME_THIS_TEST();

    WithTestFile passwd(AutomaticTestFile{"ecf.passwd"}, passwd_file_content({{"alice", "secret"}}));
    WithTestFile custom(AutomaticTestFile{"ecf.custom_passwd"}, passwd_file_content({{"bob", "hidden"}}));

    AuthenticationService service;
    service.set_passwd_file(passwd.path().string());
    service.set_custom_passwd_file(custom.path().string());

    // Before validation, nothing is loaded, so credentials for known users are rejected
    BOOST_CHECK(!service.is_authentic(Identity::make_user("alice", encrypted("alice", "secret"))));
    BOOST_CHECK(!service.is_authentic(Identity::make_custom_user("bob", encrypted("bob", "hidden"))));

    std::string error;
    BOOST_REQUIRE_MESSAGE(service.valid(test_host, test_port, error), error);

    BOOST_CHECK(service.is_authentic(Identity::make_user("alice", encrypted("alice", "secret"))));
    BOOST_CHECK(service.is_authentic(Identity::make_custom_user("bob", encrypted("bob", "hidden"))));
}

BOOST_AUTO_TEST_CASE(test_authentication_valid_with_debug_enabled) {
    ECF_NAME_THIS_TEST();

    WithTestFile passwd(AutomaticTestFile{"ecf.passwd"}, passwd_file_content({{"alice", "secret"}}));

    AuthenticationService service;
    service.set_debug(true);
    service.set_passwd_file(passwd.path().string());

    std::string error;
    BOOST_CHECK_MESSAGE(service.valid(test_host, test_port, error), error);
    BOOST_CHECK(service.is_authentic(Identity::make_user("alice", encrypted("alice", "secret"))));
}

// ===========================================================================
// is_authentic(): with no password file loaded
// ===========================================================================

BOOST_AUTO_TEST_CASE(test_authentication_without_files_accepts_user_with_empty_password) {
    ECF_NAME_THIS_TEST();

    AuthenticationService service;

    BOOST_CHECK(service.is_authentic(Identity::make_user("alice", "")));
    BOOST_CHECK(service.is_authentic(Identity::make_custom_user("bob", "")));
}

BOOST_AUTO_TEST_CASE(test_authentication_without_files_rejects_user_with_non_empty_password) {
    ECF_NAME_THIS_TEST();

    AuthenticationService service;

    BOOST_CHECK(!service.is_authentic(Identity::make_user("alice", "anything")));
    BOOST_CHECK(!service.is_authentic(Identity::make_custom_user("bob", "anything")));
}

BOOST_AUTO_TEST_CASE(test_authentication_without_files_rejects_empty_username) {
    ECF_NAME_THIS_TEST();

    AuthenticationService service;

    BOOST_CHECK(!service.is_authentic(Identity::make_user("", "")));
    BOOST_CHECK(!service.is_authentic(Identity::make_user("", "anything")));
    BOOST_CHECK(!service.is_authentic(Identity::make_custom_user("", "")));
    BOOST_CHECK(!service.is_authentic(Identity::make_custom_user("", "anything")));
}

BOOST_AUTO_TEST_CASE(test_authentication_without_files_rejects_none_identity) {
    ECF_NAME_THIS_TEST();

    AuthenticationService service;

    // The 'None' identity carries an empty username, and is never authentic
    BOOST_CHECK(!service.is_authentic(Identity::make_none()));
}

BOOST_AUTO_TEST_CASE(test_authentication_without_files_accepts_secure_user) {
    ECF_NAME_THIS_TEST();

    AuthenticationService service;

    BOOST_CHECK(service.is_authentic(Identity::make_secure_user("alice")));
    BOOST_CHECK(service.is_authentic(Identity::make_secure_user("")));
}

// ===========================================================================
// is_authentic(): with a regular password file loaded
// ===========================================================================

BOOST_AUTO_TEST_CASE(test_authentication_accepts_known_user_with_correct_password) {
    ECF_NAME_THIS_TEST();

    WithTestFile passwd(AutomaticTestFile{"ecf.passwd"},
                        passwd_file_content({{"alice", "secret"}, {"carol", "s3cr3t"}}));

    AuthenticationService service;
    configure_and_validate(service, passwd.path().string());

    BOOST_CHECK(service.is_authentic(Identity::make_user("alice", encrypted("alice", "secret"))));
    BOOST_CHECK(service.is_authentic(Identity::make_user("carol", encrypted("carol", "s3cr3t"))));
}

BOOST_AUTO_TEST_CASE(test_authentication_rejects_known_user_with_wrong_password) {
    ECF_NAME_THIS_TEST();

    WithTestFile passwd(AutomaticTestFile{"ecf.passwd"}, passwd_file_content({{"alice", "secret"}}));

    AuthenticationService service;
    configure_and_validate(service, passwd.path().string());

    BOOST_CHECK(!service.is_authentic(Identity::make_user("alice", encrypted("alice", "wrong"))));
}

BOOST_AUTO_TEST_CASE(test_authentication_rejects_known_user_with_plain_text_password) {
    ECF_NAME_THIS_TEST();

    // Only the ciphertext is accepted; presenting the plain text password is a failure
    WithTestFile passwd(AutomaticTestFile{"ecf.passwd"}, passwd_file_content({{"alice", "secret"}}));

    AuthenticationService service;
    configure_and_validate(service, passwd.path().string());

    BOOST_CHECK(!service.is_authentic(Identity::make_user("alice", "secret")));
}

BOOST_AUTO_TEST_CASE(test_authentication_rejects_known_user_with_empty_password) {
    ECF_NAME_THIS_TEST();

    WithTestFile passwd(AutomaticTestFile{"ecf.passwd"}, passwd_file_content({{"alice", "secret"}}));

    AuthenticationService service;
    configure_and_validate(service, passwd.path().string());

    BOOST_CHECK(!service.is_authentic(Identity::make_user("alice", "")));
}

BOOST_AUTO_TEST_CASE(test_authentication_rejects_password_encrypted_with_another_salt) {
    ECF_NAME_THIS_TEST();

    // The username is the salt; a ciphertext produced for another user is not accepted
    WithTestFile passwd(AutomaticTestFile{"ecf.passwd"}, passwd_file_content({{"alice", "secret"}}));

    AuthenticationService service;
    configure_and_validate(service, passwd.path().string());

    BOOST_CHECK(!service.is_authentic(Identity::make_user("alice", encrypted("bob", "secret"))));
}

BOOST_AUTO_TEST_CASE(test_authentication_rejects_unknown_user_with_any_password) {
    ECF_NAME_THIS_TEST();

    WithTestFile passwd(AutomaticTestFile{"ecf.passwd"}, passwd_file_content({{"alice", "secret"}}));

    AuthenticationService service;
    configure_and_validate(service, passwd.path().string());

    BOOST_CHECK(!service.is_authentic(Identity::make_user("mallory", encrypted("mallory", "secret"))));
    BOOST_CHECK(!service.is_authentic(Identity::make_user("mallory", encrypted("alice", "secret"))));
}

BOOST_AUTO_TEST_CASE(test_authentication_rejects_unknown_user_with_empty_password) {
    ECF_NAME_THIS_TEST();

    // Once a password file holds users, an unknown user is rejected even without password
    WithTestFile passwd(AutomaticTestFile{"ecf.passwd"}, passwd_file_content({{"alice", "secret"}}));

    AuthenticationService service;
    configure_and_validate(service, passwd.path().string());

    BOOST_CHECK(!service.is_authentic(Identity::make_user("mallory", "")));
}

BOOST_AUTO_TEST_CASE(test_authentication_rejects_empty_username_with_file_loaded) {
    ECF_NAME_THIS_TEST();

    WithTestFile passwd(AutomaticTestFile{"ecf.passwd"}, passwd_file_content({{"alice", "secret"}}));

    AuthenticationService service;
    configure_and_validate(service, passwd.path().string());

    BOOST_CHECK(!service.is_authentic(Identity::make_user("", "")));
    BOOST_CHECK(!service.is_authentic(Identity::make_user("", encrypted("alice", "secret"))));
    BOOST_CHECK(!service.is_authentic(Identity::make_none()));
}

BOOST_AUTO_TEST_CASE(test_authentication_username_is_case_sensitive) {
    ECF_NAME_THIS_TEST();

    WithTestFile passwd(AutomaticTestFile{"ecf.passwd"}, passwd_file_content({{"alice", "secret"}}));

    AuthenticationService service;
    configure_and_validate(service, passwd.path().string());

    BOOST_CHECK(!service.is_authentic(Identity::make_user("Alice", encrypted("Alice", "secret"))));
    BOOST_CHECK(!service.is_authentic(Identity::make_user("ALICE", encrypted("alice", "secret"))));
}

BOOST_AUTO_TEST_CASE(test_authentication_user_of_another_server_in_same_file_is_still_accepted) {
    ECF_NAME_THIS_TEST();

    // Authentication matches on username only; the host/port on a line is used solely for validation
    std::string content = "4.5.0\n"
                          "alice otherhost 9999 secret\n"
                          "bob " +
                          test_host + " " + test_port + " hidden\n";
    WithTestFile passwd(AutomaticTestFile{"ecf.passwd"}, content);

    AuthenticationService service;
    configure_and_validate(service, passwd.path().string());

    BOOST_CHECK(service.is_authentic(Identity::make_user("alice", encrypted("alice", "secret"))));
    BOOST_CHECK(service.is_authentic(Identity::make_user("bob", encrypted("bob", "hidden"))));
}

BOOST_AUTO_TEST_CASE(test_authentication_secure_user_bypasses_password_file) {
    ECF_NAME_THIS_TEST();

    WithTestFile passwd(AutomaticTestFile{"ecf.passwd"}, passwd_file_content({{"alice", "secret"}}));

    AuthenticationService service;
    configure_and_validate(service, passwd.path().string());

    // Secure identities are trusted regardless of whether the user is known
    BOOST_CHECK(service.is_authentic(Identity::make_secure_user("alice")));
    BOOST_CHECK(service.is_authentic(Identity::make_secure_user("mallory")));
}

// ===========================================================================
// is_authentic(): regular versus custom identities and files
// ===========================================================================

BOOST_AUTO_TEST_CASE(test_authentication_custom_user_is_checked_against_custom_file_only) {
    ECF_NAME_THIS_TEST();

    WithTestFile passwd(AutomaticTestFile{"ecf.passwd"}, passwd_file_content({{"alice", "secret"}}));
    WithTestFile custom(AutomaticTestFile{"ecf.custom_passwd"}, passwd_file_content({{"bob", "hidden"}}));

    AuthenticationService service;
    service.set_passwd_file(passwd.path().string());
    service.set_custom_passwd_file(custom.path().string());
    std::string error;
    BOOST_REQUIRE_MESSAGE(service.valid(test_host, test_port, error), error);

    // Each identity kind is authenticated against its own file
    BOOST_CHECK(service.is_authentic(Identity::make_user("alice", encrypted("alice", "secret"))));
    BOOST_CHECK(service.is_authentic(Identity::make_custom_user("bob", encrypted("bob", "hidden"))));

    // Credentials from one file are not accepted by the other
    BOOST_CHECK(!service.is_authentic(Identity::make_custom_user("alice", encrypted("alice", "secret"))));
    BOOST_CHECK(!service.is_authentic(Identity::make_user("bob", encrypted("bob", "hidden"))));
}

BOOST_AUTO_TEST_CASE(test_authentication_same_user_with_different_passwords_in_each_file) {
    ECF_NAME_THIS_TEST();

    WithTestFile passwd(AutomaticTestFile{"ecf.passwd"}, passwd_file_content({{"alice", "regular"}}));
    WithTestFile custom(AutomaticTestFile{"ecf.custom_passwd"}, passwd_file_content({{"alice", "custom"}}));

    AuthenticationService service;
    service.set_passwd_file(passwd.path().string());
    service.set_custom_passwd_file(custom.path().string());
    std::string error;
    BOOST_REQUIRE_MESSAGE(service.valid(test_host, test_port, error), error);

    BOOST_CHECK(service.is_authentic(Identity::make_user("alice", encrypted("alice", "regular"))));
    BOOST_CHECK(!service.is_authentic(Identity::make_user("alice", encrypted("alice", "custom"))));

    BOOST_CHECK(service.is_authentic(Identity::make_custom_user("alice", encrypted("alice", "custom"))));
    BOOST_CHECK(!service.is_authentic(Identity::make_custom_user("alice", encrypted("alice", "regular"))));
}

BOOST_AUTO_TEST_CASE(test_authentication_custom_user_without_custom_file_behaves_as_without_file) {
    ECF_NAME_THIS_TEST();

    // Only the regular file is loaded; custom identities are checked against an empty custom database
    WithTestFile passwd(AutomaticTestFile{"ecf.passwd"}, passwd_file_content({{"alice", "secret"}}));

    AuthenticationService service;
    configure_and_validate(service, passwd.path().string());

    BOOST_CHECK(service.is_authentic(Identity::make_custom_user("bob", "")));
    BOOST_CHECK(!service.is_authentic(Identity::make_custom_user("bob", "anything")));
    BOOST_CHECK(!service.is_authentic(Identity::make_custom_user("alice", encrypted("alice", "secret"))));
}

BOOST_AUTO_TEST_CASE(test_authentication_regular_user_without_regular_file_behaves_as_without_file) {
    ECF_NAME_THIS_TEST();

    // Only the custom file is loaded; regular identities are checked against an empty regular database
    WithTestFile custom(AutomaticTestFile{"ecf.custom_passwd"}, passwd_file_content({{"bob", "hidden"}}));

    AuthenticationService service;
    service.set_custom_passwd_file(custom.path().string());
    std::string error;
    BOOST_REQUIRE_MESSAGE(service.valid(test_host, test_port, error), error);

    BOOST_CHECK(service.is_authentic(Identity::make_user("alice", "")));
    BOOST_CHECK(!service.is_authentic(Identity::make_user("alice", "anything")));
    BOOST_CHECK(!service.is_authentic(Identity::make_user("bob", encrypted("bob", "hidden"))));
}

// ===========================================================================
// reload_*_passwd_file(): reloading at runtime
// ===========================================================================

BOOST_AUTO_TEST_CASE(test_authentication_reload_fails_when_no_regular_file_specified) {
    ECF_NAME_THIS_TEST();

    AuthenticationService service;

    std::string error;
    BOOST_CHECK(!service.reload_passwd_file(error));
    BOOST_CHECK(error.find("ECF_PASSWD") != std::string::npos);
    BOOST_CHECK(error.find("has not been specified") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(test_authentication_reload_fails_when_no_custom_file_specified) {
    ECF_NAME_THIS_TEST();

    AuthenticationService service;

    std::string error;
    BOOST_CHECK(!service.reload_custom_passwd_file(error));
    BOOST_CHECK(error.find("ECF_CUSTOM_PASSWD") != std::string::npos);
    BOOST_CHECK(error.find("has not been specified") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(test_authentication_reload_fails_when_regular_file_does_not_exist) {
    ECF_NAME_THIS_TEST();

    AuthenticationService service;
    service.set_passwd_file("this.file.does.not.exist.passwd");

    std::string error;
    BOOST_CHECK(!service.reload_passwd_file(error));
    BOOST_CHECK(error.find("this.file.does.not.exist.passwd") != std::string::npos);
    BOOST_CHECK(error.find("does not exist") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(test_authentication_reload_fails_when_custom_file_does_not_exist) {
    ECF_NAME_THIS_TEST();

    AuthenticationService service;
    service.set_custom_passwd_file("this.file.does.not.exist.custom_passwd");

    std::string error;
    BOOST_CHECK(!service.reload_custom_passwd_file(error));
    BOOST_CHECK(error.find("this.file.does.not.exist.custom_passwd") != std::string::npos);
    BOOST_CHECK(error.find("does not exist") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(test_authentication_reload_fails_when_regular_file_cannot_be_parsed) {
    ECF_NAME_THIS_TEST();

    WithTestFile passwd(AutomaticTestFile{"ecf.passwd"}, "this is not a password file\n");

    AuthenticationService service;
    service.set_passwd_file(passwd.path().string());

    std::string error;
    BOOST_CHECK(!service.reload_passwd_file(error));
    BOOST_CHECK(!error.empty());
}

BOOST_AUTO_TEST_CASE(test_authentication_reload_fails_when_custom_file_cannot_be_parsed) {
    ECF_NAME_THIS_TEST();

    WithTestFile custom(AutomaticTestFile{"ecf.custom_passwd"}, "this is not a password file\n");

    AuthenticationService service;
    service.set_custom_passwd_file(custom.path().string());

    std::string error;
    BOOST_CHECK(!service.reload_custom_passwd_file(error));
    BOOST_CHECK(!error.empty());
}

BOOST_AUTO_TEST_CASE(test_authentication_reload_loads_regular_file_without_prior_validation) {
    ECF_NAME_THIS_TEST();

    WithTestFile passwd(AutomaticTestFile{"ecf.passwd"}, passwd_file_content({{"alice", "secret"}}));

    AuthenticationService service;
    service.set_passwd_file(passwd.path().string());

    BOOST_CHECK(!service.is_authentic(Identity::make_user("alice", encrypted("alice", "secret"))));

    std::string error;
    BOOST_REQUIRE_MESSAGE(service.reload_passwd_file(error), error);
    BOOST_CHECK(error.empty());

    BOOST_CHECK(service.is_authentic(Identity::make_user("alice", encrypted("alice", "secret"))));
}

BOOST_AUTO_TEST_CASE(test_authentication_reload_loads_custom_file_without_prior_validation) {
    ECF_NAME_THIS_TEST();

    WithTestFile custom(AutomaticTestFile{"ecf.custom_passwd"}, passwd_file_content({{"bob", "hidden"}}));

    AuthenticationService service;
    service.set_custom_passwd_file(custom.path().string());

    BOOST_CHECK(!service.is_authentic(Identity::make_custom_user("bob", encrypted("bob", "hidden"))));

    std::string error;
    BOOST_REQUIRE_MESSAGE(service.reload_custom_passwd_file(error), error);
    BOOST_CHECK(error.empty());

    BOOST_CHECK(service.is_authentic(Identity::make_custom_user("bob", encrypted("bob", "hidden"))));
}

BOOST_AUTO_TEST_CASE(test_authentication_reload_does_not_check_users_for_host_and_port) {
    ECF_NAME_THIS_TEST();

    // Unlike valid(), reloading accepts a file whose users all target another server
    WithTestFile passwd(AutomaticTestFile{"ecf.passwd"},
                        passwd_file_content({{"alice", "secret"}}, "otherhost", "9999"));

    AuthenticationService service;
    service.set_passwd_file(passwd.path().string());

    std::string error;
    BOOST_CHECK_MESSAGE(service.reload_passwd_file(error), error);
    BOOST_CHECK(service.is_authentic(Identity::make_user("alice", encrypted("alice", "secret"))));
}

BOOST_AUTO_TEST_CASE(test_authentication_reload_picks_up_added_user) {
    ECF_NAME_THIS_TEST();

    WithTestFile passwd(AutomaticTestFile{"ecf.passwd"}, passwd_file_content({{"alice", "secret"}}));

    AuthenticationService service;
    configure_and_validate(service, passwd.path().string());

    BOOST_CHECK(!service.is_authentic(Identity::make_user("carol", encrypted("carol", "s3cr3t"))));

    // Rewrite the file, in place, with an additional user
    {
        std::ofstream os(passwd.path().string(), std::ios::out | std::ios::trunc);
        os << passwd_file_content({{"alice", "secret"}, {"carol", "s3cr3t"}});
    }

    std::string error;
    BOOST_REQUIRE_MESSAGE(service.reload_passwd_file(error), error);

    BOOST_CHECK(service.is_authentic(Identity::make_user("alice", encrypted("alice", "secret"))));
    BOOST_CHECK(service.is_authentic(Identity::make_user("carol", encrypted("carol", "s3cr3t"))));
}

BOOST_AUTO_TEST_CASE(test_authentication_reload_picks_up_removed_user) {
    ECF_NAME_THIS_TEST();

    WithTestFile passwd(AutomaticTestFile{"ecf.passwd"},
                        passwd_file_content({{"alice", "secret"}, {"carol", "s3cr3t"}}));

    AuthenticationService service;
    configure_and_validate(service, passwd.path().string());

    BOOST_CHECK(service.is_authentic(Identity::make_user("carol", encrypted("carol", "s3cr3t"))));

    {
        std::ofstream os(passwd.path().string(), std::ios::out | std::ios::trunc);
        os << passwd_file_content({{"alice", "secret"}});
    }

    std::string error;
    BOOST_REQUIRE_MESSAGE(service.reload_passwd_file(error), error);

    BOOST_CHECK(service.is_authentic(Identity::make_user("alice", encrypted("alice", "secret"))));
    BOOST_CHECK(!service.is_authentic(Identity::make_user("carol", encrypted("carol", "s3cr3t"))));
}

BOOST_AUTO_TEST_CASE(test_authentication_reload_picks_up_changed_password) {
    ECF_NAME_THIS_TEST();

    WithTestFile passwd(AutomaticTestFile{"ecf.passwd"}, passwd_file_content({{"alice", "secret"}}));

    AuthenticationService service;
    configure_and_validate(service, passwd.path().string());

    {
        std::ofstream os(passwd.path().string(), std::ios::out | std::ios::trunc);
        os << passwd_file_content({{"alice", "rotated"}});
    }

    std::string error;
    BOOST_REQUIRE_MESSAGE(service.reload_passwd_file(error), error);

    BOOST_CHECK(!service.is_authentic(Identity::make_user("alice", encrypted("alice", "secret"))));
    BOOST_CHECK(service.is_authentic(Identity::make_user("alice", encrypted("alice", "rotated"))));
}

BOOST_AUTO_TEST_CASE(test_authentication_reload_of_custom_file_picks_up_changes) {
    ECF_NAME_THIS_TEST();

    WithTestFile custom(AutomaticTestFile{"ecf.custom_passwd"}, passwd_file_content({{"bob", "hidden"}}));

    AuthenticationService service;
    service.set_custom_passwd_file(custom.path().string());
    std::string error;
    BOOST_REQUIRE_MESSAGE(service.valid(test_host, test_port, error), error);

    {
        std::ofstream os(custom.path().string(), std::ios::out | std::ios::trunc);
        os << passwd_file_content({{"dave", "other"}});
    }

    BOOST_REQUIRE_MESSAGE(service.reload_custom_passwd_file(error), error);

    BOOST_CHECK(!service.is_authentic(Identity::make_custom_user("bob", encrypted("bob", "hidden"))));
    BOOST_CHECK(service.is_authentic(Identity::make_custom_user("dave", encrypted("dave", "other"))));
}

BOOST_AUTO_TEST_CASE(test_authentication_reload_of_regular_file_does_not_affect_custom_users) {
    ECF_NAME_THIS_TEST();

    WithTestFile passwd(AutomaticTestFile{"ecf.passwd"}, passwd_file_content({{"alice", "secret"}}));
    WithTestFile custom(AutomaticTestFile{"ecf.custom_passwd"}, passwd_file_content({{"bob", "hidden"}}));

    AuthenticationService service;
    service.set_passwd_file(passwd.path().string());
    service.set_custom_passwd_file(custom.path().string());
    std::string error;
    BOOST_REQUIRE_MESSAGE(service.valid(test_host, test_port, error), error);

    {
        std::ofstream os(passwd.path().string(), std::ios::out | std::ios::trunc);
        os << passwd_file_content({{"carol", "s3cr3t"}});
    }

    BOOST_REQUIRE_MESSAGE(service.reload_passwd_file(error), error);

    BOOST_CHECK(!service.is_authentic(Identity::make_user("alice", encrypted("alice", "secret"))));
    BOOST_CHECK(service.is_authentic(Identity::make_user("carol", encrypted("carol", "s3cr3t"))));
    BOOST_CHECK(service.is_authentic(Identity::make_custom_user("bob", encrypted("bob", "hidden"))));
}

BOOST_AUTO_TEST_CASE(test_authentication_reload_after_file_removed_fails_and_keeps_previous_users) {
    ECF_NAME_THIS_TEST();

    AuthenticationService service;
    {
        WithTestFile passwd(AutomaticTestFile{"ecf.passwd"}, passwd_file_content({{"alice", "secret"}}));
        configure_and_validate(service, passwd.path().string());
    } // The file is removed here

    std::string error;
    BOOST_CHECK(!service.reload_passwd_file(error));
    BOOST_CHECK(error.find("does not exist") != std::string::npos);

    // A failed reload, due to a missing file, leaves the previously loaded users in place
    BOOST_CHECK(service.is_authentic(Identity::make_user("alice", encrypted("alice", "secret"))));
}

BOOST_AUTO_TEST_CASE(test_authentication_reload_with_debug_enabled) {
    ECF_NAME_THIS_TEST();

    WithTestFile passwd(AutomaticTestFile{"ecf.passwd"}, passwd_file_content({{"alice", "secret"}}));
    WithTestFile custom(AutomaticTestFile{"ecf.custom_passwd"}, passwd_file_content({{"bob", "hidden"}}));

    AuthenticationService service;
    service.set_debug(true);
    service.set_passwd_file(passwd.path().string());
    service.set_custom_passwd_file(custom.path().string());

    std::string error;
    BOOST_CHECK_MESSAGE(service.reload_passwd_file(error), error);
    BOOST_CHECK_MESSAGE(service.reload_custom_passwd_file(error), error);

    BOOST_CHECK(service.is_authentic(Identity::make_user("alice", encrypted("alice", "secret"))));
    BOOST_CHECK(service.is_authentic(Identity::make_custom_user("bob", encrypted("bob", "hidden"))));
}

BOOST_AUTO_TEST_CASE(test_authentication_reload_error_is_appended_to_existing_message) {
    ECF_NAME_THIS_TEST();

    AuthenticationService service;

    std::string error = "prefix: ";
    BOOST_CHECK(!service.reload_passwd_file(error));
    BOOST_CHECK_EQUAL(error.rfind("prefix: ", 0), 0u);
    BOOST_CHECK(error.size() > std::string("prefix: ").size());
}

// ===========================================================================
// End-to-end: the sequence performed by the server at start-up
// ===========================================================================

BOOST_AUTO_TEST_CASE(test_authentication_server_start_up_sequence_with_default_file_names) {
    ECF_NAME_THIS_TEST();

    // Files named as the server would expect, once the default names are expanded with host and port
    std::string regular_name = test_host + "." + test_port + ".ecf.passwd";
    std::string custom_name  = test_host + "." + test_port + ".ecf.custom_passwd";
    WithTestFile passwd(ecf::test::scaffold::NamedTestFile{regular_name}, passwd_file_content({{"alice", "secret"}}));
    WithTestFile custom(ecf::test::scaffold::NamedTestFile{custom_name}, passwd_file_content({{"bob", "hidden"}}));

    WithTestEnvironmentVariable passwd_env("ECF_PASSWD", std::string{AuthenticationService::default_passwd_file()});
    WithTestEnvironmentVariable custom_env("ECF_CUSTOM_PASSWD",
                                           std::string{AuthenticationService::default_custom_passwd_file()});

    AuthenticationService service;
    service.retrieve_passwd_file();
    service.retrieve_custom_passwd_file();

    Host host(test_host);
    service.init(host, test_port);

    BOOST_CHECK_EQUAL(service.passwd_file(), regular_name);
    BOOST_CHECK_EQUAL(service.custom_passwd_file(), custom_name);

    std::string error;
    BOOST_REQUIRE_MESSAGE(service.valid(test_host, test_port, error), error);

    BOOST_CHECK(service.is_authentic(Identity::make_user("alice", encrypted("alice", "secret"))));
    BOOST_CHECK(!service.is_authentic(Identity::make_user("alice", encrypted("alice", "wrong"))));
    BOOST_CHECK(service.is_authentic(Identity::make_custom_user("bob", encrypted("bob", "hidden"))));
    BOOST_CHECK(!service.is_authentic(Identity::make_custom_user("bob", encrypted("bob", "wrong"))));
    BOOST_CHECK(service.is_authentic(Identity::make_secure_user("anyone")));
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
