/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/base/cts/task/CtsWaitCmd.hpp"

#include <stdexcept>

#include "ecflow/base/AbstractClientEnv.hpp"
#include "ecflow/base/AbstractServer.hpp"
#include "ecflow/base/AuthenticationDetails.hpp"
#include "ecflow/base/AuthorisationDetails.hpp"
#include "ecflow/base/cts/task/TaskApi.hpp"
#include "ecflow/base/stc/PreAllocatedReply.hpp"
#include "ecflow/core/Environment.hpp"
#include "ecflow/core/Log.hpp"
#include "ecflow/core/Str.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Expression.hpp"
#include "ecflow/node/Submittable.hpp"
#include "ecflow/node/SuiteChanged.hpp"

using namespace ecf;
using namespace std;
using namespace boost;
namespace po = boost::program_options;

// #define DEBUG_ZOMBIE 1

//////////////////////////////////////////////////////////////////////////////////////////////////

CtsWaitCmd::CtsWaitCmd(const std::string& pathToTask,
                       const std::string& jobsPassword,
                       const std::string& process_or_remote_id,
                       int try_no,
                       const std::string& expression)
    : TaskCmd(pathToTask, jobsPassword, process_or_remote_id, try_no),
      expression_(expression) {
    // Parse expression to make sure its valid
    static_cast<void>(Expression::parse(expression, "CtsWaitCmd:")); // will throw for errors
}

void CtsWaitCmd::print(std::string& os) const {
    os += Str::CHILD_CMD();
    os += "wait ";
    os += expression_;
    os += " ";
    os += path_to_node();
}

bool CtsWaitCmd::equals(ClientToServerCmd* rhs) const {
    auto* the_rhs = dynamic_cast<CtsWaitCmd*>(rhs);
    if (!the_rhs) {
        return false;
    }
    if (expression_ != the_rhs->expression()) {
        return false;
    }
    return TaskCmd::equals(rhs);
}

ecf::authentication_t CtsWaitCmd::authenticate(AbstractServer& server) const {
    return implementation::do_authenticate(*this, server);
}

ecf::authorisation_t CtsWaitCmd::authorise(AbstractServer& server) const {
    return implementation::do_authorise(*this, server);
}

STC_Cmd_ptr CtsWaitCmd::doHandleRequest(AbstractServer* as) const {
    as->update_stats().task_wait_++;

    SuiteChanged1 changed(submittable_->suite());

    // Parse the expression, should not fail since client should have already check expression parses
    // The complete expression have been parsed and we have created the abstract syntax tree
    // We now need CHECK the AST for path nodes, event and meter. repeats,etc.
    // *** This will also set the Node pointers ***
    // If the expression references paths that don't exist throw an error
    // This can be captured in the ecf script, which should then abort the task
    // Otherwise we will end up blocking indefinitely
    std::unique_ptr<AstTop> ast =
        submittable_->parse_and_check_expressions(expression_, true, "CtsWaitCmd:"); // will throw for errors

    // Evaluate the expression
    if (ast->evaluate()) {

        submittable_->get_flag().clear(ecf::Flag::WAIT);

        // expression evaluates, return OK
        return PreAllocatedReply::ok_cmd();
    }

    submittable_->get_flag().set(ecf::Flag::WAIT);

    // Block/wait while expression is false
    return PreAllocatedReply::block_client_on_home_server_cmd();
}

const char* CtsWaitCmd::arg() {
    return TaskApi::waitArg();
}
const char* CtsWaitCmd::desc() {
    return "Evaluates an expression, and block while the expression is false.\n"
           "For use in the '.ecf' file *only*, hence the context is supplied via environment variables\n"
           "  arg1 = string(expression)\n\n"
           "Usage:\n"
           "  ecflow_client --wait=\"/suite/taskx == complete\"";
}

void CtsWaitCmd::addOption(boost::program_options::options_description& desc) const {
    desc.add_options()(CtsWaitCmd::arg(), po::value<string>(), CtsWaitCmd::desc());
}
void CtsWaitCmd::create(Cmd_ptr& cmd, boost::program_options::variables_map& vm, AbstractClientEnv* clientEnv) const {
    std::string expression = vm[arg()].as<std::string>();

    if (clientEnv->debug()) {
        cout << "  CtsWaitCmd::create " << CtsWaitCmd::arg() << " task_path(" << clientEnv->task_path() << ") password("
             << clientEnv->jobs_password() << ") remote_id(" << clientEnv->process_or_remote_id() << ") try_no("
             << clientEnv->task_try_no() << ") expression(" << expression << ")\n";
    }

    std::string errorMsg;
    if (!clientEnv->checkTaskPathAndPassword(errorMsg)) {
        throw std::runtime_error("CtsWaitCmd: " + errorMsg);
    }

    cmd = std::make_shared<CtsWaitCmd>(clientEnv->task_path(),
                                       clientEnv->jobs_password(),
                                       clientEnv->process_or_remote_id(),
                                       clientEnv->task_try_no(),
                                       expression);
}

std::ostream& operator<<(std::ostream& os, const CtsWaitCmd& c) {
    std::string ret;
    c.print(ret);
    os << ret;
    return os;
}

CEREAL_REGISTER_TYPE(CtsWaitCmd)
CEREAL_REGISTER_DYNAMIC_INIT(CtsWaitCmd)
