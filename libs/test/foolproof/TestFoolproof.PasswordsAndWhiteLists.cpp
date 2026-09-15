/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#include <fstream>
#include <regex>

#include <boost/test/unit_test.hpp>

#include "ecflow/test/scaffold/Naming.hpp"
#include "ecflow/test/scaffold/Provisioning.hpp"

BOOST_AUTO_TEST_SUITE(S_Foolproof)

BOOST_AUTO_TEST_SUITE(T_PasswordsAndWhitelists)

BOOST_AUTO_TEST_CASE(test_e2e_update_label_with_authentication_based_on_passwd_file) {
    ECF_NAME_THIS_TEST();

    /*
     * Description
     *
     * This test case verifies the password file authentication mechanism of ecFlow server
     *
     * Requirements
     *
     * - ecFlow accepts requests from users listed in the password file, with correct password.
     * - ecFlow refuses requests from users listed in the password file, with incorrect password.
     * - ecFlow refuses requests from users not listed in the password file.
     *
     */

    using namespace ecf::test::scaffold;

    auto cwd = MakeDirectory{}.create();

    auto user_a = User{"alice", "somesecret", ""};
    auto user_b = User{"bob", "anothersecret", ""};
    auto user_c = User{"charlie", "yetanothersecret", ""};
    auto user_d = User{"david", "adifferentsecret", ""}; // Note: not included in password file!

    auto host = MakeHost{}.create();
    auto port = MakePort{}.with(AutomaticPortValue{}).create();

    auto authentication = MakeTestFile{}
                              .with(SpecificFileLocation{"custom.passwds", cwd})
                              .with(PasswordsFile{host, port, user_a, user_b, User{"charlie", "incorrect", "r"}}.data())
                              .create();

    auto server_environment_cfg =
        MakeTestFile{}
            .with(SpecificFileLocation{"server_environment.cfg", cwd})
            .with(ServerEnvironmentFile{std::make_tuple("ECF_PASSWD", authentication.filename()),
                                        std::make_tuple("ECF_CUSTOM_PASSWD", authentication.filename())}
                      .data())
            .create();

    const auto server = MakeServer{}.with(host).with(port).with(cwd).launch();
    {
        BOOST_REQUIRE(server.ok());
        auto& s = server.value();
        BOOST_CHECK(s.pid() > 0);
        BOOST_CHECK(s.port().value() == port.value());
        BOOST_CHECK(s.host().is_valid());
    }

    auto defs = MakeTestFile{}
                    .with(SpecificFileLocation{"suite.def", cwd})
                    .with(R"--(
suite s
  family f
    task task
      label l "original_value"
  endfamily
endsuite;
)--"

                          )
                    .create();

    { // #authentication, using correct password -- load defs file % [success]
        auto client =
            RunClient{}.with(host).with(port).with(user_a).with(cwd).execute(RunClient::CommandLoad{defs.path()});
        BOOST_REQUIRE(client.ok());
        auto& c = client.value();

        ECF_TEST_DBG("Output of --load:\n" << c.stdout_buffer);
    }

    { // #authentication, using correct password -- check defs has been loaded % [success]
        auto client = RunClient{}.with(host).with(port).with(user_b).with(cwd).execute(RunClient::CommandGet{});
        BOOST_REQUIRE(client.ok());
        auto& c = client.value();
        BOOST_CHECK(c.stdout_contains(R"--(suite s)--"));
        BOOST_CHECK(c.stdout_contains(R"--(family f)--"));
        BOOST_CHECK(c.stdout_contains(R"--(task task)--"));
        BOOST_CHECK(c.stdout_contains(R"--(label l "original_value")--"));

        ECF_TEST_DBG("Output of --get:\n" << c.stdout_buffer);
    }

    { // #authentication, using incorrect password -- attempt to delete suite % [failure]
        auto client = RunClient{}.with(host).with(port).with(user_c).with(cwd).execute(RunClient::CommandDelete{"/s"});
        BOOST_REQUIRE(!client.ok());

        BOOST_CHECK(client.reason().find(
                        "Command not accepted, due to: Authentication (user) failed, due to: Incorrect credentials") !=
                    std::string::npos);
    }

    { // #authentication, using inexistent user -- attempt to delete suite % [failure]
        auto client = RunClient{}.with(host).with(port).with(user_d).with(cwd).execute(RunClient::CommandDelete{"/s"});
        BOOST_REQUIRE(!client.ok());

        BOOST_CHECK(client.reason().find(
                        "Command not accepted, due to: Authentication (user) failed, due to: Incorrect credentials") !=
                    std::string::npos);
    }
}

