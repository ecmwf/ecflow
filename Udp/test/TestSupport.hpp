/*
 * Copyright 2023- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ECFLOW_UDP_TESTSUPPORT_HPP
#define ECFLOW_UDP_TESTSUPPORT_HPP

#include <memory>
#include <thread>

#include <boost/asio.hpp>
#include <boost/filesystem.hpp>
#include <boost/process.hpp>
#include <boost/test/unit_test.hpp>

#include "ClientInvoker.hpp"
#include "Defs.hpp"
#include "EcfPortLock.hpp"
#include "File.hpp"
#include "Host.hpp"
#include "Node.hpp"
#include "NodeAttr.hpp"
#include "Str.hpp"
#include "UDPClient.hpp"

namespace bp = boost::process;

namespace ecf::test {

template <typename SERVER>
class BaseMockServer {
public:
    using hostname_t = std::string;
    using port_t     = uint16_t;

    template <typename... Args>
    BaseMockServer(hostname_t host, port_t port, Args... args) : host_{std::move(host)},
                                                                 port_{port},
                                                                 server_{} {
        BOOST_REQUIRE_MESSAGE(!host_.empty(), "unable to determine host name");
        BOOST_REQUIRE_MESSAGE(port_ > 0, "port must be larger than 0");

        server_ = SERVER::launch(host_, port_, std::forward<Args>(args)...);
        std::cout << "   MOCK: " << SERVER::designation << " has been started!" << std::endl;
    }
    BaseMockServer(const BaseMockServer&) = delete;
    BaseMockServer(BaseMockServer&&)      = delete;

    ~BaseMockServer() {
        server_.terminate();
        SERVER::cleanup(host_, port_);
        std::cout << "   MOCK: " << SERVER::designation << " has been terminated!" << std::endl;
    }

    const hostname_t& host() const { return host_; }
    uint16_t port() const { return port_; }

private:
    hostname_t host_;
    uint16_t port_;

    bp::child server_;
};

/**
 * A "mock" ecFlow server, launches the server on a separate process, and enables checking changes in Defs state
 */
class MockServer : public BaseMockServer<MockServer> {
public:
    explicit MockServer(port_t port) : BaseMockServer<MockServer>(ecf::Host{}.name(), port) {}

    void load_definition(const std::string& defs) const {
        ClientInvoker client(ecf::Str::LOCALHOST(), port());
        auto error = client.loadDefs(defs);
        BOOST_REQUIRE_MESSAGE(!error, "unable to load definitions");
        std::cout << "   MOCK: reference ecFlow suite has been loaded" << std::endl;
    }

    Meter get_meter(const std::string& path, const std::string& name) const {
        node_ptr node = get_node_at(path);
        return get_attribute_by_name(node->meters(), name);
    }

    Label get_label(const std::string& path, const std::string& name) const {
        node_ptr node = get_node_at(path);
        return get_attribute_by_name(node->labels(), name);
    }

    Event get_event(const std::string& path, const std::string& name) const {
        node_ptr node = get_node_at(path);
        return get_attribute_by_name(node->events(), name);
    }

private:
    node_ptr get_node_at(const std::string& path) const {
        std::cout << "   Creating ecflow_client connected to " << host() << ":" << port() << std::endl;
        ClientInvoker client(ecf::Str::LOCALHOST(), port());

        // load all definitions
        std::shared_ptr<Defs> defs = nullptr;
        client.sync(defs);

        // select node at intended path
        node_ptr node = defs->findAbsNode(path);
        BOOST_REQUIRE_MESSAGE(node, "unable to find node");

        return node;
    }

    template <typename ATTRIBUTE>
    static ATTRIBUTE get_attribute_by_name(const std::vector<ATTRIBUTE>& attribs, const std::string& name) {
        auto found = std::find_if(
            std::begin(attribs), std::end(attribs), [&name](const ATTRIBUTE& attrib) { return attrib.name() == name; });
        BOOST_REQUIRE_MESSAGE(found != std::end(attribs), "unable to find attribute");
        return *found;
    }

public:
    static constexpr const char* designation = "ecFlow server";

    static bp::child launch(const hostname_t& host, port_t port) {
        // Just for precaution, in case a previous run didn't clean up...
        cleanup(host, port);

        std::string invoke_command = ecf::File::find_ecf_server_path();

        BOOST_REQUIRE_MESSAGE(!invoke_command.empty(), "The server program could not be found");
        BOOST_REQUIRE_MESSAGE(boost::filesystem::exists(invoke_command),
                              "Server exe does not exist at:" << invoke_command);

        invoke_command += " --port ";
        invoke_command += std::to_string(port);
        invoke_command += " -d &";

        std::cout << "Launching ecflow_server @" << host << ":" << port << ", with: " << invoke_command << std::endl;

        bp::child child(invoke_command);

        ClientInvoker client(ecf::Str::LOCALHOST(), port);
        if (!client.wait_for_server_reply(5)) {
            BOOST_REQUIRE_MESSAGE(false, "could not launch ecflow server");
        }

        return child;
    }

