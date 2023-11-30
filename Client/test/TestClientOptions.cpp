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

#include "ClientEnvironment.hpp"
#include "ClientInvoker.hpp"
#include "ClientOptions.hpp"
#include "ClientToServerCmd.hpp"
#include "ecflow/core/CommandLine.hpp"
#include "ecflow/core/PasswordEncryption.hpp"

///
/// \brief Tests the capabilities of ClientOptions
///

template <typename REQUIRE>
void test_alter(const CommandLine& cl, REQUIRE check) {
    std::cout << "Testing command line: " << cl.original() << std::endl;

    ClientOptions options;
    ClientEnvironment environment(false);
    try {
        auto base_command    = options.parse(cl, &environment);
        auto derived_command = dynamic_cast<AlterCmd*>(base_command.get());

        BOOST_REQUIRE(derived_command);
        check(*derived_command, environment);
    }
    catch (boost::program_options::unknown_option& e) {
        BOOST_FAIL(std::string("Unexpected exception caught: ") + e.what());
    }
}

BOOST_AUTO_TEST_SUITE(ClientTestSuite)

BOOST_AUTO_TEST_CASE(test_is_able_to_process_username_and_password) {
    const char* expected_username = "username";
    const char* plain_password    = "password";
    std::string expected_password = PasswordEncryption::encrypt(plain_password, expected_username);

    // Make command line
    auto cl = CommandLine::make_command_line(
        "ecflow_client", "--user", expected_username, "--password", plain_password, "--ping");

    ClientOptions options;
    ClientEnvironment environment(false);
    options.parse(cl, &environment);

    std::string actual_username = environment.get_user_name();
    BOOST_REQUIRE(expected_username == actual_username);

    std::string actual_password = environment.get_user_password(expected_username);
    BOOST_REQUIRE(expected_password == actual_password);
}

