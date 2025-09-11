/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/base/cts/task/EventCmd.hpp"

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

bool EventCmd::equals(ClientToServerCmd* rhs) const {
    auto* the_rhs = dynamic_cast<EventCmd*>(rhs);
    if (!the_rhs) {
        return false;
    }
    if (name_ != the_rhs->name()) {
        return false;
    }
    if (value_ != the_rhs->value()) {
        return false;
    }
    return TaskCmd::equals(rhs);
}

ecf::authentication_t EventCmd::authenticate(AbstractServer& server) const {
    return implementation::do_authenticate(*this, server);
}

ecf::authorisation_t EventCmd::authorise(AbstractServer& server) const {
    return implementation::do_authorise(*this, server);
}

void EventCmd::print(std::string& os) const {
    os += Str::CHILD_CMD();
    os += "event ";
    os += name_;
    os += " ";
    if (value_) {
        os += "1 ";
    }
    else {
        os += "0 ";
    }
    os += path_to_node();
}

STC_Cmd_ptr EventCmd::doHandleRequest(AbstractServer* as) const {
    as->update_stats().task_event_++;

    { // update suite change numbers before job submission,  task_ setup during authentication
        SuiteChanged1 changed(submittable_->suite());

        // The name could either be "string" or an integer either way it should be unique
        if (!submittable_->set_event(name_, value_)) {
            std::string ss;
            ss = "Event request failed as event '";
            ss += name_;
            ss += "' does not exist on task ";
            ss += path_to_node();
            ecf::log(Log::ERR, ss);
            return PreAllocatedReply::ok_cmd();
        }
    }

    // Do job submission in case any triggers dependent on events
    as->increment_job_generation_count();
    return PreAllocatedReply::ok_cmd();
}

const char* EventCmd::arg() {
    return TaskApi::eventArg();
}
const char* EventCmd::desc() {
    return "Change event. For use in the '.ecf' script file *only*\n"
           "Hence the context is supplied via environment variables\n"
           "  arg1(string | int)     = event-name\n\n"
           "  arg2(string)(optional) = [ set | clear] default value is set\n\n"
           "If this child command is a zombie, then the default action will be to *fob*,\n"
           "i.e allow the ecflow client command to complete without an error\n"
           "The default can be overridden by using zombie attributes.\n\n"
           "Usage:\n"
           "  ecflow_client --event=ev       # set the event, default since event initial value is clear\n"
           "  ecflow_client --event=ev set   # set the event, explicit\n"
           "  ecflow_client --event=ev clear # clear the event, use when event initial value is set\n";
}

void EventCmd::addOption(boost::program_options::options_description& desc) const {
    desc.add_options()(EventCmd::arg(), po::value<vector<string>>()->multitoken(), EventCmd::desc());
}
void EventCmd::create(Cmd_ptr& cmd, boost::program_options::variables_map& vm, AbstractClientEnv* clientEnv) const {
    vector<string> args = vm[arg()].as<vector<string>>();
    std::string event;
    if (args.size() >= 1) {
        event = args[0];
    }

    bool value = true;
    if (args.size() == 2) {
        if (args[1] == "set") {
            value = true;
        }
        else if (args[1] == "clear") {
            value = false;
        }
        else {
            std::stringstream ss;
            ss << "EventCmd: The second argument must be [ set | clear ] but found " << args[1];
            throw std::runtime_error(ss.str());
        }
    }

    if (clientEnv->debug()) {
        cout << "  EventCmd::create " << EventCmd::arg() << " task_path(" << clientEnv->task_path() << ") password("
             << clientEnv->jobs_password() << ") remote_id(" << clientEnv->process_or_remote_id() << ") try_no("
             << clientEnv->task_try_no() << ") event(" << event << ")"
             << ") value(" << value << ")\n";
    }

    std::string errorMsg;
    if (!clientEnv->checkTaskPathAndPassword(errorMsg)) {
        throw std::runtime_error("EventCmd: " + errorMsg);
    }

    cmd = std::make_shared<EventCmd>(clientEnv->task_path(),
                                     clientEnv->jobs_password(),
                                     clientEnv->process_or_remote_id(),
                                     clientEnv->task_try_no(),
                                     event,
                                     value);
}

std::ostream& operator<<(std::ostream& os, const EventCmd& c) {
    std::string ret;
    c.print(ret);
    os << ret;
    return os;
}

CEREAL_REGISTER_TYPE(EventCmd)
CEREAL_REGISTER_DYNAMIC_INIT(EventCmd)