    static void cleanup(const hostname_t& host, port_t port) {
        // Clean up temporary files created by the server instance
        std::string temporaries[]{host + '.' + std::to_string(port) + ".ecf.check",
                                  host + '.' + std::to_string(port) + ".ecf.check.b",
                                  host + '.' + std::to_string(port) + ".ecf.log"};
        for (const auto& t : temporaries) {
            boost::filesystem::remove(t);
        }
    }
};

/**
 * A "mock" ecFlow UDP server, launches the server on a separate process, and enables triggering operations
 */
class MockUDPServer : public BaseMockServer<MockUDPServer> {
public:
    explicit MockUDPServer(port_t port, port_t ecflow_port)
        : BaseMockServer<MockUDPServer>("localhost", port, ecflow_port) {}

    void update_label(const std::string& path, const std::string& name, const std::string& value) {
        auto request = format_request(path, "alter_label", name, value);
        send(request);
    }

    void update_meter(const std::string& path, const std::string& name, int value) {
        auto request = format_request(path, "alter_meter", name, value);
        send(request);
    }

    void clear_event(const std::string& path, const std::string& name) {
        auto request = format_request(path, "alter_event", name, "0");
        send(request);
    }
    void set_event(const std::string& path, const std::string& name) {
        auto request = format_request(path, "alter_event", name, "1");
        send(request);
    }

    void send(const std::string& request) {
        std::cout << "   MOCK: UDP Client sending request: " << request << std::endl;
        sendRequest(port(), request);
    }

private:
    template <typename V>
    static std::string
    format_request(const std::string& path, const std::string& command, const std::string& name, V value) {
        std::ostringstream oss;
        // clang-format off
        oss << R"({)"
                << R"("method":"put",)"
                << R"("payload":)"
                << R"({)"
                    << R"("command":")" << command << R"(",)"
                    << R"("path":")" << path << R"(",)"
                    << R"("name":")" << name << R"(",)"
                    << R"("value":")"<< value << R"(")"
                << R"(})"
            << R"(})";
        // clang-format on
        return oss.str();
    }

    static void sendRequest(uint16_t port, const std::string& request) {
        const std::string host = "localhost";
        std::cout << "   Creating ecflow_udp client connected to " << host << ":" << port << std::endl;
        ecf::UDPClient client(host, std::to_string(port));
        client.send(request);

        // Wait for request to flow...
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

public:
    static constexpr const char* designation = "ecFlow UDP";

    static bp::child launch(const hostname_t& host, port_t port, port_t ecflow_port) {

        std::string invoke_command = ecf::File::root_build_dir() + "/bin/ecflow_udp";
        invoke_command += " --port ";
        invoke_command += std::to_string(port);
        invoke_command += " --ecflow_port ";
        invoke_command += std::to_string(ecflow_port);
        invoke_command += " --verbose";

        std::cout << "   Launching ecflow_udp @" << host << ":" << port << ", with: " << invoke_command << std::endl;

        bp::child server(invoke_command);

        // Wait for server to start...
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

        return server;
    }

    static void cleanup(const hostname_t& host, port_t port) {
        // No clean up needed for ecFlow UDP
    }
};

/**
 * This fixture provisions the tests with both an ecFlow server and an ecFlow UDP server.
 */
struct EnableServersFixture
{
    EnableServersFixture() : EnableServersFixture(get_ecflow_server_port(), get_ecflow_udp_port()) {}
    ~EnableServersFixture() = default;

    ecf::test::MockServer ecflow_server;
    ecf::test::MockUDPServer ecflow_udp;

private:
    static MockServer::port_t get_ecflow_server_port() {
        MockServer::port_t selected_port = 3199;
        std::cout << "   Attempting to use port: " << selected_port << std::endl;
        while (!EcfPortLock::is_free(selected_port)) {
            std::cout << "   Selected port: " << selected_port << " is not available." << std::endl;
            ++selected_port;
            std::cout << "   Attempting to use port: " << selected_port << std::endl;
        }
        std::cout << "   Found free port: " << selected_port << "\n";
        return selected_port;
    }
    static MockServer::port_t get_ecflow_udp_port() { return 3198; }

    EnableServersFixture(MockServer::port_t ecflow_server_port, MockServer::port_t ecflow_udp_port)
        : ecflow_server(ecflow_server_port),
          ecflow_udp(ecflow_udp_port, ecflow_server_port) {
        // Load 'reference' suite for tests...
        ecflow_server.load_definition("data/reference.def");
    }
};

} // namespace ecf::test

#endif
