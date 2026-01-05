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

BOOST_AUTO_TEST_SUITE(T_Basic)

BOOST_AUTO_TEST_CASE(test_creating_host) {
    ECF_NAME_THIS_TEST();

    using namespace foolproof::scaffold;

    {
        auto host = MakeHost{}.create();
        BOOST_CHECK(host.is_valid());
        BOOST_CHECK(host.value() == Host::resolve_hostname());
    }

    {
        const std::string custom_name = "custom";
        auto host                     = MakeHost{}.with(custom_name).create();
        BOOST_CHECK(host.is_valid());
        BOOST_CHECK(host.value() == custom_name);
    }

    {
        BOOST_CHECK_EXCEPTION(
            [[maybe_unused]] auto host = MakeHost{}.with(std::string{}).create(),
            std::runtime_error,
            [](const std::runtime_error& e) { return std::string(e.what()) == "host name cannot be empty\n"; });
    }
}

BOOST_AUTO_TEST_CASE(test_locking_known_ports) {
    ECF_NAME_THIS_TEST();

    using namespace ecf;
    using namespace foolproof::scaffold;

    auto port_a = MakePort{}.with(SpecificPortValue{44444}).create();
    auto port_b = MakePort{}.with(SpecificPortValue{44445}).create();
    auto port_c = MakePort{}.with(SpecificPortValue{44446}).create();

    BOOST_TEST_MESSAGE("Acquired port: " << port_a.value() << " at " << port_a.lock_location());
    BOOST_TEST_MESSAGE("Acquired port: " << port_b.value() << " at " << port_b.lock_location());
    BOOST_TEST_MESSAGE("Acquired port: " << port_c.value() << " at " << port_c.lock_location());

    BOOST_CHECK(port_a.value() == 44444);
    BOOST_CHECK(port_b.value() == 44445);
    BOOST_CHECK(port_c.value() == 44446);
    BOOST_CHECK(fs::exists(port_a.lock_location()));
    BOOST_CHECK(fs::exists(port_b.lock_location()));
    BOOST_CHECK(fs::exists(port_c.lock_location()));
}

BOOST_AUTO_TEST_CASE(test_locking_automatic_ports) {
    ECF_NAME_THIS_TEST();

    using namespace ecf;
    using namespace foolproof::scaffold;

    auto port_a = MakePort{}.with(AutomaticPortValue{}).create();
    auto port_b = MakePort{}.with(AutomaticPortValue{44444}).create();
    auto port_c = MakePort{}.with(AutomaticPortValue{44444}).create();

    BOOST_TEST_MESSAGE("Acquired port: " << port_a.value() << " at " << port_a.lock_location());
    BOOST_TEST_MESSAGE("Acquired port: " << port_b.value() << " at " << port_b.lock_location());
    BOOST_TEST_MESSAGE("Acquired port: " << port_c.value() << " at " << port_c.lock_location());

    BOOST_CHECK(port_a.value() != port_b.value());
    BOOST_CHECK(port_a.value() != port_c.value());
    BOOST_CHECK(port_b.value() != port_c.value());
    BOOST_CHECK(fs::exists(port_a.lock_location()));
    BOOST_CHECK(fs::exists(port_b.lock_location()));
    BOOST_CHECK(fs::exists(port_c.lock_location()));
}

BOOST_AUTO_TEST_CASE(test_setting_environment_variables) {
    ECF_NAME_THIS_TEST();

    using namespace foolproof::scaffold;

    {
        std::string name     = "";
        std::string expected = "value";
        BOOST_CHECK_EXCEPTION([[maybe_unused]] auto variable = MakeEnvironmentVariable{}.with(name, expected).create(),
                              MakeEnvironmentVariable::InvalidVariableName,
                              [](const MakeEnvironmentVariable::InvalidVariableName& e) {
                                  return std::string(e.what()) == "Environment variable name cannot be empty";
                              });
    }

    std::string name = "FOOLPROOF_VARIABLE";
    {
        std::string expected = "";
        auto variable        = MakeEnvironmentVariable{}.with(name, expected).create();
        auto actual          = EnvironmentVariable::get_environment_variable(name);

        BOOST_CHECK(variable.value() == expected);
        BOOST_CHECK(actual.has_value() && (actual.value() == expected));
    }
    auto retrieved = EnvironmentVariable::get_environment_variable(name);
    BOOST_CHECK(!retrieved.has_value());

    {
        std::string expected = "value";
        auto variable        = MakeEnvironmentVariable{}.with(name, expected).create();
        auto actual          = EnvironmentVariable::get_environment_variable(name);

        BOOST_CHECK(variable.value() == expected);
        BOOST_CHECK(actual.has_value() && (actual.value() == expected));
    }
    retrieved = EnvironmentVariable::get_environment_variable(name);
    BOOST_CHECK(!retrieved.has_value());
}

