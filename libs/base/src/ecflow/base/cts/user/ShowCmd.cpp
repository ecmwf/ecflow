/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/base/cts/user/ShowCmd.hpp"

#include <iostream>
#include <stdexcept>

#include "ecflow/base/AbstractClientEnv.hpp"
#include "ecflow/base/AuthenticationDetails.hpp"
#include "ecflow/base/AuthorisationDetails.hpp"
#include "ecflow/base/stc/PreAllocatedReply.hpp"

using namespace ecf;

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ShowCmd::equals(ClientToServerCmd* rhs) const {
    return (dynamic_cast<ShowCmd*>(rhs)) ? UserCmd::equals(rhs) : false;
}

ecf::authentication_t ShowCmd::authenticate(AbstractServer& server) const {
    return implementation::do_authenticate(*this, server);
}

ecf::authorisation_t ShowCmd::authorise(AbstractServer& server) const {
    return implementation::do_authorise(*this, server);
}

void ShowCmd::print(std::string& os) const {
    user_cmd(os, "show");
}
void ShowCmd::print_only(std::string& os) const {
    os += "show";
}

STC_Cmd_ptr ShowCmd::doHandleRequest(AbstractServer* as) const {
    /// Should never get called since, show command is only called on client side.
    return PreAllocatedReply::ok_cmd();
}

const char* ShowCmd::arg() {
    return "show";
}

void ShowCmd::addOption(boost::program_options::options_description& desc) const {
    desc.add_options()(ShowCmd::arg(), boost::program_options::value<std::string>()->implicit_value(std::string{}));
}
void ShowCmd::create(Cmd_ptr& cmd, boost::program_options::variables_map& vm, AbstractClientEnv* ac) const {
    std::string show_state = vm[ShowCmd::arg()].as<std::string>();

    if (ac->debug()) {
        std::cout << "  ShowCmd::create api = '" << show_state << "'.\n";
    }

    PrintStyle::Type_t style = PrintStyle::DEFS;
    if (!show_state.empty()) {
        if (show_state == "state") {
            style = PrintStyle::STATE;
        }
        else if (show_state == "migrate") {
            style = PrintStyle::MIGRATE;
        }
        else if (show_state == "defs") {
            style = PrintStyle::DEFS;
        }
        else {
            throw std::runtime_error(
                "ShowCmd::create invalid show option expected one of [ defs | state | migrate ] but found " +
                show_state);
        }
    }
    cmd = std::make_shared<ShowCmd>(style);
}

std::ostream& operator<<(std::ostream& os, const ShowCmd& c) {
    std::string ret;
    c.print(ret);
    os << ret;
    return os;
}

CEREAL_REGISTER_TYPE(ShowCmd)
CEREAL_REGISTER_DYNAMIC_INIT(ShowCmd)
