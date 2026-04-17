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

///
/// @brief Set the server host and port on the client using a string host and integer port.
///
/// @param self The client invoker.
/// @param host The server hostname.
/// @param port The server port number.
///
void ClientInvoker_set_host_port(ClientInvoker* self, const std::string& host, int port) {
    self->set_host_port(host, ecf::convert_to<std::string>(port));
}

///
/// @brief Set the retry connection period on the client from an integer number of seconds.
///
/// @param self The client invoker.
/// @param seconds The retry period in seconds.
///
void ClientInvoker_set_retry_connection_period(ClientInvoker* self, int seconds) {
    self->set_retry_connection_period(std::chrono::seconds(seconds));
}

///
/// @brief Return the current client library version string.
///
/// @param self The client invoker (unused).
/// @return The full version string of the ecFlow client library.
///
std::string ClientInvoker_version(ClientInvoker* self) {
    return ecf::Version::full();
}

///
/// @brief Query the server for its version and return it as a string.
///
/// @param self The client invoker.
/// @return The server version string.
///
std::string ClientInvoker_server_version(ClientInvoker* self) {
    self->server_version();
    return self->get_string();
}

///
/// @brief Execute a query on the server and return the result string.
///
/// @param self The client invoker.
/// @param query_type The type of query (e.g. `"state"`, `"dstate"`, `"repeat"`, `"trigger"`).
/// @param path_to_attribute The absolute node path, possibly including the attribute name.
/// @param attribute The attribute name, if not encoded in \p path_to_attribute.
/// @return A reference to the server reply string.
///
const std::string& ClientInvoker_query(ClientInvoker* self,
                                       const std::string& query_type,
                                       const std::string& path_to_attribute,
                                       const std::string& attribute) {
    self->query(query_type, path_to_attribute, attribute);
    return self->get_string();
}

///
/// @brief Retrieve the last \p lastLines lines of the server log.
///
/// @param self The client invoker.
/// @param lastLines The number of lines to retrieve from the end of the log.
/// @return A reference to the string containing the requested log lines.
///
const std::string& ClientInvoker_get_log(ClientInvoker* self, int lastLines) {
    self->getLog(lastLines);
    return self->get_string();
}

///
/// @brief Retrieve the script file for a node in edit mode.
///
/// @param self The client invoker.
/// @param absNodePath The absolute path of the node whose script is to be edited.
/// @return A reference to the script content string.
///
const std::string& ClientInvoker_edit_script_edit(ClientInvoker* self, const std::string& absNodePath) {
    self->edit_script_edit(absNodePath);
    return self->get_string();
}

///
/// @brief Pre-process the script for the given node and return the result.
///
/// @param self The client invoker.
/// @param absNodePath The absolute path of the node whose script is to be pre-processed.
/// @return A reference to the pre-processed script string.
///
const std::string& ClientInvoker_edit_script_preprocess(ClientInvoker* self, const std::string& absNodePath) {
    self->edit_script_preprocess(absNodePath);
    return self->get_string();
}

///
/// @brief Submit an edited script to the server for the given node.
///
/// @param self The client invoker.
/// @param absNodePath The absolute path of the node.
/// @param name_values A Python list of `"name=value"` strings representing used variables.
/// @param lines A Python list of strings forming the script file contents.
/// @param alias If true, submit the script as an alias.
/// @param run If true, run the submitted script immediately.
/// @return The result code from the submission.
///
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
        used_variables.push_back(std::make_pair(namv[i].substr(0, pos), namv[i].substr(pos + 1, namv[i].length())));
    }
    return self->edit_script_submit(absNodePath, used_variables, file_contents, alias, run);
}

///
/// @brief Retrieve a task-related file from the server.
///
/// @param self The client invoker.
/// @param absNodePath The absolute path of the node.
/// @param file_type The type of file to retrieve (e.g. `"script"`, `"job"`, `"jobout"`, `"manual"`, ...).
/// @param max_lines The maximum number of lines to retrieve (as a string).
/// @param as_bytes If true, return the content as a bytes object; otherwise as a str.
/// @return The file content as a Python `str` or `bytes` object.
///
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

