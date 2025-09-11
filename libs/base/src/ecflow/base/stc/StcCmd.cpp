/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/base/stc/StcCmd.hpp"

#include <iostream>

#include "ecflow/base/cts/ClientToServerCmd.hpp"

std::string StcCmd::print() const {
    switch (api_) {
        case StcCmd::OK:
            return "cmd:Ok";
        case StcCmd::BLOCK_CLIENT_SERVER_HALTED:
            return "cmd:Server_halted";
        case StcCmd::BLOCK_CLIENT_ON_HOME_SERVER:
            return "cmd:Wait";
        case StcCmd::DELETE_ALL:
            return "cmd:delete_all";
        case StcCmd::END_OF_FILE:
            return "cmd:end_of_file";
        case StcCmd::INVALID_ARGUMENT:
            return "cmd:Invalid_argument";
        default:
            assert(false);
            break;
    }
    assert(false); // unknown command
    return "cmd:Unknown??";
}

// Client context
bool StcCmd::handle_server_response(ServerReply& server_reply, Cmd_ptr cts_cmd, bool debug) const {
    bool ret = false;
    switch (api_) {
        case StcCmd::OK: {
            if (debug) {
                std::cout << "  StcCmd::handle_server_response OK\n";
            }
            ret = true;
            break;
        }
        case StcCmd::BLOCK_CLIENT_SERVER_HALTED: {
            if (debug) {
                std::cout << "  StcCmd::handle_server_response BLOCK_CLIENT_SERVER_HALTED\n";
            }
            server_reply.set_block_client_server_halted(); // requires further work, by ClientInvoker
            break;
        }
        case StcCmd::BLOCK_CLIENT_ON_HOME_SERVER: {
            if (debug) {
                std::cout << "  StcCmd::handle_server_response BLOCK_CLIENT_ON_HOME_SERVER\n";
            }
            server_reply.set_block_client_on_home_server(); // requires further work, by ClientInvoker
            break;
        }
        case StcCmd::DELETE_ALL: {
            if (debug) {
                std::cout << "  StcCmd::handle_server_response DELETE_ALL\n";
            }
            server_reply.set_client_defs(defs_ptr());
            server_reply.set_client_node(node_ptr());
            server_reply.set_client_handle(0);
            ret = true;
            break;
        }
        case StcCmd::END_OF_FILE: {
            if (debug) {
                std::cout << "  StcCmd::handle_server_response END_OF_FILE\n";
            }
            server_reply.set_eof(); // requires further work, by ClientInvoker
            std::string ss;
            ss += "Error: request( ";
            ss += cts_cmd->print_short();
            ss += " ) failed! Server replied with: EOF(Server did not reply or mixing ssl and non-ssl)\n";
            server_reply.set_error_msg(ss);
            break;
        }
        case StcCmd::INVALID_ARGUMENT: {
            // This is created on the client side, after detecting a INVALID_ARGUMENT reply from the server
            // This keeps compatibility with 4 servers
            if (debug) {
                std::cout << "  StcCmd::handle_server_response INVALID_ARGUMENT\n";
            }
            server_reply.set_invalid_argument(); // requires further work, by ClientInvoker
            std::string ss;
            ss += "Error: request( ";
            ss += cts_cmd->print_short();
            ss += " ) failed! Server replied with: invalid_argument(Could not decode client protocol)\n";
            server_reply.set_error_msg(ss);
            break;
        }
        default:
            assert(false);
            break;
    }
    return ret;
}

bool StcCmd::equals(ServerToClientCmd* rhs) const {
    auto* the_rhs = dynamic_cast<StcCmd*>(rhs);
    if (!the_rhs) {
        return false;
    }
    if (api_ != the_rhs->api()) {
        return false;
    }
    return ServerToClientCmd::equals(rhs);
}

std::ostream& operator<<(std::ostream& os, const StcCmd& c) {
    os << c.print();
    return os;
}
