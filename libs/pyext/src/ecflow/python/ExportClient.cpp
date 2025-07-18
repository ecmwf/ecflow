/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include <algorithm> // for std::transform

#include "ecflow/base/WhyCmd.hpp"
#ifdef ECF_OPENSSL
    #include "ecflow/base/Openssl.hpp"
#endif
#include "ecflow/client/ClientInvoker.hpp"
#include "ecflow/client/UrlCmd.hpp"
#include "ecflow/core/Converter.hpp"
#include "ecflow/core/Log.hpp"
#include "ecflow/core/NState.hpp"
#include "ecflow/core/Version.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/python/ClientDoc.hpp"
#include "ecflow/python/PythonBinding.hpp"
#include "ecflow/python/PythonUtil.hpp"

namespace {

/**
 * This class acts as a scope guard, ensuring that the ClientInvoker CLI is set to a desired state within the scope.
 *
 * Upon entering the scope, the current state is recorded and the then set to the desired value (default is true).
 * On scope exit, the state is set back to its original value.
 */
class ClientInvokerScopeState {
public:
    explicit ClientInvokerScopeState(ClientInvoker* self, bool desired = true)
        : _self(self),
          previous_(self->cli()), // record the previous state
          desired_(desired) {
        self->set_cli(desired_); // set the desired state
    }
    ~ClientInvokerScopeState() { _self->set_cli(previous_); } // restore the previous state

private:
    ClientInvoker* _self;
    bool previous_{};
    bool desired_{};
};

void ClientInvoker_set_host_port(ClientInvoker* self, const std::string& host, int port) {
    self->set_host_port(host, ecf::convert_to<std::string>(port));
}

void ClientInvoker_set_retry_connection_period(ClientInvoker* self, int seconds) {
    self->set_retry_connection_period(std::chrono::seconds(seconds));
}

std::string ClientInvoker_version(ClientInvoker* self) {
    return ecf::Version::full();
}
std::string ClientInvoker_server_version(ClientInvoker* self) {
    self->server_version();
    return self->get_string();
}

const std::string& ClientInvoker_query(ClientInvoker* self,
                                       const std::string& query_type,
                                       const std::string& path_to_attribute,
                                       const std::string& attribute) {
    self->query(query_type, path_to_attribute, attribute);
    return self->get_string();
}

const std::string& ClientInvoker_get_log(ClientInvoker* self, int lastLines) {
    self->getLog(lastLines);
    return self->get_string();
}

const std::string& ClientInvoker_edit_script_edit(ClientInvoker* self, const std::string& absNodePath) {
    self->edit_script_edit(absNodePath);
    return self->get_string();
}

const std::string& ClientInvoker_edit_script_preprocess(ClientInvoker* self, const std::string& absNodePath) {
    self->edit_script_preprocess(absNodePath);
    return self->get_string();
}

int ClientInvoker_edit_script_submit(ClientInvoker* self,
                                     const std::string& absNodePath,
                                     const py::list& name_values,
                                     const py::list& lines,
                                     bool alias = false,
                                     bool run   = true) {
    std::vector<std::string> file_contents;
    pyutil_list_to_str_vec(lines, file_contents);

    std::vector<std::string> namv;
    pyutil_list_to_str_vec(name_values, namv);
    NameValueVec used_variables;
    char sep = '=';
    for (size_t i = 0; i < namv.size(); ++i) {
        std::string::size_type pos = namv[i].find(sep);
        used_variables.push_back(std::make_pair(namv[i].substr(0, pos - 1), namv[i].substr(pos + 1, namv[i].length())));
    }
    return self->edit_script_submit(absNodePath, used_variables, file_contents, alias, run);
}

py::object ClientInvoker_get_file(ClientInvoker* self,
                                  const std::string& absNodePath,
                                  const std::string& file_type = "script",
                                  const std::string& max_lines = "10000",
                                  bool as_bytes                = false) {
    self->file(absNodePath, file_type, max_lines);
    const std::string& s = self->get_string();

    auto convert_string_to_pyobject = [](const std::string& s, bool as_bytes) -> py::object {
        PyObject* content = nullptr;
        if (as_bytes) {
            PyObject* memory =
                PyMemoryView_FromMemory(const_cast<char*>(s.data()), static_cast<ssize_t>(s.size()), PyBUF_READ);
            content = PyBytes_FromObject(memory);
        }
        else {
            content = PyUnicode_FromStringAndSize(s.data(), static_cast<ssize_t>(s.size()));
        }
        return py::reinterpret_steal<py::object>(content);
    };

    return convert_string_to_pyobject(s, as_bytes);
}

const std::string& ClientInvoker_stats(ClientInvoker* self, bool to_stdout = true) {
    self->stats();
    if (to_stdout) {
        std::cout << self->server_reply().get_string() << std::endl;
    }
    return self->server_reply().get_string();
}

void ClientInvoker_stats_reset(ClientInvoker* self) {
    self->stats_reset();
}

py::list ClientInvoker_suites(ClientInvoker* self) {
    self->suites();
    const std::vector<std::string>& the_suites = self->server_reply().get_string_vec();
    py::list list;
    size_t the_size = the_suites.size();
    for (size_t i = 0; i < the_size; i++) {
        list.append(the_suites[i]);
    }
    return list;
}

bool ClientInvoker_news_local(ClientInvoker* self) {
    self->news_local();
    return self->get_news();
}

void ClientInvoker_free_trigger_dep(ClientInvoker* self, const std::string& path) {
    self->freeDep(path, true /*trigger*/, false /*all*/, false /*date*/, false /*time*/);
}

void ClientInvoker_free_date_dep(ClientInvoker* self, const std::string& path) {
    self->freeDep(path, false /*trigger*/, false /*all*/, true /*date*/, false /*time*/);
}

void ClientInvoker_free_time_dep(ClientInvoker* self, const std::string& path) {
    self->freeDep(path, false /*trigger*/, false /*all*/, false /*date*/, true /*time*/);
}

void ClientInvoker_free_all_dep(ClientInvoker* self, const std::string& path) {
    self->freeDep(path, false /*trigger*/, true /*all*/, false /*date*/, false /*time*/);
}

void ClientInvoker_free_trigger_dep1(ClientInvoker* self, const py::list& list) {
    std::vector<std::string> paths;
    pyutil_list_to_str_vec(list, paths);
    self->freeDep(paths, true /*trigger*/, false /*all*/, false /*date*/, false /*time*/);
}

void ClientInvoker_free_date_dep1(ClientInvoker* self, const py::list& list) {
    std::vector<std::string> paths;
    pyutil_list_to_str_vec(list, paths);
    self->freeDep(paths, false /*trigger*/, false /*all*/, true /*date*/, false /*time*/);
}

void ClientInvoker_free_time_dep1(ClientInvoker* self, const py::list& list) {
    std::vector<std::string> paths;
    pyutil_list_to_str_vec(list, paths);
    self->freeDep(paths, false /*trigger*/, false /*all*/, false /*date*/, true /*time*/);
}

void ClientInvoker_free_all_dep1(ClientInvoker* self, const py::list& list) {
    std::vector<std::string> paths;
    pyutil_list_to_str_vec(list, paths);
    self->freeDep(paths, false /*trigger*/, true /*all*/, false /*date*/, false /*time*/);
}

void ClientInvoker_force_state(ClientInvoker* self, const std::string& path, NState::State state) {
    self->force(path, NState::toString(state), false);
}

void ClientInvoker_force_states(ClientInvoker* self, const py::list& list, NState::State state) {
    std::vector<std::string> paths;
    pyutil_list_to_str_vec(list, paths);
    self->force(paths, NState::toString(state), false);
}

void ClientInvoker_force_state_recursive(ClientInvoker* self, const std::string& path, NState::State state) {
    self->force(path, NState::toString(state), true);
}

void ClientInvoker_force_states_recursive(ClientInvoker* self, const py::list& list, NState::State state) {
    std::vector<std::string> paths;
    pyutil_list_to_str_vec(list, paths);
    self->force(paths, NState::toString(state), true);
}

void ClientInvoker_force_event(ClientInvoker* self, const std::string& path, const std::string& set_or_clear) {
    self->force(path, set_or_clear);
}

void ClientInvoker_force_events(ClientInvoker* self, const py::list& list, const std::string& set_or_clear) {
    std::vector<std::string> paths;
    pyutil_list_to_str_vec(list, paths);
    self->force(paths, set_or_clear);
}

void ClientInvoker_run(ClientInvoker* self, const std::string& path, bool force) {
    self->run(path, force);
}

void ClientInvoker_runs(ClientInvoker* self, const py::list& list, bool force) {
    std::vector<std::string> paths;
    pyutil_list_to_str_vec(list, paths);
    self->run(paths, force);
}

void ClientInvoker_requeue(ClientInvoker* self, std::string path, const std::string& option) {
    self->requeue(path, option);
}

void ClientInvoker_requeue_s(ClientInvoker* self, const py::list& list, const std::string& option) {
    std::vector<std::string> paths;
    pyutil_list_to_str_vec(list, paths);
    self->requeue(paths, option);
}

void ClientInvoker_suspend(ClientInvoker* self, const std::string& path) {
    self->suspend(path);
}

void ClientInvoker_suspend_s(ClientInvoker* self, const py::list& list) {
    std::vector<std::string> paths;
    pyutil_list_to_str_vec(list, paths);
    self->suspend(paths);
}

void ClientInvoker_resume(ClientInvoker* self, const std::string& path) {
    self->resume(path);
}

void ClientInvoker_resume_s(ClientInvoker* self, const py::list& list) {
    std::vector<std::string> paths;
    pyutil_list_to_str_vec(list, paths);
    self->resume(paths);
}
void ClientInvoker_archive(ClientInvoker* self, const std::string& path) {
    self->archive(path);
}

void ClientInvoker_archive_s(ClientInvoker* self, const py::list& list) {
    std::vector<std::string> paths;
    pyutil_list_to_str_vec(list, paths);
    self->archive(paths);
}

void ClientInvoker_restore(ClientInvoker* self, const std::string& path) {
    self->restore(path);
}

void ClientInvoker_restore_s(ClientInvoker* self, const py::list& list) {
    std::vector<std::string> paths;
    pyutil_list_to_str_vec(list, paths);
    self->restore(paths);
}
void ClientInvoker_status(ClientInvoker* self, const std::string& path) {
    self->status(path);
}

void ClientInvoker_status_s(ClientInvoker* self, const py::list& list) {
    std::vector<std::string> paths;
    pyutil_list_to_str_vec(list, paths);
    self->status(paths);
}

void ClientInvoker_kill(ClientInvoker* self, const std::string& path) {
    self->kill(path);
}

void ClientInvoker_kill_s(ClientInvoker* self, const py::list& list) {
    std::vector<std::string> paths;
    pyutil_list_to_str_vec(list, paths);
    self->kill(paths);
}

const std::string& ClientInvoker_check(ClientInvoker* self, const std::string& node_path) {
    self->check(node_path);
    return self->get_string();
}

const std::string& ClientInvoker_check_s(ClientInvoker* self, const py::list& list) {
    std::vector<std::string> paths;
    pyutil_list_to_str_vec(list, paths);
    self->check(paths);
    return self->get_string();
}

void ClientInvoker_delete_node_s(ClientInvoker* self, const py::list& list, bool force) {
    std::vector<std::string> paths;
    pyutil_list_to_str_vec(list, paths);
    self->delete_nodes(paths, force);
}

void ClientInvoker_ch_suites(ClientInvoker* self) {
    ClientInvokerScopeState cli(self);
    self->ch_suites();
}

void ClientInvoker_ch_register(ClientInvoker* self, bool auto_add_new_suites, const py::list& list) {
    std::vector<std::string> suites;
    pyutil_list_to_str_vec(list, suites);
    self->ch_register(auto_add_new_suites, suites);
}

void ClientInvoker_ch_add(ClientInvoker* self, int client_handle, const py::list& list) {
    std::vector<std::string> suites;
    pyutil_list_to_str_vec(list, suites);
    self->ch_add(client_handle, suites);
}

void ClientInvoker_ch1_add(ClientInvoker* self, const py::list& list) {
    std::vector<std::string> suites;
    pyutil_list_to_str_vec(list, suites);
    self->ch1_add(suites);
}

void ClientInvoker_ch_remove(ClientInvoker* self, int client_handle, const py::list& list) {
    std::vector<std::string> suites;
    pyutil_list_to_str_vec(list, suites);
    self->ch_remove(client_handle, suites);
}

void ClientInvoker_ch1_remove(ClientInvoker* self, const py::list& list) {
    std::vector<std::string> suites;
    pyutil_list_to_str_vec(list, suites);
    self->ch1_remove(suites);
}

void ClientInvoker_replace_1(ClientInvoker* self, const std::string& absNodePath, defs_ptr client_defs) {
    // This ClientInvoker function wrapper avoids exposing the optional boolean arguments to the Python API.
    self->replace_1(absNodePath, client_defs);
}

void ClientInvoker_replace_2(ClientInvoker* self,
                             const std::string& absNodePath,
                             const std::string& path_to_client_defs) {
    // This ClientInvoker function wrapper avoids exposing the optional boolean arguments to the Python API.
    self->replace(absNodePath, path_to_client_defs);
}

void ClientInvoker_order(ClientInvoker* self, const std::string& absNodePath, const std::string& the_order) {
    self->order(absNodePath, the_order);
}

void ClientInvoker_alter(ClientInvoker* self,
                         const std::string& path,
                         const std::string& alterType, /* one of [ add | change | delete | set_flag | clear_flag ] */
                         const std::string& attrType,
                         const std::string& name  = "",
                         const std::string& value = "") {
    self->alter(path, alterType, attrType, name, value);
}

void ClientInvoker_alter_s(ClientInvoker* self,
                           const py::list& list,
                           const std::string& alterType, /* one of [ add | change | delete | set_flag | clear_flag ] */
                           const std::string& attrType,
                           const std::string& name  = "",
                           const std::string& value = "") {
    std::vector<std::string> paths;
    pyutil_list_to_str_vec(list, paths);
    self->check(paths);
    self->alter(paths, alterType, attrType, name, value);
}

void ClientInvoker_alter_sort(ClientInvoker* self,
                              const std::string& path,
                              const std::string& attribute_name,
                              bool recursive = true) {
    self->alter_sort(std::vector<std::string>(1, path), attribute_name, recursive);
}

void ClientInvoker_alter_sort_s(ClientInvoker* self,
                                const py::list& list,
                                const std::string& attribute_name,
                                bool recursive = true) {
    std::vector<std::string> paths;
    pyutil_list_to_str_vec(list, paths);
    self->check(paths);
    self->alter_sort(paths, attribute_name, recursive);
}

void ClientInvoker_set_child_pid(ClientInvoker* self, int pid) {
    self->set_child_pid(ecf::convert_to<std::string>(pid));
}

void ClientInvoker_set_child_init_add_vars(ClientInvoker* self, const py::dict& dict) {
    std::vector<std::pair<std::string, std::string>> vars;
    pyutil_dict_to_str_vec(dict, vars);

    std::vector<Variable> vec;
    std::transform(vars.begin(),
                   vars.end(),
                   std::back_inserter(vec),
                   [](const std::pair<std::string, std::string>& var) { return Variable(var.first, var.second); });

    self->set_child_init_add_vars(vec);
}

void ClientInvoker_set_child_init_add_vars2(ClientInvoker* self, const py::list& dict) {
    std::vector<Variable> vec;
    pyutil_list_to_str_vec(dict, vec);
    self->set_child_init_add_vars(vec);
}

void ClientInvoker_set_child_complete_del_vars(ClientInvoker* self, const py::list& dict) {
    std::vector<std::string> vars;
    pyutil_list_to_str_vec(dict, vars);
    self->set_child_complete_del_vars(vars);
}

std::shared_ptr<ClientInvoker> ClientInvoker_enter(std::shared_ptr<ClientInvoker> self) {
    return self;
}

bool ClientInvoker_exit(std::shared_ptr<ClientInvoker> self,
                        const py::object& type,
                        const py::object& value,
                        const py::object& traceback) {
    self->ch1_drop();
    return false;
}

const std::vector<Zombie>& ClientInvoker_zombieGet(ClientInvoker* self, int pid) {
    self->zombieGet();
    return self->server_reply().zombies();
}

void ClientInvoker_zombieFobCli(ClientInvoker* self, const py::list& list) {
    std::vector<std::string> paths;
    pyutil_list_to_str_vec(list, paths);
    self->zombieFobCliPaths(paths);
}
void ClientInvoker_zombieFailCli(ClientInvoker* self, const py::list& list) {
    std::vector<std::string> paths;
    pyutil_list_to_str_vec(list, paths);
    self->zombieFailCliPaths(paths);
}
void ClientInvoker_zombieAdoptCli(ClientInvoker* self, const py::list& list) {
    std::vector<std::string> paths;
    pyutil_list_to_str_vec(list, paths);
    self->zombieAdoptCliPaths(paths);
}
void ClientInvoker_zombieBlockCli(ClientInvoker* self, const py::list& list) {
    std::vector<std::string> paths;
    pyutil_list_to_str_vec(list, paths);
    self->zombieBlockCliPaths(paths);
}
void ClientInvoker_zombieRemoveCli(ClientInvoker* self, const py::list& list) {
    std::vector<std::string> paths;
    pyutil_list_to_str_vec(list, paths);
    self->zombieRemoveCliPaths(paths);
}
void ClientInvoker_zombieKillCli(ClientInvoker* self, const py::list& list) {
    std::vector<std::string> paths;
    pyutil_list_to_str_vec(list, paths);
    self->zombieKillCliPaths(paths);
}

/**
 * @brief Creates a new instance of ClientInvoker with the specified arguments.
 *
 * This ensures that the ClientInvoker instance is properly initialized considering
 * the environment variable ECF_SSL.
 *
 * @tparam ARGS
 * @param args
 * @return the newly created ClientInvoker instance.
 */
template <typename... ARGS>
std::shared_ptr<ClientInvoker> ClientInvoker_make(const ARGS&... args) {
    // (1) Create a new instance of ClientInvoker
    auto ci = std::make_shared<ClientInvoker>(args...);

#if defined(ECF_OPENSSL)
    // (2) Set up SSL, if needed (i.e. if the ECF_SSL environment variable is set)
    // Important: this is necessary because the ClientInvoker constructor
    // loads all the environment variables except ECF_SSL, as setting up SSL
    // might depend on the host and port (potentially provided as CLI options).

    if (auto ecf_ssl = ::getenv("ECF_SSL"); ecf_ssl) {
        ci->enable_ssl_if_defined();
    }
#endif

    return ci;
}

#if defined(ECF_OPENSSL)
void ClientInvoker_enable_ssl(ClientInvoker* self) {
    if (auto ecf_ssl = ::getenv("ECF_SSL"); ecf_ssl) {
        self->enable_ssl_if_defined();
    }
    else {
        self->enable_ssl();
    }
}
#endif

} // namespace