///
/// @brief Retrieve server statistics and optionally print them to stdout.
///
/// @param self The client invoker.
/// @param to_stdout If true, print the statistics to standard output.
/// @return A reference to the statistics string from the server reply.
///
const std::string& ClientInvoker_stats(ClientInvoker* self, bool to_stdout = true) {
    self->stats();
    if (to_stdout) {
        std::cout << self->server_reply().get_string() << std::endl;
    }
    return self->server_reply().get_string();
}

///
/// @brief Reset the server statistics counters.
///
/// @param self The client invoker.
///
void ClientInvoker_stats_reset(ClientInvoker* self) {
    self->stats_reset();
}

///
/// @brief Retrieve the list of suite names registered on the server.
///
/// @param self The client invoker.
/// @return A Python list of suite name strings.
///
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

///
/// @brief Query whether the local definition is out of sync with the server.
///
/// @param self The client invoker.
/// @return true if the server has news (i.e. the local defs may be stale), false otherwise.
///
bool ClientInvoker_news_local(ClientInvoker* self) {
    self->news_local();
    return self->get_news();
}

///
/// @brief Free the trigger dependency on the node at the given path.
///
/// @param self The client invoker.
/// @param path The absolute path of the node.
///
void ClientInvoker_free_trigger_dep(ClientInvoker* self, const std::string& path) {
    self->freeDep(path, true /*trigger*/, false /*all*/, false /*date*/, false /*time*/);
}

///
/// @brief Free the date dependency on the node at the given path.
///
/// @param self The client invoker.
/// @param path The absolute path of the node.
///
void ClientInvoker_free_date_dep(ClientInvoker* self, const std::string& path) {
    self->freeDep(path, false /*trigger*/, false /*all*/, true /*date*/, false /*time*/);
}

///
/// @brief Free the time dependency on the node at the given path.
///
/// @param self The client invoker.
/// @param path The absolute path of the node.
///
void ClientInvoker_free_time_dep(ClientInvoker* self, const std::string& path) {
    self->freeDep(path, false /*trigger*/, false /*all*/, false /*date*/, true /*time*/);
}

///
/// @brief Free all dependencies on the node at the given path.
///
/// @param self The client invoker.
/// @param path The absolute path of the node.
///
void ClientInvoker_free_all_dep(ClientInvoker* self, const std::string& path) {
    self->freeDep(path, false /*trigger*/, true /*all*/, false /*date*/, false /*time*/);
}

///
/// @brief Free the trigger dependency on all nodes in the given list of paths.
///
/// @param self The client invoker.
/// @param list A Python list of absolute node paths.
///
void ClientInvoker_free_trigger_dep1(ClientInvoker* self, const py::list& list) {
    std::vector<std::string> paths;
    pyutil_list_to_str_vec(list, paths);
    self->freeDep(paths, true /*trigger*/, false /*all*/, false /*date*/, false /*time*/);
}

///
/// @brief Free the date dependency on all nodes in the given list of paths.
///
/// @param self The client invoker.
/// @param list A Python list of absolute node paths.
///
void ClientInvoker_free_date_dep1(ClientInvoker* self, const py::list& list) {
    std::vector<std::string> paths;
    pyutil_list_to_str_vec(list, paths);
    self->freeDep(paths, false /*trigger*/, false /*all*/, true /*date*/, false /*time*/);
}

///
/// @brief Free the time dependency on all nodes in the given list of paths.
///
/// @param self The client invoker.
/// @param list A Python list of absolute node paths.
///
void ClientInvoker_free_time_dep1(ClientInvoker* self, const py::list& list) {
    std::vector<std::string> paths;
    pyutil_list_to_str_vec(list, paths);
    self->freeDep(paths, false /*trigger*/, false /*all*/, false /*date*/, true /*time*/);
}

///
/// @brief Free all dependencies on all nodes in the given list of paths.
///
/// @param self The client invoker.
/// @param list A Python list of absolute node paths.
///
void ClientInvoker_free_all_dep1(ClientInvoker* self, const py::list& list) {
    std::vector<std::string> paths;
    pyutil_list_to_str_vec(list, paths);
    self->freeDep(paths, false /*trigger*/, true /*all*/, false /*date*/, false /*time*/);
}