BOOST_AUTO_TEST_CASE(test_is_able_handle_alter_change) {

    /*
     * --alter change <type> <name> <value> <path> [<path>...]
     * ************************************************************************************************************** */

    std::vector<std::vector<std::string>> paths_set = {
        {"/node1"}, {"/node1", "/node2"}, {"/node1", "/node2", "/node3"}};

    for (const auto& paths : paths_set) {

        using Expected = AlterCmd::Change_attr_type;

        /*
         * --alter change variable <name> <value> <path> [<path>...]
         * ---------------------------------------------------------------------------------------------------------- */

        std::vector<std::string> variable_values = {"",
                                                    "--dashes at beginning of value",
                                                    "a value with --dashes inside",
                                                    "    value starting with spaces",
                                                    "value ending with spaces      ",
                                                    "   some value surrounded by spaces      ",
                                                    "/some/valid/path"};

        for (const auto& value : variable_values) {
            {
                auto cl = CommandLine::make_command_line(
                    "ecflow_client", "--alter", "change", "variable", "name", value, paths);
                test_alter(cl, [&](const AlterCmd& command, const ClientEnvironment& env) {
                    BOOST_REQUIRE_EQUAL(command.name(), "name");
                    BOOST_REQUIRE_EQUAL(command.value(), value);
                    BOOST_REQUIRE_EQUAL(command.change_attr_type(), Expected::VARIABLE);
                    BOOST_REQUIRE_EQUAL(command.paths().size(), paths.size());
                    for (size_t i = 0; i < paths.size(); ++i) {
                        BOOST_REQUIRE(command.paths()[i] == paths[i]);
                    }
                    BOOST_REQUIRE_EQUAL(env.host(), "localhost");
                    BOOST_REQUIRE_EQUAL(env.port(), "3141");
                    BOOST_REQUIRE_EQUAL(env.debug(), false);
                });
            }
            {
                auto cl = CommandLine::make_command_line(
                    "ecflow_client", "--alter", "change", "variable", "name", value, paths, "--debug");
                test_alter(cl, [&](const AlterCmd& command, const ClientEnvironment& env) {
                    BOOST_REQUIRE_EQUAL(command.name(), "name");
                    BOOST_REQUIRE_EQUAL(command.value(), value);
                    BOOST_REQUIRE_EQUAL(command.change_attr_type(), Expected::VARIABLE);
                    BOOST_REQUIRE_EQUAL(command.paths().size(), paths.size());
                    for (size_t i = 0; i < paths.size(); ++i) {
                        BOOST_REQUIRE(command.paths()[i] == paths[i]);
                    }
                    BOOST_REQUIRE_EQUAL(env.host(), "localhost");
                    BOOST_REQUIRE_EQUAL(env.port(), "3141");
                    BOOST_REQUIRE_EQUAL(env.debug(), true);
                });
            }
        }

        /*
         * --alter change label <name> <value> <path> [<path>...]
         * ---------------------------------------------------------------------------------------------------------- */
        std::vector<std::string> label_values = {"",
                                                 "--dashes at beginning of value",
                                                 "a value with --dashes inside",
                                                 "    value starting with spaces",
                                                 "value ending with spaces      ",
                                                 "   some value surrounded by spaces      ",
                                                 "/some/valid/path"};

        for (const auto& value : label_values) {
            {
                auto cl =
                    CommandLine::make_command_line("ecflow_client", "--alter", "change", "label", "name", value, paths);
                test_alter(cl, [&](const AlterCmd& command, const ClientEnvironment& env) {
                    BOOST_REQUIRE_EQUAL(command.name(), "name");
                    BOOST_REQUIRE_EQUAL(command.value(), value);
                    BOOST_REQUIRE_EQUAL(command.change_attr_type(), Expected::LABEL);
                    BOOST_REQUIRE_EQUAL(command.paths().size(), paths.size());
                    for (size_t i = 0; i < paths.size(); ++i) {
                        BOOST_REQUIRE(command.paths()[i] == paths[i]);
                    }
                    BOOST_REQUIRE_EQUAL(env.host(), "localhost");
                    BOOST_REQUIRE_EQUAL(env.port(), "3141");
                    BOOST_REQUIRE_EQUAL(env.debug(), false);
                });
            }
            {
                auto cl = CommandLine::make_command_line(
                    "ecflow_client", "--alter", "change", "label", "name", value, paths, "--debug");
                test_alter(cl, [&](const AlterCmd& command, const ClientEnvironment& env) {
                    BOOST_REQUIRE_EQUAL(command.name(), "name");
                    BOOST_REQUIRE_EQUAL(command.value(), value);
                    BOOST_REQUIRE_EQUAL(command.change_attr_type(), Expected::LABEL);
                    BOOST_REQUIRE_EQUAL(command.paths().size(), paths.size());
                    for (size_t i = 0; i < paths.size(); ++i) {
                        BOOST_REQUIRE(command.paths()[i] == paths[i]);
                    }
                    BOOST_REQUIRE_EQUAL(env.host(), "localhost");
                    BOOST_REQUIRE_EQUAL(env.port(), "3141");
                    BOOST_REQUIRE_EQUAL(env.debug(), true);
                });
            }
        }

        /*
         * --alter change event <name> <value> <path> [<path>...]
         * ---------------------------------------------------------------------------------------------------------- */
        // No value provided
        {
            auto cl = CommandLine::make_command_line("ecflow_client", "--alter", "change", "event", "name", paths);
            test_alter(cl, [&](const AlterCmd& command, const ClientEnvironment& env) {
                BOOST_REQUIRE_EQUAL(command.name(), "name");
                BOOST_REQUIRE_EQUAL(command.value(), "");
                BOOST_REQUIRE_EQUAL(command.change_attr_type(), Expected::EVENT);
                BOOST_REQUIRE_EQUAL(command.paths().size(), paths.size());
                for (size_t i = 0; i < paths.size(); ++i) {
                    BOOST_REQUIRE(command.paths()[i] == paths[i]);
                }
                BOOST_REQUIRE_EQUAL(env.host(), "localhost");
                BOOST_REQUIRE_EQUAL(env.port(), "3141");
                BOOST_REQUIRE_EQUAL(env.debug(), false);
            });
        }
        {
            auto cl =
                CommandLine::make_command_line("ecflow_client", "--alter", "change", "event", "name", paths, "--debug");
            test_alter(cl, [&](const AlterCmd& command, const ClientEnvironment& env) {
                BOOST_REQUIRE_EQUAL(command.name(), "name");
                BOOST_REQUIRE_EQUAL(command.value(), "");
                BOOST_REQUIRE_EQUAL(command.change_attr_type(), Expected::EVENT);
                BOOST_REQUIRE_EQUAL(command.paths().size(), paths.size());
                for (size_t i = 0; i < paths.size(); ++i) {
                    BOOST_REQUIRE(command.paths()[i] == paths[i]);
                }
                BOOST_REQUIRE_EQUAL(env.host(), "localhost");
                BOOST_REQUIRE_EQUAL(env.port(), "3141");
                BOOST_REQUIRE_EQUAL(env.debug(), true);
            });
        }
        // Explicitly provided value
        std::vector<std::string> event_values = {"set", "clear"};
        for (const auto& value : event_values) {
            {
                auto cl =
                    CommandLine::make_command_line("ecflow_client", "--alter", "change", "event", "name", value, paths);
                test_alter(cl, [&](const AlterCmd& command, const ClientEnvironment& env) {
                    BOOST_REQUIRE_EQUAL(command.name(), "name");
                    BOOST_REQUIRE_EQUAL(command.value(), value);
                    BOOST_REQUIRE_EQUAL(command.change_attr_type(), Expected::EVENT);
                    BOOST_REQUIRE_EQUAL(command.paths().size(), paths.size());
                    for (size_t i = 0; i < paths.size(); ++i) {
                        BOOST_REQUIRE(command.paths()[i] == paths[i]);
                    }
                    BOOST_REQUIRE_EQUAL(env.host(), "localhost");
                    BOOST_REQUIRE_EQUAL(env.port(), "3141");
                    BOOST_REQUIRE_EQUAL(env.debug(), false);
                });
            }
            {
                auto cl = CommandLine::make_command_line(
                    "ecflow_client", "--alter", "change", "event", "name", value, paths, "--debug");
                test_alter(cl, [&](const AlterCmd& command, const ClientEnvironment& env) {
                    BOOST_REQUIRE_EQUAL(command.name(), "name");
                    BOOST_REQUIRE_EQUAL(command.value(), value);
                    BOOST_REQUIRE_EQUAL(command.change_attr_type(), Expected::EVENT);
                    BOOST_REQUIRE_EQUAL(command.paths().size(), paths.size());
                    for (size_t i = 0; i < paths.size(); ++i) {
                        BOOST_REQUIRE(command.paths()[i] == paths[i]);
                    }
                    BOOST_REQUIRE_EQUAL(env.host(), "localhost");
                    BOOST_REQUIRE_EQUAL(env.port(), "3141");
                    BOOST_REQUIRE_EQUAL(env.debug(), true);
                });
            }
        }

        /*
         * --alter change meter <name> <value> <path> [<path>...]
         * ---------------------------------------------------------------------------------------------------------- */
        std::vector<std::string> meter_values = {"0", "100"};
        for (const auto& value : meter_values) {
            {
                auto cl =
                    CommandLine::make_command_line("ecflow_client", "--alter", "change", "meter", "name", value, paths);
                test_alter(cl, [&](const AlterCmd& command, const ClientEnvironment& env) {
                    BOOST_REQUIRE_EQUAL(command.name(), "name");
                    BOOST_REQUIRE_EQUAL(command.value(), value);
                    BOOST_REQUIRE_EQUAL(command.change_attr_type(), Expected::METER);
                    BOOST_REQUIRE_EQUAL(command.paths().size(), paths.size());
                    for (size_t i = 0; i < paths.size(); ++i) {
                        BOOST_REQUIRE(command.paths()[i] == paths[i]);
                    }
                    BOOST_REQUIRE_EQUAL(env.host(), "localhost");
                    BOOST_REQUIRE_EQUAL(env.port(), "3141");
                    BOOST_REQUIRE_EQUAL(env.debug(), false);
                });
            }
            {
                auto cl = CommandLine::make_command_line(
                    "ecflow_client", "--alter", "change", "meter", "name", value, paths, "--debug");
                test_alter(cl, [&](const AlterCmd& command, const ClientEnvironment& env) {
                    BOOST_REQUIRE_EQUAL(command.name(), "name");
                    BOOST_REQUIRE_EQUAL(command.value(), value);
                    BOOST_REQUIRE_EQUAL(command.change_attr_type(), Expected::METER);
                    BOOST_REQUIRE_EQUAL(command.paths().size(), paths.size());
                    for (size_t i = 0; i < paths.size(); ++i) {
                        BOOST_REQUIRE(command.paths()[i] == paths[i]);
                    }
                    BOOST_REQUIRE_EQUAL(env.host(), "localhost");
                    BOOST_REQUIRE_EQUAL(env.port(), "3141");
                    BOOST_REQUIRE_EQUAL(env.debug(), true);
                });
            }
        }

        /*
         * --alter change clock_type ( hybrid | real ) <path> [<path>...]
         * ---------------------------------------------------------------------------------------------------------- */
        std::vector<std::string> clock_type_values = {"hybrid", "real"};
        for (const auto& value : clock_type_values) {
            {
                auto cl =
                    CommandLine::make_command_line("ecflow_client", "--alter", "change", "clock_type", value, paths);
                test_alter(cl, [&](const AlterCmd& command, const ClientEnvironment& env) {
                    BOOST_REQUIRE_EQUAL(command.name(), value);
                    BOOST_REQUIRE_EQUAL(command.value(), "");
                    BOOST_REQUIRE_EQUAL(command.change_attr_type(), Expected::CLOCK_TYPE);
                    BOOST_REQUIRE_EQUAL(command.paths().size(), paths.size());
                    for (size_t i = 0; i < paths.size(); ++i) {
                        BOOST_REQUIRE(command.paths()[i] == paths[i]);
                    }
                    BOOST_REQUIRE_EQUAL(env.host(), "localhost");
                    BOOST_REQUIRE_EQUAL(env.port(), "3141");
                    BOOST_REQUIRE_EQUAL(env.debug(), false);
                });
            }
            {
                auto cl = CommandLine::make_command_line(
                    "ecflow_client", "--alter", "change", "clock_type", value, paths, "--port", "1234");
                test_alter(cl, [&](const AlterCmd& command, const ClientEnvironment& env) {
                    BOOST_REQUIRE_EQUAL(command.name(), value);
                    BOOST_REQUIRE_EQUAL(command.value(), "");
                    BOOST_REQUIRE_EQUAL(command.change_attr_type(), Expected::CLOCK_TYPE);
                    BOOST_REQUIRE_EQUAL(command.paths().size(), paths.size());
                    for (size_t i = 0; i < paths.size(); ++i) {
                        BOOST_REQUIRE(command.paths()[i] == paths[i]);
                    }
                    BOOST_REQUIRE_EQUAL(env.host(), "localhost");
                    BOOST_REQUIRE_EQUAL(env.port(), "1234");
                    BOOST_REQUIRE_EQUAL(env.debug(), false);
                });
            }
        }

        /*
         * --alter change clock_gain <path> [<path>...]
         * ---------------------------------------------------------------------------------------------------------- */
        {
            auto cl =
                CommandLine::make_command_line("ecflow_client", "--alter", "change", "clock_gain", "2023.01.01", paths);
            test_alter(cl, [&](const AlterCmd& command, const ClientEnvironment& env) {
                BOOST_REQUIRE_EQUAL(command.name(), "2023.01.01");
                BOOST_REQUIRE_EQUAL(command.value(), "");
                BOOST_REQUIRE_EQUAL(command.change_attr_type(), Expected::CLOCK_GAIN);
                BOOST_REQUIRE_EQUAL(command.paths().size(), paths.size());
                for (size_t i = 0; i < paths.size(); ++i) {
                    BOOST_REQUIRE(command.paths()[i] == paths[i]);
                }
                BOOST_REQUIRE_EQUAL(env.host(), "localhost");
                BOOST_REQUIRE_EQUAL(env.port(), "3141");
                BOOST_REQUIRE_EQUAL(env.debug(), false);
            });
        }
        {
            auto cl = CommandLine::make_command_line(
                "ecflow_client", "--alter", "change", "clock_gain", "2023.01.01", paths, "--debug");
            test_alter(cl, [&](const AlterCmd& command, const ClientEnvironment& env) {
                BOOST_REQUIRE_EQUAL(command.name(), "2023.01.01");
                BOOST_REQUIRE_EQUAL(command.value(), "");
                BOOST_REQUIRE_EQUAL(command.change_attr_type(), Expected::CLOCK_GAIN);
                BOOST_REQUIRE_EQUAL(command.paths().size(), paths.size());
                for (size_t i = 0; i < paths.size(); ++i) {
                    BOOST_REQUIRE(command.paths()[i] == paths[i]);
                }
                BOOST_REQUIRE_EQUAL(env.host(), "localhost");
                BOOST_REQUIRE_EQUAL(env.port(), "3141");
                BOOST_REQUIRE_EQUAL(env.debug(), true);
            });
        }

        /*
         * --alter change clock_sync <path> [<path>...]
         * ---------------------------------------------------------------------------------------------------------- */
        {
            auto cl = CommandLine::make_command_line("ecflow_client", "--alter", "change", "clock_sync", paths);
            test_alter(cl, [&](const AlterCmd& command, const ClientEnvironment& env) {
                BOOST_REQUIRE_EQUAL(command.name(), "");
                BOOST_REQUIRE_EQUAL(command.value(), "");
                BOOST_REQUIRE_EQUAL(command.change_attr_type(), Expected::CLOCK_SYNC);
                BOOST_REQUIRE_EQUAL(command.paths().size(), paths.size());
                for (size_t i = 0; i < paths.size(); ++i) {
                    BOOST_REQUIRE(command.paths()[i] == paths[i]);
                }
                BOOST_REQUIRE_EQUAL(env.host(), "localhost");
                BOOST_REQUIRE_EQUAL(env.port(), "3141");
                BOOST_REQUIRE_EQUAL(env.debug(), false);
            });
        }
        {
            auto cl =
                CommandLine::make_command_line("ecflow_client", "--alter", "change", "clock_sync", paths, "--debug");
            test_alter(cl, [&](const AlterCmd& command, const ClientEnvironment& env) {
                BOOST_REQUIRE_EQUAL(command.name(), "");
                BOOST_REQUIRE_EQUAL(command.value(), "");
                BOOST_REQUIRE_EQUAL(command.change_attr_type(), Expected::CLOCK_SYNC);
                BOOST_REQUIRE_EQUAL(command.paths().size(), paths.size());
                for (size_t i = 0; i < paths.size(); ++i) {
                    BOOST_REQUIRE(command.paths()[i] == paths[i]);
                }
                BOOST_REQUIRE_EQUAL(env.host(), "localhost");
                BOOST_REQUIRE_EQUAL(env.port(), "3141");
                BOOST_REQUIRE_EQUAL(env.debug(), true);
            });
        }

        /*
         * --alter change late <value>  <path> [<path>...]
         * ---------------------------------------------------------------------------------------------------------- */
        std::vector<std::string> late_values = {"late -s +00:15  -a  20:00  -c +02:00"};
        for (const auto& value : late_values) {
            {
                auto cl = CommandLine::make_command_line("ecflow_client", "--alter", "change", "late", value, paths);
                test_alter(cl, [&](const AlterCmd& command, const ClientEnvironment& env) {
                    BOOST_REQUIRE_EQUAL(command.name(), value);
                    BOOST_REQUIRE_EQUAL(command.value(), "");
                    BOOST_REQUIRE_EQUAL(command.change_attr_type(), Expected::LATE);
                    BOOST_REQUIRE_EQUAL(command.paths().size(), paths.size());
                    for (size_t i = 0; i < paths.size(); ++i) {
                        BOOST_REQUIRE(command.paths()[i] == paths[i]);
                    }
                    BOOST_REQUIRE_EQUAL(env.host(), "localhost");
                    BOOST_REQUIRE_EQUAL(env.port(), "3141");
                    BOOST_REQUIRE_EQUAL(env.debug(), false);
                });
            }
            {
                auto cl = CommandLine::make_command_line(
                    "ecflow_client", "--alter", "change", "late", value, paths, "--debug");
                test_alter(cl, [&](const AlterCmd& command, const ClientEnvironment& env) {
                    BOOST_REQUIRE_EQUAL(command.name(), value);
                    BOOST_REQUIRE_EQUAL(command.value(), "");
                    BOOST_REQUIRE_EQUAL(command.change_attr_type(), Expected::LATE);
                    BOOST_REQUIRE_EQUAL(command.paths().size(), paths.size());
                    for (size_t i = 0; i < paths.size(); ++i) {
                        BOOST_REQUIRE(command.paths()[i] == paths[i]);
                    }
                    BOOST_REQUIRE_EQUAL(env.host(), "localhost");
                    BOOST_REQUIRE_EQUAL(env.port(), "3141");
                    BOOST_REQUIRE_EQUAL(env.debug(), true);
                });
            }
        }

    } // paths
}

