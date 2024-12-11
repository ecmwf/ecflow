/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_base_test_TestHelper_HPP
#define ecflow_base_test_TestHelper_HPP

#include <boost/test/test_tools.hpp>

#include "MockServer.hpp"
#include "ecflow/base/ClientToServerRequest.hpp"
#include "ecflow/base/cts/ClientToServerCmd.hpp"
#include "ecflow/base/stc/ServerToClientCmd.hpp"
#include "ecflow/node/Node.hpp"

// defines statics utility functions used by more than one test
class TestHelper {
public:
    static std::string invokeRequest(Defs* defs, Cmd_ptr theCmd, bool check_change_numbers = true) {

        // Check that state change happens
        unsigned int state_change_no  = Ecf::state_change_no();
        unsigned int modify_change_no = Ecf::modify_change_no();

        // Setup command for invocation
        ClientToServerRequest cmd_request;
        cmd_request.set_cmd(theCmd);
        // std::cout << cmd_request << "\n";

        MockServer server(defs);
        STC_Cmd_ptr result = cmd_request.handleRequest(&server);
        BOOST_CHECK_MESSAGE(result, "ClientToServerRequest " << cmd_request << " returned NULL\n");
        BOOST_CHECK_MESSAGE(result->error().empty(), cmd_request << " " << result->error());
        if (check_change_numbers) {
            BOOST_CHECK_MESSAGE(state_change_no != Ecf::state_change_no() ||
                                    modify_change_no != Ecf::modify_change_no(),
                                "State & modify change numbers unaltered by command " << cmd_request);

            // Some tests(TestSSyncCmd_CH1.cpp), create defs where invariants like change numbers are deliberately
            // different
            std::string error_msg;
            BOOST_CHECK_MESSAGE(defs->checkInvariants(error_msg),
                                "invokeRequest checkInvariants failed " << error_msg << " for cmd " << cmd_request);
        }
        if (result)
            return result->get_string();
        return std::string();
    }

    static std::string invokeRequest(AbstractServer* server, Cmd_ptr theCmd, bool check_change_numbers = true) {

        // Check that state change happens
        unsigned int state_change_no  = Ecf::state_change_no();
        unsigned int modify_change_no = Ecf::modify_change_no();

        // Setup command for invocation
        ClientToServerRequest cmd_request;
        cmd_request.set_cmd(theCmd);
        // std::cout << cmd_request << "\n";

        STC_Cmd_ptr result = cmd_request.handleRequest(server);
        BOOST_CHECK_MESSAGE(result, "ClientToServerRequest " << cmd_request << " returned NULL\n");
        BOOST_CHECK_MESSAGE(result->error().empty(), cmd_request << " " << result->error());
        if (check_change_numbers) {
            BOOST_CHECK_MESSAGE(state_change_no != Ecf::state_change_no() ||
                                    modify_change_no != Ecf::modify_change_no(),
                                "State & modify change numbers unaltered by command " << cmd_request);

            // Some tests(TestSSyncCmd_CH1.cpp), create defs where invariants like change numbers are deliberately
            // different
            std::string error_msg;
            BOOST_CHECK_MESSAGE(server->defs()->checkInvariants(error_msg),
                                "invokeRequest checkInvariants failed " << error_msg << " for cmd " << cmd_request);
        }
        if (result)
            return result->get_string();
        return std::string();
    }

    static void invokeFailureRequest(Defs* defs, Cmd_ptr theCmd) {

        ClientToServerRequest cmd_request;
        cmd_request.set_cmd(theCmd);

        MockServer server(defs);

        // We expect this to fail
        BOOST_CHECK_THROW(cmd_request.handleRequest(&server), std::runtime_error);
    }

    static void invokeFailureRequest(MockServer& server, Cmd_ptr theCmd) {

        ClientToServerRequest cmd_request;
        cmd_request.set_cmd(theCmd);

        BOOST_CHECK_THROW(cmd_request.handleRequest(&server), std::runtime_error);

        //		STC_Cmd_ptr result = cmd_request.handleRequest(&server);
        //		BOOST_CHECK_MESSAGE( !result->ok(), "ClientToServerRequest " << cmd_request << " was expected to
        // fail \n");
    }

    static void test_state(node_ptr n, NState::State expected) {
        BOOST_REQUIRE_MESSAGE(n->state() == expected,
                              "Expected state " << NState::toString(expected) << " but found "
                                                << NState::toString(n->state()) << " for " << n->debugNodePath());
    }

    static void test_state(node_ptr n, DState::State expected) {
        BOOST_REQUIRE_MESSAGE(n->dstate() == expected,
                              "Expected state " << DState::toString(expected) << " but found "
                                                << DState::toString(n->dstate()) << " for " << n->debugNodePath());
    }

private:
    TestHelper() = default;
};

#endif /* ecflow_base_test_TestHelper_HPP */