///
/// @brief Force the node at the given path to the specified NState (non-recursive).
///
/// @param self The client invoker.
/// @param path The absolute path of the node.
/// @param state The target NState value.
///
void ClientInvoker_force_state(ClientInvoker* self, const std::string& path, NState::State state) {
    self->force(path, NState::toString(state), false);
}

///
/// @brief Force all nodes in the given list of paths to the specified NState (non-recursive).
///
/// @param self The client invoker.
/// @param list A Python list of absolute node paths.
/// @param state The target NState value.
///
void ClientInvoker_force_states(ClientInvoker* self, const py::list& list, NState::State state) {
    std::vector<std::string> paths;
    pyutil_list_to_str_vec(list, paths);
    self->force(paths, NState::toString(state), false);
}

///
/// @brief Force the node at the given path and all its descendants to the specified NState.
///
/// @param self The client invoker.
/// @param path The absolute path of the node.
/// @param state The target NState value.
///
void ClientInvoker_force_state_recursive(ClientInvoker* self, const std::string& path, NState::State state) {
    self->force(path, NState::toString(state), true);
}

///
/// @brief Force all nodes in the given list of paths and their descendants to the specified NState.
///
/// @param self The client invoker.
/// @param list A Python list of absolute node paths.
/// @param state The target NState value.
///
void ClientInvoker_force_states_recursive(ClientInvoker* self, const py::list& list, NState::State state) {
    std::vector<std::string> paths;
    pyutil_list_to_str_vec(list, paths);
    self->force(paths, NState::toString(state), true);
}

///
/// @brief Force an event on the node at the given path to be set or cleared.
///
/// @param self The client invoker.
/// @param path The absolute path of the node (including the event name).
/// @param set_or_clear Either `"set"` or `"clear"`.
///
void ClientInvoker_force_event(ClientInvoker* self, const std::string& path, const std::string& set_or_clear) {
    self->force(path, set_or_clear);
}

///
/// @brief Force events on all nodes in the given list of paths to be set or cleared.
///
/// @param self The client invoker.
/// @param list A Python list of absolute node paths (each including the event name).
/// @param set_or_clear Either `"set"` or `"clear"`.
///
void ClientInvoker_force_events(ClientInvoker* self, const py::list& list, const std::string& set_or_clear) {
    std::vector<std::string> paths;
    pyutil_list_to_str_vec(list, paths);
    self->force(paths, set_or_clear);
}

///
/// @brief Run (submit) the node at the given path, optionally forcing it even if already active.
///
/// @param self The client invoker.
/// @param path The absolute path of the node to run.
/// @param force If true, submit the node even if it is currently active or submitted.
///
void ClientInvoker_run(ClientInvoker* self, const std::string& path, bool force) {
    self->run(path, force);
}

///
/// @brief Run (submit) all nodes in the given list of paths.
///
/// @param self The client invoker.
/// @param list A Python list of absolute node paths to run.
/// @param force If true, submit nodes even if they are currently active or submitted.
///
void ClientInvoker_runs(ClientInvoker* self, const py::list& list, bool force) {
    std::vector<std::string> paths;
    pyutil_list_to_str_vec(list, paths);
    self->run(paths, force);
}

///
/// @brief Requeue the node at the given path on the server.
///
/// @param self The client invoker.
/// @param path The absolute path of the node to requeue.
/// @param option The requeue option string (e.g. `""`, `"abort"`, `"force"`).
///
void ClientInvoker_requeue(ClientInvoker* self, std::string path, const std::string& option) {
    self->requeue(path, option);
}

///
/// @brief Requeue all nodes in the given list of paths on the server.
///
/// @param self The client invoker.
/// @param list A Python list of absolute node paths to requeue.
/// @param option The requeue option string (e.g. `""`, `"abort"`, `"force"`).
///
void ClientInvoker_requeue_s(ClientInvoker* self, const py::list& list, const std::string& option) {
    std::vector<std::string> paths;
    pyutil_list_to_str_vec(list, paths);
    self->requeue(paths, option);
}

///
/// @brief Suspend the node at the given path on the server.
///
/// @param self The client invoker.
/// @param path The absolute path of the node to suspend.
///
void ClientInvoker_suspend(ClientInvoker* self, const std::string& path) {
    self->suspend(path);
}

