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

#include "ecflow/client/HostsFile.hpp"
#include "ecflow/core/Converter.hpp"
#include "ecflow/core/Filesystem.hpp"
#include "ecflow/test/scaffold/Naming.hpp"
#include "ecflow/test/scaffold/Provisioning.hpp"

BOOST_AUTO_TEST_SUITE(S_Client)

BOOST_AUTO_TEST_SUITE(T_HostsFile)

static int default_port = 1234;

BOOST_AUTO_TEST_CASE(test_can_load_empty_hosts_file) {
    ECF_NAME_THIS_TEST();

    std::string contents;
    std::istringstream stream(contents);

    auto hf = ecf::HostsFile::parse(stream, default_port);
    BOOST_REQUIRE(hf.hosts().empty());
}

BOOST_AUTO_TEST_CASE(test_can_load_empty_lines_hosts_file) {
    ECF_NAME_THIS_TEST();

    std::string contents = R"(



)";
    std::istringstream stream(contents);

    auto hf = ecf::HostsFile::parse(stream, default_port);
    BOOST_REQUIRE(hf.hosts().empty());
}

BOOST_AUTO_TEST_CASE(test_can_load_hosts_file_with_only_comments) {
    ECF_NAME_THIS_TEST();

    std::string contents = R"(
# This is a comment
      # Another comment line
)";
    std::istringstream stream(contents);

    auto hf = ecf::HostsFile::parse(stream, default_port);
    BOOST_REQUIRE(hf.hosts().empty());
}

BOOST_AUTO_TEST_CASE(test_can_load_hosts_file_with_default_port) {
    ECF_NAME_THIS_TEST();

    std::string contents = R"(
# This is a comment
host1      # Another comment line

# Last comment
)";
    std::istringstream stream(contents);

    auto hf = ecf::HostsFile::parse(stream, default_port);
    BOOST_REQUIRE(hf.hosts().size() == 1);
    BOOST_REQUIRE(hf.hosts()[0].first == "host1");
    BOOST_REQUIRE(hf.hosts()[0].second == std::to_string(default_port));
}

BOOST_AUTO_TEST_CASE(test_can_load_hosts_file_with_multiple_entries) {
    ECF_NAME_THIS_TEST();

    std::string contents = R"(
# This is a comment
host1      # Another comment line

host2:5678

# Last comment
)";
    std::istringstream stream(contents);

    auto hf = ecf::HostsFile::parse(stream, default_port);
    BOOST_REQUIRE(hf.hosts().size() == 2);
    BOOST_REQUIRE(hf.hosts()[0].first == "host1");
    BOOST_REQUIRE(hf.hosts()[0].second == std::to_string(default_port));
    BOOST_REQUIRE(hf.hosts()[1].first == "host2");
    BOOST_REQUIRE(hf.hosts()[1].second == "5678");
}

BOOST_AUTO_TEST_CASE(test_fails_when_parsing_empty_port) {
    ECF_NAME_THIS_TEST();

    std::string contents = R"(
host1:
)";
    std::istringstream stream(contents);

    auto hf = ecf::HostsFile::parse(stream, default_port);
    BOOST_REQUIRE(hf.hosts().size() == 1);
    BOOST_REQUIRE(hf.hosts()[0].first == "host1");
    BOOST_REQUIRE(hf.hosts()[0].second == std::to_string(default_port));
    // n.b. The default port is used if provided an empty port field
}

BOOST_AUTO_TEST_CASE(test_fails_when_parsing_invalid_negative_port) {
    ECF_NAME_THIS_TEST();

    std::string contents = R"(
host1:-1234
)";
    std::istringstream stream(contents);

    BOOST_REQUIRE_THROW(ecf::HostsFile::parse(stream, default_port), ecf::HostsFileFailure);
}

BOOST_AUTO_TEST_CASE(test_fails_when_parsing_invalid_textual_port) {
    ECF_NAME_THIS_TEST();

    std::string contents = R"(
host1:invalid
)";
    std::istringstream stream(contents);

    BOOST_REQUIRE_THROW(ecf::HostsFile::parse(stream, default_port), ecf::HostsFileFailure);
}

BOOST_AUTO_TEST_CASE(test_can_parse_from_file) {
    ECF_NAME_THIS_TEST();

    auto content = fs::path{"test_hostsfile.txt"};
    WithTestFile hostsfile(content.c_str(), R"(

host1:3142
host2:3143

host3

host4:3147

longnamehost5

)");

    auto hf = ecf::HostsFile::parse(content, default_port);

    BOOST_REQUIRE(hf.hosts().size() == 5);
    BOOST_REQUIRE(hf.hosts()[0].first == "host1");
    BOOST_REQUIRE(hf.hosts()[0].second == "3142");
    BOOST_REQUIRE(hf.hosts()[1].first == "host2");
    BOOST_REQUIRE(hf.hosts()[1].second == "3143");
    BOOST_REQUIRE(hf.hosts()[2].first == "host3");
    BOOST_REQUIRE(hf.hosts()[2].second == std::to_string(default_port));
    BOOST_REQUIRE(hf.hosts()[3].first == "host4");
    BOOST_REQUIRE(hf.hosts()[3].second == "3147");
    BOOST_REQUIRE(hf.hosts()[4].first == "longnamehost5");
    BOOST_REQUIRE(hf.hosts()[4].second == std::to_string(default_port));
}

BOOST_AUTO_TEST_CASE(test_can_parse_from_file_with_comments) {
    ECF_NAME_THIS_TEST();

    auto content = fs::path{"test_hostsfile.txt"};
    WithTestFile hostsfile(content.c_str(), R"(
# Some comment
host1    # Another comment
  host2:5678
)");

    auto hf = ecf::HostsFile::parse(content, default_port);

    BOOST_REQUIRE(hf.hosts().size() == 2);
    BOOST_REQUIRE(hf.hosts()[0].first == "host1");
    BOOST_REQUIRE(hf.hosts()[0].second == std::to_string(default_port));
    BOOST_REQUIRE(hf.hosts()[1].first == "host2");
    BOOST_REQUIRE(hf.hosts()[1].second == "5678");
}

BOOST_AUTO_TEST_CASE(test_fails_when_parsing_nonexisten_file) {
    ECF_NAME_THIS_TEST();

    fs::path content = "test_nonexistent.txt";

    BOOST_REQUIRE_THROW(ecf::HostsFile::parse(content, 1234), ecf::HostsFileFailure);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
