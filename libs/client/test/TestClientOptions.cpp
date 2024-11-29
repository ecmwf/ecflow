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

#include "ecflow/base/cts/task/AbortCmd.hpp"
#include "ecflow/base/cts/task/CompleteCmd.hpp"
#include "ecflow/base/cts/task/CtsWaitCmd.hpp"
#include "ecflow/base/cts/task/EventCmd.hpp"
#include "ecflow/base/cts/task/InitCmd.hpp"
#include "ecflow/base/cts/task/LabelCmd.hpp"
#include "ecflow/base/cts/task/MeterCmd.hpp"
#include "ecflow/base/cts/task/QueueCmd.hpp"
#include "ecflow/base/cts/user/AlterCmd.hpp"
#include "ecflow/base/cts/user/BeginCmd.hpp"
#include "ecflow/base/cts/user/PathsCmd.hpp"
#include "ecflow/client/ClientEnvironment.hpp"
#include "ecflow/client/ClientInvoker.hpp"
#include "ecflow/client/ClientOptions.hpp"
#include "ecflow/core/CommandLine.hpp"
#include "ecflow/core/PasswordEncryption.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

///
/// \brief Tests the capabilities of ClientOptions
///

template <typename COMMAND, typename REQUIRE>
void test_user_command(const CommandLine& cl, REQUIRE check) {
    std::cout << "Testing command line: " << cl.original() << std::endl;

    ClientOptions options;
    ClientEnvironment environment(false);
    try {
        auto base_command    = options.parse(cl, &environment);
        auto derived_command = dynamic_cast<COMMAND*>(base_command.get());

        BOOST_REQUIRE(derived_command);
        check(*derived_command, environment);
    }
    catch (boost::program_options::unknown_option& e) {
        BOOST_CHECK_MESSAGE(false, std::string("Unexpected exception caught: ") + e.what());
    }
}

template <typename COMMAND, typename REQUIRE>
void test_task_command(const CommandLine& cl, REQUIRE check) {
    std::cout << "Testing command line: " << cl.original() << std::endl;

    ClientOptions options;
    ClientEnvironment environment(false);
    // Setup environment with some defaults
    environment.set_child_path("/path/to/child");
    environment.set_child_password("password");
    try {
        auto base_command    = options.parse(cl, &environment);
        auto derived_command = dynamic_cast<COMMAND*>(base_command.get());

        BOOST_REQUIRE(derived_command);
        check(*derived_command, environment);
    }
    catch (boost::program_options::unknown_option& e) {
        BOOST_CHECK_MESSAGE(false, std::string("Unexpected exception caught: ") + e.what());
    }
}

BOOST_AUTO_TEST_SUITE(S_Client)

BOOST_AUTO_TEST_SUITE(T_ClientOptions)

BOOST_AUTO_TEST_SUITE(T_Generic) // test generic options