///
/// @brief Suspend all nodes in the given list of paths on the server.
///
/// @param self The client invoker.
/// @param list A Python list of absolute node paths to suspend.
///
void ClientInvoker_suspend_s(ClientInvoker* self, const py::list& list) {
    std::vector<std::string> paths;
    pyutil_list_to_str_vec(list, paths);
    self->suspend(paths);
}

///
/// @brief Resume the node at the given path on the server.
///
/// @param self The client invoker.
/// @param path The absolute path of the node to resume.
///
void ClientInvoker_resume(ClientInvoker* self, const std::string& path) {
    self->resume(path);
}

///
/// @brief Resume all nodes in the given list of paths on the server.
///
/// @param self The client invoker.
/// @param list A Python list of absolute node paths to resume.
///
void ClientInvoker_resume_s(ClientInvoker* self, const py::list& list) {
    std::vector<std::string> paths;
    pyutil_list_to_str_vec(list, paths);
    self->resume(paths);
}

///
/// @brief Archive the node at the given path on the server.
///
/// @param self The client invoker.
/// @param path The absolute path of the node to archive.
///
void ClientInvoker_archive(ClientInvoker* self, const std::string& path) {
    self->archive(path);
}

///
/// @brief Archive all nodes in the given list of paths on the server.
///
/// @param self The client invoker.
/// @param list A Python list of absolute node paths to archive.
///
void ClientInvoker_archive_s(ClientInvoker* self, const py::list& list) {
    std::vector<std::string> paths;
    pyutil_list_to_str_vec(list, paths);
    self->archive(paths);
}

///
/// @brief Restore the archived node at the given path on the server.
///
/// @param self The client invoker.
/// @param path The absolute path of the archived node to restore.
///
void ClientInvoker_restore(ClientInvoker* self, const std::string& path) {
    self->restore(path);
}

///
/// @brief Restore all archived nodes in the given list of paths on the server.
///
/// @param self The client invoker.
/// @param list A Python list of absolute node paths to restore.
///
void ClientInvoker_restore_s(ClientInvoker* self, const py::list& list) {
    std::vector<std::string> paths;
    pyutil_list_to_str_vec(list, paths);
    self->restore(paths);
}

///
/// @brief Request a status update for the node at the given path from the server.
///
/// @param self The client invoker.
/// @param path The absolute path of the node.
///
void ClientInvoker_status(ClientInvoker* self, const std::string& path) {
    self->status(path);
}

///
/// @brief Request a status update for all nodes in the given list of paths from the server.
///
/// @param self The client invoker.
/// @param list A Python list of absolute node paths.
///
void ClientInvoker_status_s(ClientInvoker* self, const py::list& list) {
    std::vector<std::string> paths;
    pyutil_list_to_str_vec(list, paths);
    self->status(paths);
}

///
/// @brief Kill the job associated with the node at the given path.
///
/// @param self The client invoker.
/// @param path The absolute path of the node whose job is to be killed.
///
void ClientInvoker_kill(ClientInvoker* self, const std::string& path) {
    self->kill(path);
}

///
/// @brief Kill the jobs associated with all nodes in the given list of paths.
///
/// @param self The client invoker.
/// @param list A Python list of absolute node paths.
///
void ClientInvoker_kill_s(ClientInvoker* self, const py::list& list) {
    std::vector<std::string> paths;
    pyutil_list_to_str_vec(list, paths);
    self->kill(paths);
}

///
/// @brief Check the expression syntax and job-creation validity for a single node path.
///
/// @param self The client invoker.
/// @param node_path The absolute path of the node to check.
/// @return A reference to the string containing any error messages.
///
const std::string& ClientInvoker_check(ClientInvoker* self, const std::string& node_path) {
    self->check(node_path);
    return self->get_string();
}

///
/// @brief Check the expression syntax and job-creation validity for a list of node paths.
///
/// @param self The client invoker.
/// @param list A Python list of absolute node paths to check.
/// @return A reference to the string containing any error messages.
///
const std::string& ClientInvoker_check_s(ClientInvoker* self, const py::list& list) {
    std::vector<std::string> paths;
    pyutil_list_to_str_vec(list, paths);
    self->check(paths);
    return self->get_string();
}