BOOST_AUTO_TEST_CASE(test_e2e_update_label_with_authorisation_based_on_whitelist_file) {
    ECF_NAME_THIS_TEST();

    /*
     * Description
     *
     * This test case verifies the whitelist file authorisation mechanism of ecFlow server
     *
     * Requirements
     *
     * - ecFlow accepts write requests from users set as 'rw' in the whitelist file.
     * - ecFlow accepts read requests from users set as 'rw' in the whitelist file.
     * - ecFlow accepts read requests from users set as 'r' in the whitelist file.
     * - ecFlow refuses write requests from users set as 'r' in the whitelist file.
     * - ecFlow refuses all requests from users not in the whitelist file.
     *
     */

    using namespace ecf::test::scaffold;

    auto cwd = MakeDirectory{}.create();

    auto user_a = User{"alice", "somesecret", "rw"};
    auto user_b = User{"bob", "anothersecret", "r"};
    auto user_c = User{"charlie", "yetanothersecret", "r"};
    auto user_d = User{"david", "topsecret", "rw"}; // Note: not included in whitelist file!

    auto host = MakeHost{}.create();
    auto port = MakePort{}.with(AutomaticPortValue{}).create();

    auto authentication = MakeTestFile{}
                              .with(SpecificFileLocation{"custom.passwds", cwd})
                              .with(PasswordsFile{host, port, user_a, user_b, User{"charlie", "incorrect", "r"}}.data())
                              .create();

    auto authorisation = MakeTestFile{}
                             .with(SpecificFileLocation{"custom.lists", cwd})
                             .with(WhitelistFile{user_a, user_b, user_c, user_d}.data())
                             .create();

    auto server_environment_cfg =
        MakeTestFile{}
            .with(SpecificFileLocation{"server_environment.cfg", cwd})
            .with(ServerEnvironmentFile{std::make_tuple("ECF_PASSWD", authentication.filename()),
                                        std::make_tuple("ECF_CUSTOM_PASSWD", authentication.filename()),
                                        std::make_tuple("ECF_LISTS", authorisation.filename())}
                      .data())
            .create();

    const auto server = MakeServer{}.with(host).with(port).with(cwd).launch();
    {
        BOOST_REQUIRE(server.ok());
        auto& s = server.value();
        BOOST_CHECK(s.pid() > 0);
        BOOST_CHECK(s.port().value() == port.value());
        BOOST_CHECK(s.host().is_valid());
    }

    auto defs = MakeTestFile{}
                    .with(SpecificFileLocation{"suite.def", cwd})
                    .with(R"--(
suite s
  family f
    task task
      label l "original_value"
  endfamily
endsuite;
)--")
                    .create();

    { // #authorisation, perform write operation with only "r" access -- load defs file % [failure]
        auto client =
            RunClient{}.with(host).with(port).with(user_b).with(cwd).execute(RunClient::CommandLoad{defs.path()});
        BOOST_REQUIRE(!client.ok());
        BOOST_CHECK(
            client.reason().find(
                "Command not accepted, due to: Authorisation (user) failed, due to: Insufficient permissions") !=
            std::string::npos);
    }

    { // #authorisation, perform write operation with "rw" access -- load defs file % [success]
        auto client =
            RunClient{}.with(host).with(port).with(user_a).with(cwd).execute(RunClient::CommandLoad{defs.path()});
        BOOST_REQUIRE(client.ok());
        auto& c = client.value();

        ECF_TEST_DBG("Output of --load:\n" << c.stdout_buffer);
    }

    { // #authorisation, perform read operation with only "r" access -- retrieve defs state  % [success]
        auto client = RunClient{}.with(host).with(port).with(user_b).with(cwd).execute(RunClient::CommandGetState{});
        BOOST_REQUIRE(client.ok());
        auto c = client.value();
        BOOST_CHECK(c.stdout_contains(R"--(suite s)--"));
        BOOST_CHECK(c.stdout_contains(R"--(family f)--"));
        BOOST_CHECK(c.stdout_contains(R"--(task task)--"));
        BOOST_CHECK(c.stdout_contains(R"--(label l "original_value")--"));

        ECF_TEST_DBG("Output of --get_state:\n" << c.stdout_buffer);
    }

    { // #authorisation, perform read operation with "rw" access -- retrieve defs state % [success]
        auto client = RunClient{}.with(host).with(port).with(user_a).with(cwd).execute(RunClient::CommandGetState{});
        BOOST_REQUIRE(client.ok());
        auto c = client.value();
        BOOST_CHECK(c.stdout_contains(R"--(suite s)--"));
        BOOST_CHECK(c.stdout_contains(R"--(family f)--"));
        BOOST_CHECK(c.stdout_contains(R"--(task task)--"));
        BOOST_CHECK(c.stdout_contains(R"--(label l "original_value")--"));

        ECF_TEST_DBG("Output of --get_state:\n" << c.stdout_buffer);
    }

    { // #authorisation, perform write operation with "rw" access -- update label value % [success]
        auto client = RunClient{}.with(host).with(port).with(user_a).with(cwd).execute(
            RunClient::CommandAlterUpdateLabel{"/s/f/task", "l", "updated_value"});
        BOOST_REQUIRE(client.ok());
        auto c = client.value();

        ECF_TEST_DBG("Output of --alter label:\n" << c.stdout_buffer);
    }

    { // ... check that previous update took effect
        auto client = RunClient{}.with(host).with(port).with(user_a).with(cwd).execute(RunClient::CommandGetState{});
        BOOST_REQUIRE(client.ok());
        auto c = client.value();
        BOOST_CHECK(c.stdout_contains(R"--(suite s)--"));
        BOOST_CHECK(c.stdout_contains(R"--(family f)--"));
        BOOST_CHECK(c.stdout_contains(R"--(task task)--"));
        BOOST_CHECK(c.stdout_contains(R"--(label l "original_value" # "updated_value")--"));

        ECF_TEST_DBG("Output of --get_state:\n" << c.stdout_buffer);
    }

    { // #authorisation, perform write operation with only "r" access -- update label value % [failure]
        auto client = RunClient{}.with(host).with(port).with(user_b).with(cwd).execute(
            RunClient::CommandAlterUpdateLabel{"/s/f/task", "l", "attempted_value"});
        BOOST_REQUIRE(!client.ok());

        BOOST_CHECK(
            client.reason().find(
                "Command not accepted, due to: Authorisation (user) failed, due to: Insufficient permissions") !=
            std::string::npos);
    }

    { // ... check that previous update did not take effect
        auto client = RunClient{}.with(host).with(port).with(user_b).with(cwd).execute(RunClient::CommandGetState{});
        BOOST_REQUIRE(client.ok());
        auto c = client.value();
        BOOST_CHECK(c.stdout_contains(R"--(label l "original_value" # "updated_value")--"));

        ECF_TEST_DBG("Output of --get_state:\n" << c.stdout_buffer);
    }

    { // #authentication, using incorrect password -- attempt to update label value % [failure]
        auto client = RunClient{}.with(host).with(port).with(user_c).with(cwd).execute(
            RunClient::CommandAlterUpdateLabel{"/s/f/task", "l", "attempted_value"});
        BOOST_REQUIRE(!client.ok());

        BOOST_CHECK(client.reason().find(
                        "Command not accepted, due to: Authentication (user) failed, due to: Incorrect credentials") !=
                    std::string::npos);
    }

    { // ... check that previous update did not take effect
        auto client = RunClient{}.with(host).with(port).with(user_a).with(cwd).execute(RunClient::CommandGetState{});
        BOOST_REQUIRE(client.ok());
        auto c = client.value();
        BOOST_CHECK(c.stdout_contains(R"--(label l "original_value" # "updated_value")--"));

        ECF_TEST_DBG("Output of --get_state:\n" << c.stdout_buffer);
    }

    { // #authentication, inexistent user -- attempt to update label value % [failure]
        auto client = RunClient{}.with(host).with(port).with(user_d).with(cwd).execute(
            RunClient::CommandAlterUpdateLabel{"/s/f/task", "l", "attempted_value"});
        BOOST_REQUIRE(!client.ok());

        BOOST_CHECK(client.reason().find(
                        "Command not accepted, due to: Authentication (user) failed, due to: Incorrect credentials") !=
                    std::string::npos);
    }

    { // ... check that previous update did not take effect
        auto client = RunClient{}.with(host).with(port).with(user_a).with(cwd).execute(RunClient::CommandGetState{});
        BOOST_REQUIRE(client.ok());
        auto c = client.value();
        BOOST_CHECK(c.stdout_contains(R"--(label l "original_value" # "updated_value")--"));

        ECF_TEST_DBG("Output of --get_state:\n" << c.stdout_buffer);
    }
}

