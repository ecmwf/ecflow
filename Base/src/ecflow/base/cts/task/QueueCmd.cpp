/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/base/cts/task/QueueCmd.hpp"

#include <stdexcept>

#include "ecflow/attribute/QueueAttr.hpp"
#include "ecflow/base/AbstractClientEnv.hpp"
#include "ecflow/base/AbstractServer.hpp"
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

bool QueueCmd::equals(ClientToServerCmd* rhs) const {
    auto* the_rhs = dynamic_cast<QueueCmd*>(rhs);
    if (!the_rhs)
        return false;
    if (name_ != the_rhs->name())
        return false;
    if (action_ != the_rhs->action())
        return false;
    if (step_ != the_rhs->step())
        return false;
    if (path_to_node_with_queue_ != the_rhs->path_to_node_with_queue())
        return false;
    return TaskCmd::equals(rhs);
}

void QueueCmd::print(std::string& os) const {
    os += Str::CHILD_CMD();
    os += TaskApi::queue_arg();
    os += " ";
    os += name_;
    os += " ";
    os += action_;
    os += " ";
    os += step_;
    os += " ";
    if (path_to_node_with_queue_.empty()) {
        os += path_to_node();
        return;
    }

    os += path_to_node_with_queue_;
    os += " ";
    os += path_to_node();
}

STC_Cmd_ptr QueueCmd::doHandleRequest(AbstractServer* as) const {
    as->update_stats().task_queue_++;
    std::string result;

    //////////////////////////////////////////////////////////////////////////////
    // Return the current string value, and then increment the index
    //////////////////////////////////////////////////////////////////////////////
    { // Added scope for SuiteChanged1 changed: i.e update suite change numbers before job submission
        // submittable_ setup during authentication
        SuiteChanged1 changed(submittable_->suite());

        if (!path_to_node_with_queue_.empty()) {
            Defs* defs = submittable_->defs();
            if (defs) {
                node_ptr node_with_queue = defs->findAbsNode(path_to_node_with_queue_);
                if (node_with_queue) {

                    QueueAttr& queue_attr = node_with_queue->findQueue(name_);
                    if (queue_attr.empty()) {
                        std::stringstream ss;
                        ss << "QueueCmd:: Could not find queue of name " << name_ << ", on input node "
                           << path_to_node_with_queue_;
                        return PreAllocatedReply::error_cmd(ss.str());
                    }

                    result = handle_queue(queue_attr);
                }
                else {
                    std::stringstream ss;
                    ss << "QueueCmd:: Could not find node at path " << path_to_node_with_queue_;
                    return PreAllocatedReply::error_cmd(ss.str());
                }
            }
        }
        else {
            bool fnd_queue        = false;
            QueueAttr& queue_attr = submittable_->findQueue(name_);
            if (queue_attr.empty()) {
                Node* parent = submittable_->parent();
                while (parent) {
                    QueueAttr& queue_attr1 = parent->findQueue(name_);
                    if (!queue_attr1.empty()) {
                        fnd_queue = true;
                        result    = handle_queue(queue_attr1);
                        break;
                    }
                    parent = parent->parent();
                }
            }
            else {
                fnd_queue = true;
                result    = handle_queue(queue_attr);
            }

            if (!fnd_queue) {
                std::stringstream ss;
                ss << "QueueCmd:: Could not find queue " << name_ << " Up the node hierarchy";
                return PreAllocatedReply::error_cmd(ss.str());
            }
        }
    }

    // Do job submission in case any triggers dependent on QueueAttr
    as->increment_job_generation_count();

    if (result.empty())
        return PreAllocatedReply::ok_cmd();
    return PreAllocatedReply::string_cmd(result);
}

std::string QueueCmd::handle_queue(QueueAttr& queue_attr) const {
    if (queue_attr.empty()) {
        std::stringstream ss;
        ss << "QueueCmd:: Could not find queue of name " << name_ << " . Program error !";
        throw std::runtime_error(ss.str());
    }

    if (action_ == "active")
        return queue_attr.active(); // return current index and value, make active, update index
    if (action_ == "complete")
        queue_attr.complete(step_);
    if (action_ == "aborted")
        queue_attr.aborted(step_);
    if (action_ == "no_of_aborted")
        return queue_attr.no_of_aborted();
    if (action_ == "reset")
        queue_attr.reset_index_to_first_queued_or_aborted();

    return std::string();
}