BOOST_AUTO_TEST_CASE(test_is_able_to_process_username_and_password) {
    ECF_NAME_THIS_TEST();

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

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(T_Abort) // --abort (AbortCmd)

BOOST_AUTO_TEST_CASE(test_is_able_to_handle_abort) {
    ECF_NAME_THIS_TEST();

    {
        auto cl = CommandLine::make_command_line("ecflow_client", "--abort");
        test_task_command<AbortCmd>(
            cl, [&](const auto& command, const ClientEnvironment& env) { BOOST_CHECK_EQUAL(command.reason(), ""); });
    }
    {
        auto cl = CommandLine::make_command_line("ecflow_client", "--abort=");
        test_task_command<AbortCmd>(
            cl, [&](const auto& command, const ClientEnvironment& env) { BOOST_CHECK_EQUAL(command.reason(), ""); });
    }
    {
        auto cl = CommandLine::make_command_line("ecflow_client", "--abort=somereason");
        test_task_command<AbortCmd>(cl, [&](const auto& command, const ClientEnvironment& env) {
            BOOST_CHECK_EQUAL(command.reason(), "somereason");
        });
    }
    {
        auto cl = CommandLine::make_command_line("ecflow_client", "--abort", "some reason with spaces");
        test_task_command<AbortCmd>(cl, [&](const auto& command, const ClientEnvironment& env) {
            BOOST_CHECK_EQUAL(command.reason(), "some reason with spaces");
        });
    }
    {
        auto cl = CommandLine::make_command_line("ecflow_client", "--abort=some reason with spaces ");
        test_task_command<AbortCmd>(cl, [&](const auto& command, const ClientEnvironment& env) {
            BOOST_CHECK_EQUAL(command.reason(), "some reason with spaces ");
        });
    }
    {
        auto cl = CommandLine::make_command_line("ecflow_client", "--abort=some", "reason");
        test_task_command<AbortCmd>(cl, [&](const auto& command, const ClientEnvironment& env) {
            BOOST_CHECK_EQUAL(command.reason(), "some"); // Important: the reason is only "some"!
        });
    }
    {
        auto cl = CommandLine::make_command_line("ecflow_client", "--abort=--dashed reason");
        test_task_command<AbortCmd>(cl, [&](const auto& command, const ClientEnvironment& env) {
            BOOST_CHECK_EQUAL(command.reason(), "--dashed reason");
        });
    }
    {
        auto cl = CommandLine::make_command_line("ecflow_client", "--abort", "--dashed reason");
        test_task_command<AbortCmd>(cl, [&](const auto& command, const ClientEnvironment& env) {
            BOOST_CHECK_EQUAL(command.reason(), "--dashed reason");
        });
    }
    {
        auto cl = CommandLine::make_command_line("ecflow_client", "--abort", "--dashed reason", "--debug");
        test_task_command<AbortCmd>(cl, [&](const auto& command, const ClientEnvironment& env) {
            BOOST_CHECK_EQUAL(command.reason(), "--dashed reason");
            BOOST_CHECK(env.debug());
        });
    }
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(T_Alter) // --alter (AlterCmd)

BOOST_AUTO_TEST_CASE(test_is_able_handle_alter_change) {
    ECF_NAME_THIS_TEST();

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
                test_user_command<AlterCmd>(cl, [&](const AlterCmd& command, const ClientEnvironment& env) {
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
                test_user_command<AlterCmd>(cl, [&](const AlterCmd& command, const ClientEnvironment& env) {
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
                test_user_command<AlterCmd>(cl, [&](const AlterCmd& command, const ClientEnvironment& env) {
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
                test_user_command<AlterCmd>(cl, [&](const AlterCmd& command, const ClientEnvironment& env) {
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
            test_user_command<AlterCmd>(cl, [&](const AlterCmd& command, const ClientEnvironment& env) {
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
            test_user_command<AlterCmd>(cl, [&](const AlterCmd& command, const ClientEnvironment& env) {
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
                test_user_command<AlterCmd>(cl, [&](const AlterCmd& command, const ClientEnvironment& env) {
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
                test_user_command<AlterCmd>(cl, [&](const AlterCmd& command, const ClientEnvironment& env) {
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
                test_user_command<AlterCmd>(cl, [&](const AlterCmd& command, const ClientEnvironment& env) {
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
                test_user_command<AlterCmd>(cl, [&](const AlterCmd& command, const ClientEnvironment& env) {
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
                test_user_command<AlterCmd>(cl, [&](const AlterCmd& command, const ClientEnvironment& env) {
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
                test_user_command<AlterCmd>(cl, [&](const AlterCmd& command, const ClientEnvironment& env) {
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
            test_user_command<AlterCmd>(cl, [&](const AlterCmd& command, const ClientEnvironment& env) {
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
            test_user_command<AlterCmd>(cl, [&](const AlterCmd& command, const ClientEnvironment& env) {
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
            test_user_command<AlterCmd>(cl, [&](const AlterCmd& command, const ClientEnvironment& env) {
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
            test_user_command<AlterCmd>(cl, [&](const AlterCmd& command, const ClientEnvironment& env) {
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
                test_user_command<AlterCmd>(cl, [&](const AlterCmd& command, const ClientEnvironment& env) {
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
                test_user_command<AlterCmd>(cl, [&](const AlterCmd& command, const ClientEnvironment& env) {
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
    ECF_NAME_THIS_TEST();

    /*
     * --alter add <type> <name> <value> <path> [<path>...]
     * ************************************************************************************************************** */

    std::vector<std::vector<std::string>> paths_set = {
        {"/node1"}, {"/node1", "/node2"}, {"/node1", "/node2", "/node3"}};

    for (const auto& paths : paths_set) {

        using Expected = AlterCmd::Add_attr_type;

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
                test_user_command<AlterCmd>(cl, [&](const AlterCmd& command, const ClientEnvironment& env) {
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
                test_user_command<AlterCmd>(cl, [&](const AlterCmd& command, const ClientEnvironment& env) {
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
    ECF_NAME_THIS_TEST();

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
            test_user_command<AlterCmd>(cl, [&](const AlterCmd& command, const ClientEnvironment& env) {
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
            test_user_command<AlterCmd>(cl, [&](const AlterCmd& command, const ClientEnvironment& env) {
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
            test_user_command<AlterCmd>(cl, [&](const AlterCmd& command, const ClientEnvironment& env) {
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
            test_user_command<AlterCmd>(cl, [&](const AlterCmd& command, const ClientEnvironment& env) {
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
            test_user_command<AlterCmd>(cl, [&](const AlterCmd& command, const ClientEnvironment& env) {
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
            test_user_command<AlterCmd>(cl, [&](const AlterCmd& command, const ClientEnvironment& env) {
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
            test_user_command<AlterCmd>(cl, [&](const AlterCmd& command, const ClientEnvironment& env) {
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
            test_user_command<AlterCmd>(cl, [&](const AlterCmd& command, const ClientEnvironment& env) {
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
            test_user_command<AlterCmd>(cl, [&](const AlterCmd& command, const ClientEnvironment& env) {
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
            test_user_command<AlterCmd>(cl, [&](const AlterCmd& command, const ClientEnvironment& env) {
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
            test_user_command<AlterCmd>(cl, [&](const AlterCmd& command, const ClientEnvironment& env) {
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
            test_user_command<AlterCmd>(cl, [&](const AlterCmd& command, const ClientEnvironment& env) {
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

BOOST_AUTO_TEST_SUITE(T_Begin) // --begin (BeginCmd)

BOOST_AUTO_TEST_CASE(test_is_able_to_handle_begin) {
    ECF_NAME_THIS_TEST();

    using Command = BeginCmd;
    {
        auto cl = CommandLine::make_command_line("ecflow_client", "--begin");
        test_task_command<Command>(cl, [&](const BeginCmd& command, const ClientEnvironment& env) {
            BOOST_CHECK_EQUAL(command.suiteName(), "");
            BOOST_CHECK_EQUAL(command.force(), false);
        });
    }
    {
        auto cl = CommandLine::make_command_line("ecflow_client", "--begin=somesuite");
        test_task_command<Command>(cl, [&](const BeginCmd& command, const ClientEnvironment& env) {
            BOOST_CHECK_EQUAL(command.suiteName(), "somesuite");
            BOOST_CHECK_EQUAL(command.force(), false);
        });
    }
    {
        auto cl = CommandLine::make_command_line("ecflow_client", "--begin=suite --force");
        test_task_command<Command>(cl, [&](const BeginCmd& command, const ClientEnvironment& env) {
            BOOST_CHECK_EQUAL(command.suiteName(), "suite");
            BOOST_CHECK_EQUAL(command.force(), true);
        });
    }
    {
        auto cl = CommandLine::make_command_line("ecflow_client", R"(--begin="suite --force")");
        test_task_command<Command>(cl, [&](const BeginCmd& command, const ClientEnvironment& env) {
            BOOST_CHECK_EQUAL(command.suiteName(), "suite");
            BOOST_CHECK_EQUAL(command.force(), true);
        });
    }
    {
        auto cl = CommandLine::make_command_line("ecflow_client", "--begin", "suite --force");
        test_task_command<Command>(cl, [&](const BeginCmd& command, const ClientEnvironment& env) {
            BOOST_CHECK_EQUAL(command.suiteName(), "suite");
            BOOST_CHECK_EQUAL(command.force(), true);
        });
    }
    {
        auto cl = CommandLine::make_command_line("ecflow_client", R"(--begin="--force")");
        test_task_command<Command>(cl, [&](const BeginCmd& command, const ClientEnvironment& env) {
            BOOST_CHECK_EQUAL(command.suiteName(), "");
            BOOST_CHECK_EQUAL(command.force(), true);
        });
    }
    {
        auto cl = CommandLine::make_command_line("ecflow_client", "--begin", "--force");
        // This is an invalid, as --begin only takes a single argument (potentialy with --force as second value)
    }
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(T_Check) // --check (PathsCmd)

BOOST_AUTO_TEST_CASE(test_is_able_to_handle_check) {
    ECF_NAME_THIS_TEST();

    using Command = PathsCmd;
    {
        auto cl = CommandLine::make_command_line("ecflow_client", "--check=_all_");
        test_user_command<Command>(cl, [&](const auto& command, const ClientEnvironment& env) {
            BOOST_CHECK_EQUAL(command.api(), PathsCmd::CHECK);
            BOOST_CHECK_EQUAL(command.paths().size(), static_cast<size_t>(0));
            BOOST_CHECK_EQUAL(command.force(), false);
        });
    }
    {
        auto cl = CommandLine::make_command_line("ecflow_client", "--check", "_all_");
        test_user_command<Command>(cl, [&](const auto& command, const ClientEnvironment& env) {
            BOOST_CHECK_EQUAL(command.api(), PathsCmd::CHECK);
            BOOST_CHECK_EQUAL(command.paths().size(), static_cast<size_t>(0));
            BOOST_CHECK_EQUAL(command.force(), false);
        });
    }
    {
        auto cl = CommandLine::make_command_line("ecflow_client", "--check=/");
        test_user_command<Command>(cl, [&](const auto& command, const ClientEnvironment& env) {
            BOOST_CHECK_EQUAL(command.api(), PathsCmd::CHECK);
            BOOST_CHECK_EQUAL(command.paths().size(), static_cast<size_t>(0));
            BOOST_CHECK_EQUAL(command.force(), false);
        });
    }
    {
        auto cl = CommandLine::make_command_line("ecflow_client", "--check", "/");
        test_user_command<Command>(cl, [&](const auto& command, const ClientEnvironment& env) {
            BOOST_CHECK_EQUAL(command.api(), PathsCmd::CHECK);
            BOOST_CHECK_EQUAL(command.paths().size(), static_cast<size_t>(0));
            BOOST_CHECK_EQUAL(command.force(), false);
        });
    }

    {
        auto cl = CommandLine::make_command_line("ecflow_client", "--check=/s1", "/s2/f2", "/s3/f3/t3");
        test_user_command<Command>(cl, [&](const auto& command, const ClientEnvironment& env) {
            BOOST_CHECK_EQUAL(command.api(), PathsCmd::CHECK);
            BOOST_REQUIRE_EQUAL(command.paths().size(), static_cast<size_t>(3));
            BOOST_CHECK_EQUAL(command.paths()[0], "/s1");
            BOOST_CHECK_EQUAL(command.paths()[1], "/s2/f2");
            BOOST_CHECK_EQUAL(command.paths()[2], "/s3/f3/t3");
            BOOST_CHECK_EQUAL(command.force(), false);
        });
    }
    {
        auto cl = CommandLine::make_command_line("ecflow_client", "--check", "/s1", "/s2/f2", "/s3/f3/t3");
        test_user_command<Command>(cl, [&](const auto& command, const ClientEnvironment& env) {
            BOOST_CHECK_EQUAL(command.api(), PathsCmd::CHECK);
            BOOST_CHECK_EQUAL(command.paths().size(), static_cast<size_t>(3));
            BOOST_CHECK_EQUAL(command.paths()[0], "/s1");
            BOOST_CHECK_EQUAL(command.paths()[1], "/s2/f2");
            BOOST_CHECK_EQUAL(command.paths()[2], "/s3/f3/t3");
            BOOST_CHECK_EQUAL(command.force(), false);
        });
    }
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(T_Complete) // --complete (CompleteCmd)

BOOST_AUTO_TEST_CASE(test_is_able_to_handle_complete) {
    ECF_NAME_THIS_TEST();

    using Command = CompleteCmd;
    {
        auto cl = CommandLine::make_command_line("ecflow_client", "--complete");
        test_task_command<Command>(cl, [&](const auto& command, const ClientEnvironment& env) {
            BOOST_CHECK(command.variables_to_delete().empty());
        });
    }
    {
        auto cl = CommandLine::make_command_line("ecflow_client", "--complete", "--remove", "var1");
        test_task_command<Command>(cl, [&](const auto& command, const ClientEnvironment& env) {
            BOOST_REQUIRE_EQUAL(command.variables_to_delete().size(), static_cast<size_t>(1));
            BOOST_REQUIRE_EQUAL(command.variables_to_delete()[0], "var1");
        });
    }
    {
        auto cl = CommandLine::make_command_line("ecflow_client", "--complete", "--remove", "var1", "var2");
        test_task_command<Command>(cl, [&](const auto& command, const ClientEnvironment& env) {
            BOOST_REQUIRE_EQUAL(command.variables_to_delete().size(), static_cast<size_t>(2));
            BOOST_REQUIRE_EQUAL(command.variables_to_delete()[0], "var1");
            BOOST_REQUIRE_EQUAL(command.variables_to_delete()[1], "var2");
        });
    }
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(T_Event) // --event (EventCmd)

BOOST_AUTO_TEST_CASE(test_is_able_to_handle_event) {
    ECF_NAME_THIS_TEST();

    using Command = EventCmd;
    {
        auto cl = CommandLine::make_command_line("ecflow_client", "--event=name");
        test_task_command<Command>(cl, [&](const auto& command, const ClientEnvironment& env) {
            BOOST_CHECK_EQUAL(command.name(), "name");
            BOOST_CHECK_EQUAL(command.value(), true);
        });
    }
    {
        auto cl = CommandLine::make_command_line("ecflow_client", "--event", "name");
        test_task_command<Command>(cl, [&](const auto& command, const ClientEnvironment& env) {
            BOOST_CHECK_EQUAL(command.name(), "name");
            BOOST_CHECK_EQUAL(command.value(), true);
        });
    }
    {
        auto cl = CommandLine::make_command_line("ecflow_client", "--event=name", "set");
        test_task_command<Command>(cl, [&](const auto& command, const ClientEnvironment& env) {
            BOOST_CHECK_EQUAL(command.name(), "name");
            BOOST_CHECK_EQUAL(command.value(), true);
        });
    }
    {
        auto cl = CommandLine::make_command_line("ecflow_client", "--event", "name", "set");
        test_task_command<Command>(cl, [&](const auto& command, const ClientEnvironment& env) {
            BOOST_CHECK_EQUAL(command.name(), "name");
            BOOST_CHECK_EQUAL(command.value(), true);
        });
    }
    {
        auto cl = CommandLine::make_command_line("ecflow_client", "--event=name", "clear");
        test_task_command<Command>(cl, [&](const auto& command, const ClientEnvironment& env) {
            BOOST_CHECK_EQUAL(command.name(), "name");
            BOOST_CHECK_EQUAL(command.value(), false);
        });
    }
    {
        auto cl = CommandLine::make_command_line("ecflow_client", "--event", "name", "clear");
        test_task_command<Command>(cl, [&](const auto& command, const ClientEnvironment& env) {
            BOOST_CHECK_EQUAL(command.name(), "name");
            BOOST_CHECK_EQUAL(command.value(), false);
        });
    }
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(T_Init) // --init (InitCmd)

BOOST_AUTO_TEST_CASE(test_is_able_to_handle_init) {
    ECF_NAME_THIS_TEST();

    using Command = InitCmd;
    {
        auto cl = CommandLine::make_command_line("ecflow_client", "--init=1234");
        test_task_command<Command>(cl, [&](const auto& command, const ClientEnvironment& env) {
            BOOST_CHECK_EQUAL(command.process_or_remote_id(), "1234");
            BOOST_CHECK_EQUAL(command.variables_to_add().size(), static_cast<size_t>(0));
        });
    }
    {
        auto cl = CommandLine::make_command_line("ecflow_client", "--init", "1234");
        test_task_command<Command>(cl, [&](const auto& command, const ClientEnvironment& env) {
            BOOST_CHECK_EQUAL(command.process_or_remote_id(), "1234");
            BOOST_CHECK_EQUAL(command.variables_to_add().size(), static_cast<size_t>(0));
        });
    }
    {
        auto cl = CommandLine::make_command_line("ecflow_client", "--init=1234", "--add", "name1=value1");
        test_task_command<Command>(cl, [&](const auto& command, const ClientEnvironment& env) {
            BOOST_CHECK_EQUAL(command.process_or_remote_id(), "1234");
            BOOST_REQUIRE_EQUAL(command.variables_to_add().size(), static_cast<size_t>(1));
            BOOST_CHECK_EQUAL(command.variables_to_add()[0].name(), "name1");
            BOOST_CHECK_EQUAL(command.variables_to_add()[0].theValue(), "value1");
        });
    }
    {
        auto cl =
            CommandLine::make_command_line("ecflow_client", "--init=1234", "--add", "name1=--value1", "name2=--value2");
        test_task_command<Command>(cl, [&](const auto& command, const ClientEnvironment& env) {
            BOOST_CHECK_EQUAL(command.process_or_remote_id(), "1234");
            BOOST_REQUIRE_EQUAL(command.variables_to_add().size(), static_cast<size_t>(2));
            BOOST_CHECK_EQUAL(command.variables_to_add()[0].name(), "name1");
            BOOST_CHECK_EQUAL(command.variables_to_add()[0].theValue(), "--value1");
            BOOST_CHECK_EQUAL(command.variables_to_add()[1].name(), "name2");
            BOOST_CHECK_EQUAL(command.variables_to_add()[1].theValue(), "--value2");
        });
    }
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(T_Label) // --label (LabelCmd)

BOOST_AUTO_TEST_CASE(test_is_able_to_handle_label) {
    ECF_NAME_THIS_TEST();

    using Command = LabelCmd;
    {
        auto cl = CommandLine::make_command_line("ecflow_client", "--label=name", "some value with spaces");
        test_task_command<Command>(cl, [&](const auto& command, const ClientEnvironment& env) {
            BOOST_CHECK_EQUAL(command.name(), "name");
            BOOST_CHECK_EQUAL(command.label(), "some value with spaces");
        });
    }
    {
        auto cl = CommandLine::make_command_line("ecflow_client", "--label", "name", "some value with spaces");
        test_task_command<Command>(cl, [&](const auto& command, const ClientEnvironment& env) {
            BOOST_CHECK_EQUAL(command.name(), "name");
            BOOST_CHECK_EQUAL(command.label(), "some value with spaces");
        });
    }
    {
        auto cl = CommandLine::make_command_line("ecflow_client", "--label=name", R"(some "quoted" value)");
        test_task_command<Command>(cl, [&](const auto& command, const ClientEnvironment& env) {
            BOOST_CHECK_EQUAL(command.name(), "name");
            BOOST_CHECK_EQUAL(command.label(), R"(some "quoted" value)");
        });
    }
    {
        auto cl = CommandLine::make_command_line("ecflow_client", "--label", "name", R"(some "quoted" value)");
        test_task_command<Command>(cl, [&](const auto& command, const ClientEnvironment& env) {
            BOOST_CHECK_EQUAL(command.name(), "name");
            BOOST_CHECK_EQUAL(command.label(), R"(some "quoted" value)");
        });
    }
    {
        auto cl = CommandLine::make_command_line("ecflow_client", "--label=name", "-j64");
        test_task_command<Command>(cl, [&](const auto& command, const ClientEnvironment& env) {
            BOOST_CHECK_EQUAL(command.name(), "name");
            BOOST_CHECK_EQUAL(command.label(), "-j64");
        });
    }
    {
        auto cl = CommandLine::make_command_line("ecflow_client", "--label=name", "--long-option");
        test_task_command<Command>(cl, [&](const auto& command, const ClientEnvironment& env) {
            BOOST_CHECK_EQUAL(command.name(), "name");
            BOOST_CHECK_EQUAL(command.label(), "--long-option");
        });
    }
    {
        auto cl = CommandLine::make_command_line("ecflow_client", "--label=name", "--option=value");
        test_task_command<Command>(cl, [&](const auto& command, const ClientEnvironment& env) {
            BOOST_CHECK_EQUAL(command.name(), "name");
            BOOST_CHECK_EQUAL(command.label(), "--option=value");
        });
    }
    {
        auto cl = CommandLine::make_command_line("ecflow_client", "--label=name", "--debug", "--debug");
        test_task_command<Command>(cl, [&](const auto& command, const ClientEnvironment& env) {
            BOOST_CHECK_EQUAL(command.name(), "name");
            BOOST_CHECK_EQUAL(command.label(), "--debug");
        });
    }
    {
        auto cl = CommandLine::make_command_line("ecflow_client", "--label=name", "--port", "--debug", "--port", "123");
        test_task_command<Command>(cl, [&](const auto& command, const ClientEnvironment& env) {
            BOOST_CHECK_EQUAL(command.name(), "name");
            BOOST_CHECK_EQUAL(command.label(), "--port");
        });
    }
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(T_Meter) // --meter (MeterCmd)

BOOST_AUTO_TEST_CASE(test_is_able_to_handle_meter) {
    ECF_NAME_THIS_TEST();

    using Command = MeterCmd;
    {
        auto cl = CommandLine::make_command_line("ecflow_client", "--meter=name", "0");
        test_task_command<Command>(cl, [&](const auto& command, const ClientEnvironment& env) {
            BOOST_CHECK_EQUAL(command.name(), "name");
            BOOST_CHECK_EQUAL(command.value(), 0);
        });
    }
    {
        auto cl = CommandLine::make_command_line("ecflow_client", "--meter", "name", "0");
        test_task_command<Command>(cl, [&](const auto& command, const ClientEnvironment& env) {
            BOOST_CHECK_EQUAL(command.name(), "name");
            BOOST_CHECK_EQUAL(command.value(), 0);
        });
    }
    {
        auto cl = CommandLine::make_command_line("ecflow_client", "--meter=name", "-1");
        test_task_command<Command>(cl, [&](const auto& command, const ClientEnvironment& env) {
            BOOST_CHECK_EQUAL(command.name(), "name");
            BOOST_CHECK_EQUAL(command.value(), -1);
        });
    }
    {
        auto cl = CommandLine::make_command_line("ecflow_client", "--meter", "name", "-1");
        test_task_command<Command>(cl, [&](const auto& command, const ClientEnvironment& env) {
            BOOST_CHECK_EQUAL(command.name(), "name");
            BOOST_CHECK_EQUAL(command.value(), -1);
        });
    }
    {
        auto cl = CommandLine::make_command_line("ecflow_client", "--meter=name", "10");
        test_task_command<Command>(cl, [&](const auto& command, const ClientEnvironment& env) {
            BOOST_CHECK_EQUAL(command.name(), "name");
            BOOST_CHECK_EQUAL(command.value(), 10);
        });
    }
    {
        auto cl = CommandLine::make_command_line("ecflow_client", "--meter=name", "20", "--debug");
        test_task_command<Command>(cl, [&](const auto& command, const ClientEnvironment& env) {
            BOOST_CHECK_EQUAL(command.name(), "name");
            BOOST_CHECK_EQUAL(command.value(), 20);
        });
    }
    {
        auto cl = CommandLine::make_command_line("ecflow_client", "--meter", "name", "-20", "--debug");
        test_task_command<Command>(cl, [&](const auto& command, const ClientEnvironment& env) {
            BOOST_CHECK_EQUAL(command.name(), "name");
            BOOST_CHECK_EQUAL(command.value(), -20);
        });
    }
    {
        auto cl = CommandLine::make_command_line("ecflow_client", "--meter=name", "-100", "--debug", "--port", "1234");
        test_task_command<Command>(cl, [&](const auto& command, const ClientEnvironment& env) {
            BOOST_CHECK_EQUAL(command.name(), "name");
            BOOST_CHECK_EQUAL(command.value(), -100);
        });
    }
    {
        auto cl =
            CommandLine::make_command_line("ecflow_client", "--meter", "name", "-100", "--debug", "--port", "1234");
        test_task_command<Command>(cl, [&](const auto& command, const ClientEnvironment& env) {
            BOOST_CHECK_EQUAL(command.name(), "name");
            BOOST_CHECK_EQUAL(command.value(), -100);
        });
    }
    {
        auto cl = CommandLine::make_command_line(
            "ecflow_client", "--port", "3141", "--host", "ecflow", "--meter", "step", "-1", "--debug");
        test_task_command<Command>(cl, [&](const auto& command, const ClientEnvironment& env) {
            BOOST_CHECK_EQUAL(command.name(), "step");
            BOOST_CHECK_EQUAL(command.value(), -1);
        });
    }
    {
        auto cl = CommandLine::make_command_line(
            "ecflow_client", "--port", "3141", "--host", "ecflow", "--meter", "step", "-1", "--debug");
        test_task_command<Command>(cl, [&](const auto& command, const ClientEnvironment& env) {
            BOOST_CHECK_EQUAL(command.name(), "step");
            BOOST_CHECK_EQUAL(command.value(), -1);
        });
    }
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(T_Queue) // --queue (QueueCmd)

BOOST_AUTO_TEST_CASE(test_is_able_to_handle_queue) {
    ECF_NAME_THIS_TEST();

    using Command = QueueCmd;
    {
        auto cl = CommandLine::make_command_line("ecflow_client", "--queue=name", "active");
        test_task_command<Command>(cl, [&](const auto& command, const ClientEnvironment& env) {
            BOOST_CHECK_EQUAL(command.name(), "name");
            BOOST_CHECK_EQUAL(command.action(), "active");
        });
    }
    {
        auto cl = CommandLine::make_command_line("ecflow_client", "--queue", "name", "active");
        test_task_command<Command>(cl, [&](const auto& command, const ClientEnvironment& env) {
            BOOST_CHECK_EQUAL(command.name(), "name");
            BOOST_CHECK_EQUAL(command.action(), "active");
        });
    }
    {
        auto cl = CommandLine::make_command_line("ecflow_client", "--queue", "name", "active", "/s/f/t");
        test_task_command<Command>(cl, [&](const auto& command, const ClientEnvironment& env) {
            BOOST_CHECK_EQUAL(command.name(), "name");
            BOOST_CHECK_EQUAL(command.action(), "active");
            BOOST_CHECK_EQUAL(command.path_to_node_with_queue(), "/s/f/t");
        });
    }
    {
        auto cl = CommandLine::make_command_line("ecflow_client", "--queue=name", "complete", "42");
        test_task_command<Command>(cl, [&](const auto& command, const ClientEnvironment& env) {
            BOOST_CHECK_EQUAL(command.name(), "name");
            BOOST_CHECK_EQUAL(command.action(), "complete");
            BOOST_CHECK_EQUAL(command.step(), "42");
        });
    }
    {
        auto cl = CommandLine::make_command_line("ecflow_client", "--queue", "name", "complete", "42");
        test_task_command<Command>(cl, [&](const auto& command, const ClientEnvironment& env) {
            BOOST_CHECK_EQUAL(command.name(), "name");
            BOOST_CHECK_EQUAL(command.action(), "complete");
            BOOST_CHECK_EQUAL(command.step(), "42");
        });
    }
    {
        auto cl = CommandLine::make_command_line("ecflow_client", "--queue=name", "aborted", "42");
        test_task_command<Command>(cl, [&](const auto& command, const ClientEnvironment& env) {
            BOOST_CHECK_EQUAL(command.name(), "name");
            BOOST_CHECK_EQUAL(command.action(), "aborted");
            BOOST_CHECK_EQUAL(command.step(), "42");
        });
    }
    {
        auto cl = CommandLine::make_command_line("ecflow_client", "--queue", "name", "aborted", "42");
        test_task_command<Command>(cl, [&](const auto& command, const ClientEnvironment& env) {
            BOOST_CHECK_EQUAL(command.name(), "name");
            BOOST_CHECK_EQUAL(command.action(), "aborted");
            BOOST_CHECK_EQUAL(command.step(), "42");
        });
    }
    {
        auto cl = CommandLine::make_command_line("ecflow_client", "--queue=name", "no_of_aborted");
        test_task_command<Command>(cl, [&](const auto& command, const ClientEnvironment& env) {
            BOOST_CHECK_EQUAL(command.name(), "name");
            BOOST_CHECK_EQUAL(command.action(), "no_of_aborted");
        });
    }
    {
        auto cl = CommandLine::make_command_line("ecflow_client", "--queue", "name", "no_of_aborted");
        test_task_command<Command>(cl, [&](const auto& command, const ClientEnvironment& env) {
            BOOST_CHECK_EQUAL(command.name(), "name");
            BOOST_CHECK_EQUAL(command.action(), "no_of_aborted");
        });
    }
    {
        auto cl = CommandLine::make_command_line("ecflow_client", "--queue=name", "no_of_aborted", "/s/f/t");
        test_task_command<Command>(cl, [&](const auto& command, const ClientEnvironment& env) {
            BOOST_CHECK_EQUAL(command.name(), "name");
            BOOST_CHECK_EQUAL(command.action(), "no_of_aborted");
            BOOST_CHECK_EQUAL(command.path_to_node_with_queue(), "/s/f/t");
        });
    }
    {
        auto cl = CommandLine::make_command_line("ecflow_client", "--queue=name", "reset");
        test_task_command<Command>(cl, [&](const auto& command, const ClientEnvironment& env) {
            BOOST_CHECK_EQUAL(command.name(), "name");
            BOOST_CHECK_EQUAL(command.action(), "reset");
        });
    }
    {
        auto cl = CommandLine::make_command_line("ecflow_client", "--queue=name", "reset", "/s/f/t");
        test_task_command<Command>(cl, [&](const auto& command, const ClientEnvironment& env) {
            BOOST_CHECK_EQUAL(command.name(), "name");
            BOOST_CHECK_EQUAL(command.action(), "reset");
            BOOST_CHECK_EQUAL(command.path_to_node_with_queue(), "/s/f/t");
        });
    }
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(T_Wait) // --wait (WaitCmd)

BOOST_AUTO_TEST_CASE(test_is_able_to_handle_wait) {
    ECF_NAME_THIS_TEST();

    using Command = CtsWaitCmd;
    {
        auto cl = CommandLine::make_command_line("ecflow_client", "--wait=/suite/taskx == complete");
        test_task_command<Command>(cl, [&](const auto& command, const ClientEnvironment& env) {
            BOOST_CHECK_EQUAL(command.expression(), "/suite/taskx == complete");
        });
    }
    {
        auto cl = CommandLine::make_command_line("ecflow_client", "--wait", "/suite/taskx == complete");
        test_task_command<Command>(cl, [&](const auto& command, const ClientEnvironment& env) {
            BOOST_CHECK_EQUAL(command.expression(), "/suite/taskx == complete");
        });
    }
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