///
/// @brief Delete the nodes at the given list of paths from the server.
///
/// @param self The client invoker.
/// @param list A Python list of absolute node paths to delete.
/// @param force If true, delete even if the node is active or submitted.
///
void ClientInvoker_delete_node_s(ClientInvoker* self, const py::list& list, bool force) {
    std::vector<std::string> paths;
    pyutil_list_to_str_vec(list, paths);
    self->delete_nodes(paths, force);
}

///
/// @brief Display the registered client handles on the server using CLI output.
///
/// Uses `ClientInvokerScopeState` to temporarily enable CLI mode for the call.
///
/// @param self The client invoker.
///
void ClientInvoker_ch_suites(ClientInvoker* self) {
    ClientInvokerScopeState cli(self);
    self->ch_suites();
}

///
/// @brief Register a new client handle with a set of suite names on the server.
///
/// @param self The client invoker.
/// @param auto_add_new_suites If true, automatically add newly created suites to this handle.
/// @param list A Python list of suite name strings to register.
///
void ClientInvoker_ch_register(ClientInvoker* self, bool auto_add_new_suites, const py::list& list) {
    std::vector<std::string> suites;
    pyutil_list_to_str_vec(list, suites);
    self->ch_register(auto_add_new_suites, suites);
}

///
/// @brief Add suite names to an existing client handle.
///
/// @param self The client invoker.
/// @param client_handle The client handle to add suites to.
/// @param list A Python list of suite name strings to add.
///
void ClientInvoker_ch_add(ClientInvoker* self, int client_handle, const py::list& list) {
    std::vector<std::string> suites;
    pyutil_list_to_str_vec(list, suites);
    self->ch_add(client_handle, suites);
}

///
/// @brief Add suite names to the most recently registered client handle.
///
/// @param self The client invoker.
/// @param list A Python list of suite name strings to add.
///
void ClientInvoker_ch1_add(ClientInvoker* self, const py::list& list) {
    std::vector<std::string> suites;
    pyutil_list_to_str_vec(list, suites);
    self->ch1_add(suites);
}

///
/// @brief Remove suite names from an existing client handle.
///
/// @param self The client invoker.
/// @param client_handle The client handle to remove suites from.
/// @param list A Python list of suite name strings to remove.
///
void ClientInvoker_ch_remove(ClientInvoker* self, int client_handle, const py::list& list) {
    std::vector<std::string> suites;
    pyutil_list_to_str_vec(list, suites);
    self->ch_remove(client_handle, suites);
}

///
/// @brief Remove suite names from the most recently registered client handle.
///
/// @param self The client invoker.
/// @param list A Python list of suite name strings to remove.
///
void ClientInvoker_ch1_remove(ClientInvoker* self, const py::list& list) {
    std::vector<std::string> suites;
    pyutil_list_to_str_vec(list, suites);
    self->ch1_remove(suites);
}

///
/// @brief Replace a node on the server using an in-memory Defs object.
///
/// Wraps `ClientInvoker::replace_1` without exposing the optional boolean arguments to Python.
///
/// @param self The client invoker.
/// @param absNodePath The absolute path of the node to replace.
/// @param client_defs The Defs containing the replacement node.
///
void ClientInvoker_replace_1(ClientInvoker* self, const std::string& absNodePath, defs_ptr client_defs) {
    // This ClientInvoker function wrapper avoids exposing the optional boolean arguments to the Python API.
    self->replace_1(absNodePath, client_defs);
}

///
/// @brief Replace a node on the server using a path to a defs file on disk.
///
/// Wraps `ClientInvoker::replace` without exposing the optional boolean arguments to Python.
///
/// @param self The client invoker.
/// @param absNodePath The absolute path of the node to replace.
/// @param path_to_client_defs The filesystem path to the defs file containing the replacement node.
///
void ClientInvoker_replace_2(ClientInvoker* self,
                             const std::string& absNodePath,
                             const std::string& path_to_client_defs) {
    // This ClientInvoker function wrapper avoids exposing the optional boolean arguments to the Python API.
    self->replace(absNodePath, path_to_client_defs);
}