BOOST_AUTO_TEST_CASE(test_is_able_handle_alter_add) {

    /*
     * --alter add <type> <name> <value> <path> [<path>...]
     * ************************************************************************************************************** */

    std::vector<std::vector<std::string>> paths_set = {
        {"/node1"}, {"/node1", "/node2"}, {"/node1", "/node2", "/node3"}};

    for (const auto& paths : paths_set) {

        using Expected                           = AlterCmd::Add_attr_type;

        std::vector<std::string> variable_values = {"",
                                                    "--dashes at beginning of value",
                                                    "a value with --dashes inside",
                                                    "    value starting with spaces",
                                                    "value ending with spaces      ",
                                                    "   some value surrounded by spaces      ",
                                                    "/some/valid/path"};

        /*
         * --alter add variable <path> [<path>...]
         * ---------------------------------------------------------------------------------------------------------- */
        for (const auto& value : variable_values) {
            {
                auto cl =
                    CommandLine::make_command_line("ecflow_client", "--alter", "add", "variable", "name", value, paths);
                test_alter(cl, [&](const AlterCmd& command, const ClientEnvironment& env) {
                    BOOST_REQUIRE_EQUAL(command.name(), "name");
                    BOOST_REQUIRE_EQUAL(command.value(), value);
                    BOOST_REQUIRE_EQUAL(command.add_attr_type(), Expected::ADD_VARIABLE);
                    BOOST_REQUIRE_EQUAL(command.paths().size(), paths.size());
                    for (size_t i = 0; i < paths.size(); ++i) {
                        BOOST_REQUIRE(command.paths()[i] == paths[i]);
                    }
                    BOOST_REQUIRE_EQUAL(env.host(), "localhost");
                    BOOST_REQUIRE_EQUAL(env.port(), "3141");
                    BOOST_REQUIRE_EQUAL(env.debug(), false);
                });
            }
            {
                auto cl = CommandLine::make_command_line(
                    "ecflow_client", "--alter", "add", "variable", "name", value, paths, "--debug");
                test_alter(cl, [&](const AlterCmd& command, const ClientEnvironment& env) {
                    BOOST_REQUIRE_EQUAL(command.name(), "name");
                    BOOST_REQUIRE_EQUAL(command.value(), value);
                    BOOST_REQUIRE_EQUAL(command.add_attr_type(), Expected::ADD_VARIABLE);
                    BOOST_REQUIRE_EQUAL(command.paths().size(), paths.size());
                    for (size_t i = 0; i < paths.size(); ++i) {
                        BOOST_REQUIRE(command.paths()[i] == paths[i]);
                    }
                    BOOST_REQUIRE_EQUAL(env.host(), "localhost");
                    BOOST_REQUIRE_EQUAL(env.port(), "3141");
                    BOOST_REQUIRE_EQUAL(env.debug(), true);
                });
            }
        }
    } // paths
}