BOOST_AUTO_TEST_CASE(test_e2e_reload_whitelist) {
    ECF_NAME_THIS_TEST();

    /*
     * Description
     *
     * This test case ensures that the 'reloadwsfile' command effectively reloads the content of a white list file.
     *
     * Requirements
     *
     * - The 'reloadwsfile' command loads the content of the whitelist file.
     *
     */

    using namespace ecf::test::scaffold;

    auto cwd = MakeDirectory{}.create();

    auto user_a = User{"alice", "somesecret", "rw"};
    auto user_b = User{"bob", "anothersecret", "r"};

    auto host = MakeHost{}.create();
    auto port = MakePort{}.with(AutomaticPortValue{}).create();

    auto authentication = MakeTestFile{}
                              .with(SpecificFileLocation{"custom.passwds", cwd})
                              .with(PasswordsFile{host, port, user_a, user_b}.data())
                              .create();

    auto authorisation = MakeTestFile{}
                             .with(SpecificFileLocation{"custom.lists", cwd})
                             .with(WhitelistFile{user_a, user_b}.data())
                             .create();

    auto server_environment_cfg =
        MakeTestFile{}
            .with(SpecificFileLocation{"server_environment.cfg", cwd})
            .with(ServerEnvironmentFile{std::make_tuple("ECF_PASSWD", authentication.filename()),
                                        std::make_tuple("ECF_CUSTOM_PASSWD", authentication.filename()),
                                        std::make_tuple("ECF_LISTS", authorisation.filename())}
                      .data())
            .create();

    const auto server = MakeServer{}.with(host).with(port).with(cwd).launch();
    {
        BOOST_REQUIRE(server.ok());
        auto& s = server.value();
        BOOST_CHECK(s.pid() > 0);
        BOOST_CHECK(s.port().value() == port.value());
        BOOST_CHECK(s.host().is_valid());
    }

    auto defs = MakeTestFile{}
                    .with(SpecificFileLocation{"suite.def", cwd})
                    .with(R"--(
suite s
  family f
    task task
      label l "original_value"
  endfamily
endsuite;
)--")
                    .create();

    { // #authorisation, perform write operation with "rw" access -- load defs file % [success]
        auto client =
            RunClient{}.with(host).with(port).with(user_a).with(cwd).execute(RunClient::CommandLoad{defs.path()});
        BOOST_REQUIRE(client.ok());
        auto& c = client.value();

        ECF_TEST_DBG("Output of --load:\n" << c.stdout_buffer);
    }

    { // #authorisation, perform write operation with "rw" access -- update label value % [success]
        auto client = RunClient{}.with(host).with(port).with(user_a).with(cwd).execute(
            RunClient::CommandAlterUpdateLabel{"/s/f/task", "l", "updated_value"});
        BOOST_REQUIRE(client.ok());
        auto c = client.value();

        ECF_TEST_DBG("Output of --alter label:\n" << c.stdout_buffer);
    }

    { // #authorisation, perform write operation with only "r" access -- update label value % [failure]
        auto client = RunClient{}.with(host).with(port).with(user_b).with(cwd).execute(
            RunClient::CommandAlterUpdateLabel{"/s/f/task", "l", "another_updated_value"});
        BOOST_REQUIRE(!client.ok());

        BOOST_CHECK(
            client.reason().find(
                "Command not accepted, due to: Authorisation (user) failed, due to: Insufficient permissions") !=
            std::string::npos);
    }

    user_a = User{"alice", "somesecret", "r"};
    user_b = User{"bob", "anothersecret", "rw"};

    authorisation = MakeTestFile{}
                        .with(SpecificFileLocation{"custom.lists", cwd})
                        .with(WhitelistFile{user_a, user_b}.data())
                        .create();

    { // #authorisation, perform write operation with "rw" access -- reload whitelist file % [success]
        auto client =
            RunClient{}.with(host).with(port).with(user_a).with(cwd).execute(RunClient::CommandReloadWhitelist{});
        BOOST_REQUIRE(client.ok());
        auto& c = client.value();

        ECF_TEST_DBG("Output of --reloadwsfile:\n" << c.stdout_buffer);
    }

    { // #authorisation, perform write operation with only "r" access -- update label value % [failure]
        auto client = RunClient{}.with(host).with(port).with(user_a).with(cwd).execute(
            RunClient::CommandAlterUpdateLabel{"/s/f/task", "l", "another_updated_value"});
        BOOST_REQUIRE(!client.ok());

        BOOST_CHECK(
            client.reason().find(
                "Command not accepted, due to: Authorisation (user) failed, due to: Insufficient permissions") !=
            std::string::npos);
    }

    { // #authorisation, perform write operation with only "rw" access -- update label value % [success]
        auto client = RunClient{}.with(host).with(port).with(user_b).with(cwd).execute(
            RunClient::CommandAlterUpdateLabel{"/s/f/task", "l", "updated_value"});
        BOOST_REQUIRE(client.ok());
        auto c = client.value();

        ECF_TEST_DBG("Output of --alter label:\n" << c.stdout_buffer);
    }
}