BOOST_AUTO_TEST_CASE(test_creating_working_directory) {
    ECF_NAME_THIS_TEST();

    using namespace foolproof::scaffold;

    auto cwd = MakeDirectory{}.create();

    BOOST_CHECK(!cwd.path().string().empty());
    BOOST_CHECK(fs::exists(cwd.path()));
    BOOST_CHECK(fs::is_directory(cwd.path()));
    BOOST_CHECK(cwd.path().is_absolute());
}

BOOST_AUTO_TEST_CASE(test_e2e_basic_ping) {
    ECF_NAME_THIS_TEST();

    using namespace foolproof::scaffold;

    auto cwd = MakeDirectory{}.create();

    auto host = MakeHost{}.create();
    auto port = MakePort{}.with(AutomaticPortValue{}).create();

    const auto server = MakeServer{}.with(host).with(port).with(cwd).launch();
    {
        BOOST_REQUIRE(server.ok());
        auto& s = server.value();
        BOOST_CHECK(s.pid() > 0);
        BOOST_CHECK(s.port().value() == port.value());
        BOOST_CHECK(s.host().is_valid());
    }

    {
        auto client = RunClient{}.with(host).with(port).with(cwd).execute(RunClient::CommandPing{});
        BOOST_CHECK(client.ok());
        auto& c = client.value();

        ECF_TEST_DBG("Output of --ping:\n" << c.stdout_buffer);
    }

    // Notice the automatic cleaned up when exiting scope!
    // . shutdown server
    // . remove port lock file
    // . remove working directory
}

BOOST_AUTO_TEST_CASE(test_e2e_basic_load) {
    ECF_NAME_THIS_TEST();

    using namespace foolproof::scaffold;

    auto cwd = MakeDirectory{}.create();

    auto host = MakeHost{}.create();
    auto port = MakePort{}.with(AutomaticPortValue{}).create();

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
                    .with(
                        R"--(
suite s
  family f
    task task
  endfamily
endsuite;
)--")
                    .create();

    {
        auto client = RunClient{}.with(host).with(port).with(cwd).execute(RunClient::CommandLoad{defs.path()});
        BOOST_REQUIRE(client.ok());
        auto& c = client.value();

        ECF_TEST_DBG("Output of --load:\n" << c.stdout_buffer);
    }

    {
        auto client = RunClient{}.with(host).with(port).with(cwd).execute(RunClient::CommandGet{});
        BOOST_REQUIRE(client.ok());
        auto& c = client.value();
        BOOST_CHECK(c.stdout_contains("suite s"));
        BOOST_CHECK(c.stdout_contains("family f"));
        BOOST_CHECK(c.stdout_contains("task task"));

        ECF_TEST_DBG("Output of --get:\n" << c.stdout_buffer);
    }

    {
        auto client = RunClient{}.with(host).with(port).with(cwd).execute(RunClient::CommandGetState{});
        BOOST_REQUIRE(client.ok());
        auto& c = client.value();
        BOOST_CHECK(c.stdout_contains("suite s"));
        BOOST_CHECK(c.stdout_contains("family f"));
        BOOST_CHECK(c.stdout_contains("task task"));

        ECF_TEST_DBG("Output of --get_state:\n" << c.stdout_buffer);
    }
}

BOOST_AUTO_TEST_CASE(test_e2e_basic_label_update_with_passwds) {
    ECF_NAME_THIS_TEST();

    using namespace foolproof::scaffold;

    auto cwd = MakeDirectory{}.create();

    auto user_a = User{"alice", "somesecret"};
    auto user_b = User{"bob", "anothersecret"};
    auto user_c = User{"charlie", "yetanothersecret"};

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
}

BOOST_AUTO_TEST_CASE(test_e2e_basic_label_update_with_whitelist) {
    ECF_NAME_THIS_TEST();

    using namespace foolproof::scaffold;

    auto cwd = MakeDirectory{}.create();

    auto user_a = User{"alice", "somesecret", "rw"};
    auto user_b = User{"bob", "anothersecret", "r"};
    auto user_c = User{"charlie", "yetanothersecret", "r"};
    auto user_d = User{"david", "topsecret", "rw"};

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

BOOST_AUTO_TEST_CASE(test_e2e_basic_reload_whitelist) {
    ECF_NAME_THIS_TEST();

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

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
