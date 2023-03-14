/*
 * Copyright 2009-2023 ECMWF.
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

#include <boost/asio.hpp>
#include <boost/filesystem.hpp>
#include <boost/process.hpp>
#include <boost/test/unit_test.hpp>

#include "ClientInvoker.hpp"
#include "Defs.hpp"
#include "File.hpp"
#include "Host.hpp"
#include "Node.hpp"
#include "NodeAttr.hpp"
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
        ClientInvoker client(host(), port());
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
        ClientInvoker client(host(), port());

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

        std::string invoke_command = ecf::File::root_build_dir() + "/bin/ecflow_server";
        invoke_command += " --port ";
        invoke_command += std::to_string(port);
        invoke_command += " -d &";

        bp::child child(invoke_command);

        ClientInvoker client(host, port);
        if (!client.wait_for_server_reply(1)) {
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
        auto request = format_request(path, "label", name, value);
        send(request);
    }

    void update_meter(const std::string& path, const std::string& name, int value) {
        auto request = format_request(path, "meter", name, value);
        send(request);
    }

    void clear_event(const std::string& path, const std::string& name) {
        auto request = format_request(path, "event", name, "clear");
        send(request);
    }
    void set_event(const std::string& path, const std::string& name) {
        auto request = format_request(path, "event", name, "set");
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
        std::ostringstream os;
        os << R"-({"method": "put", "payload": {"command": ")-" << command << R"-(", "path": ")-" << path << R"-(", "name": ")-"
           << name << R"-(", "value": ")-" << value << R"-("}})-";
        return os.str();
    }

    static void sendRequest(uint16_t port, const std::string& request) {
        ecf::UDPClient client("localhost", std::to_string(port));
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
    EnableServersFixture() : ecflow_server(42424), ecflow_udp(42425, 42424) {
        // Load 'reference' suite for tests...
        ecflow_server.load_definition("data/reference.def");
    }
    ~EnableServersFixture() = default;

    ecf::test::MockServer ecflow_server;
    ecf::test::MockUDPServer ecflow_udp;
};

} // namespace ecf::test

#endif
