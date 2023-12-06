/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/base/cts/task/AbortCmd.hpp"

#include <stdexcept>

#include "ecflow/attribute/QueueAttr.hpp"
#include "ecflow/base/AbstractClientEnv.hpp"
#include "ecflow/base/AbstractServer.hpp"
#include "ecflow/base/cts/ClientToServerCmd.hpp"
#include "ecflow/base/cts/task/TaskApi.hpp"
#include "ecflow/base/stc/PreAllocatedReply.hpp"
#include "ecflow/core/Log.hpp"
#include "ecflow/core/Str.hpp"
#include "ecflow/node/Expression.hpp"
#include "ecflow/node/Submittable.hpp"
#include "ecflow/node/SuiteChanged.hpp"

using namespace ecf;
using namespace std;
using namespace boost;
namespace po = boost::program_options;

AbortCmd::AbortCmd(const std::string& pathToTask,
                   const std::string& jobsPassword,
                   const std::string& process_or_remote_id,
                   int try_no,
                   const std::string& reason)
    : TaskCmd(pathToTask, jobsPassword, process_or_remote_id, try_no),
      reason_(reason) {
    if (!reason_.empty()) {
        // Do not use "\n" | ';' in Submittable::abr_, as this can mess up, --migrate output
        // Which would then affect --load.
        Str::replace(reason_, "\n", "");
        Str::replace(reason_, ";", " ");
    }
}

void AbortCmd::print(std::string& os) const {
    os += Str::CHILD_CMD();
    os += "abort ";
    os += path_to_node();
    os += "  ";
    os += reason_;
}

bool AbortCmd::equals(ClientToServerCmd* rhs) const {
    auto* the_rhs = dynamic_cast<AbortCmd*>(rhs);
    if (!the_rhs)
        return false;
    if (reason_ != the_rhs->reason())
        return false;
    return TaskCmd::equals(rhs);
}

STC_Cmd_ptr AbortCmd::doHandleRequest(AbstractServer* as) const {
    as->update_stats().task_abort_++;

    assert(isWrite()); // isWrite used in handleRequest() to control check pointing

    {
        /// If there is an associated zombie, remove from the list
        as->zombie_ctrl().remove(submittable_);

        // update suite change numbers before job submission, submittable_ setup during authentication
        SuiteChanged1 changed(submittable_->suite());

        string theReason = reason_;
        if (theReason.empty())
            theReason = "Trap raised in job file";

        submittable_->aborted(theReason); // will set task->set_state(NState::ABORTED);
    }

    // Do job submission in case any triggers dependent on NState::ABORTED
    // If task try number is less than ECF_TRIES we attempt to re-submit the job.(ie if still in limit)
    as->increment_job_generation_count();
    return PreAllocatedReply::ok_cmd();
}

const char* AbortCmd::arg() {
    return TaskApi::abortArg();
}
const char* AbortCmd::desc() {
    return "Mark task as aborted. For use in the '.ecf' script file *only*\n"
           "Hence the context is supplied via environment variables\n"
           "  arg1 = (optional) string(reason)\n"
           "         Optionally provide a reason why the abort was raised\n\n"
           "If this child command is a zombie, then the default action will be to *block*.\n"
           "The default can be overridden by using zombie attributes.\n"
           "Otherwise the blocking period is defined by ECF_TIMEOUT.\n\n"
           "Usage:\n"
           "  ecflow_client --abort=reasonX";
}

void AbortCmd::addOption(boost::program_options::options_description& desc) const {
    desc.add_options()(AbortCmd::arg(), po::value<string>()->implicit_value(string()), AbortCmd::desc());
}
void AbortCmd::create(Cmd_ptr& cmd, boost::program_options::variables_map& vm, AbstractClientEnv* clientEnv) const {
    std::string reason = vm[arg()].as<std::string>();

    if (clientEnv->debug())
        cout << "  AbortCmd::create " << AbortCmd::arg() << " task_path(" << clientEnv->task_path() << ") password("
             << clientEnv->jobs_password() << ") remote_id(" << clientEnv->process_or_remote_id() << ") try_no("
             << clientEnv->task_try_no() << ") reason(" << reason << ")\n";

    std::string errorMsg;
    if (!clientEnv->checkTaskPathAndPassword(errorMsg)) {
        throw std::runtime_error("AbortCmd: " + errorMsg);
    }

    cmd = std::make_shared<AbortCmd>(clientEnv->task_path(),
                                     clientEnv->jobs_password(),
                                     clientEnv->process_or_remote_id(),
                                     clientEnv->task_try_no(),
                                     reason);
}

std::ostream& operator<<(std::ostream& os, const AbortCmd& c) {
    std::string ret;
    c.print(ret);
    os << ret;
    return os;
}

CEREAL_REGISTER_TYPE(AbortCmd)
CEREAL_REGISTER_DYNAMIC_INIT(AbortCmd)
