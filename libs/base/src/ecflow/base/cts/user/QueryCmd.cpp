/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/base/cts/user/QueryCmd.hpp"

#include <iostream>
#include <stdexcept>

#include "ecflow/base/AbstractClientEnv.hpp"
#include "ecflow/base/AbstractServer.hpp"
#include "ecflow/base/AuthenticationDetails.hpp"
#include "ecflow/base/AuthorisationDetails.hpp"
#include "ecflow/base/cts/user/CtsApi.hpp"
#include "ecflow/base/stc/PreAllocatedReply.hpp"
#include "ecflow/core/Converter.hpp"
#include "ecflow/core/Extract.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Expression.hpp"
#include "ecflow/node/Limit.hpp"
#include "ecflow/node/Node.hpp"

using namespace ecf;
using namespace std;
using namespace boost;
namespace po = boost::program_options;

//////////////////////////////////////////////////////////////////////////////////////////////////

QueryCmd::~QueryCmd() = default;

void QueryCmd::print(std::string& os) const {
    // path_to_task_ is only used in logging, so we know which task initiated the query, can be empty when invoked via
    // cmd line
    user_cmd(os, CtsApi::to_string(CtsApi::query(query_type_, path_to_attribute_, attribute_)) + path_to_task_);
}
void QueryCmd::print_only(std::string& os) const {
    os += CtsApi::to_string(CtsApi::query(query_type_, path_to_attribute_, attribute_));
    os += path_to_task_;
}

bool QueryCmd::equals(ClientToServerCmd* rhs) const {
    auto* the_rhs = dynamic_cast<QueryCmd*>(rhs);
    if (!the_rhs) {
        return false;
    }
    if (query_type_ != the_rhs->query_type()) {
        return false;
    }
    if (path_to_attribute_ != the_rhs->path_to_attribute()) {
        return false;
    }
    if (attribute_ != the_rhs->attribute()) {
        return false;
    }
    if (path_to_task_ != the_rhs->path_to_task()) {
        return false;
    }
    return UserCmd::equals(rhs);
}

ecf::authentication_t QueryCmd::authenticate(AbstractServer& server) const {
    return implementation::do_authenticate(*this, server);
}

ecf::authorisation_t QueryCmd::authorise(AbstractServer& server) const {
    return implementation::do_authorise(*this, server);
}

void QueryCmd::addOption(boost::program_options::options_description& desc) const {
    desc.add_options()(QueryCmd::arg(), po::value<vector<string>>()->multitoken(), QueryCmd::desc());
}

void QueryCmd::create(Cmd_ptr& cmd, boost::program_options::variables_map& vm, AbstractClientEnv* clientEnv) const {
    vector<string> args = vm[arg()].as<vector<string>>();

    if (clientEnv->debug()) {
        dumpVecArgs(QueryCmd::arg(), args);
        cout << "  QueryCmd::create " << QueryCmd::arg() << " task_path(" << clientEnv->task_path() << ")\n";
    }

    std::string query_type;
    std::string path_to_attribute;
    std::string attribute;

    if (args.size()) {
        query_type = args[0];
    }
    if (query_type == "event" || query_type == "meter" || query_type == "label" || query_type == "variable" ||
        query_type == "limit" || query_type == "limit_max") {
        // second argument must be <path>:event_or_meter_or_label_or_variable_or_limit
        std::string path_and_name;
        if (args.size() == 2) {
            path_and_name = args[1];
            if (!Extract::pathAndName(path_and_name, path_to_attribute, attribute)) {
                throw std::runtime_error("QueryCmd: second argument must be of the form <path>:name for query\n, where "
                                         "name is [event | meter | label | variable | limit | limit_max ] name " +
                                         query_type + " : could not extract from:" + path_and_name);
            }
        }
        else {
            std::stringstream ss;
            ss << "QueryCmd: second argument must be of the form <path>:name for query\n where name is [event | meter "
                  "| label | variable | limit | limit_max] name "
               << query_type;
            ss << " args size = " << args.size() << " expected 2 arguments";
            throw std::runtime_error(ss.str());
        }
        if (attribute.empty()) {
            throw std::runtime_error("QueryCmd: no attribute specified: query type: " + query_type +
                                     " path+attribute: " + path_and_name + "\n" + string(QueryCmd::desc()));
        }
    }
    else if (query_type == "trigger") {
        for (size_t i = 1; i < args.size(); i++) {
            if (i == 1) {
                path_to_attribute = args[i];
            }
            if (i == 2) {
                attribute = args[i];
                (void)Expression::parse(attribute, "QueryCmd:"); // will throw if expression does not parse
            }
        }
        if (attribute.empty()) {
            throw std::runtime_error("QueryCmd: no attribute specified: query type: trigger\n" +
                                     string(QueryCmd::desc()));
        }
    }
    else if (query_type == "state" || query_type == "dstate") {
        // for state and dstate attribute is empty
        if (args.size() > 1) {
            path_to_attribute = args[1];
        }
        if (args.size() > 2) {
            throw std::runtime_error("QueryCmd: invalid (state | dstate) query : " + args[2]);
        }
    }
    else if (query_type == "repeat") {
        // for repeat attribute can only be next or prev
        if (args.size() > 1) {
            path_to_attribute = args[1];
        }
        if (args.size() == 3) {
            attribute = args[2];
            if (attribute != "next" && attribute != "prev") {
                throw std::runtime_error("QueryCmd: invalid (repeat) query expected 'next' or 'prev' but found " +
                                         attribute);
            }
        }
        if (args.size() > 3) {
            throw std::runtime_error("QueryCmd: invalid (repeat) query : " + args[3]);
        }
    }
    else {
        throw std::runtime_error("QueryCmd: first argument must be one of [ state | dstate | repeat | event | meter | "
                                 "variable | trigger ] but found:" +
                                 query_type);
    }

    if (path_to_attribute.empty() || (!path_to_attribute.empty() && path_to_attribute[0] != '/')) {
        throw std::runtime_error("QueryCmd: invalid path to attribute: " + path_to_attribute);
    }

    // path_to_task can be empty if invoked via the command line. ( used for logging, i.e identifying which task invoked
    // this command) However if invoked from the shell/python we expect the path_to_task(ECF_NAME) to have been set
    std::string path_to_task = clientEnv->task_path(); // can be empty, when cmd called from command line
    if (!path_to_task.empty() && path_to_task[0] != '/') {
        throw std::runtime_error("QueryCmd: invalid path to task: " + path_to_task);
    }

    cmd = std::make_shared<QueryCmd>(query_type, path_to_attribute, attribute, path_to_task);
}