const char* QueueCmd::arg() {
    return TaskApi::queue_arg();
}
const char* QueueCmd::desc() {
    return "QueueCmd. For use in the '.ecf' script file *only*\n"
           "Hence the context is supplied via environment variables\n"
           "  arg1(string) = queue-name:\n"
           "  arg2(string) = action: [active | aborted | complete | no_of_aborted | reset ]\n"
           "     active: returns the first queued/aborted step, the return string is the queue value from the "
           "definition\n"
           "     no_of_aborted: returns number of aborted steps as a string, i.e 10\n"
           "     reset: sets the index to the first queued/aborted step. Allows steps to be reprocessed for errors\n"
           "  arg2(string) = step: value returned from step=$(ecflow_client --queue=queue_name active)\n"
           "                This is only valid for complete and aborted steps\n"
           "  arg4(string) = path: (optional). The path where the queue is defined.\n"
           "                 By default we search for the queue up the node tree.\n\n"
           "If this child command is a zombie, then the default action will be to *block*,\n"
           "The default can be overridden by using zombie attributes."
           "If the path to the queue is not defined, then this command will\n"
           "search for the queue up the node hierarchy. If no queue found, command fails\n\n"
           "Usage:\n"
           "step=\"\"\n"
           "QNAME=\"my_queue_name\"\n"
           "while [1 == 1 ] ; do\n"
           "   # this return the first queued/aborted step, then increments to next step, return <NULL> when all steps "
           "processed\n"
           "   step=$(ecflow_client --queue=$QNAME active) # of the form string  i.e \"003\". this step is now active\n"
           "   if [[ $step == \"<NULL>\" ]] ; then\n"
           "        break;\n"
           "   fi\n"
           "   ...\n"
           "   ecflow_client --queue=$QNAME complete $step   # tell ecflow this step completed\n"
           "done\n"
           "\n"
           "trap() { ecflow_client --queue=$QNAME aborted $step # tell ecflow this step failed }\n";
}

void QueueCmd::addOption(boost::program_options::options_description& desc) const {
    desc.add_options()(QueueCmd::arg(), po::value<vector<string>>()->multitoken(), QueueCmd::desc());
}
void QueueCmd::create(Cmd_ptr& cmd, boost::program_options::variables_map& vm, AbstractClientEnv* clientEnv) const {
    vector<string> args = vm[arg()].as<vector<string>>();

    if (clientEnv->debug()) {
        dumpVecArgs(QueueCmd::arg(), args);
        cout << "  QueueCmd::create " << QueueCmd::arg() << " task_path(" << clientEnv->task_path() << ") password("
             << clientEnv->jobs_password() << ") remote_id(" << clientEnv->process_or_remote_id() << ") try_no("
             << clientEnv->task_try_no() << ")\n";
    }

    // expect:
    //   <queue-name> [active | aborted | complete | no_of_aborted | reset ] step <path to node with queue>
    std::string queue_name, step;
    std::string path_to_node_with_queue, action;
    for (size_t i = 0; i < args.size(); i++) {
        if (i == 0)
            queue_name = args[i];
        else {
            if (args[i] == "active" || args[i] == "aborted" || args[i] == "complete" || args[i] == "no_of_aborted" ||
                args[i] == "reset") {
                action = args[i];
            }
            else if (args[i].find('/') != std::string::npos)
                path_to_node_with_queue = args[i];
            else
                step = args[i];
        }
    }
    if (clientEnv->debug()) {
        cout << "  QueueCmd::create "
             << "queue-name:(" << queue_name << ") action:(" << action << ") step:(" << step
             << ") path_to_node_with_queue:(" << path_to_node_with_queue << ")\n";
    }

    if (args.size() == 4 && path_to_node_with_queue.empty()) {
        std::stringstream ss;
        ss << "QueueCmd: The fourth argument if specified must provide a path to a node where the queue resides.\n"
           << "No path specified. " << args[3];
        throw std::runtime_error(ss.str());
    }

    if (args.empty() || queue_name.empty() || action.empty()) {
        std::stringstream ss;
        ss << "QueueCmd: incorrect argument specified, expected at least two arguments but found " << args.size()
           << " Please specify <queue-name> [active | aborted | complete | no_of_aborted | reset ] step <path to node "
              "with queue>(optional) i.e\n"
           << "--queue=name active  # active does not need a step\n"
           << "--queue=name active /path/to/node/with/queue\n"
           << "--queue=name aborted $step\n"
           << "--queue=name complete $step\n"
           << "--queue=name no_of_aborted\n"
           << "--queue=name reset\n";
        throw std::runtime_error(ss.str());
    }
    if ((action == "complete" || action == "aborted") && step.empty()) {
        std::stringstream ss;
        ss << "QueueCmd: when --queue=name complete || --queue=name aborted is used a step must be provided e.g.\n"
           << "  ecflow_client --queue=name aborted $step\n"
           << "  ecflow_client --queue=name complete $step\n"
           << "where step is value returned from active, such as\n"
           << "  step=$(ecflow_client --queue=name active)\n";
        throw std::runtime_error(ss.str());
    }
    if ((action == "active" || action == "reset" || action == "no_of_aborted") && !step.empty()) {
        throw std::runtime_error("QueueCmd: step should not be used with active, reset or no_of_aborted.");
    }

    string msg;
    if (!Str::valid_name(queue_name, msg)) {
        throw std::runtime_error("QueueCmd: Invalid queue name : " + msg);
    }

    std::string errorMsg;
    if (!clientEnv->checkTaskPathAndPassword(errorMsg)) {
        throw std::runtime_error("QueueCmd: " + errorMsg);
    }

    cmd = std::make_shared<QueueCmd>(clientEnv->task_path(),
                                     clientEnv->jobs_password(),
                                     clientEnv->process_or_remote_id(),
                                     clientEnv->task_try_no(),
                                     queue_name,
                                     action,
                                     step,
                                     path_to_node_with_queue);
}

std::ostream& operator<<(std::ostream& os, const QueueCmd& c) {
    std::string ret;
    c.print(ret);
    os << ret;
    return os;
}

CEREAL_REGISTER_TYPE(QueueCmd)
CEREAL_REGISTER_DYNAMIC_INIT(QueueCmd)