///
/// @brief Change the order of a node relative to its siblings.
///
/// @param self The client invoker.
/// @param absNodePath The absolute path of the node to reorder.
/// @param the_order The order directive (e.g. `"top"`, `"bottom"`, `"up"`, `"down"`, `"alpha"`).
///
void ClientInvoker_order(ClientInvoker* self, const std::string& absNodePath, const std::string& the_order) {
    self->order(absNodePath, the_order);
}

///
/// @brief Alter an attribute on the node at the given path.
///
/// @param self The client invoker.
/// @param path The absolute path of the node.
/// @param alterType The alter operation: one of `add`, `change`, `delete`, `set_flag`, `clear_flag`.
/// @param attrType The attribute type name.
/// @param name The attribute name (optional).
/// @param value The attribute value (optional).
///
void ClientInvoker_alter(ClientInvoker* self,
                         const std::string& path,
                         const std::string& alterType, /* one of [ add | change | delete | set_flag | clear_flag ] */
                         const std::string& attrType,
                         const std::string& name  = "",
                         const std::string& value = "") {
    self->alter(path, alterType, attrType, name, value);
}

///
/// @brief Alter an attribute on all nodes in the given list of paths.
///
/// @param self The client invoker.
/// @param list A Python list of absolute node paths.
/// @param alterType The alter operation: one of `add`, `change`, `delete`, `set_flag`, `clear_flag`.
/// @param attrType The attribute type name.
/// @param name The attribute name (optional).
/// @param value The attribute value (optional).
///
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

///
/// @brief Sort attributes of the given type on a single node, optionally recursively.
///
/// @param self The client invoker.
/// @param path The absolute path of the node.
/// @param attribute_name The attribute type name to sort.
/// @param recursive If true, sort recursively through all child nodes.
///
void ClientInvoker_alter_sort(ClientInvoker* self,
                              const std::string& path,
                              const std::string& attribute_name,
                              bool recursive = true) {
    self->alter_sort(std::vector<std::string>(1, path), attribute_name, recursive);
}

///
/// @brief Sort attributes of the given type on all nodes in the given list of paths.
///
/// @param self The client invoker.
/// @param list A Python list of absolute node paths.
/// @param attribute_name The attribute type name to sort.
/// @param recursive If true, sort recursively through all child nodes.
///
void ClientInvoker_alter_sort_s(ClientInvoker* self,
                                const py::list& list,
                                const std::string& attribute_name,
                                bool recursive = true) {
    std::vector<std::string> paths;
    pyutil_list_to_str_vec(list, paths);
    self->check(paths);
    self->alter_sort(paths, attribute_name, recursive);
}

///
/// @brief Set the child process PID from an integer value.
///
/// Converts the integer to a string before passing it to `ClientInvoker::set_child_pid`.
///
/// @param self The client invoker.
/// @param pid The process ID of the child job.
///
void ClientInvoker_set_child_pid(ClientInvoker* self, int pid) {
    self->set_child_pid(ecf::convert_to<std::string>(pid));
}

///
/// @brief Set additional variables to add during child init, from a Python dictionary.
///
/// @param self The client invoker.
/// @param dict A Python dictionary mapping variable names (str) to values (str).
///
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

///
/// @brief Set additional variables to add during child init, from a Python list of Variable objects.
///
/// @param self The client invoker.
/// @param dict A Python list of Variable objects.
///
void ClientInvoker_set_child_init_add_vars2(ClientInvoker* self, const py::list& dict) {
    std::vector<Variable> vec;
    pyutil_list_to_str_vec(dict, vec);
    self->set_child_init_add_vars(vec);
}

///
/// @brief Set variables to delete during child complete, from a Python list of variable names.
///
/// @param self The client invoker.
/// @param dict A Python list of variable name strings to delete on completion.
///
void ClientInvoker_set_child_complete_del_vars(ClientInvoker* self, const py::list& dict) {
    std::vector<std::string> vars;
    pyutil_list_to_str_vec(dict, vars);
    self->set_child_complete_del_vars(vars);
}

///
/// @brief Implement the Python context manager entry protocol.
///
/// @param self The client invoker.
/// @return \p self, enabling use in a `with` statement.
///
std::shared_ptr<ClientInvoker> ClientInvoker_enter(std::shared_ptr<ClientInvoker> self) {
    return self;
}