const char* QueryCmd::arg() {
    return CtsApi::queryArg();
}

const char* QueryCmd::desc() {
    return "Query the status of attributes\n"
           " i.e state,dstate,repeat,event,meter,label,variable or trigger expression without blocking\n"
           " - state     return [unknown | complete | queued |             aborted | submitted | active] to standard "
           "out\n"
           " - dstate    return [unknown | complete | queued | suspended | aborted | submitted | active] to standard "
           "out\n"
           " - repeat    returns current value as a string to standard out\n"
           " - event     return 'set' | 'clear' to standard out\n"
           " - meter     return value of the meter to standard out\n"
           " - limit     return current value of limit to standard out\n"
           " - limit_max return limit max value to standard out\n"
           " - label     return new value otherwise the old value\n"
           " - variable  return value of the variable, repeat or generated variable to standard out,\n"
           "             will search up the node tree\n"
           " - trigger   returns 'true' if the expression is true, otherwise 'false'\n\n"
           "If this command is called within a '.ecf' script we will additionally log the task calling this command\n"
           "This is required to aid debugging for excessive use of this command\n"
           "The command will fail if the node path to the attribute does not exist in the definition and if:\n"
           " - repeat   The repeat is not found\n"
           " - event    The event is not found\n"
           " - meter    The meter is not found\n"
           " - limit/limit_max The limit is not found\n"
           " - label    The label is not found\n"
           " - variable No user or generated variable or repeat of that name found on node, or any of its parents\n"
           " - trigger  Trigger does not parse, or reference to nodes/attributes in the expression are not valid\n"
           "Arguments:\n"
           "  arg1 = [ state | dstate | repeat | event | meter | label | variable | trigger | limit | limit_max ]\n"
           "  arg2 = <path> | <path>:name where name is name of a event, meter, label, limit or variable\n"
           "  arg3 = trigger expression | prev | next # prev,next only used when arg1 is repeat\n\n"
           "Usage:\n"
           " ecflow_client --query state /                                     # return top level state to standard "
           "out\n"
           " ecflow_client --query state /path/to/node                         # return node state to standard out\n"
           " ecflow_client --query dstate /path/to/node                        # state that can included suspended\n"
           " ecflow_client --query repeat /path/to/node                        # return the current value as a string\n"
           " ecflow_client --query repeat /path/to/node prev                   # return the previous value as a "
           "string\n"
           " ecflow_client --query repeat /path/to/node next                   # return the next value as a string\n"
           " ecflow_client --query event /path/to/task/with/event:event_name   # return set | clear to standard out\n"
           " ecflow_client --query meter /path/to/task/with/meter:meter_name   # returns the current value of the "
           "meter to standard out\n"
           " ecflow_client --query limit /path/to/task/with/limit:limit_name   # returns the current value of the "
           "limit to standard out\n"
           " ecflow_client --query limit_max /path/to/task/with/limit:limit_name # returns the max value of the limit "
           "to standard out\n"
           " ecflow_client --query label /path/to/task/with/label:label_name   # returns the current value of the "
           "label to standard out\n"
           " ecflow_client --query variable /path/to/task/with/var:var_name    # returns the variable value to "
           "standard out\n"
           " ecflow_client --query trigger /path/to/node/with/trigger \"/suite/task == complete\" # return true if "
           "expression evaluates false otherwise\n";
}