void export_Client(py::module& m) {

    static const char* ClientInvoker_version_doc = "Returns the current client version";

    static const char* ClientInvoker_server_version_doc =
        "Returns the server version, can throw for old servers, that did not implement this request.";

    static const char* ClientInvoker_set_user_name_doc =
        "Set user name. A password must be provided in the file <host>.<port>.ecf.custom_passwd";

    static const char* ClientInvoker_get_host_doc =
        "Returns the host, assume set_host_port() has been set, otherwise return localhost";

    static const char* ClientInvoker_get_port_doc =
        "Returns the port, assume set_host_port() has been set, otherwise returns 3141";

    static const char* ClientInvoker_set_auto_sync_doc =
        "If true automatically sync with local definition after each call.";

    static const char* ClientInvoker_enable_http_doc = "Enable HTTP communication";

    static const char* ClientInvoker_enable_https_doc = "Enable HTTPS communication";

    static const char* ClientInvoker_set_zombie_child_timeout_doc =
        "Set timeout for zombie child commands,that cannot connect to server, default is 24 hours. "
        "The input is required to be in seconds";

    static const char* ClientInvoker_debug_server_on_doc =
        "Enable server debug, Will dump to standard out on server host.";

    static const char* ClientInvoker_debug_server_off_doc = "Disable server debug";

    static const char* ClientInvoker_debug_doc = "enable/disable client api debug";

    static const char* ClientInvoker_child_init_doc = "Child command,notify server job has started";

    static const char* ClientInvoker_child_abort_doc =
        "Child command,notify server job has aborted, can provide an optional reason";

    static const char* ClientInvoker_child_event_doc =
        "Child command,notify server event occurred, requires the event name";

    static const char* ClientInvoker_child_meter_doc =
        "Child command,notify server meter changed, requires meter name and value";

    static const char* ClientInvoker_child_label_doc =
        "Child command,notify server label changed, requires label name, and new value";

    static const char* ClientInvoker_child_wait_doc = "Child command,wait for expression to come true";

    static const char* ClientInvoker_child_queue_doc =
        "Child command,active:return current step as string, then increment index,"
        "requires queue name, and optionally path to node with the queue";

    static const char* ClientInvoker_child_complete_doc = "Child command,notify server job has complete";

    // Need std::shared_ptr<ClientInvoker>, to add support for with( __enter__,__exit__)
    py::class_<ClientInvoker, std::shared_ptr<ClientInvoker>>(m, "Client", ClientDoc::class_client())

        .def(py::init(&ClientInvoker_make<>))
        .def(py::init(&ClientInvoker_make<const std::string&>))
        .def(py::init(&ClientInvoker_make<const std::string&, const std::string&>))
        .def(py::init(&ClientInvoker_make<const std::string&, int>))

        // *** Context Manager ***

        .def("__enter__", &ClientInvoker_enter)
        .def("__exit__", &ClientInvoker_exit)

        // *** User commands ***

        .def("version", &ClientInvoker_version, ClientInvoker_version_doc)

        .def("server_version", &ClientInvoker_server_version, ClientInvoker_server_version_doc)

        .def("set_user_name", &ClientInvoker::set_user_name, ClientInvoker_set_user_name_doc)
        .def("set_host_port", &ClientInvoker::set_host_port, ClientDoc::set_host_port())
        .def("set_host_port", &ClientInvoker::set_hostport)
        .def("set_host_port", &ClientInvoker_set_host_port)
        .def("get_host", &ClientInvoker::host, py::return_value_policy::reference, ClientInvoker_get_host_doc)
        .def("get_port", &ClientInvoker::port, py::return_value_policy::reference, ClientInvoker_get_port_doc)

        .def("set_retry_connection_period",
             &ClientInvoker_set_retry_connection_period,
             ClientDoc::set_retry_connection_period())

        .def("set_connection_attempts", &ClientInvoker::set_connection_attempts, ClientDoc::set_connection_attempts())

        .def("set_auto_sync", &ClientInvoker::set_auto_sync, ClientInvoker_set_auto_sync_doc)

        .def("is_auto_sync_enabled", &ClientInvoker::is_auto_sync_enabled, "Returns true if automatic syncing enabled")

        .def("get_defs", &ClientInvoker::defs, ClientDoc::get_defs())

        .def("reset", &ClientInvoker::reset, "reset client definition, and handle number")

        .def("in_sync", &ClientInvoker::in_sync, ClientDoc::in_sync())

        .def("edit_script_edit",
             &ClientInvoker_edit_script_edit,
             py::return_value_policy::reference,
             ClientDoc::edit_script_edit())
        .def("edit_script_preprocess",
             &ClientInvoker_edit_script_preprocess,
             py::return_value_policy::reference,
             ClientDoc::edit_script_preprocess())
        .def("edit_script_submit", &ClientInvoker_edit_script_submit, ClientDoc::edit_script_submit())

        .def("get_log", &ClientInvoker_get_log, py::return_value_policy::reference, ClientDoc::get_log())
        .def("new_log", &ClientInvoker::new_log, py::arg("path") = "", ClientDoc::new_log())
        .def("clear_log", &ClientInvoker::clearLog, ClientDoc::clear_log())
        .def("flush_log", &ClientInvoker::flushLog, ClientDoc::flush_log())
        .def("log_msg", &ClientInvoker::logMsg, ClientDoc::log_msg())

        .def("restart_server", &ClientInvoker::restartServer, ClientDoc::restart_server())
        .def("halt_server", &ClientInvoker::haltServer, ClientDoc::halt_server())
        .def("shutdown_server", &ClientInvoker::shutdownServer, ClientDoc::shutdown_server())
        .def("terminate_server", &ClientInvoker::terminateServer, ClientDoc::terminate_server())

        .def("wait_for_server_reply",
             &ClientInvoker::wait_for_server_reply,
             py::arg("time_out") = 60,
             ClientDoc::wait_for_server_reply())

        .def("load",
             &ClientInvoker::loadDefs,
             py::arg("path_to_defs"),
             py::arg("force")      = false,
             py::arg("check_only") = false,
             py::arg("print")      = false,
             py::arg("stats")      = false,
             ClientDoc::load_defs())
        .def("load", &ClientInvoker::load, py::arg("defs"), py::arg("force") = false, ClientDoc::load())

        .def("get_server_defs", &ClientInvoker::getDefs, ClientDoc::get_server_defs())

        .def("sync_local", &ClientInvoker::sync_local, py::arg("sync_suite_clock") = false, ClientDoc::sync())

        .def("news_local", &ClientInvoker_news_local, ClientDoc::news())

        .def_property_readonly(
            "changed_node_paths", &ClientInvoker::changed_node_paths, ClientDoc::changed_node_paths())

        .def("suites", &ClientInvoker_suites, ClientDoc::suites())

        .def("ch_register", &ClientInvoker_ch_register, ClientDoc::ch_register())
        .def("ch_suites", &ClientInvoker_ch_suites, ClientDoc::ch_suites())
        .def("ch_handle", &ClientInvoker::client_handle, ClientDoc::ch_register())
        .def("ch_drop", &ClientInvoker::ch_drop, ClientDoc::ch_drop())
        .def("ch_drop", &ClientInvoker::ch1_drop)
        .def("ch_drop_user", &ClientInvoker::ch_drop_user, ClientDoc::ch_drop_user())
        .def("ch_add", &ClientInvoker_ch_add, ClientDoc::ch_add())
        .def("ch_add", &ClientInvoker_ch1_add)
        .def("ch_remove", &ClientInvoker_ch_remove, ClientDoc::ch_remove())
        .def("ch_remove", &ClientInvoker_ch1_remove)
        .def("ch_auto_add", &ClientInvoker::ch_auto_add, ClientDoc::ch_auto_add())
        .def("ch_auto_add", &ClientInvoker::ch1_auto_add)

        .def("checkpt",
             &ClientInvoker::checkPtDefs,
             py::arg("mode")                     = ecf::CheckPt::UNDEFINED,
             py::arg("check_pt_interval")        = 0,
             py::arg("check_pt_save_alarm_time") = 0,
             ClientDoc::checkpt())
        .def("restore_from_checkpt", &ClientInvoker::restoreDefsFromCheckPt, ClientDoc::restore_from_checkpt())

        .def("reload_wl_file", &ClientInvoker::reloadwsfile, ClientDoc::reload_wl_file())
        .def("reload_passwd_file", &ClientInvoker::reloadpasswdfile, "reload the passwd file. <host>.<port>.ecf.passwd")
        .def("reload_custom_passwd_file",
             &ClientInvoker::reloadcustompasswdfile,
             "reload the custom passwd file. <host>.<port>.ecf.custom_passwd. For users using ECF_USER or --user or "
             "set_user_name()")

        .def("requeue", &ClientInvoker_requeue, py::arg("abs_node_path"), py::arg("option") = "", ClientDoc::requeue())
        .def("requeue", &ClientInvoker_requeue_s, py::arg("paths"), py::arg("option") = "")

        .def("free_trigger_dep", &ClientInvoker_free_trigger_dep, ClientDoc::free_trigger_dep())
        .def("free_trigger_dep", &ClientInvoker_free_trigger_dep1)

        .def("free_date_dep", &ClientInvoker_free_date_dep, ClientDoc::free_date_dep())
        .def("free_date_dep", &ClientInvoker_free_date_dep1)

        .def("free_time_dep", &ClientInvoker_free_time_dep, ClientDoc::free_time_dep())
        .def("free_time_dep", &ClientInvoker_free_time_dep1)

        .def("free_all_dep", &ClientInvoker_free_all_dep, ClientDoc::free_all_dep())
        .def("free_all_dep", &ClientInvoker_free_all_dep1)

        .def("ping", &ClientInvoker::pingServer, ClientDoc::ping())

        .def("stats",
             &ClientInvoker_stats, // This prints to stdout, so we need to use a call guard to redirect output
             py::arg("to_stdout") = true,
             py::call_guard<py::scoped_ostream_redirect, py::scoped_estream_redirect>(),
             ClientDoc::stats())

        .def("stats_reset", &ClientInvoker_stats_reset, ClientDoc::stats_reset())

        .def("get_file",
             &ClientInvoker_get_file,
             py::arg("task"),
             py::arg("type")      = "script",
             py::arg("max_lines") = "10000",
             py::arg("as_bytes")  = false,
             ClientDoc::get_file())

        .def("plug", &ClientInvoker::plug, ClientDoc::plug())

        .def("query",
             &ClientInvoker_query,
             py::arg("query_type"),
             py::arg("path_to_attribute"),
             py::arg("attribute") = std::string{},
             py::return_value_policy::reference,
             ClientDoc::query())

        .def("alter",
             &ClientInvoker_alter_s,
             py::arg("paths"),
             py::arg("alter_type"),
             py::arg("attribute_type"),
             py::arg("name")  = "",
             py::arg("value") = "",
             ClientDoc::alter())
        .def("alter",
             &ClientInvoker_alter,
             py::arg("abs_node_path"),
             py::arg("alter_type"),
             py::arg("attribute_type"),
             py::arg("name")  = "",
             py::arg("value") = "")

        .def("sort_attributes",
             &ClientInvoker_alter_sort,
             py::arg("abs_node_path"),
             py::arg("attribute_name"),
             py::arg("recursive") = true)
        .def("sort_attributes",
             &ClientInvoker_alter_sort_s,
             py::arg("paths"),
             py::arg("attribute_name"),
             py::arg("recursive") = true)

        .def("force_event", &ClientInvoker_force_event, ClientDoc::force_event())
        .def("force_event", &ClientInvoker_force_events)

        .def("force_state", &ClientInvoker_force_state, ClientDoc::force_state())
        .def("force_state", &ClientInvoker_force_states)

        .def("force_state_recursive", &ClientInvoker_force_state_recursive, ClientDoc::force_state_recursive())
        .def("force_state_recursive", &ClientInvoker_force_states_recursive)

        .def("replace", &ClientInvoker::replace, ClientDoc::replace())
        .def("replace", &ClientInvoker::replace_1)
        .def("replace", &ClientInvoker_replace_1)
        .def("replace", &ClientInvoker_replace_2)

        .def("order", &ClientInvoker_order, ClientDoc::order())

        .def("group", &ClientInvoker::group, ClientDoc::group())

        .def("begin_suite",
             &ClientInvoker::begin,
             py::arg("suite_name"),
             py::arg("force") = false,
             ClientDoc::begin_suite())

        .def("begin_all_suites", &ClientInvoker::begin_all_suites, py::arg("force") = false, ClientDoc::begin_all())

        .def("job_generation", &ClientInvoker::job_gen, ClientDoc::job_gen())

        .def("run", &ClientInvoker_run, ClientDoc::run())
        .def("run", &ClientInvoker_runs)

        .def("check", &ClientInvoker_check, py::return_value_policy::reference, ClientDoc::check())
        .def("check", &ClientInvoker_check_s, py::return_value_policy::reference)

        .def("kill", &ClientInvoker_kill, ClientDoc::kill())
        .def("kill", &ClientInvoker_kill_s)

        .def("status", &ClientInvoker_status, ClientDoc::status())
        .def("status", &ClientInvoker_status_s)

        .def("suspend", &ClientInvoker_suspend, ClientDoc::suspend())
        .def("suspend", &ClientInvoker_suspend_s)

        .def("resume", &ClientInvoker_resume, ClientDoc::resume())
        .def("resume", &ClientInvoker_resume_s)

        .def("archive", &ClientInvoker_archive, ClientDoc::archive())
        .def("archive", &ClientInvoker_archive_s)

        .def("restore", &ClientInvoker_restore, ClientDoc::restore())
        .def("restore", &ClientInvoker_restore_s)

        .def("delete",
             &ClientInvoker::delete_node,
             py::arg("abs_node_path"),
             py::arg("force") = false,
             ClientDoc::delete_node())
        .def("delete", &ClientInvoker_delete_node_s, py::arg("paths"), py::arg("force") = false)

        .def("delete_all", &ClientInvoker::delete_all, py::arg("force") = false, ClientDoc::delete_all())

        .def("debug_server_on", &ClientInvoker::debug_server_on, ClientInvoker_debug_server_on_doc)
        .def("debug_server_off", &ClientInvoker::debug_server_off, ClientInvoker_debug_server_off_doc)

        .def("debug", &ClientInvoker::debug, ClientInvoker_debug_doc)

#ifdef ECF_OPENSSL
        .def("enable_ssl", ClientInvoker_enable_ssl, ClientDoc::enable_ssl())
        .def("disable_ssl", &ClientInvoker::disable_ssl, ClientDoc::disable_ssl())
        .def("get_certificate", &ClientInvoker::get_certificate, ClientDoc::get_certificate())
#endif

        .def("enable_http", &ClientInvoker::enable_http, ClientInvoker_enable_http_doc)
        .def("enable_https", &ClientInvoker::enable_https, ClientInvoker_enable_https_doc)

        .def("zombie_get", &ClientInvoker_zombieGet, py::return_value_policy::reference)
        .def("zombie_fob", &ClientInvoker::zombieFobCli)
        .def("zombie_fail", &ClientInvoker::zombieFailCli)
        .def("zombie_adopt", &ClientInvoker::zombieAdoptCli)
        .def("zombie_block", &ClientInvoker::zombieBlockCli)
        .def("zombie_remove", &ClientInvoker::zombieRemoveCli)
        .def("zombie_kill", &ClientInvoker::zombieKillCli)
        .def("zombie_fob", &ClientInvoker_zombieFobCli)
        .def("zombie_fail", &ClientInvoker_zombieFailCli)
        .def("zombie_adopt", &ClientInvoker_zombieAdoptCli)
        .def("zombie_block", &ClientInvoker_zombieBlockCli)
        .def("zombie_remove", &ClientInvoker_zombieRemoveCli)
        .def("zombie_kill", &ClientInvoker_zombieKillCli)

        .def("set_child_path", &ClientInvoker::set_child_path, ClientDoc::set_child_path())
        .def("set_child_password", &ClientInvoker::set_child_password, ClientDoc::set_child_password())
        .def("set_child_pid", &ClientInvoker::set_child_pid, ClientDoc::set_child_pid())
        .def("set_child_pid", &ClientInvoker_set_child_pid, ClientDoc::set_child_pid())
        .def("set_child_try_no", &ClientInvoker::set_child_try_no, ClientDoc::set_child_try_no())
        .def("set_child_timeout", &ClientInvoker::set_child_timeout, ClientDoc::set_child_timeout())
        .def("set_child_init_add_vars", &ClientInvoker_set_child_init_add_vars, ClientDoc::set_child_init_add_vars())
        .def("set_child_init_add_vars", &ClientInvoker_set_child_init_add_vars2, ClientDoc::set_child_init_add_vars())
        .def("set_child_complete_del_vars",
             &ClientInvoker_set_child_complete_del_vars,
             ClientDoc::set_child_complete_del_vars())

        .def("set_zombie_child_timeout",
             &ClientInvoker::set_zombie_child_timeout,
             ClientInvoker_set_zombie_child_timeout_doc)

        // *** Task (or Child) commands ***

        .def("child_init", &ClientInvoker::child_init, ClientInvoker_child_init_doc)

        .def("child_abort", &ClientInvoker::child_abort, py::arg("reason") = "", ClientInvoker_child_abort_doc)

        .def("child_event",
             &ClientInvoker::child_event,
             py::arg("event_name"),
             py::arg("value") = true,
             ClientInvoker_child_event_doc)

        .def("child_meter", &ClientInvoker::child_meter, ClientInvoker_child_meter_doc)

        .def("child_label", &ClientInvoker::child_label, ClientInvoker_child_label_doc)

        .def("child_wait", &ClientInvoker::child_wait, ClientInvoker_child_wait_doc)

        .def("child_queue",
             &ClientInvoker::child_queue,
             py::arg("queue_name"),
             py::arg("action"),
             py::arg("step")                    = "",
             py::arg("path_to_node_with_queue") = "",
             ClientInvoker_child_queue_doc)

        .def("child_complete", &ClientInvoker::child_complete, ClientInvoker_child_complete_doc);

    constexpr const char* WhyCmd_doc = "The why command reports, the reason why a node is not running.\n\n"
                                       "It needs the  definition structure and the path to node\n"
                                       "\nConstructor::\n\n"
                                       "   WhyCmd(defs, node_path)\n"
                                       "      defs_ptr  defs   : pointer to a definition structure\n"
                                       "      string node_path : The node path\n\n"
                                       "\nExceptions:\n\n"
                                       "- raises RuntimeError if the definition is empty\n"
                                       "- raises RuntimeError if the node path is empty\n"
                                       "- raises RuntimeError if the node path cannot be found in the definition\n"
                                       "\nUsage::\n\n"
                                       "   try:\n"
                                       "      ci = Client()\n"
                                       "      ci.sync_local()\n"
                                       "      ask = WhyCmd(ci.get_defs(),'/suite/family')\n"
                                       "      print(ask.why())\n"
                                       "   except RuntimeError, e:\n"
                                       "       print(str(e))\n\n";

    py::class_<WhyCmd>(m, "WhyCmd", WhyCmd_doc)

        .def(py::init<defs_ptr, std::string>())
        .def("why", &WhyCmd::why, "returns a '/n' separated string, with reasons why node is not running");

    constexpr const char* UrlCmd_doc =
        "Executes a command ECF_URL_CMD to display a url.\n\n"
        "It needs the definition structure and the path to node.\n"
        "\nConstructor::\n\n"
        "   UrlCmd(defs, node_path)\n"
        "      defs_ptr  defs   : pointer to a definition structure\n"
        "      string node_path : The node path.\n\n"
        "\nExceptions\n\n"
        "- raises RuntimeError if the definition is empty\n"
        "- raises RuntimeError if the node path is empty\n"
        "- raises RuntimeError if the node path cannot be found in the definition\n"
        "- raises RuntimeError if ECF_URL_CMD not defined or if variable substitution fails\n"
        "\nUsage:\n"
        "Lets assume that the server has the following definition::\n\n"
        "   suite s\n"
        "      edit ECF_URL_CMD  \"${BROWSER:=firefox} -new-tab %ECF_URL_BASE%/%ECF_URL%\"\n"
        "      edit ECF_URL_BASE \"http://www.ecmwf.int\"\n"
        "      family f\n"
        "         task t1\n"
        "            edit ECF_URL \"publications/manuals/ecflow\"\n"
        "         task t2\n"
        "            edit ECF_URL index.html\n\n"
        "::\n\n"
        "   try:\n"
        "      ci = Client()\n"
        "      ci.sync_local()\n"
        "      url = UrlCmd(ci.get_defs(),'/suite/family/task')\n"
        "      print(url.execute())\n"
        "   except RuntimeError, e:\n"
        "       print(str(e))\n\n";

    py::class_<UrlCmd>(m, "UrlCmd", UrlCmd_doc)

        .def(py::init<defs_ptr, std::string>())
        .def("execute", &UrlCmd::execute, "Displays url in the chosen browser");
}