BOOST_AUTO_TEST_CASE(test_is_able_handle_alter_delete) {
    /*
     * --alter delete <type> <name> <path> [<path>...]
     * ************************************************************************************************************** */

    std::vector<std::vector<std::string>> paths_set = {
        {"/node1"}, {"/node1", "/node2"}, {"/node1", "/node2", "/node3"}};

    for (const auto& paths : paths_set) {

        using Expected = AlterCmd::Delete_attr_type;

        /*
         * --alter delete <type> <name> <path> [<path>...]
         * ---------------------------------------------------------------------------------------------------------- */
        // No explicit name, means delete all variables
        {
            auto cl =
                CommandLine::make_command_line("ecflow_client", "--alter", "delete", "variable", paths, "--debug");
            test_alter(cl, [&](const AlterCmd& command, const ClientEnvironment& env) {
                BOOST_REQUIRE_EQUAL(command.name(), "");
                BOOST_REQUIRE_EQUAL(command.value(), "");
                BOOST_REQUIRE_EQUAL(command.delete_attr_type(), Expected::DEL_VARIABLE);
                BOOST_REQUIRE_EQUAL(command.paths().size(), paths.size());
                for (size_t i = 0; i < paths.size(); ++i) {
                    BOOST_REQUIRE(command.paths()[i] == paths[i]);
                }
                BOOST_REQUIRE_EQUAL(env.host(), "localhost");
                BOOST_REQUIRE_EQUAL(env.port(), "3141");
                BOOST_REQUIRE_EQUAL(env.debug(), true);
            });
        }
        {
            auto cl = CommandLine::make_command_line("ecflow_client", "--alter", "delete", "variable", "name", paths);
            test_alter(cl, [&](const AlterCmd& command, const ClientEnvironment& env) {
                BOOST_REQUIRE_EQUAL(command.name(), "name");
                BOOST_REQUIRE_EQUAL(command.value(), "");
                BOOST_REQUIRE_EQUAL(command.delete_attr_type(), Expected::DEL_VARIABLE);
                BOOST_REQUIRE_EQUAL(command.paths().size(), paths.size());
                for (size_t i = 0; i < paths.size(); ++i) {
                    BOOST_REQUIRE(command.paths()[i] == paths[i]);
                }
                BOOST_REQUIRE_EQUAL(env.host(), "localhost");
                BOOST_REQUIRE_EQUAL(env.port(), "3141");
                BOOST_REQUIRE_EQUAL(env.debug(), false);
            });
        }
        {
            auto cl = CommandLine::make_command_line(
                "ecflow_client", "--alter", "delete", "variable", "name", paths, "--debug");
            test_alter(cl, [&](const AlterCmd& command, const ClientEnvironment& env) {
                BOOST_REQUIRE_EQUAL(command.name(), "name");
                BOOST_REQUIRE_EQUAL(command.value(), "");
                BOOST_REQUIRE_EQUAL(command.delete_attr_type(), Expected::DEL_VARIABLE);
                BOOST_REQUIRE_EQUAL(command.paths().size(), paths.size());
                for (size_t i = 0; i < paths.size(); ++i) {
                    BOOST_REQUIRE(command.paths()[i] == paths[i]);
                }
                BOOST_REQUIRE_EQUAL(env.host(), "localhost");
                BOOST_REQUIRE_EQUAL(env.port(), "3141");
                BOOST_REQUIRE_EQUAL(env.debug(), true);
            });
        }

        /*
         * --alter delete label <name> <path> [<path>...]
         * ---------------------------------------------------------------------------------------------------------- */
        // No explicit name, means delete all labels
        {
            auto cl = CommandLine::make_command_line("ecflow_client", "--alter", "delete", "label", paths);
            test_alter(cl, [&](const AlterCmd& command, const ClientEnvironment& env) {
                BOOST_REQUIRE_EQUAL(command.name(), "");
                BOOST_REQUIRE_EQUAL(command.value(), "");
                BOOST_REQUIRE_EQUAL(command.delete_attr_type(), Expected::DEL_LABEL);
                BOOST_REQUIRE_EQUAL(command.paths().size(), paths.size());
                for (size_t i = 0; i < paths.size(); ++i) {
                    BOOST_REQUIRE(command.paths()[i] == paths[i]);
                }
                BOOST_REQUIRE_EQUAL(env.host(), "localhost");
                BOOST_REQUIRE_EQUAL(env.port(), "3141");
                BOOST_REQUIRE_EQUAL(env.debug(), false);
            });
        }
        {
            auto cl = CommandLine::make_command_line("ecflow_client", "--alter", "delete", "label", "name", paths);
            test_alter(cl, [&](const AlterCmd& command, const ClientEnvironment& env) {
                BOOST_REQUIRE_EQUAL(command.name(), "name");
                BOOST_REQUIRE_EQUAL(command.value(), "");
                BOOST_REQUIRE_EQUAL(command.delete_attr_type(), Expected::DEL_LABEL);
                BOOST_REQUIRE_EQUAL(command.paths().size(), paths.size());
                for (size_t i = 0; i < paths.size(); ++i) {
                    BOOST_REQUIRE(command.paths()[i] == paths[i]);
                }
                BOOST_REQUIRE_EQUAL(env.host(), "localhost");
                BOOST_REQUIRE_EQUAL(env.port(), "3141");
                BOOST_REQUIRE_EQUAL(env.debug(), false);
            });
        }
        {
            auto cl =
                CommandLine::make_command_line("ecflow_client", "--alter", "delete", "label", "name", paths, "--debug");
            test_alter(cl, [&](const AlterCmd& command, const ClientEnvironment& env) {
                BOOST_REQUIRE_EQUAL(command.name(), "name");
                BOOST_REQUIRE_EQUAL(command.value(), "");
                BOOST_REQUIRE_EQUAL(command.delete_attr_type(), Expected::DEL_LABEL);
                BOOST_REQUIRE_EQUAL(command.paths().size(), paths.size());
                for (size_t i = 0; i < paths.size(); ++i) {
                    BOOST_REQUIRE(command.paths()[i] == paths[i]);
                }
                BOOST_REQUIRE_EQUAL(env.host(), "localhost");
                BOOST_REQUIRE_EQUAL(env.port(), "3141");
                BOOST_REQUIRE_EQUAL(env.debug(), true);
            });
        }

        /*
         * --alter delete event <name> <path> [<path>...]
         * ---------------------------------------------------------------------------------------------------------- */
        // No explicit name, means delete all events
        {
            auto cl = CommandLine::make_command_line("ecflow_client", "--alter", "delete", "event", paths);
            test_alter(cl, [&](const AlterCmd& command, const ClientEnvironment& env) {
                BOOST_REQUIRE_EQUAL(command.name(), "");
                BOOST_REQUIRE_EQUAL(command.value(), "");
                BOOST_REQUIRE_EQUAL(command.delete_attr_type(), Expected::DEL_EVENT);
                BOOST_REQUIRE_EQUAL(command.paths().size(), paths.size());
                for (size_t i = 0; i < paths.size(); ++i) {
                    BOOST_REQUIRE(command.paths()[i] == paths[i]);
                }
                BOOST_REQUIRE_EQUAL(env.host(), "localhost");
                BOOST_REQUIRE_EQUAL(env.port(), "3141");
                BOOST_REQUIRE_EQUAL(env.debug(), false);
            });
        }
        {
            auto cl = CommandLine::make_command_line("ecflow_client", "--alter", "delete", "event", "name", paths);
            test_alter(cl, [&](const AlterCmd& command, const ClientEnvironment& env) {
                BOOST_REQUIRE_EQUAL(command.name(), "name");
                BOOST_REQUIRE_EQUAL(command.value(), "");
                BOOST_REQUIRE_EQUAL(command.delete_attr_type(), Expected::DEL_EVENT);
                BOOST_REQUIRE_EQUAL(command.paths().size(), paths.size());
                for (size_t i = 0; i < paths.size(); ++i) {
                    BOOST_REQUIRE(command.paths()[i] == paths[i]);
                }
                BOOST_REQUIRE_EQUAL(env.host(), "localhost");
                BOOST_REQUIRE_EQUAL(env.port(), "3141");
                BOOST_REQUIRE_EQUAL(env.debug(), false);
            });
        }
        {
            auto cl =
                CommandLine::make_command_line("ecflow_client", "--alter", "delete", "event", "name", paths, "--debug");
            test_alter(cl, [&](const AlterCmd& command, const ClientEnvironment& env) {
                BOOST_REQUIRE_EQUAL(command.name(), "name");
                BOOST_REQUIRE_EQUAL(command.value(), "");
                BOOST_REQUIRE_EQUAL(command.delete_attr_type(), Expected::DEL_EVENT);
                BOOST_REQUIRE_EQUAL(command.paths().size(), paths.size());
                for (size_t i = 0; i < paths.size(); ++i) {
                    BOOST_REQUIRE(command.paths()[i] == paths[i]);
                }
                BOOST_REQUIRE_EQUAL(env.host(), "localhost");
                BOOST_REQUIRE_EQUAL(env.port(), "3141");
                BOOST_REQUIRE_EQUAL(env.debug(), true);
            });
        }
        {
            auto cl = CommandLine::make_command_line("ecflow_client",
                                                     "--host=default",
                                                     "--port",
                                                     "8888",
                                                     "--alter",
                                                     "delete",
                                                     "event",
                                                     "name",
                                                     paths,
                                                     "--debug");
            test_alter(cl, [&](const AlterCmd& command, const ClientEnvironment& env) {
                BOOST_REQUIRE_EQUAL(command.name(), "name");
                BOOST_REQUIRE_EQUAL(command.value(), "");
                BOOST_REQUIRE_EQUAL(command.delete_attr_type(), Expected::DEL_EVENT);
                BOOST_REQUIRE_EQUAL(command.paths().size(), paths.size());
                for (size_t i = 0; i < paths.size(); ++i) {
                    BOOST_REQUIRE(command.paths()[i] == paths[i]);
                }
                BOOST_REQUIRE_EQUAL(env.host(), "default");
                BOOST_REQUIRE_EQUAL(env.port(), "8888");
                BOOST_REQUIRE_EQUAL(env.debug(), true);
            });
        }
        {
            auto cl = CommandLine::make_command_line("ecflow_client",
                                                     "--alter",
                                                     "delete",
                                                     "event",
                                                     "name",
                                                     paths,
                                                     "--host",
                                                     "default",
                                                     "--debug",
                                                     "--port",
                                                     "8888");
            test_alter(cl, [&](const AlterCmd& command, const ClientEnvironment& env) {
                BOOST_REQUIRE_EQUAL(command.name(), "name");
                BOOST_REQUIRE_EQUAL(command.value(), "");
                BOOST_REQUIRE_EQUAL(command.delete_attr_type(), Expected::DEL_EVENT);
                BOOST_REQUIRE_EQUAL(command.paths().size(), paths.size());
                for (size_t i = 0; i < paths.size(); ++i) {
                    BOOST_REQUIRE(command.paths()[i] == paths[i]);
                }
                BOOST_REQUIRE_EQUAL(env.host(), "default");
                BOOST_REQUIRE_EQUAL(env.port(), "8888");
                BOOST_REQUIRE_EQUAL(env.debug(), true);
            });
        }
        {
            auto cl = CommandLine::make_command_line("ecflow_client", "--alter", "delete", "event", paths, "--debug");
            test_alter(cl, [&](const AlterCmd& command, const ClientEnvironment& env) {
                BOOST_REQUIRE_EQUAL(command.name(), "");
                BOOST_REQUIRE_EQUAL(command.value(), "");
                BOOST_REQUIRE_EQUAL(command.delete_attr_type(), Expected::DEL_EVENT);
                BOOST_REQUIRE_EQUAL(command.paths().size(), paths.size());
                for (size_t i = 0; i < paths.size(); ++i) {
                    BOOST_REQUIRE(command.paths()[i] == paths[i]);
                }
                BOOST_REQUIRE_EQUAL(env.host(), "localhost");
                BOOST_REQUIRE_EQUAL(env.port(), "3141");
                BOOST_REQUIRE_EQUAL(env.debug(), true);
            });
        }
    } // paths
}

BOOST_AUTO_TEST_SUITE_END()
