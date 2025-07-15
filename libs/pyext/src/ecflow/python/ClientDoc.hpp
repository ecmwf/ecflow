/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_python_ClientDoc_HPP
#define ecflow_python_ClientDoc_HPP

// ===========================================================================
// IMPORTANT: These appear as python doc strings.
//            Additionally, they are auto-documented using sphinx-docs
//            Hence the doc strings use reStructuredText markup.
// ===========================================================================
class ClientDoc {
public:
    ClientDoc()                            = delete;
    ClientDoc(const ClientDoc&)            = delete;
    ClientDoc& operator=(const ClientDoc&) = delete;

    static const char* class_client();
    static const char* set_host_port();
    static const char* set_retry_connection_period();
    static const char* set_connection_attempts();
    static const char* get_defs();
    static const char* edit_script_edit();
    static const char* edit_script_preprocess();
    static const char* edit_script_submit();
    static const char* get_log();
    static const char* new_log();
    static const char* clear_log();
    static const char* flush_log();
    static const char* log_msg();
    static const char* restart_server();
    static const char* halt_server();
    static const char* shutdown_server();
    static const char* terminate_server();
    static const char* wait_for_server_reply();
    static const char* load_defs();
    static const char* load();
    static const char* get_server_defs();
    static const char* sync();
    static const char* in_sync();
    static const char* news();
    static const char* changed_node_paths();
    static const char* checkpt();
    static const char* restore_from_checkpt();
    static const char* reload_wl_file();
    static const char* run();
    static const char* requeue();
    static const char* free_date_dep();
    static const char* free_trigger_dep();
    static const char* free_time_dep();
    static const char* free_all_dep();
    static const char* ping();
    static const char* stats();
    static const char* stats_reset();
    static const char* suites();
    static const char* ch_register();
    static const char* ch_suites();
    static const char* ch_drop();
    static const char* ch_drop_user();
    static const char* ch_add();
    static const char* ch_remove();
    static const char* ch_auto_add();
    static const char* get_file();
    static const char* plug();
    static const char* query();
    static const char* alter();
    static const char* force_state();
    static const char* force_state_recursive();
    static const char* force_event();
    static const char* replace();
    static const char* kill();
    static const char* check();
    static const char* status();
    static const char* order();
    static const char* group();
    static const char* begin_suite();
    static const char* begin_all();
    static const char* suspend();
    static const char* resume();
    static const char* job_gen();
    static const char* delete_node();
    static const char* delete_all();
    static const char* enable_ssl();
    static const char* disable_ssl();
    static const char* get_certificate();
    static const char* archive();
    static const char* restore();
    static const char* set_child_path();
    static const char* set_child_password();
    static const char* set_child_pid();
    static const char* set_child_try_no();
    static const char* set_child_timeout();
    static const char* set_child_init_add_vars();
    static const char* set_child_complete_del_vars();
};

#endif /* ecflow_python_ClientDoc_HPP */