BOOST_AUTO_TEST_CASE(test_e2e_reload_whitelist_created_after_server_is_launched) {
    ECF_NAME_THIS_TEST();

    /*
     * Description
     *
     * This test case ensures that the 'reloadwsfile' command effectively reloads the content of a white list file,
     * created after the server is launched.
     *
     * Requirements
     *
     * - The 'reloadwsfile' command loads the content of the whitelist file, created after the server is launched.
     *
     */

    using namespace ecf::test::scaffold;

    auto cwd = MakeDirectory{}.create();

    auto user_a = User{"alice", "somesecret", "rw"};
    auto user_b = User{"bob", "anothersecret", "r"};

    auto host = MakeHost{}.create();
    auto port = MakePort{}.with(AutomaticPortValue{}).create();

    auto authentication = MakeTestFile{}
                              .with(SpecificFileLocation{"custom.passwds", cwd})
                              .with(PasswordsFile{host, port, user_a, user_b}.data())
                              .create();

    auto authorisation_file = fs::path{"custom.lists"};

    auto server_environment_cfg =
        MakeTestFile{}
            .with(SpecificFileLocation{"server_environment.cfg", cwd})
            .with(ServerEnvironmentFile{std::make_tuple("ECF_PASSWD", authentication.filename()),
                                        std::make_tuple("ECF_CUSTOM_PASSWD", authentication.filename()),
                                        std::make_tuple("ECF_LISTS", authorisation_file)}
                      .data())
            .create();

    const auto server = MakeServer{}.with(host).with(port).with(cwd).launch();
    {
        BOOST_REQUIRE(server.ok());
        auto& s = server.value();
        BOOST_CHECK(s.pid() > 0);
        BOOST_CHECK(s.port().value() == port.value());
        BOOST_CHECK(s.host().is_valid());
    }

    auto defs = MakeTestFile{}
                    .with(SpecificFileLocation{"suite.def", cwd})
                    .with(R"--(
suite s
  family f
    task task
      label l "original_value"
  endfamily
endsuite;
)--")
                    .create();

    { // #authorisation, perform write operation (without whitelist) -- load defs file % [success]
        auto client =
            RunClient{}.with(host).with(port).with(user_a).with(cwd).execute(RunClient::CommandLoad{defs.path()});
        BOOST_REQUIRE(client.ok());
        auto& c = client.value();

        ECF_TEST_DBG("Output of --load:\n" << c.stdout_buffer);
    }

    { // #authorisation, perform write operation (without whitelist) -- update label value % [success]
        auto client = RunClient{}.with(host).with(port).with(user_a).with(cwd).execute(
            RunClient::CommandAlterUpdateLabel{"/s/f/task", "l", "updated_value"});
        BOOST_REQUIRE(client.ok());
        auto c = client.value();

        ECF_TEST_DBG("Output of --alter label:\n" << c.stdout_buffer);
    }

    { // #authorisation, perform write operation (without whitelist) -- update label value % [success]
        auto client = RunClient{}.with(host).with(port).with(user_b).with(cwd).execute(
            RunClient::CommandAlterUpdateLabel{"/s/f/task", "l", "another_updated_value"});
        BOOST_REQUIRE(client.ok());
        auto c = client.value();

        ECF_TEST_DBG("Output of --alter label:\n" << c.stdout_buffer);
    }

    auto authorisation = MakeTestFile{}
                             .with(SpecificFileLocation{authorisation_file, cwd})
                             .with(WhitelistFile{user_a, user_b}.data())
                             .create();

    { // #authorisation, perform write operation (without whitelist) -- reload whitelist file % [success]
        auto client =
            RunClient{}.with(host).with(port).with(user_a).with(cwd).execute(RunClient::CommandReloadWhitelist{});
        BOOST_REQUIRE(client.ok());
        auto c = client.value();

        ECF_TEST_DBG("Output of --reloadwsfile:\n" << c.stdout_buffer);
    }

    { // #authorisation, perform write operation with "rw" access -- update label value % [success]
        auto client = RunClient{}.with(host).with(port).with(user_a).with(cwd).execute(
            RunClient::CommandAlterUpdateLabel{"/s/f/task", "l", "updated_value"});
        BOOST_REQUIRE(client.ok());
        auto c = client.value();

        ECF_TEST_DBG("Output of --alter label:\n" << c.stdout_buffer);
    }

    { // #authorisation, perform write operation with only "r" access -- update label value % [failure]
        auto client = RunClient{}.with(host).with(port).with(user_b).with(cwd).execute(
            RunClient::CommandAlterUpdateLabel{"/s/f/task", "l", "another_updated_value"});
        BOOST_REQUIRE(!client.ok());

        BOOST_CHECK(
            client.reason().find(
                "Command not accepted, due to: Authorisation (user) failed, due to: Insufficient permissions") !=
            std::string::npos);
    }
}

BOOST_AUTO_TEST_CASE(test_e2e_rejection_message_includes_username) {
    ECF_NAME_THIS_TEST();

    /*
     * Description
     *
     * This test case verifies that, when a command is rejected, the rejection message returned to the client
     * includes the rejected command, the originating user name and the client host (within square brackets).
     * See ECFLOW-2097.
     *
     * Requirements
     *
     * - An authorisation failure message ends with the rejected command, user and host, as:
     *   [--<command> :<username>@<host>].
     * - An authentication failure message ends with the rejected command, user and host, as:
     *   [--<command> :<username>@<host>].
     *
     */

    using namespace ecf::test::scaffold;

    auto cwd = MakeDirectory{}.create();

    auto user_alice   = User{"alice", "somesecret", "rw"};
    auto user_bob     = User{"bob", "anothersecret", "r"};
    auto user_charlie = User{"charlie", "topsecret", "rw"}; // Note: not included in password file!

    auto host = MakeHost{}.create();
    auto port = MakePort{}.with(AutomaticPortValue{}).create();

    auto authentication = MakeTestFile{}
                              .with(SpecificFileLocation{"custom.passwds", cwd})
                              .with(PasswordsFile{host, port, user_alice, user_bob}.data())
                              .create();

    auto authorisation = MakeTestFile{}
                             .with(SpecificFileLocation{"custom.lists", cwd})
                             .with(WhitelistFile{user_alice, user_bob}.data())
                             .create();

    auto server_environment_cfg =
        MakeTestFile{}
            .with(SpecificFileLocation{"server_environment.cfg", cwd})
            .with(ServerEnvironmentFile{std::make_tuple("ECF_PASSWD", authentication.filename()),
                                        std::make_tuple("ECF_CUSTOM_PASSWD", authentication.filename()),
                                        std::make_tuple("ECF_LISTS", authorisation.filename())}
                      .data())
            .create();

    const auto server = MakeServer{}.with(host).with(port).with(cwd).launch();
    {
        BOOST_REQUIRE(server.ok());
        auto& s = server.value();
        BOOST_CHECK(s.pid() > 0);
        BOOST_CHECK(s.port().value() == port.value());
        BOOST_CHECK(s.host().is_valid());
    }

    auto defs = MakeTestFile{}
                    .with(SpecificFileLocation{"suite.def", cwd})
                    .with(R"--(
suite s
  family f
    task task
      label l "original_value"
  endfamily
endsuite;
)--")
                    .create();

    { // #authorisation, load defs file with alice's "rw" access % [success]
        auto client =
            RunClient{}.with(host).with(port).with(user_alice).with(cwd).execute(RunClient::CommandLoad{defs.path()});
        BOOST_REQUIRE(client.ok());
    }

    { // #authorisation, perform write operation with bob's "r" only access
        auto client = RunClient{}.with(host).with(port).with(user_bob).with(cwd).execute(
            RunClient::CommandAlterUpdateLabel{"/s/f/task", "l", "attempted_value"});
        BOOST_REQUIRE(!client.ok());

        BOOST_CHECK(client.reason().find("Command not accepted, due to: Authorisation (user) failed, due to: "
                                         "Insufficient permissions") != std::string::npos);
        // The rejected command, originating user name and client host are present in the rejection message
        BOOST_CHECK(client.reason().find("[--alter change label l attempted_value /s/f/task :bob@" + host.value() +
                                         "]") != std::string::npos);
    }

    { // #authentication, perform operation with unknown user (i.e. not present in password file)
        auto client = RunClient{}
                          .with(host)
                          .with(port)
                          .with(user_charlie)
                          .with(cwd)
                          .execute(RunClient::CommandAlterUpdateLabel{"/s/f/task", "l", "attempted_value"});
        BOOST_REQUIRE(!client.ok());

        BOOST_CHECK(client.reason().find(
                        "Command not accepted, due to: Authentication (user) failed, due to: Incorrect credentials") !=
                    std::string::npos);
        // The rejected command, originating user name and client host are present in the rejection message
        BOOST_CHECK(client.reason().find("[--alter change label l attempted_value /s/f/task :charlie@" + host.value() +
                                         "]") != std::string::npos);
    }
}

