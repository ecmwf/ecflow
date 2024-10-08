/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/http/ApiV1Impl.hpp"

#include <optional>
#include <string>

#include "ecflow/core/Child.hpp"
#include "ecflow/core/Converter.hpp"
#include "ecflow/core/DState.hpp"
#include "ecflow/core/NState.hpp"
#include "ecflow/core/Overload.hpp"
#include "ecflow/core/Str.hpp"
#include "ecflow/http/BasicAuth.hpp"
#include "ecflow/http/Client.hpp"
#include "ecflow/http/HttpServerException.hpp"
#include "ecflow/http/Options.hpp"
#if defined(ECF_OPENSSL)
    #include "ecflow/http/TokenStorage.hpp"
#endif
#include "ecflow/http/DefsStorage.hpp"
#include "ecflow/http/HttpLibrary.hpp"
#include "ecflow/http/TreeGeneration.hpp"
#include "ecflow/http/TypeToJson.hpp"
#include "ecflow/node/Alias.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/DefsTreeVisitor.hpp"
#include "ecflow/node/Family.hpp"
#include "ecflow/node/Limit.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/node/Task.hpp"
#include "ecflow/node/parser/DefsStructureParser.hpp"

// For token based authentication, this user account
// needs to have full access to server as it's acting
// in lieu of the actual user

