/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/base/cts/task/InitCmd.hpp"

#include <stdexcept>

#include <cereal/cereal.hpp>

#include "ecflow/base/AbstractClientEnv.hpp"
#include "ecflow/base/AbstractServer.hpp"
#include "ecflow/base/AuthenticationDetails.hpp"
#include "ecflow/base/AuthorisationDetails.hpp"
#include "ecflow/base/cts/task/TaskApi.hpp"
#include "ecflow/base/stc/PreAllocatedReply.hpp"
#include "ecflow/core/Str.hpp"
#include "ecflow/node/Submittable.hpp"
#include "ecflow/node/SuiteChanged.hpp"

using namespace ecf;
using namespace std;
using namespace boost;
namespace po = boost::program_options;

void InitCmd::print(std::string& os) const {
    os += Str::CHILD_CMD();
    os += "init ";
    os += path_to_node();
    if (!var_to_add_.empty()) {
        os += " --add";
        for (const auto& var_to_add : var_to_add_) {
            os += " ";
            os += var_to_add.name();
            os += "=";
            os += var_to_add.theValue();
        }
    }
}

bool InitCmd::equals(ClientToServerCmd* rhs) const {
    auto* the_rhs = dynamic_cast<InitCmd*>(rhs);
    if (!the_rhs)
        return false;
    if (var_to_add_ != the_rhs->variables_to_add())
        return false;
    return TaskCmd::equals(rhs);
}

ecf::authentication_t InitCmd::authenticate(AbstractServer& server) const {
    return implementation::do_authenticate(*this, server);
}

ecf::authorisation_t InitCmd::authorise(AbstractServer& server) const {
    return implementation::do_authorise(*this, server);
}

STC_Cmd_ptr InitCmd::doHandleRequest(AbstractServer* as) const {
    as->update_stats().task_init_++;

    { // update suite change numbers before job submission. submittable_ setup during authentication
        SuiteChanged1 changed(submittable_->suite());
        submittable_->init(process_or_remote_id()); // will set task->set_state(NState::ACTIVE);

        for (const auto& var_to_add : var_to_add_) {
            submittable_->addVariable(var_to_add); // will update or add variable
        }
    }

    // Do job submission in case any triggers dependent on NState::ACTIVE
    as->increment_job_generation_count();
    return PreAllocatedReply::ok_cmd();
}

const char* InitCmd::arg() {
    return TaskApi::initArg();
}
const char* InitCmd::desc() {
    return "Mark task as started(active). For use in the '.ecf' script file *only*\n"
           "Hence the context is supplied via environment variables.\n"
           "  arg1(string)         = process_or_remote_id The process id of the job or remote_id\n"
           "                         Using remote id allows the jobs to be killed\n"
           "  arg2(--add)(optional)= add/update variables as name value pairs\n\n"
           "If this child command is a zombie, then the default action will be to *block*.\n"
           "The default can be overridden by using zombie attributes.\n"
           "Otherwise the blocking period is defined by ECF_TIMEOUT.\n\n"
           "Usage:\n"
           "  ecflow_client --init=$$\n"
           "  ecflow_client --init=$$ --add name=value name2=value2 # add/update variables to task";
}

void InitCmd::addOption(boost::program_options::options_description& desc) const {
    desc.add_options()(InitCmd::arg(), po::value<string>(), InitCmd::desc())(
        "add",
        po::value<vector<string>>()->multitoken(),
        "Add variables e.g. name1=value1 name2=value2. Can only be used in combination with --init command.");
}

void InitCmd::create(Cmd_ptr& cmd, boost::program_options::variables_map& vm, AbstractClientEnv* clientEnv) const {
    std::string process_or_remote_id = vm[arg()].as<std::string>();

    if (clientEnv->debug())
        cout << "  InitCmd::create " << InitCmd::arg() << "  clientEnv->task_path(" << clientEnv->task_path()
             << ") clientEnv->jobs_password(" << clientEnv->jobs_password() << ") clientEnv->process_or_remote_id("
             << clientEnv->process_or_remote_id() << ") clientEnv->task_try_no(" << clientEnv->task_try_no()
             << ") process_or_remote_id(" << process_or_remote_id << ") clientEnv->under_test("
             << clientEnv->under_test() << ")\n";

    std::string errorMsg;
    if (!clientEnv->checkTaskPathAndPassword(errorMsg)) {
        throw std::runtime_error("InitCmd: " + errorMsg);
    }

    /// if ECF_RID is specified then it *MUST* be the same as input argument
    /// On cca we ECF_RID can be specified under test, and therefore fail this check, hence we use
    /// clientEnv->under_test()
    if (!clientEnv->under_test() && !clientEnv->process_or_remote_id().empty() &&
        clientEnv->process_or_remote_id() != process_or_remote_id) {
        std::stringstream ss;
        ss << "remote id(" << process_or_remote_id
           << ") passed as an argument, not the same the client environment ECF_RID("
           << clientEnv->process_or_remote_id() << ")";
        throw std::runtime_error(ss.str());
    }

    std::vector<Variable> variable_vec;
    if (vm.count("add")) {
        vector<string> var_args = vm["add"].as<vector<string>>();
        if (!var_args.empty()) {
            variable_vec.reserve(var_args.size());
            for (const auto& v : var_args) {
                std::vector<std::string> tokens;
                Str::split(v, tokens, "=");
                if (tokens.size() != 2) {
                    throw std::runtime_error(
                        "Could not parse variable provided to --add; Expected  var1=value1 var2=value2 but found " + v);
                }
                variable_vec.emplace_back(tokens[0], tokens[1]);
            }
        }
    }

    cmd = std::make_shared<InitCmd>(clientEnv->task_path(),
                                    clientEnv->jobs_password(),
                                    process_or_remote_id,
                                    clientEnv->task_try_no(),
                                    variable_vec);
}

std::ostream& operator<<(std::ostream& os, const InitCmd& c) {
    std::string ret;
    c.print(ret);
    os << ret;
    return os;
}

CEREAL_REGISTER_TYPE(InitCmd)
CEREAL_REGISTER_DYNAMIC_INIT(InitCmd)
