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

#include "TestSupport.hpp"
#include "ecflow/test/scaffold/Naming.hpp"
#include "ecflow/test/scaffold/Process.hpp"
#include "ecflow/test/scaffold/Provisioning.hpp"

namespace ut = boost::unit_test;

BOOST_AUTO_TEST_SUITE(S_UDP)

BOOST_AUTO_TEST_SUITE(T_UDPServerLaunch)

BOOST_AUTO_TEST_CASE(can_launch_udp_server_default_parameters) {
    ECF_NAME_THIS_TEST();

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

    auto server = ecf::test::scaffold::Process(
        ecf::File::root_build_dir() + "/bin/ecflow_udp",
        {"--ecflow_port", "31415", "--ecflow_host", "custom", "--port", "8989", "--verbose"});
    std::this_thread::sleep_for(std::chrono::seconds(1));
    server.terminate();

    auto out = server.read_stdout();
    std::cout << out << std::endl;
    BOOST_CHECK(out.find("(error): ") == std::string::npos);
    BOOST_CHECK(out.find("(fatal): ") == std::string::npos);

    BOOST_CHECK(out.find("using custom ecflow host: custom") != std::string::npos);
    BOOST_CHECK(out.find("using custom ecflow port: 31415") != std::string::npos);
    BOOST_CHECK(out.find("using protocol TCP to communicate with ecFlow") != std::string::npos);
    BOOST_CHECK(out.find("using UDP port: 8989") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(can_launch_udp_server_custom_parameters_with_http_verbose) {
    ECF_NAME_THIS_TEST();

    auto server = ecf::test::scaffold::Process(
        ecf::File::root_build_dir() + "/bin/ecflow_udp",
        {"--ecflow_port", "31415", "--ecflow_host", "custom", "--port", "8989", "--http", "--verbose"});
    std::this_thread::sleep_for(std::chrono::seconds(1));
    server.terminate();

    auto out = server.read_stdout();
    std::cout << out << std::endl;
    BOOST_CHECK(out.find("(error): ") == std::string::npos);
    BOOST_CHECK(out.find("(fatal): ") == std::string::npos);

    BOOST_CHECK(out.find("using custom ecflow host: custom") != std::string::npos);
    BOOST_CHECK(out.find("using custom ecflow port: 31415") != std::string::npos);
    BOOST_CHECK(out.find("using protocol HTTP to communicate with ecFlow") != std::string::npos);
    BOOST_CHECK(out.find("using UDP port: 8989") != std::string::npos);
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

    WithTestEnvironmentVariable ecf_port("ECF_PORT", "31415");
    WithTestEnvironmentVariable ecf_host("ECF_HOST", "custom");
    WithTestEnvironmentVariable ecf_udp_port("ECF_UDP_PORT", "8989");

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
    BOOST_CHECK(out.find("using UDP port: 8989") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(launch_udp_server_based_on_cli_options_overridden_envvar_custom_parameters) {
    ECF_NAME_THIS_TEST();

    using namespace ecf::test::scaffold;

    WithTestEnvironmentVariable ecf_port("ECF_PORT", "31415");
    WithTestEnvironmentVariable ecf_host("ECF_HOST", "custom");
    WithTestEnvironmentVariable ecf_udp_port("ECF_UDP_PORT", "8989");

    auto server = ecf::test::scaffold::Process(
        ecf::File::root_build_dir() + "/bin/ecflow_udp",
        {"--ecflow_port", "44444", "--ecflow_host", "customx", "--port", "44445", "--verbose"});
    std::this_thread::sleep_for(std::chrono::seconds(1));
    server.terminate();

    auto out = server.read_stdout();
    std::cout << out << std::endl;
    BOOST_CHECK(out.find("(error): ") == std::string::npos);
    BOOST_CHECK(out.find("(fatal): ") == std::string::npos);

    BOOST_CHECK(out.find("using custom ecflow host: customx") != std::string::npos);
    BOOST_CHECK(out.find("using custom ecflow port: 44444") != std::string::npos);
    BOOST_CHECK(out.find("using protocol TCP to communicate with ecFlow") != std::string::npos);
    BOOST_CHECK(out.find("using UDP port: 44445") != std::string::npos);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
