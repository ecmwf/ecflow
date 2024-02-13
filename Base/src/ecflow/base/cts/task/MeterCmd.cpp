/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/base/cts/task/MeterCmd.hpp"

#include <stdexcept>

#include "ecflow/base/AbstractClientEnv.hpp"
#include "ecflow/base/AbstractServer.hpp"
#include "ecflow/base/cts/task/TaskApi.hpp"
#include "ecflow/base/stc/PreAllocatedReply.hpp"
#include "ecflow/core/Converter.hpp"
#include "ecflow/core/Log.hpp"
#include "ecflow/core/Str.hpp"
#include "ecflow/node/Submittable.hpp"
#include "ecflow/node/SuiteChanged.hpp"

using namespace ecf;
using namespace std;
using namespace boost;
namespace po = boost::program_options;

bool MeterCmd::equals(ClientToServerCmd* rhs) const {
    auto* the_rhs = dynamic_cast<MeterCmd*>(rhs);
    if (!the_rhs)
        return false;
    if (name_ != the_rhs->name())
        return false;
    if (value_ != the_rhs->value())
        return false;
    return TaskCmd::equals(rhs);
}

void MeterCmd::print(std::string& os) const {
    os += Str::CHILD_CMD();
    os += "meter ";
    os += name_;
    os += " ";
    os += ecf::convert_to<std::string>(value_);
    os += " ";
    os += path_to_node();
}

STC_Cmd_ptr MeterCmd::doHandleRequest(AbstractServer* as) const {
    as->update_stats().task_meter_++;

    { // Added scope for SuiteChanged1 changed: i.e update suite change numbers before job submission
        // submittable_ setup during authentication
        SuiteChanged1 changed(submittable_->suite());

        /// Allow meter to set any valid value that is in range because:
        ///   - When we have a network failure, and restoration. The meter tasks, will come in random, order.
        ///   - When task is executed without a requee the meter value will less than maximum
        ///
        /// This has *IMPLICATION*, if the meter is used in a trigger, using a equality
        /// operator, then the trigger will always hold.  hence suite designers need to
        /// aware of this.
        try {

            Meter& the_meter = submittable_->find_meter(name_);
            if (the_meter.empty()) {
                LOG(Log::ERR,
                    "MeterCmd::doHandleRequest: failed as meter '" << name_ << "' does not exist on task "
                                                                   << path_to_node());
                return PreAllocatedReply::ok_cmd();
            }

            /// Invalid meter values(out or range) will raise exceptions.
            /// Just ignore the request rather than failing client cmd
            the_meter.set_value(value_);
        }
        catch (std::exception& e) {
            LOG(Log::ERR, "MeterCmd::doHandleRequest: failed for task " << path_to_node() << ". " << e.what());
            return PreAllocatedReply::ok_cmd();
        }
    }

    // Do job submission in case any triggers dependent on meters
    as->increment_job_generation_count();
    return PreAllocatedReply::ok_cmd();
}

const char* MeterCmd::arg() {
    return TaskApi::meterArg();
}
const char* MeterCmd::desc() {
    return "Change meter. For use in the '.ecf' script file *only*\n"
           "Hence the context is supplied via environment variables\n"
           "  arg1(string) = meter-name\n"
           "  arg2(int)    = the new meter value\n\n"
           "If this child command is a zombie, then the default action will be to *fob*,\n"
           "i.e allow the ecflow client command to complete without an error\n"
           "The default can be overridden by using zombie attributes.\n\n"
           "Usage:\n"
           "  ecflow_client --meter=my_meter 20";
}

void MeterCmd::addOption(boost::program_options::options_description& desc) const {
    desc.add_options()(MeterCmd::arg(), po::value<vector<string>>()->multitoken(), MeterCmd::desc());
}
void MeterCmd::create(Cmd_ptr& cmd, boost::program_options::variables_map& vm, AbstractClientEnv* clientEnv) const {
    vector<string> args = vm[arg()].as<vector<string>>();

    if (clientEnv->debug()) {
        dumpVecArgs(MeterCmd::arg(), args);
        cout << "  MeterCmd::create " << MeterCmd::arg() << " task_path(" << clientEnv->task_path() << ") password("
             << clientEnv->jobs_password() << ") remote_id(" << clientEnv->process_or_remote_id() << ") try_no("
             << clientEnv->task_try_no() << ")\n";
    }

    if (args.size() != 2) {
        std::stringstream ss;
        ss << "MeterCmd: Two arguments expected, found " << args.size()
           << " Please specify <meter-name> <meter-value>, ie --meter=name 100\n";
        throw std::runtime_error(ss.str());
    }

    int value = 0;
    try {
        std::string strVal = args[1];
        value              = ecf::convert_to<int>(strVal);
    }
    catch (const ecf::bad_conversion&) {
        throw std::runtime_error("MeterCmd: Second argument must be a integer, i.e. --meter=name 100\n");
    }

    std::string errorMsg;
    if (!clientEnv->checkTaskPathAndPassword(errorMsg)) {
        throw std::runtime_error("MeterCmd: " + errorMsg);
    }

    cmd = std::make_shared<MeterCmd>(clientEnv->task_path(),
                                     clientEnv->jobs_password(),
                                     clientEnv->process_or_remote_id(),
                                     clientEnv->task_try_no(),
                                     args[0],
                                     value);
}

std::ostream& operator<<(std::ostream& os, const MeterCmd& c) {
    std::string ret;
    c.print(ret);
    os << ret;
    return os;
}

CEREAL_REGISTER_TYPE(MeterCmd)
CEREAL_REGISTER_DYNAMIC_INIT(MeterCmd)