namespace ecf::http {

namespace {

ojson make_json_response(std::string_view path, std::string_view message) {
    return ojson::object({{"path", path}, {"message", message}});
}

} // namespace

ojson make_node_json(node_ptr node) {
    return ojson::object(
        {{"type", ecf::algorithm::tolower(node->debugType())}, {"name", node->name()}, {"children", ojson::array()}});
}

void print_sparser_node(ojson& j, const std::vector<node_ptr>& nodes) {
    for (const auto& node : nodes) {
        const std::string type = ecf::algorithm::tolower(node->debugType());

        j[node->name()] = ojson::object({});
        if (type == "family") {
            print_sparser_node(j[node->name()], std::dynamic_pointer_cast<Family>(node)->nodeVec());
        }
    }
}

void print_node(ojson& j, const std::vector<node_ptr>& nodes, bool add_id = false) {
    j["children"].get_ptr<ojson::array_t*>()->reserve(nodes.size());
    for (const auto& node : nodes) {
        const std::string type = ecf::algorithm::tolower(node->debugType());
        auto jnode             = make_node_json(node);

        j["children"].push_back(jnode);
        if (type == "family") {
            print_node(j["children"].back(), std::dynamic_pointer_cast<Family>(node)->nodeVec(), add_id);
        }
    }
}

node_ptr get_node(const std::string& path) {
    node_ptr node = get_defs()->findAbsNode(path);
    if (node.get() == nullptr) {
        throw HttpServerException(HttpStatusCode::client_error_not_found, "Path " + path + " not found");
    }

    return node;
}

ojson get_node_status(const httplib::Request& request) {
    const std::string path = request.matches[1];

    auto client = get_client(request);
    client->query("dstate", path, "");

    ojson j;
    j["status"] = client->server_reply().get_string();
    j["path"]   = path;

    if (node_ptr node = get_node(path); node) {
        j["default_status"] = DState::to_string(node->defStatus());

        std::vector<std::string> why;
        node->bottom_up_why(why);
        j["why"]                  = ojson::object({});
        j["why"]["bottom_up_why"] = why;
        why.clear();
        node->top_down_why(why);
        j["why"]["top_down_why"] = why;
    }

    return j;
}

template <typename T>
void apply_to_parents(const Node* node, T&& func) {
    for (const Node* up = node; up != nullptr; up = up->parent()) {
        func(up);
    }
}

ojson get_basic_node_tree(const std::string& path) {
    BasicTree tree;
    DefsTreeVisitor(get_defs(), tree).visit_at(path);
    return tree.content();
}

ojson get_full_node_tree(const std::string& path, bool with_id, bool with_gen_vars) {
    FullTree tree{with_id, with_gen_vars};
    DefsTreeVisitor(get_defs(), tree).visit_at(path);
    return tree.content();
}

ojson get_sparser_node_tree(const std::string& path) {
    ojson j;

    if (path == "/") {
        const std::vector<suite_ptr> suites = get_defs()->suiteVec();
        for (const auto& suite : suites) {
            j[suite->name()] = ojson::object({});

            print_sparser_node(j[suite->name()], suite->nodeVec());
        }
    }
    else {
        node_ptr node = get_node(path);

        if (node == nullptr) {
            throw HttpServerException(HttpStatusCode::client_error_not_found, "Node " + path + " not found");
        }

        const std::string type = ecf::algorithm::tolower(node->debugType());

        if (type == "family") {
            print_sparser_node(j, std::dynamic_pointer_cast<Family>(node)->nodeVec());
        }
        else if (type == "suite") {
            print_sparser_node(j, std::dynamic_pointer_cast<Suite>(node)->nodeVec());
        }
    }

    return j;
}

ojson get_node_tree(const std::string& path, bool add_id = false) {
    ojson j;

    if (path == "/") {
        const std::vector<suite_ptr> suites = get_defs()->suiteVec();
        j["suites"]                         = ojson::array();
        j["suites"].get_ptr<ojson::array_t*>()->reserve(suites.size());

        for (const auto& suite : suites) {
            ojson js = make_node_json(suite);

            print_node(js, suite->nodeVec(), add_id);

            j["suites"].push_back(js);
        }
    }
    else {
        node_ptr node = get_node(path);

        if (node == nullptr) {
            throw HttpServerException(HttpStatusCode::client_error_not_found, "Node " + path + " not found");
        }

        j = make_node_json(node);

        const std::string type = ecf::algorithm::tolower(node->debugType());

        if (type == "family") {
            print_node(j, std::dynamic_pointer_cast<Family>(node)->nodeVec(), add_id);
        }
        else if (type == "suite") {
            print_node(j, std::dynamic_pointer_cast<Suite>(node)->nodeVec(), add_id);
        }
    }

    return j;
}

ojson get_suites() {
    auto suites = get_defs()->suiteVec();

    ojson j = ojson::array();
    for (const auto& s : suites) {
        j.push_back(s->name());
    }
    return j;
}

ojson get_server_attributes() {

    ojson j;

    j["variables"] = get_defs()->server().user_variables();

    for (auto v : get_defs()->server().server_variables()) {
        ojson _j    = v;
        _j["const"] = true;
        j["variables"].push_back(_j);
    }
    return j;
}

ojson get_node_attributes(const std::string& path) {
    ojson j;

    node_ptr node = get_node(path);

    j["meters"]      = node->meters();
    j["limits"]      = node->limits();
    j["inlimits"]    = node->inlimits();
    j["events"]      = node->events();
    j["variables"]   = node->variables();
    j["labels"]      = node->labels();
    j["dates"]       = node->dates();
    j["days"]        = node->days();
    j["crons"]       = node->crons();
    j["times"]       = node->timeVec();
    j["todays"]      = node->todayVec();
    j["repeat"]      = node->repeat();
    j["path"]        = path;
    j["trigger"]     = node->get_trigger();
    j["complete"]    = node->get_complete();
    j["flag"]        = node->get_flag();
    j["late"]        = node->get_late();
    j["zombies"]     = node->zombies();
    j["generics"]    = node->generics();
    j["queues"]      = node->queues();
    j["autocancel"]  = node->get_autocancel();
    j["autoarchive"] = node->get_autoarchive();
    j["autorestore"] = node->get_autorestore();
    j["avisos"]      = node->avisos();
    j["mirrors"]     = node->mirrors();

    {
        // Collect 'normal' variables
        auto vars = node->variables();
        // ... and generated variables
        node->gen_variables(vars);

        j["variables"] = vars;
    }

    apply_to_parents(node->parent(), [&j](const Node* n) {
        // Collect 'normal' variables
        auto vars = n->variables();
        // ... and generated variables
        n->gen_variables(vars);

        j["inherited_variables"][n->name()] = vars;
    });

    auto server_variables = get_defs()->server().server_variables();
    auto user_variables   = get_defs()->server().user_variables();

    server_variables.insert(server_variables.end(), user_variables.begin(), user_variables.end());

    j["inherited_variables"]["server"] = server_variables;

    return j;
}

ojson get_node_definition(const std::string& path) {
    ojson j;

    node_ptr node = get_node(path);

    j["definition"] = node->print();
    j["path"]       = path;

    return j;
}

ojson get_node_output(const httplib::Request& request) {
    const std::string path = request.matches[1];

    auto client = get_client(request);

    ojson j;

    client->file(path, "jobout");
    j["job_output"] = client->server_reply().get_string();

    try {
        client->file(path, "kill");
        j["kill_output"] = client->server_reply().get_string();
    }
    catch (const std::exception& e [[maybe_unused]]) {
        j["kill_output"] = "";
    }

    try {
        client->file(path, "stat");
        j["status_output"] = client->server_reply().get_string();
    }
    catch (const std::exception& e [[maybe_unused]]) {
        j["status_output"] = "";
    }

    return j;
}

void add_suite(const httplib::Request& request, httplib::Response& response) {
    const std::string path = request.matches[1];
    const ojson payload    = ojson::parse(request.body);

    const std::string defs = payload.at("definition");

    DefsStructureParser parser(defs);

    {
        std::string errorMsg;
        std::string warningMsg;

        if (parser.doParse(errorMsg, warningMsg) == false) {
            throw HttpServerException(HttpStatusCode::client_error_bad_request,
                                      "Invalid definition: " + errorMsg + "/" + warningMsg);
        }
    }

    node_ptr node = parser.the_node_ptr();

    auto defsptr = Defs::create();
    {
        std::string errorMsg;
        std::string warningMsg;
        defsptr->restore_from_string(defs, errorMsg, warningMsg);
    }

    try {
        if (json_type_to_string(payload.at("auto_add_externs")) == "true") {
            defsptr->auto_add_externs(true);
        }
    }
    catch (const ojson::exception& e [[maybe_unused]]) {
        // Nothing to do...
    }

    auto client = get_client(request);
    client->load(defsptr);

    ojson j;
    j["message"] = "Suite(s) created successfully";
    response.set_content(j.dump(), "application/json");
    response.status = HttpStatusCode::success_created;
    response.set_header("Location", request.path + "/" + node->name() + "/definition");
}

ojson update_node_definition(const httplib::Request& request) {
    const std::string path         = request.matches[1];
    const ojson payload            = ojson::parse(request.body);
    const std::string new_defs_str = payload.at("definition");
    const bool force               = payload.value("force", false);

    // Support "partial node adding/replacement", meaning that
    // user can give us just the definition of a new task, and
    // we will add that to the suite.

    // this takes a long time, but we need a copy as we are changing
    // the defs locally here

    auto defs   = std::make_shared<Defs>(*get_defs());
    auto parent = defs->findAbsNode(path);

    if (parent == nullptr) {
        throw HttpServerException(HttpStatusCode::client_error_not_found, "Path not found");
    }

    DefsStructureParser parser(new_defs_str);

    {
        std::string errorMsg;
        std::string warningMsg;

        if (parser.doParse(errorMsg, warningMsg) == false) {
            throw HttpServerException(HttpStatusCode::client_error_bad_request,
                                      "Invalid definition: " + errorMsg + "/" + warningMsg);
        }
    }

    // If a child with the same name exists already, it needs to be removed
    // before adding it with this new definition
    node_ptr node = parser.the_node_ptr();
    size_t pos    = 0;
    if (auto child = parent->findImmediateChild(node->name(), pos); child) {
        parent->removeChild(child.get());
    }

    {
        std::string errorMsg;
        if (parent->isAddChildOk(node.get(), errorMsg) == false) {
            throw HttpServerException(HttpStatusCode::client_error_bad_request, errorMsg);
        }
    }

    parent->addChild(node);

    auto client = get_client(request);
    client->replace(path, defs->print(), true, force);

    return make_json_response(path, "Definition updated successfully");
}

namespace /* __anonymous__ */ {

template <typename T>
void add_attribute_to_node(node_ptr node, const T& attr);

template <>
void add_attribute_to_node(node_ptr node, const Meter& attr) {
    node->addMeter(attr);
}

template <>
void add_attribute_to_node(node_ptr node, const Event& attr) {
    node->addEvent(attr);
}

template <>
void add_attribute_to_node(node_ptr node, const ecf::CronAttr& attr) {
    node->addCron(attr);
}

template <>
void add_attribute_to_node(node_ptr node, const std::string& attr) {
    node->add_complete(attr);
}

template <>
void add_attribute_to_node(node_ptr node, const ecf::AutoCancelAttr& attr) {
    node->addAutoCancel(attr);
}

template <>
void add_attribute_to_node(node_ptr node, const ecf::AutoRestoreAttr& attr) {
    node->add_autorestore(attr);
}

template <>
void add_attribute_to_node(node_ptr node, const ecf::AutoArchiveAttr& attr) {
    node->add_autoarchive(attr);
}

} // namespace

template <typename T>
void add_attribute_to_path(const T& attr, const httplib::Request& request) {
    const std::string path = request.matches[1];

    auto defs   = std::make_shared<Defs>(*get_defs());
    auto parent = defs->findAbsNode(path);

    if (parent.get() == nullptr) {
        throw HttpServerException(HttpStatusCode::client_error_not_found, "Path " + path + " not found");
    }
    add_attribute_to_node(parent, attr);

    auto client = get_client(request);
    client->replace(path, defs->print(), false, false);
}

void remove_attribute_from_path(std::string_view type, const httplib::Request& request) {
    const std::string path = request.matches[1];

    auto defs   = std::make_shared<Defs>(*get_defs());
    auto parent = defs->findAbsNode(path);

    if (parent.get() == nullptr) {
        throw HttpServerException(HttpStatusCode::client_error_not_found, "Path " + path + " not found");
    }

    if (type == "autocancel") {
        parent->deleteAutoCancel();
    }
    else if (type == "autorestore") {
        parent->deleteAutoRestore();
    }
    else if (type == "autoarchive") {
        parent->deleteAutoArchive();
    }

    auto client = get_client(request);
    client->replace(path, defs->print(), false, false);
}

template <typename T>
void update_attribute_in_path(const T& attr, std::string_view type, const httplib::Request& request) {
    const std::string path = request.matches[1];

    auto defs   = std::make_shared<Defs>(*get_defs());
    auto parent = defs->findAbsNode(path);

    if (parent.get() == nullptr) {
        throw HttpServerException(HttpStatusCode::client_error_not_found, "Path " + path + " not found");
    }

    if (type == "autocancel") {
        parent->deleteAutoCancel();
    }
    else if (type == "autorestore") {
        parent->deleteAutoRestore();
    }
    else if (type == "autoarchive") {
        parent->deleteAutoArchive();
    }

    add_attribute_to_node(parent, attr);

    auto client = get_client(request);
    client->replace(path, defs->print(), false, false);
}

template <typename T>
T create_from_text(const std::string& line);

template <>
ecf::AutoArchiveAttr create_from_text(const std::string& line) {
    if (line.find_first_of(':') == std::string::npos) {
        try {
            return ecf::AutoArchiveAttr(std::stoi(line));
        }
        catch (const std::exception& e) {
            throw HttpServerException(HttpStatusCode::client_error_bad_request,
                                      "Invalid value for autoarchive: " + line + " (" + e.what() + ")");
        }
    }
    else {
        int hour      = 0;
        int min       = 0;
        bool relative = ecf::TimeSeries::getTime(line, hour, min);
        return ecf::AutoArchiveAttr(ecf::TimeSlot(hour, min), relative);
    }
}
template <>
ecf::AutoCancelAttr create_from_text(const std::string& line) {
    if (line.find_first_of(':') == std::string::npos) {
        try {
            return ecf::AutoCancelAttr(std::stoi(line));
        }
        catch (const std::exception& e) {
            throw HttpServerException(HttpStatusCode::client_error_bad_request,
                                      "Invalid value for autocancel: " + line + " (" + e.what() + ")");
        }
    }
    else {
        int hour      = 0;
        int min       = 0;
        bool relative = ecf::TimeSeries::getTime(line, hour, min);
        return ecf::AutoCancelAttr(ecf::TimeSlot(hour, min), relative);
    }
}
template <>
ecf::AutoRestoreAttr create_from_text(const std::string& line) {
    std::vector<std::string> tasks;
    ecf::Str::split(line, tasks, " ");

    return ecf::AutoRestoreAttr(tasks);
}

ojson add_node_attribute(const httplib::Request& request) {
    const std::string path = request.matches[1];
    const ojson payload    = ojson::parse(request.body);

    const std::string type  = payload.at("type");
    const std::string value = json_type_to_string(payload.at("value"));

    auto client = get_client(request);

    if (type == "meter") {
        const std::string name = payload.at("name");
        int min                = stoi(json_type_to_string(payload.at("min")));
        int max                = stoi(json_type_to_string(payload.at("max")));
        const Meter m(name, min, max, std::numeric_limits<int>::max(), stoi(value));
        add_attribute_to_path(m, request);
        // Adding a meter always results to having its value == min, even if the value
        // of argument 'value' is something different
        client->alter(path, "change", "meter", name, value);
    }
    else if (type == "event") {
        const std::string name = payload.at("name");
        const Event e(std::numeric_limits<int>::max(), name, (value == "true") ? true : false);
        add_attribute_to_path(e, request);
        if (value != "set" && value != "true" && value != "clear" && value != "false") {
            throw HttpServerException(HttpStatusCode::client_error_bad_request,
                                      "'value' for event must be one of: set/true, clear/false");
        }
        std::string value_ = (value == "true" || value == "set") ? "set" : "clear";
        client->alter(path, "change", "event", name, value_);
    }
    else if (type == "aviso") {
        const std::string name = payload.at("name");
        client->alter(path, "add", "aviso", name, value);
    }
    else if (type == "mirror") {
        const std::string name = payload.at("name");
        client->alter(path, "add", "mirror", name, value);
    }
    else if (type == "cron") {
        add_attribute_to_path(ecf::CronAttr::create(value), request);
    }
    else if (type == "complete") {
        add_attribute_to_path(value, request);
    }
    else if (type == "autocancel") {
        add_attribute_to_path(create_from_text<ecf::AutoCancelAttr>(value), request);
    }
    else if (type == "autorestore") {
        add_attribute_to_path(create_from_text<ecf::AutoRestoreAttr>(value), request);
    }
    else if (type == "autoarchive") {
        add_attribute_to_path(create_from_text<ecf::AutoArchiveAttr>(value), request);
    }
    else if (type == "time" || type == "day" || type == "date" || type == "today" || type == "late") {
        client->alter(path, "add", type, value);
    }
    else {
        const std::string name = payload.at("name");
        client->alter(path, "add", type, name, value);
    }

    return make_json_response(path, "Attribute added successfully");
}

namespace /* __anonymous__ */ {

ojson update_node_attribute_by_task(const httplib::Request& request) {
    const std::string path = request.matches[1];

    const ojson payload    = ojson::parse(request.body);
    const std::string type = payload.at("type");
    const std::string name = payload.at("name");

    // this is a child command call
    if (ecf::Child::valid_child_cmd(type) == false) {
        throw HttpServerException(HttpStatusCode::client_error_bad_request,
                                  "Invalid action for child command: " + name);
    }

    auto client = get_client_for_tasks(request, payload);

    struct Ancillary
    {
        std::string type;
        std::string action;
        std::string value;
    };
    std::optional<Ancillary> ancillary;

    if (type == "event") {
        const std::string value = payload.at("value");
        if (value != "set" && value != "true" && value != "clear" && value != "false") {
            throw HttpServerException(HttpStatusCode::client_error_bad_request,
                                      "'value' for event must be one of: set/true, clear/false");
        }
        client->child_event(name, (value == "true" || value == "set"));
    }
    else if (type == "meter") {
        const std::string value = payload.at("value");
        client->child_meter(name, std::stoi(value));
    }
    else if (type == "label") {
        const std::string value = payload.at("value");
        client->child_label(name, value);
    }
    else if (type == "queue") {
        auto queue_action = payload.at("queue_action").get<std::string>();
        auto queue_step   = payload.contains("queue_step") ? payload.at("queue_step").get<std::string>() : "";
        auto queue_path   = payload.contains("queue_path") ? payload.at("queue_path").get<std::string>() : "";
        std::string reply = client->child_queue(name, queue_action, queue_step, queue_path);

        if (queue_action == "active" || queue_action == "no_of_aborted") {
            ancillary = std::make_optional(Ancillary{type, queue_action, reply});
        }
    }
    else {
        throw HttpServerException(HttpStatusCode::server_error_not_implemented,
                                  "Child action " + type + ", on attribute " + name + ", is not supported");
    }

    ojson j = make_json_response(path, "Attribute changed successfully");
    if (ancillary) {
        if (ancillary->action == "active") {
            j["step"] = ancillary->value;
        }
        else if (ancillary->action == "no_of_aborted") {
            j["no_of_aborted"] = ancillary->value;
        }
    }
    return j;
}

ojson update_node_attribute_by_user(const httplib::Request& request) {
    const std::string path = request.matches[1];
    const ojson payload    = ojson::parse(request.body);

    std::string type = payload.at("type");

    auto client = get_client(request);

    if (type == "repeat" || type == "late" || type == "complete") {
        const std::string value = json_type_to_string(payload.at("value"));
        client->alter(path, "change", type, value);
    }
    else if (type == "today" || type == "time" || type == "late") {
        const std::string value     = json_type_to_string(payload.at("value"));
        const std::string old_value = json_type_to_string(payload.at("old_value"));
        client->alter(path, "change", type, old_value, value);
    }
    else if (type == "day" || type == "date") {
        const std::string value     = json_type_to_string(payload.at("value"));
        const std::string old_value = json_type_to_string(payload.at("old_value"));
        client->alter(path, "delete", type, old_value);
        client->alter(path, "add", type, value);
    }
    else if (type == "aviso" || type == "mirror") {
        const std::string value = json_type_to_string(payload.at("value"));
        client->alter(path, "change", type, value);
    }
    else if (type == "autocancel") {
        const std::string value = json_type_to_string(payload.at("value"));
        update_attribute_in_path(create_from_text<ecf::AutoCancelAttr>(value), type, request);
        trigger_defs_update();
    }
    else if (type == "autorestore") {
        const std::string value = json_type_to_string(payload.at("value"));
        update_attribute_in_path(create_from_text<ecf::AutoRestoreAttr>(value), type, request);
        trigger_defs_update();
    }
    else if (type == "autoarchive") {
        const std::string value = json_type_to_string(payload.at("value"));
        update_attribute_in_path(create_from_text<ecf::AutoArchiveAttr>(value), type, request);
        trigger_defs_update();
    }
    else if (type == "cron") {
        const std::string value     = json_type_to_string(payload.at("value"));
        const std::string old_value = json_type_to_string(payload.at("old_value"));
        client->alter(path, "delete", type, old_value);
        trigger_defs_update([&] { add_attribute_to_path(ecf::CronAttr::create(value), request); });
    }
    else if (type == "limit") {
        const std::string name = payload.at("name");
        const static std::vector<std::string> keys{"max", "value"};
        std::string value;
        try {
            value = json_type_to_string(payload.at("value"));
            type  = type + "_value";
        }
        catch (const ojson::out_of_range& e [[maybe_unused]]) {
            try {
                value = json_type_to_string(payload.at("max"));
                type  = type + "_max";
            }
            catch (const ojson::out_of_range& e [[maybe_unused]]) {
                throw HttpServerException(HttpStatusCode::client_error_bad_request,
                                          "For limit either 'max' or 'value' must be defined");
            }
        }
        client->alter(path, "change", type, name, value);
    }
    else {
        const std::string name = payload.at("name");
        std::string value      = json_type_to_string(payload.at("value"));
        if (type == "event") {
            if (value != "set" && value != "true" && value != "clear" && value != "false") {
                throw HttpServerException(HttpStatusCode::client_error_bad_request,
                                          "'value' for event must be one of: set/true, clear/false");
            }
            value = (value == "true" || value == "set") ? "set" : "clear";
        }
        client->alter(path, "change", type, name, value);
    }

    return make_json_response(path, "Attribute changed successfully");
}

} // namespace

ojson update_node_attribute(const httplib::Request& request) {

    const ojson payload = ojson::parse(request.body);

    std::string type = payload.at("type");

    //
    // Important: updating node attributes is not a straightforward matter!
    //
    // Firstly, it can be done either as a child command or a user command.
    //
    // Secondly, for user commands, the current client framework does not cover all attribute operations:
    //  * Some attributes can be added, changed and delete with ClientInvoker
    //  * Some attributes can be either added, changed or deleted with ClientInvoker
    //  * Some attributes cannot be modified in any way with ClientInvoker
    //
    // As we want the API to be consistent, we allow to modify all ecFlow attributes,
    // whether the Client code supports it or not.
    //

    if (payload.contains("ECF_NAME")) {
        return update_node_attribute_by_task(request);
    }
    else {
        return update_node_attribute_by_user(request);
    }
}

ojson delete_node_attribute(const httplib::Request& request) {
    const std::string path = request.matches[1];
    const ojson payload    = ojson::parse(request.body);

    const std::string type = payload.at("type");
    auto client            = get_client(request);

    if (type == "time" || type == "today" || type == "day" || type == "date" || type == "cron" || type == "late" ||
        type == "complete") {
        const std::string value = payload.at("value");
        client->alter(path, "delete", type, value);
    }
    else if (type == "autocancel" || type == "autorestore" || type == "autoarchive") {
        remove_attribute_from_path(type, request);
    }
    else {
        const std::string name = payload.at("name");
        client->alter(path, "delete", type, name);
    }

    return make_json_response(path, "Attribute deleted successfully");
}

ojson add_server_attribute(const httplib::Request& request) {
    const ojson payload    = ojson::parse(request.body);
    const std::string type = payload.at("type");

    if (type != "variable") {
        throw HttpServerException(HttpStatusCode::client_error_bad_request,
                                  "Only variables can be added to server attributes");
    }

    const std::string name  = payload.at("name");
    const std::string value = json_type_to_string(payload.at("value"));

    auto client = get_client(request);
    client->alter("/", "add", type, name, value);

    return make_json_response("/", "Attribute added successfully");
}

ojson update_server_attribute(const httplib::Request& request) {
    const ojson payload     = ojson::parse(request.body);
    const std::string type  = payload.at("type");
    const std::string name  = payload.at("name");
    const std::string value = payload.at("value");

    if (std::string x; !get_defs()->server().find_user_variable(name, x)) {
        throw HttpServerException(HttpStatusCode::client_error_not_found, "User variable not found");
    }
    else if (type != "variable") {
        throw HttpServerException(HttpStatusCode::client_error_bad_request,
                                  "Only variables can be updated in server attributes");
    }

    auto client = get_client(request);
    client->alter("/", "change", type, name, value);

    return make_json_response("/", "Attribute changed successfully");
}

ojson delete_server_attribute(const httplib::Request& request) {

    const ojson payload = ojson::parse(request.body);

    const std::string type = payload.at("type");
    const std::string name = payload.at("name");

    if (std::string x; !get_defs()->server().find_user_variable(name, x)) {
        throw HttpServerException(HttpStatusCode::client_error_not_found, "User variable not found");
    }
    else if (type != "variable") {
        throw HttpServerException(HttpStatusCode::client_error_bad_request,
                                  "Only variables can be removed from server attributes");
    }

    auto client = get_client(request);
    client->alter("/", "delete", type, name);

    return make_json_response("/", "Attribute deleted successfully");
}

namespace /* __anonymous__ */ {

ojson update_node_status_by_task(const httplib::Request& request) {
    const std::string path = request.matches[1];
    const ojson payload    = ojson::parse(request.body);
    const std::string name = payload.at("action");

    // this is a child command call
    if (ecf::Child::valid_child_cmd(name) == false) {
        throw HttpServerException(HttpStatusCode::client_error_bad_request,
                                  "Invalid action for child command: " + name);
    }

    auto client = get_client_for_tasks(request, payload);

    if (name == "init") {
        client->child_init();
    }
    else if (name == "abort") {
        client->child_abort(payload.value("abort_why", ""));
    }
    else if (name == "complete") {
        client->child_complete();
    }
    else if (name == "wait") {
        client->child_wait(payload.at("wait_expression"));
    }
    else {
        throw HttpServerException(HttpStatusCode::server_error_not_implemented,
                                  "Child action " + name + " not supported");
    }

    return make_json_response(path, "Status changed successfully");
}

ojson update_node_status_by_user(const httplib::Request& request) {
    const std::string path = request.matches[1];
    const ojson payload    = ojson::parse(request.body);
    const std::string name = payload.at("action");

    auto client = get_client(request);

    bool recursive = payload.value("recursive", false);
    bool force     = payload.value("force", false);

    if (name == "begin") {
        client->begin(path);
    }
    else if (name == "resume") {
        client->resume(path);
    }
    else if (name == "requeue") {
        client->requeue(path);
    }
    else if (name == "suspend") {
        client->suspend(path);
    }
    else if (name == "defstatus") {
        client->alter(path, "change", name, payload.at("defstatus_value").get<std::string>());
    }
    else if (name == "execute") {
        client->run(path, true);
    }
    else if (name == "archive") {
        client->archive(path, force);
    }
    else if (name == "restore") {
        client->restore(path);
    }
    else {
        std::string name_ = name;
        if (name_ == "abort") {
            name_ = "aborted";
        }
        else if (name_ == "rerun") {
            name_ = "queued";
        }
        else if (name_ == "execute") {
            name_ = "run";
        }
        else if (name_ == "submit") {
            name_ = "submitted";
        }

        client->force(path, name_, recursive);
    }

    return make_json_response(path, "Status changed successfully");
}

} // namespace

ojson update_node_status(const httplib::Request& request) {
    if (const ojson payload = ojson::parse(request.body); payload.contains("ECF_NAME")) {
        return update_node_status_by_task(request);
    }
    else {
        return update_node_status_by_user(request);
    }
}

ojson update_script_content(const httplib::Request& request) {
    const std::string path = request.matches[1];
    const ojson payload    = ojson::parse(request.body);

    const std::string script = payload.at("script");

    std::vector<std::string> lines;
    lines.reserve(20);
    std::stringstream ss(script);

    for (std::string line; std::getline(ss, line, '\n');) {
        lines.push_back(line);
    }

    auto client = get_client(request);
    client->edit_script_submit(path, {}, lines, false, false);

    return make_json_response(path, "Script submitted successfully");
}

} // namespace ecf::http