BOOST_AUTO_TEST_CASE(test_e2e_refused_news_leaves_log_well_formed) {
    ECF_NAME_THIS_TEST();

    /*
     * Description
     *
     * This test case verifies that a refused --news request does not corrupt the server log.
     *
     * The --news command is logged without a trailing newline, so that the reply can append its outcome and
     * terminate the line. Older servers logged the command before checking the credentials, and thus a refused
     * --news left the log line open; the next record written by the server (the refusal itself) was then glued
     * onto the end of the same line, and both records became unparseable.
     *
     * Requirements
     *
     * - Every line in the server log starts with a record marker (e.g. MSG:[hh:mm:ss d.m.yyyy]).
     * - No line in the server log contains a record marker after the first column.
     * - The refusal of a --news request, due to failed authentication or authorisation, is logged on its own line,
     *   and identifies the rejected command, the user and the client host.
     * - An accepted --news request is logged on its own, terminated line.
     *
     */

    using namespace ecf::test::scaffold;

    auto cwd = MakeDirectory{}.create();

    auto user_alice   = User{"alice", "somesecret", "rw"};
    auto user_bob     = User{"bob", "anothersecret", "r"};  // Note: not included in whitelist file!
    auto user_charlie = User{"charlie", "topsecret", "rw"}; // Note: not included in password file!

    auto host = MakeHost{}.create();
    auto port = MakePort{}.with(AutomaticPortValue{}).create();

    auto authentication = MakeTestFile{}
                              .with(SpecificFileLocation{"custom.passwds", cwd})
                              .with(PasswordsFile{host, port, user_alice, user_bob}.data())
                              .create();

    auto authorisation =
        MakeTestFile{}.with(SpecificFileLocation{"custom.lists", cwd}).with(WhitelistFile{user_alice}.data()).create();

    const auto log_path = cwd.path() / "custom.ecf.log";

    auto server_environment_cfg =
        MakeTestFile{}
            .with(SpecificFileLocation{"server_environment.cfg", cwd})
            .with(ServerEnvironmentFile{std::make_tuple("ECF_PASSWD", authentication.filename()),
                                        std::make_tuple("ECF_CUSTOM_PASSWD", authentication.filename()),
                                        std::make_tuple("ECF_LISTS", authorisation.filename()),
                                        std::make_tuple("ECF_LOG", log_path.string())}
                      .data())
            .create();

    const auto server = MakeServer{}.with(host).with(port).with(cwd).launch();
    {
        BOOST_REQUIRE(server.ok());
        auto& s = server.value();
        BOOST_CHECK(s.pid() > 0);
        BOOST_CHECK(s.port().value() == port.value());
        BOOST_CHECK(s.host().is_valid());
    }

    { // #authorisation, request news with alice's "rw" access % [success]
        auto client = RunClient{}.with(host).with(port).with(user_alice).with(cwd).execute(RunClient::CommandNews{});
        BOOST_REQUIRE(client.ok());
    }

    { // #authorisation, request news with bob, who is not in the whitelist % [failure]
        auto client = RunClient{}.with(host).with(port).with(user_bob).with(cwd).execute(RunClient::CommandNews{});
        BOOST_REQUIRE(!client.ok());
        BOOST_CHECK(client.reason().find("Command not accepted, due to: Authorisation (user) failed, due to: "
                                         "Insufficient permissions") != std::string::npos);
    }

    { // ... the next record must land on its own line, not on the (refused) news line
        auto client = RunClient{}.with(host).with(port).with(user_alice).with(cwd).execute(RunClient::CommandPing{});
        BOOST_REQUIRE(client.ok());
    }

    { // #authentication, request news with charlie, who is not in the password file % [failure]
        auto client = RunClient{}.with(host).with(port).with(user_charlie).with(cwd).execute(RunClient::CommandNews{});
        BOOST_REQUIRE(!client.ok());
        BOOST_CHECK(client.reason().find(
                        "Command not accepted, due to: Authentication (user) failed, due to: Incorrect credentials") !=
                    std::string::npos);
    }

    { // ... the next record must land on its own line, not on the (refused) news line
        auto client = RunClient{}.with(host).with(port).with(user_alice).with(cwd).execute(RunClient::CommandPing{});
        BOOST_REQUIRE(client.ok());
    }

    // Inspect the server log (flushed by the server after each request)

    std::vector<std::string> lines;
    {
        BOOST_REQUIRE_MESSAGE(fs::exists(log_path), "The server log file exists at " << log_path);
        std::ifstream ifs(log_path);
        for (std::string line; std::getline(ifs, line);) {
            if (!line.empty()) {
                lines.push_back(line);
            }
        }
        BOOST_REQUIRE(!lines.empty());
    }

    const std::regex record_marker{R"(^(MSG|LOG|ERR|WAR|DBG|OTH):\[\d{1,2}:\d{2}:\d{2} \d{1,2}\.\d{1,2}\.\d{4}\])"};
    const std::regex embedded_record_marker{R"(.+(MSG|LOG|ERR|WAR|DBG|OTH):\[\d{1,2}:\d{2}:\d{2} )"};

    const std::string accepted_news_record = "--news=0 0 0 :alice@" + host.value();
    const std::string refused_news_bob     = "Command not accepted, due to: Authorisation (user) failed, due to: "
                                             "Insufficient permissions [--news=0 0 0 :bob@" +
                                         host.value() + "]";
    const std::string refused_news_charlie = "Command not accepted, due to: Authentication (user) failed, due to: "
                                             "Incorrect credentials detected [--news=0 0 0 :charlie@" +
                                             host.value() + "]";

    size_t accepted_news    = 0;
    size_t refusals_bob     = 0;
    size_t refusals_charlie = 0;
    for (const auto& line : lines) {
        ECF_TEST_DBG("Log line: " << line);
        BOOST_CHECK_MESSAGE(std::regex_search(line, record_marker), "Line starts with a record marker: " << line);
        BOOST_CHECK_MESSAGE(!std::regex_search(line, embedded_record_marker),
                            "Line holds a single record (no embedded record marker): " << line);
        if (line.find(accepted_news_record) != std::string::npos) {
            accepted_news++;
        }
        if (line.find(refused_news_bob) != std::string::npos) {
            refusals_bob++;
        }
        if (line.find(refused_news_charlie) != std::string::npos) {
            refusals_charlie++;
        }
    }

    // The accepted --news is logged on its own (terminated) line
    BOOST_CHECK_EQUAL(accepted_news, 1);
    // Each refusal is logged on its own line, identifying the rejected command, the user and the client host
    BOOST_CHECK_EQUAL(refusals_bob, 1);
    BOOST_CHECK_EQUAL(refusals_charlie, 1);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(T_PasswordFile)

BOOST_AUTO_TEST_CASE(test_e2e_load_password_file_using_just_defaults) {
    ECF_NAME_THIS_TEST();

    /*
     * Description
     *
     * This test case verifies the correct file name is used to load the user password, when:
     *  - not provided with any additional configuration
     *
     * Requirements
     *
     * - ecFlow uses the default password file name to load 'regular' user credentials.
     * - ecFlow considers `<host>.<port>.ecf.passwd` as the default 'regular' user password file name.
     *
     * - ecFlow uses the default password file name to load 'custom' user credentials.
     * - ecFlow considers `<host>.<port>.ecf.custom_passwd` as the default 'custom' user password file name.
     *
     */

    using namespace ecf::test::scaffold;

    auto cwd = MakeDirectory{}.create();

    auto host = MakeHost{}.create();
    auto port = MakePort{}.with(AutomaticPortValue{}).create();

    auto user = User{"alice", "somesecret", "rw"};

    std::string default_passwd        = "ecf.passwd";
    std::string default_custom_passwd = "ecf.custom_passwd";
    std::string actual_passwd         = host.value() + "." + std::to_string(port.value()) + "." + default_passwd;
    std::string actual_custom_passwd  = host.value() + "." + std::to_string(port.value()) + "." + default_custom_passwd;

    auto authentication_1 = MakeTestFile{}
                                .with(SpecificFileLocation{actual_passwd, cwd})
                                .with(PasswordsFile{host, port, user}.data())
                                .create();
    auto authentication_2 = MakeTestFile{}
                                .with(SpecificFileLocation{actual_custom_passwd, cwd})
                                .with(PasswordsFile{host, port, user}.data())
                                .create();

    auto server = MakeServer{}.with(host).with(port).with(cwd).launch();
    {
        BOOST_REQUIRE(server.ok());
        auto& s = server.value();
        BOOST_CHECK(s.pid() > 0);
        BOOST_CHECK(s.port().value() == port.value());
        BOOST_CHECK(s.host().is_valid());
    }

    auto s          = server.get();
    auto [out, err] = s.shutdown();

    ECF_TEST_DBG("\n\n\n****\n\n" << out << "\n\n\n");

    BOOST_CHECK(out.find("Password file located at '" + actual_passwd + "'") != std::string::npos);
    BOOST_CHECK(out.find("Password file successfully loaded") != std::string::npos);
    BOOST_CHECK(out.find("Custom Password file located at '" + actual_custom_passwd + "'") != std::string::npos);
    BOOST_CHECK(out.find("Custom Password file successfully loaded") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(test_e2e_load_password_file_defined_in_envvar_with_default_value) {
    ECF_NAME_THIS_TEST();

    /*
     * Description
     *
     * This test case verifies the correct file name is used to load the user password, when:
     *  - `ECF_PASSWD=ecf.passwd` environment variable is exported
     *  - `ECF_CUSTOM_PASSWD=ecf.custom_passwd` environment variable is exported
     *
     * Requirements
     *
     * - ecFlow uses the default password file name to load 'regular' user credentials.
     * - ecFlow considers `<host>.<port>.ecf.passwd` as the default 'regular' user password file name.
     *
     * - ecFlow uses the default password file name to load 'custom' user credentials.
     * - ecFlow considers `<host>.<port>.ecf.custom_passwd` as the default 'custom' user password file name.
     *
     */

    using namespace ecf::test::scaffold;

    auto cwd = MakeDirectory{}.create();

    auto host = MakeHost{}.create();
    auto port = MakePort{}.with(AutomaticPortValue{}).create();

    auto user = User{"alice", "somesecret", "rw"};

    std::string default_passwd        = "ecf.passwd";
    std::string default_custom_passwd = "ecf.custom_passwd";
    std::string actual_passwd         = host.value() + "." + std::to_string(port.value()) + "." + default_passwd;
    std::string actual_custom_passwd  = host.value() + "." + std::to_string(port.value()) + "." + default_custom_passwd;

    auto authentication_1 = MakeTestFile{}
                                .with(SpecificFileLocation{actual_passwd, cwd})
                                .with(PasswordsFile{host, port, user}.data())
                                .create();
    auto authentication_2 = MakeTestFile{}
                                .with(SpecificFileLocation{actual_custom_passwd, cwd})
                                .with(PasswordsFile{host, port, user}.data())
                                .create();

    auto passwd_envvar        = MakeEnvironmentVariable{}.with("ECF_PASSWD", default_passwd).create();
    auto custom_passwd_envvar = MakeEnvironmentVariable{}.with("ECF_CUSTOM_PASSWD", default_custom_passwd).create();

    auto server = MakeServer{}.with(host).with(port).with(cwd).launch();
    {
        BOOST_REQUIRE(server.ok());
        auto& s = server.value();
        BOOST_CHECK(s.pid() > 0);
        BOOST_CHECK(s.port().value() == port.value());
        BOOST_CHECK(s.host().is_valid());
    }

    auto s          = server.get();
    auto [out, err] = s.shutdown();

    BOOST_CHECK(out.find("Password file located at '" + actual_passwd + "'") != std::string::npos);
    BOOST_CHECK(out.find("Password file successfully loaded") != std::string::npos);
    BOOST_CHECK(out.find("Custom Password file located at '" + actual_custom_passwd + "'") != std::string::npos);
    BOOST_CHECK(out.find("Custom Password file successfully loaded") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(test_e2e_load_password_file_defined_in_cfg_with_default_value) {
    ECF_NAME_THIS_TEST();

    /*
     * Description
     *
     * This test case verifies the correct file name is used to load the user password, when:
     *  - `ECF_PASSWD=ecf.passwd` variable is provided in server_environment.cfg
     *  - `ECF_CUSTOM_PASSWD=ecf.custom_passwd` variable is provided in server_environment.cfg
     *
     * Requirements
     *
     * - ecFlow uses the default password file name to load 'regular' user credentials.
     * - ecFlow considers `<host>.<port>.ecf.passwd` as the default 'regular' user password file name.
     *
     * - ecFlow uses the default password file name to load 'custom' user credentials.
     * - ecFlow considers `<host>.<port>.ecf.custom_passwd` as the default 'custom' user password file name.
     *
     */

    using namespace ecf::test::scaffold;

    auto cwd = MakeDirectory{}.create();

    auto host = MakeHost{}.create();
    auto port = MakePort{}.with(AutomaticPortValue{}).create();

    auto user = User{"alice", "somesecret", "rw"};

    std::string default_passwd        = "ecf.passwd";
    std::string default_custom_passwd = "ecf.custom_passwd";
    std::string actual_passwd         = host.value() + "." + std::to_string(port.value()) + "." + default_passwd;
    std::string actual_custom_passwd  = host.value() + "." + std::to_string(port.value()) + "." + default_custom_passwd;

    auto authentication_1 = MakeTestFile{}
                                .with(SpecificFileLocation{actual_passwd, cwd})
                                .with(PasswordsFile{host, port, user}.data())
                                .create();
    auto authentication_2 = MakeTestFile{}
                                .with(SpecificFileLocation{actual_custom_passwd, cwd})
                                .with(PasswordsFile{host, port, user}.data())
                                .create();

    auto server_environment_cfg =
        MakeTestFile{}
            .with(SpecificFileLocation{"server_environment.cfg", cwd})
            .with(ServerEnvironmentFile{std::make_tuple("ECF_PASSWD", default_passwd),
                                        std::make_tuple("ECF_CUSTOM_PASSWD", default_custom_passwd)}
                      .data())
            .create();

    auto server = MakeServer{}.with(host).with(port).with(cwd).launch();
    {
        BOOST_REQUIRE(server.ok());
        auto& s = server.value();
        BOOST_CHECK(s.pid() > 0);
        BOOST_CHECK(s.port().value() == port.value());
        BOOST_CHECK(s.host().is_valid());
    }

    auto s          = server.get();
    auto [out, err] = s.shutdown();

    BOOST_CHECK(out.find("Password file located at '" + actual_passwd + "'") != std::string::npos);
    BOOST_CHECK(out.find("Password file successfully loaded") != std::string::npos);
    BOOST_CHECK(out.find("Custom Password file located at '" + actual_custom_passwd + "'") != std::string::npos);
    BOOST_CHECK(out.find("Custom Password file successfully loaded") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(test_e2e_load_password_file_defined_in_envvar_with_specific_value) {
    ECF_NAME_THIS_TEST();

    /*
     * Description
     *
     * This test case verifies the correct file name is used to load the user password, when:
     *  - `ECF_PASSWD=<specific-value>` environment variable is exported
     *  - `ECF_CUSTOM_PASSWD=<specific-value>` environment variable is exported
     *
     * Requirements
     *
     * - ecFlow uses the specific password file name to load 'regular' user credentials.
     * - ecFlow considers `<specific-value>` as the default 'regular' user password file name.
     *
     * - ecFlow uses the specific password file name to load 'custom' user credentials.
     * - ecFlow considers `<specific-value>` as the default 'custom' user password file name.
     *
     */

    using namespace ecf::test::scaffold;

    auto cwd = MakeDirectory{}.create();

    auto host = MakeHost{}.create();
    auto port = MakePort{}.with(AutomaticPortValue{}).create();

    auto user = User{"alice", "somesecret", "rw"};

    std::string specific_passwd        = "specific.passwd";
    std::string specific_custom_passwd = "specific.custom_passwd";

    auto authentication_1 = MakeTestFile{}
                                .with(SpecificFileLocation{specific_passwd, cwd})
                                .with(PasswordsFile{host, port, user}.data())
                                .create();
    auto authentication_2 = MakeTestFile{}
                                .with(SpecificFileLocation{specific_custom_passwd, cwd})
                                .with(PasswordsFile{host, port, user}.data())
                                .create();

    auto passwd_envvar        = MakeEnvironmentVariable{}.with("ECF_PASSWD", specific_passwd).create();
    auto custom_passwd_envvar = MakeEnvironmentVariable{}.with("ECF_CUSTOM_PASSWD", specific_custom_passwd).create();

    auto server = MakeServer{}.with(host).with(port).with(cwd).launch();
    {
        BOOST_REQUIRE(server.ok());
        auto& s = server.value();
        BOOST_CHECK(s.pid() > 0);
        BOOST_CHECK(s.port().value() == port.value());
        BOOST_CHECK(s.host().is_valid());
    }

    auto s          = server.get();
    auto [out, err] = s.shutdown();

    ECF_TEST_DBG("\n\n\n****\n\n" << out << "\n\n\n");

    BOOST_CHECK(out.find("Password file located at '" + specific_passwd + "'") != std::string::npos);
    BOOST_CHECK(out.find("Password file successfully loaded") != std::string::npos);
    BOOST_CHECK(out.find("Custom Password file located at '" + specific_custom_passwd + "'") != std::string::npos);
    BOOST_CHECK(out.find("Custom Password file successfully loaded") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(test_e2e_load_password_file_defined_in_cfg_with_specific_value) {
    ECF_NAME_THIS_TEST();

    /*
     * Description
     *
     * This test case verifies the correct file name is used to load the user password, when:
     *  - `ECF_PASSWD=<specific-value>` variable is provided in server_environment.cfg
     *  - `ECF_CUSTOM_PASSWD=<specific-value>` variable is provided in server_environment.cfg
     *
     * Requirements
     *
     * - ecFlow uses the specific password file name to load 'regular' user credentials.
     * - ecFlow considers `<specific-value>` as the default 'regular' user password file name.
     *
     * - ecFlow uses the specific password file name to load 'custom' user credentials.
     * - ecFlow considers `<specific-value>` as the default 'custom' user password file name.
     *
     */

    using namespace ecf::test::scaffold;

    auto cwd = MakeDirectory{}.create();

    auto host = MakeHost{}.create();
    auto port = MakePort{}.with(AutomaticPortValue{}).create();

    auto user = User{"alice", "somesecret", "rw"};

    std::string specific_passwd        = "specific.passwd";
    std::string specific_custom_passwd = "specific.custom_passwd";

    auto authentication_1 = MakeTestFile{}
                                .with(SpecificFileLocation{specific_passwd, cwd})
                                .with(PasswordsFile{host, port, user}.data())
                                .create();
    auto authentication_2 = MakeTestFile{}
                                .with(SpecificFileLocation{specific_custom_passwd, cwd})
                                .with(PasswordsFile{host, port, user}.data())
                                .create();

    auto server_environment_cfg =
        MakeTestFile{}
            .with(SpecificFileLocation{"server_environment.cfg", cwd})
            .with(ServerEnvironmentFile{std::make_tuple("ECF_PASSWD", specific_passwd),
                                        std::make_tuple("ECF_CUSTOM_PASSWD", specific_custom_passwd)}
                      .data())
            .create();

    auto server = MakeServer{}.with(host).with(port).with(cwd).launch();
    {
        BOOST_REQUIRE(server.ok());
        auto& s = server.value();
        BOOST_CHECK(s.pid() > 0);
        BOOST_CHECK(s.port().value() == port.value());
        BOOST_CHECK(s.host().is_valid());
    }

    auto s          = server.get();
    auto [out, err] = s.shutdown();

    BOOST_CHECK(out.find("Password file located at '" + specific_passwd + "'") != std::string::npos);
    BOOST_CHECK(out.find("Password file successfully loaded") != std::string::npos);
    BOOST_CHECK(out.find("Custom Password file located at '" + specific_custom_passwd + "'") != std::string::npos);
    BOOST_CHECK(out.find("Custom Password file successfully loaded") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(test_e2e_load_password_file_defined_in_cfg_with_specific_value_and_then_overriden_by_envvar) {
    ECF_NAME_THIS_TEST();

    /*
     * Description
     *
     * This test case verifies the correct file name is used to load the user password, when:
     *  - `ECF_PASSWD=<original-value>` variable is provided in server_environment.cfg
     *  - `ECF_CUSTOM_PASSWD=<original-value>` variable is provided in server_environment.cfg
     *  - `ECF_PASSWD=<specific-value>` environment variable is exported
     *  - `ECF_CUSTOM_PASSWD=<specific-value>` environment variable is exported (note that the original value is
     *
     * Requirements
     *
     * - ecFlow uses the specific password file name to load 'regular' user credentials.
     * - ecFlow considers that `<specific-value>` overrides `<original-value>` as the 'regular' user password file name.
     *
     * - ecFlow uses the specific password file name to load 'custom' user credentials.
     * - ecFlow considers that `<specific-value>` overrides `<original-value>` as the 'custom' user password file name.
     *
     */

    using namespace ecf::test::scaffold;

    auto cwd = MakeDirectory{}.create();

    auto host = MakeHost{}.create();
    auto port = MakePort{}.with(AutomaticPortValue{}).create();

    auto user = User{"alice", "somesecret", "rw"};

    std::string specific_passwd        = "specific.passwd";
    std::string original_passwd        = specific_passwd + "_extra";
    std::string specific_custom_passwd = "specific.custom_passwd";
    std::string original_custom_passwd = specific_custom_passwd + "_extra";

    auto authentication_1 = MakeTestFile{}
                                .with(SpecificFileLocation{specific_passwd, cwd})
                                .with(PasswordsFile{host, port, user}.data())
                                .create();
    auto authentication_2 = MakeTestFile{}
                                .with(SpecificFileLocation{specific_custom_passwd, cwd})
                                .with(PasswordsFile{host, port, user}.data())
                                .create();

    auto passwd_envvar        = MakeEnvironmentVariable{}.with("ECF_PASSWD", specific_passwd).create();
    auto custom_passwd_envvar = MakeEnvironmentVariable{}.with("ECF_CUSTOM_PASSWD", specific_custom_passwd).create();

    auto server_environment_cfg =
        MakeTestFile{}
            .with(SpecificFileLocation{"server_environment.cfg", cwd})
            .with(ServerEnvironmentFile{std::make_tuple("ECF_PASSWD", original_passwd),
                                        std::make_tuple("ECF_CUSTOM_PASSWD", original_custom_passwd)}
                      .data())
            .create();

    auto server = MakeServer{}.with(host).with(port).with(cwd).launch();
    {
        BOOST_REQUIRE(server.ok());
        auto& s = server.value();
        BOOST_CHECK(s.pid() > 0);
        BOOST_CHECK(s.port().value() == port.value());
        BOOST_CHECK(s.host().is_valid());
    }

    auto s          = server.get();
    auto [out, err] = s.shutdown();

    ECF_TEST_DBG("\n\n\n****\n\n" << out << "\n\n\n");

    BOOST_CHECK(out.find("Password file located at '" + specific_passwd + "'") != std::string::npos);
    BOOST_CHECK(out.find("Password file successfully loaded") != std::string::npos);
    BOOST_CHECK(out.find("Custom Password file located at '" + specific_custom_passwd + "'") != std::string::npos);
    BOOST_CHECK(out.find("Custom Password file successfully loaded") != std::string::npos);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
