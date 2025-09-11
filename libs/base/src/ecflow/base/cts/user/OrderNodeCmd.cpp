/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/base/cts/user/OrderNodeCmd.hpp"

#include <stdexcept>

#include "ecflow/base/AbstractClientEnv.hpp"
#include "ecflow/base/AbstractServer.hpp"
#include "ecflow/base/AuthenticationDetails.hpp"
#include "ecflow/base/AuthorisationDetails.hpp"
#include "ecflow/base/cts/user/CtsApi.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Node.hpp"

using namespace ecf;
using namespace std;
using namespace boost;
namespace po = boost::program_options;

bool OrderNodeCmd::equals(ClientToServerCmd* rhs) const {
    auto* the_rhs = dynamic_cast<OrderNodeCmd*>(rhs);
    if (!the_rhs) {
        return false;
    }
    if (absNodepath_ != the_rhs->pathToNode()) {
        return false;
    }
    if (option_ != the_rhs->option()) {
        return false;
    }
    return UserCmd::equals(rhs);
}

ecf::authentication_t OrderNodeCmd::authenticate(AbstractServer& server) const {
    return implementation::do_authenticate(*this, server);
}

ecf::authorisation_t OrderNodeCmd::authorise(AbstractServer& server) const {
    return implementation::do_authorise(*this, server);
}

void OrderNodeCmd::print(std::string& os) const {
    user_cmd(os, CtsApi::to_string(CtsApi::order(absNodepath_, NOrder::toString(option_))));
}
void OrderNodeCmd::print_only(std::string& os) const {
    os += CtsApi::to_string(CtsApi::order(absNodepath_, NOrder::toString(option_)));
}

STC_Cmd_ptr OrderNodeCmd::doHandleRequest(AbstractServer* as) const {
    assert(isWrite()); // isWrite used in handleRequest() to control check pointing

    as->update_stats().order_node_++;

    Defs* defs       = as->defs().get();
    node_ptr theNode = find_node_for_edit(defs, absNodepath_);

    Node* theParent = theNode->parent();
    if (theParent) {
        theParent->order(theNode.get(), option_);
    }
    else {
        defs->order(theNode.get(), option_);
    }

    return doJobSubmission(as);
}

// bool OrderNodeCmd::authenticate(AbstractServer* as, STC_Cmd_ptr& cmd) const {
//     return do_authenticate(as, cmd, absNodepath_);
// }

const char* OrderNodeCmd::arg() {
    return CtsApi::orderArg();
}
const char* OrderNodeCmd::desc() {
    return "Re-orders the nodes held by the server\n"
           "  arg1 = node path\n"
           "  arg2 = [ top | bottom | alpha | order | up | down | runtime]\n"
           "It should be noted that in the absence of triggers and time/date dependencies,\n"
           "the tasks are submitted in order.\n"
           "This changes the order and hence affects the submission order::\n\n"
           "   o top     raises the node within its parent, so that it is first\n"
           "   o bottom  lowers the node within its parent, so that it is last\n"
           "   o alpha   Arranges for all the peers of selected note to be sorted alphabetically (case-insensitive)\n"
           "   o order   Arranges for all the peers of selected note to be sorted in reverse "
           "alphabet(case-insensitive)\n"
           "   o up      Moves the selected node up one place amongst its peers\n"
           "   o down    Moves the selected node down one place amongst its peers\n\n"
           "   o runtime Orders the nodes according to state change runtime\n"
           "             for families by accumulated runtime of its children\n"
           "             useful to submit the task that take longer earlier\n\n"
           "This command can fail because:\n"
           "- The node path does not exist in the server\n"
           "- The order_type is not does not match one of arg2\n"
           "Usage:\n"
           "  --order=/suite/f1 top  # move node f1 to the top";
}

void OrderNodeCmd::addOption(boost::program_options::options_description& desc) const {
    desc.add_options()(OrderNodeCmd::arg(), po::value<vector<string>>()->multitoken(), OrderNodeCmd::desc());
}
void OrderNodeCmd::create(Cmd_ptr& cmd, boost::program_options::variables_map& vm, AbstractClientEnv* ac) const {
    vector<string> args = vm[OrderNodeCmd::arg()].as<vector<string>>();

    if (ac->debug()) {
        dumpVecArgs(OrderNodeCmd::arg(), args);
    }

    if (args.size() != 2) {
        std::stringstream ss;
        ss << "OrderNodeCmd: Two arguments expected. Please specify one of:\n";
        ss << OrderNodeCmd::arg() << " pathToNode top\n";
        ss << OrderNodeCmd::arg() << " pathToNode bottom\n";
        ss << OrderNodeCmd::arg() << " pathToNode alpha\n";
        ss << OrderNodeCmd::arg() << " pathToNode order\n";
        ss << OrderNodeCmd::arg() << " pathToNode up\n";
        ss << OrderNodeCmd::arg() << " pathToNode down\n";
        ss << OrderNodeCmd::arg() << " pathToNode runtime\n";
        throw std::runtime_error(ss.str());
    }

    if (!NOrder::isValid(args[1])) {
        throw std::runtime_error("OrderNodeCmd: Invalid second option: please specify one of [ top, bottom, alpha, "
                                 "order, up, down, runtime]\n");
    }

    cmd = std::make_shared<OrderNodeCmd>(args[0], NOrder::toOrder(args[1]));
}

std::ostream& operator<<(std::ostream& os, const OrderNodeCmd& c) {
    std::string ret;
    c.print(ret);
    os << ret;
    return os;
}

CEREAL_REGISTER_TYPE(OrderNodeCmd)
CEREAL_REGISTER_DYNAMIC_INIT(OrderNodeCmd)
