/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#include <boost/test/unit_test.hpp>

#include "TestSupport.hpp"
#include "ecflow/test/scaffold/Naming.hpp"
#include "ecflow/test/scaffold/Process.hpp"
#include "ecflow/test/scaffold/Provisioning.hpp"

namespace ut = boost::unit_test;

namespace {

///
/// @brief Reserves the UDP port for a launched ecflow_udp server.
///
/// The port number is locked through the scaffold, and its UDP port probed, so that concurrent test runs on the
/// same host never launch two servers on one port. Candidates start at 32000, below the ephemeral range of Linux
/// (32768 upwards) and macOS (49152 upwards), and away from the ranges used by the other server-launching tests.
///
/// @return The reserved port, released when destroyed
///
ecf::test::scaffold::Port reserve_udp_port() {
    using namespace ecf::test::scaffold;
    return MakePort{}.with(AutomaticPortValue{32000, Transport::UDP}).create();
}

} // namespace

BOOST_AUTO_TEST_SUITE(S_UDP)

BOOST_AUTO_TEST_SUITE(T_UDPServerLaunch)

BOOST_AUTO_TEST_CASE(can_launch_udp_server_default_parameters) {
    ECF_NAME_THIS_TEST();

    using namespace ecf::test::scaffold;

    // The UDP port comes from the environment, so that the command line stays free of options
    auto udp_port = reserve_udp_port();
    WithTestEnvironmentVariable ecf_udp_port("ECF_UDP_PORT", std::to_string(udp_port.value()));

    auto server = ecf::test::scaffold::Process(ecf::File::root_build_dir() + "/bin/ecflow_udp", {});
    std::this_thread::sleep_for(std::chrono::seconds(1));
    server.terminate();

    auto out = server.read_stdout();
    std::cout << out << std::endl;
    BOOST_CHECK(out.find("(error): ") == std::string::npos);
    BOOST_CHECK(out.find("(fatal): ") == std::string::npos);
}

BOOST_AUTO_TEST_CASE(can_launch_udp_server_default_parameters_verbose) {
    ECF_NAME_THIS_TEST();

    using namespace ecf::test::scaffold;

    // The UDP port comes from the environment, so that the command line carries only the option under test
    auto udp_port = reserve_udp_port();
    WithTestEnvironmentVariable ecf_udp_port("ECF_UDP_PORT", std::to_string(udp_port.value()));

    auto server = ecf::test::scaffold::Process(ecf::File::root_build_dir() + "/bin/ecflow_udp", {"--verbose"});
    std::this_thread::sleep_for(std::chrono::seconds(1));
    server.terminate();

    auto out = server.read_stdout();
    std::cout << out << std::endl;
    BOOST_CHECK(out.find("(error): ") == std::string::npos);
    BOOST_CHECK(out.find("(fatal): ") == std::string::npos);
}

BOOST_AUTO_TEST_CASE(can_launch_udp_server_custom_parameters_with_tcp_verbose) {
    ECF_NAME_THIS_TEST();

    auto udp_port = reserve_udp_port();
    auto server   = ecf::test::scaffold::Process(
        ecf::File::root_build_dir() + "/bin/ecflow_udp",
        {"--ecflow_port", "31415", "--ecflow_host", "custom", "--port", std::to_string(udp_port.value()), "--verbose"});
    std::this_thread::sleep_for(std::chrono::seconds(1));
    server.terminate();

    auto out = server.read_stdout();
    std::cout << out << std::endl;
    BOOST_CHECK(out.find("(error): ") == std::string::npos);
    BOOST_CHECK(out.find("(fatal): ") == std::string::npos);

    BOOST_CHECK(out.find("using custom ecflow host: custom") != std::string::npos);
    BOOST_CHECK(out.find("using custom ecflow port: 31415") != std::string::npos);
    BOOST_CHECK(out.find("using protocol TCP to communicate with ecFlow") != std::string::npos);
    BOOST_CHECK(out.find("using UDP port: " + std::to_string(udp_port.value())) != std::string::npos);
}

