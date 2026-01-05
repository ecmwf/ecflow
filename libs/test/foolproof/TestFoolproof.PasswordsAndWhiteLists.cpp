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

#include "scaffold/Naming.hpp"
#include "scaffold/Provisioning.hpp"

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

    using namespace foolproof::scaffold;

    auto cwd = MakeDirectory{}.create();

    auto user_a = User{"alice", "somesecret"};
    auto user_b = User{"bob", "anothersecret"};
    auto user_c = User{"charlie", "yetanothersecret"};
    auto user_d = User{"david", "adifferentsecret"}; // Note: not included in password file!

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

        BOOST_CHECK(
            client.reason().find(
                "Command not accepted, due to: Authentication (user command) failed, due to: Incorrect credentials") !=
            std::string::npos);
    }

    { // #authentication, using inexistent user -- attempt to delete suite % [failure]
        auto client = RunClient{}.with(host).with(port).with(user_d).with(cwd).execute(RunClient::CommandDelete{"/s"});
        BOOST_REQUIRE(!client.ok());

        BOOST_CHECK(
            client.reason().find(
                "Command not accepted, due to: Authentication (user command) failed, due to: Incorrect credentials") !=
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

    using namespace foolproof::scaffold;

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
            RunClient::CommandUpdateLabel{"/s/f/task", "l", "updated_value"});
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
            RunClient::CommandUpdateLabel{"/s/f/task", "l", "attempted_value"});
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
            RunClient::CommandUpdateLabel{"/s/f/task", "l", "attempted_value"});
        BOOST_REQUIRE(!client.ok());

        BOOST_CHECK(
            client.reason().find(
                "Command not accepted, due to: Authentication (user command) failed, due to: Incorrect credentials") !=
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
            RunClient::CommandUpdateLabel{"/s/f/task", "l", "attempted_value"});
        BOOST_REQUIRE(!client.ok());

        BOOST_CHECK(
            client.reason().find(
                "Command not accepted, due to: Authentication (user command) failed, due to: Incorrect credentials") !=
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

    using namespace foolproof::scaffold;

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
            RunClient::CommandUpdateLabel{"/s/f/task", "l", "updated_value"});
        BOOST_REQUIRE(client.ok());
        auto c = client.value();

        ECF_TEST_DBG("Output of --alter label:\n" << c.stdout_buffer);
    }

    { // #authorisation, perform write operation with only "r" access -- update label value % [failure]
        auto client = RunClient{}.with(host).with(port).with(user_b).with(cwd).execute(
            RunClient::CommandUpdateLabel{"/s/f/task", "l", "another_updated_value"});
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
            RunClient::CommandUpdateLabel{"/s/f/task", "l", "another_updated_value"});
        BOOST_REQUIRE(!client.ok());

        BOOST_CHECK(
            client.reason().find(
                "Command not accepted, due to: Authorisation (user) failed, due to: Insufficient permissions") !=
            std::string::npos);
    }

    { // #authorisation, perform write operation with only "rw" access -- update label value % [success]
        auto client = RunClient{}.with(host).with(port).with(user_b).with(cwd).execute(
            RunClient::CommandUpdateLabel{"/s/f/task", "l", "updated_value"});
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

    using namespace foolproof::scaffold;

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
            RunClient::CommandUpdateLabel{"/s/f/task", "l", "updated_value"});
        BOOST_REQUIRE(client.ok());
        auto c = client.value();

        ECF_TEST_DBG("Output of --alter label:\n" << c.stdout_buffer);
    }

    { // #authorisation, perform write operation (without whitelist) -- update label value % [success]
        auto client = RunClient{}.with(host).with(port).with(user_b).with(cwd).execute(
            RunClient::CommandUpdateLabel{"/s/f/task", "l", "another_updated_value"});
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
            RunClient::CommandUpdateLabel{"/s/f/task", "l", "updated_value"});
        BOOST_REQUIRE(client.ok());
        auto c = client.value();

        ECF_TEST_DBG("Output of --alter label:\n" << c.stdout_buffer);
    }

    { // #authorisation, perform write operation with only "r" access -- update label value % [failure]
        auto client = RunClient{}.with(host).with(port).with(user_b).with(cwd).execute(
            RunClient::CommandUpdateLabel{"/s/f/task", "l", "another_updated_value"});
        BOOST_REQUIRE(!client.ok());

        BOOST_CHECK(
            client.reason().find(
                "Command not accepted, due to: Authorisation (user) failed, due to: Insufficient permissions") !=
            std::string::npos);
    }
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