///
/// @brief Implement the Python context manager exit protocol.
///
/// Drops the client handle (ch1_drop) on exit.
///
/// @return false (exceptions are not suppressed).
///
bool ClientInvoker_exit(std::shared_ptr<ClientInvoker> self,
                        const py::object& type,
                        const py::object& value,
                        const py::object& traceback) {
    self->ch1_drop();
    return false;
}

///
/// @brief Retrieve the current list of zombie processes from the server.
///
/// @param self The client invoker.
/// @param pid Unused parameter (retained for API compatibility).
/// @return A const reference to the vector of Zombie objects from the server reply.
///
const std::vector<Zombie>& ClientInvoker_zombieGet(ClientInvoker* self, int pid) {
    self->zombieGet();
    return self->server_reply().zombies();
}

///
/// @brief Send the fob (allow-once) action for zombie processes at the given paths via CLI.
///
/// @param self The client invoker.
/// @param list A Python list of absolute node paths identifying zombie processes.
///
void ClientInvoker_zombieFobCli(ClientInvoker* self, const py::list& list) {
    std::vector<std::string> paths;
    pyutil_list_to_str_vec(list, paths);
    self->zombieFobCliPaths(paths);
}

///
/// @brief Send the fail action for zombie processes at the given paths via CLI.
///
/// @param self The client invoker.
/// @param list A Python list of absolute node paths identifying zombie processes.
///
void ClientInvoker_zombieFailCli(ClientInvoker* self, const py::list& list) {
    std::vector<std::string> paths;
    pyutil_list_to_str_vec(list, paths);
    self->zombieFailCliPaths(paths);
}

///
/// @brief Send the adopt action for zombie processes at the given paths via CLI.
///
/// @param self The client invoker.
/// @param list A Python list of absolute node paths identifying zombie processes.
///
void ClientInvoker_zombieAdoptCli(ClientInvoker* self, const py::list& list) {
    std::vector<std::string> paths;
    pyutil_list_to_str_vec(list, paths);
    self->zombieAdoptCliPaths(paths);
}

///
/// @brief Send the block action for zombie processes at the given paths via CLI.
///
/// @param self The client invoker.
/// @param list A Python list of absolute node paths identifying zombie processes.
///
void ClientInvoker_zombieBlockCli(ClientInvoker* self, const py::list& list) {
    std::vector<std::string> paths;
    pyutil_list_to_str_vec(list, paths);
    self->zombieBlockCliPaths(paths);
}

///
/// @brief Send the remove action for zombie processes at the given paths via CLI.
///
/// @param self The client invoker.
/// @param list A Python list of absolute node paths identifying zombie processes.
///
void ClientInvoker_zombieRemoveCli(ClientInvoker* self, const py::list& list) {
    std::vector<std::string> paths;
    pyutil_list_to_str_vec(list, paths);
    self->zombieRemoveCliPaths(paths);
}

///
/// @brief Send the kill action for zombie processes at the given paths via CLI.
///
/// @param self The client invoker.
/// @param list A Python list of absolute node paths identifying zombie processes.
///
void ClientInvoker_zombieKillCli(ClientInvoker* self, const py::list& list) {
    std::vector<std::string> paths;
    pyutil_list_to_str_vec(list, paths);
    self->zombieKillCliPaths(paths);
}

///
/// @brief Create a new ClientInvoker instance, enabling SSL if the `ECF_SSL` environment variable is set.
///
/// This factory function ensures the ClientInvoker is properly initialised with SSL
/// when `ECF_SSL` is present in the environment. The constructor does not set up SSL
/// automatically because SSL configuration may depend on the host and port.
///
/// @tparam ARGS Constructor argument types.
/// @param args Constructor arguments forwarded to `ClientInvoker`.
/// @return A shared pointer to the newly created ClientInvoker.
///
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
///
/// @brief Enable SSL on the client, honouring the `ECF_SSL` environment variable if set.
///
/// If `ECF_SSL` is set, calls `enable_ssl_if_defined()`; otherwise calls `enable_ssl()`.
///
/// @param self The client invoker.
///
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