BOOST_AUTO_TEST_CASE(can_launch_udp_server_custom_parameters_with_http_verbose) {
    ECF_NAME_THIS_TEST();

    auto udp_port = reserve_udp_port();
    auto server   = ecf::test::scaffold::Process(ecf::File::root_build_dir() + "/bin/ecflow_udp",
                                                 {"--ecflow_port",
                                                  "31415",
                                                  "--ecflow_host",
                                                  "custom",
                                                  "--port",
                                                  std::to_string(udp_port.value()),
                                                  "--http",
                                                  "--verbose"});
    std::this_thread::sleep_for(std::chrono::seconds(1));
    server.terminate();

    auto out = server.read_stdout();
    std::cout << out << std::endl;
    BOOST_CHECK(out.find("(error): ") == std::string::npos);
    BOOST_CHECK(out.find("(fatal): ") == std::string::npos);

    BOOST_CHECK(out.find("using custom ecflow host: custom") != std::string::npos);
    BOOST_CHECK(out.find("using custom ecflow port: 31415") != std::string::npos);
    BOOST_CHECK(out.find("using protocol HTTP to communicate with ecFlow") != std::string::npos);
    BOOST_CHECK(out.find("using UDP port: " + std::to_string(udp_port.value())) != std::string::npos);
}

BOOST_AUTO_TEST_CASE(can_detect_invalid_httpx_option) {
    ECF_NAME_THIS_TEST();

    auto server = ecf::test::scaffold::Process(ecf::File::root_build_dir() + "/bin/ecflow_udp", {"--httpx"});
    std::this_thread::sleep_for(std::chrono::seconds(1));
    server.terminate();

    auto out = server.read_stdout();
    std::cout << out << std::endl;
    BOOST_CHECK(out.find("(fatal): ") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(launch_udp_server_based_on_envvar_custom_parameters) {
    ECF_NAME_THIS_TEST();

    using namespace ecf::test::scaffold;

    auto udp_port = reserve_udp_port();
    WithTestEnvironmentVariable ecf_port("ECF_PORT", "31415");
    WithTestEnvironmentVariable ecf_host("ECF_HOST", "custom");
    WithTestEnvironmentVariable ecf_udp_port("ECF_UDP_PORT", std::to_string(udp_port.value()));

    auto server =
        ecf::test::scaffold::Process(ecf::File::root_build_dir() + "/bin/ecflow_udp", {"--http", "--verbose"});
    std::this_thread::sleep_for(std::chrono::seconds(1));
    server.terminate();

    auto out = server.read_stdout();
    std::cout << out << std::endl;
    BOOST_CHECK(out.find("(error): ") == std::string::npos);
    BOOST_CHECK(out.find("(fatal): ") == std::string::npos);

    BOOST_CHECK(out.find("using custom ecflow host: custom") != std::string::npos);
    BOOST_CHECK(out.find("using custom ecflow port: 31415") != std::string::npos);
    BOOST_CHECK(out.find("using protocol HTTP to communicate with ecFlow") != std::string::npos);
    BOOST_CHECK(out.find("using UDP port: " + std::to_string(udp_port.value())) != std::string::npos);
}

BOOST_AUTO_TEST_CASE(launch_udp_server_based_on_cli_options_overridden_envvar_custom_parameters) {
    ECF_NAME_THIS_TEST();

    using namespace ecf::test::scaffold;

    // Two ports are reserved: the environment names one, the command line another, and the latter must win
    auto env_udp_port = reserve_udp_port();
    auto udp_port     = reserve_udp_port();
    WithTestEnvironmentVariable ecf_port("ECF_PORT", "31415");
    WithTestEnvironmentVariable ecf_host("ECF_HOST", "custom");
    WithTestEnvironmentVariable ecf_udp_port("ECF_UDP_PORT", std::to_string(env_udp_port.value()));

    auto server = ecf::test::scaffold::Process(ecf::File::root_build_dir() + "/bin/ecflow_udp",
                                               {"--ecflow_port",
                                                "31416",
                                                "--ecflow_host",
                                                "customx",
                                                "--port",
                                                std::to_string(udp_port.value()),
                                                "--verbose"});
    std::this_thread::sleep_for(std::chrono::seconds(1));
    server.terminate();

    auto out = server.read_stdout();
    std::cout << out << std::endl;
    BOOST_CHECK(out.find("(error): ") == std::string::npos);
    BOOST_CHECK(out.find("(fatal): ") == std::string::npos);

    BOOST_CHECK(out.find("using custom ecflow host: customx") != std::string::npos);
    BOOST_CHECK(out.find("using custom ecflow port: 31416") != std::string::npos);
    BOOST_CHECK(out.find("using protocol TCP to communicate with ecFlow") != std::string::npos);
    BOOST_CHECK(out.find("using UDP port: " + std::to_string(udp_port.value())) != std::string::npos);
    BOOST_CHECK(out.find("using UDP port: " + std::to_string(env_udp_port.value())) == std::string::npos);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