STC_Cmd_ptr QueryCmd::doHandleRequest(AbstractServer* as) const {
    as->update_stats().query_++;

    Defs* defs = as->defs().get();

    if (path_to_attribute_ == "/") {
        if (query_type_ == "state") {
            return PreAllocatedReply::string_cmd(NState::toString(defs->state()));
        }
        std::stringstream ss;
        ss << "QueryCmd: The only valid query for the server is 'state', i.e. ecflow_client --query state /";
        throw std::runtime_error(ss.str());
    }

    node_ptr node = find_node(defs, path_to_attribute_);

    if (query_type_ == "event") {
        const Event& event = node->findEventByNameOrNumber(attribute_);
        if (event.empty()) {
            std::stringstream ss;
            ss << "QueryCmd: Cannot find event " << attribute_ << " on node " << path_to_attribute_;
            throw std::runtime_error(ss.str());
        }
        if (event.value()) {
            return PreAllocatedReply::string_cmd(Event::SET());
        }
        return PreAllocatedReply::string_cmd(Event::CLEAR());
    }

    if (query_type_ == "meter") {
        const Meter& meter = node->findMeter(attribute_);
        if (meter.empty()) {
            std::stringstream ss;
            ss << "QueryCmd: Cannot find meter " << attribute_ << " on node " << path_to_attribute_;
            throw std::runtime_error(ss.str());
        }
        return PreAllocatedReply::string_cmd(ecf::convert_to<std::string>(meter.value()));
    }

    if (query_type_ == "limit") {
        limit_ptr limit = node->find_limit(attribute_);
        if (!limit.get()) {
            throw std::runtime_error("QueryCmd: Could not find limit " + attribute_);
        }
        return PreAllocatedReply::string_cmd(ecf::convert_to<std::string>(limit->value()));
    }
    if (query_type_ == "limit_max") {
        limit_ptr limit = node->find_limit(attribute_);
        if (!limit.get()) {
            throw std::runtime_error("QueryCmd: Could not find limit " + attribute_);
        }
        return PreAllocatedReply::string_cmd(ecf::convert_to<std::string>(limit->theLimit()));
    }

    if (query_type_ == "label") {
        const Label& label = node->find_label(attribute_);
        if (label.empty()) {
            std::stringstream ss;
            ss << "QueryCmd: Cannot find label " << attribute_ << " on node " << path_to_attribute_;
            throw std::runtime_error(ss.str());
        }
        if (label.new_value().empty()) {
            return PreAllocatedReply::string_cmd(label.value());
        }
        return PreAllocatedReply::string_cmd(label.new_value());
    }

    if (query_type_ == "variable") {
        std::string the_value;
        if (node->findParentVariableValue(attribute_, the_value)) {
            return PreAllocatedReply::string_cmd(the_value);
        }
        std::stringstream ss;
        ss << "QueryCmd: Cannot find variable, repeat or generated var' of name " << attribute_ << " on node "
           << path_to_attribute_ << " or its parents";
        throw std::runtime_error(ss.str());
    }

    if (query_type_ == "trigger") {
        std::unique_ptr<AstTop> ast =
            node->parse_and_check_expressions(attribute_, true, "QueryCmd:"); // will throw for errors
        if (ast->evaluate()) {
            return PreAllocatedReply::string_cmd("true");
        }
        return PreAllocatedReply::string_cmd("false");
    }

    if (query_type_ == "state") {
        return PreAllocatedReply::string_cmd(NState::toString(node->state()));
    }

    if (query_type_ == "dstate") {
        return PreAllocatedReply::string_cmd(DState::to_string(node->dstate()));
    }

    if (query_type_ == "repeat") {
        const Repeat& repeat = node->repeat();
        if (repeat.empty()) {
            std::stringstream ss;
            ss << "QueryCmd: Cannot find repeat on node " << path_to_attribute_;
            throw std::runtime_error(ss.str());
        }
        if (attribute_.empty()) {
            return PreAllocatedReply::string_cmd(repeat.valueAsString());
        }
        if (attribute_ == "next") {
            return PreAllocatedReply::string_cmd(repeat.next_value_as_string());
        }
        if (attribute_ == "prev") {
            return PreAllocatedReply::string_cmd(repeat.prev_value_as_string());
        }
        std::stringstream ss;
        ss << "QueryCmd: invalid repeat attribute expected next | prev but found " << attribute_;
        throw std::runtime_error(ss.str());
    }
    else {
        std::stringstream ss;
        ss << "QueryCmd: unrecognised query_type " << query_type_;
        throw std::runtime_error(ss.str());
    }

    return PreAllocatedReply::ok_cmd();
}

std::ostream& operator<<(std::ostream& os, const QueryCmd& c) {
    std::string ret;
    c.print(ret);
    os << ret;
    return os;
}

CEREAL_REGISTER_TYPE(QueryCmd)
CEREAL_REGISTER_DYNAMIC_INIT(QueryCmd)
