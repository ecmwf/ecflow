/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/base/cts/task/LabelCmd.hpp"

#include <stdexcept>

#include "ecflow/base/AbstractClientEnv.hpp"
#include "ecflow/base/AbstractServer.hpp"
#include "ecflow/base/AuthenticationDetails.hpp"
#include "ecflow/base/AuthorisationDetails.hpp"
#include "ecflow/base/cts/task/TaskApi.hpp"
#include "ecflow/base/stc/PreAllocatedReply.hpp"
#include "ecflow/core/Log.hpp"
#include "ecflow/core/Str.hpp"
#include "ecflow/node/Submittable.hpp"
#include "ecflow/node/SuiteChanged.hpp"

using namespace ecf;
using namespace std;
using namespace boost;
namespace po = boost::program_options;

bool LabelCmd::equals(ClientToServerCmd* rhs) const {
    auto* the_rhs = dynamic_cast<LabelCmd*>(rhs);
    if (!the_rhs)
        return false;
    if (name_ != the_rhs->name())
        return false;
    if (label_ != the_rhs->label())
        return false;
    return TaskCmd::equals(rhs);
}

ecf::authentication_t LabelCmd::authenticate(AbstractServer& server) const {
    return implementation::do_authenticate(*this, server);
}

ecf::authorisation_t LabelCmd::authorise(AbstractServer& server) const {
    return implementation::do_authorise(*this, server);
}

void LabelCmd::print(std::string& os) const {
    os += Str::CHILD_CMD();
    os += "label ";
    os += name_;
    os += " '";
    os += label_;
    os += "' ";
    os += path_to_node();
}

STC_Cmd_ptr LabelCmd::doHandleRequest(AbstractServer* as) const {
    as->update_stats().task_label_++;

    assert(isWrite()); // isWrite used in handleRequest() to control check pointing

    // submittable_ setup during authentication
    if (submittable_->findLabel(name_)) {

        SuiteChanged1 changed(submittable_->suite());
        submittable_->changeLabel(name_, label_);
    }
    // else {
    //   // ECFLOW-175, avoid filling up log file. Can get thousands of these messages, especially form MARS
    //   std::string ss;
    //   ss = "Label request failed as label '"; ss += name_; ss += "' does not exist on task "; ss += path_to_node();
    //	  ecf::log(Log::ERR,ss);
    //}

    // Note: reclaiming memory for label_ earlier make *no* difference to performance of server

    return PreAllocatedReply::ok_cmd();
}

const char* LabelCmd::arg() {
    return TaskApi::labelArg();
}
const char* LabelCmd::desc() {
    return "Change Label. For use in the '.ecf' script file *only*\n"
           "Hence the context is supplied via environment variables\n"
           "  arg1 = label-name\n"
           "  arg2 = The new label value\n"
           "         The labels values can be single or multi-line(space separated quoted strings)\n\n"
           "If this child command is a zombie, then the default action will be to *fob*,\n"
           "i.e allow the ecflow client command to complete without an error\n"
           "The default can be overridden by using zombie attributes.\n\n"
           "Usage:\n"
           "  ecflow_client --label=progressed merlin";
}

void LabelCmd::addOption(boost::program_options::options_description& desc) const {
    desc.add_options()(LabelCmd::arg(), po::value<vector<string>>()->multitoken(), LabelCmd::desc());
}
void LabelCmd::create(Cmd_ptr& cmd, boost::program_options::variables_map& vm, AbstractClientEnv* clientEnv) const {
    vector<string> args = vm[arg()].as<vector<string>>();

    if (clientEnv->debug()) {
        dumpVecArgs(LabelCmd::arg(), args);
        cout << "  LabelCmd::create " << LabelCmd::arg() << " task_path(" << clientEnv->task_path() << ") password("
             << clientEnv->jobs_password() << ") remote_id(" << clientEnv->process_or_remote_id() << ") try_no("
             << clientEnv->task_try_no() << ")\n";
    }

    if (args.size() < 2) {
        std::stringstream ss;
        ss << "LabelCmd: At least 2 arguments expected. Please specify: <label-name> <label-value>\n";
        throw std::runtime_error(ss.str());
    }

    std::string labelName = args[0];
    args.erase(args.begin()); // remove name from vector of strings
    std::string labelValue;
    for (size_t i = 0; i < args.size(); i++) {
        if (i != 0)
            labelValue += " ";
        labelValue += args[i];
    }

    std::string errorMsg;
    if (!clientEnv->checkTaskPathAndPassword(errorMsg)) {
        throw std::runtime_error("LabelCmd: " + errorMsg);
    }

    cmd = std::make_shared<LabelCmd>(clientEnv->task_path(),
                                     clientEnv->jobs_password(),
                                     clientEnv->process_or_remote_id(),
                                     clientEnv->task_try_no(),
                                     labelName,
                                     labelValue);
}

std::ostream& operator<<(std::ostream& os, const LabelCmd& c) {
    std::string ret;
    c.print(ret);
    os << ret;
    return os;
}

CEREAL_REGISTER_TYPE(LabelCmd)
CEREAL_REGISTER_DYNAMIC_INIT(LabelCmd)
