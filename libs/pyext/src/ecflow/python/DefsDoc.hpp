/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_python_DefsDoc_HPP
#define ecflow_python_DefsDoc_HPP

// ===========================================================================
// IMPORTANT: These appear as python doc strings.
//            Additionally, they are auto-documented using sphinx-docs
//            Hence the doc strings use reStructuredText markup.
// ===========================================================================
class DefsDoc {
public:
    DefsDoc()                          = delete;
    DefsDoc(const DefsDoc&)            = delete;
    DefsDoc& operator=(const DefsDoc&) = delete;

    static const char* add();
    static const char* abs_node_path_doc();
    static const char* part_expression_doc();
    static const char* expression_doc();
    static const char* add_trigger_doc();
    static const char* trigger();
    static const char* add_variable_doc();
    static const char* add_label_doc();
    static const char* add_aviso_doc();
    static const char* add_mirror_doc();
    static const char* add_limit_doc();
    static const char* add_inlimit_doc();
    static const char* node_doc();
    static const char* node_container_doc();
    static const char* submittable_doc();
    static const char* task_doc();
    static const char* alias_doc();
    static const char* family_doc();
    static const char* add_suite_doc();
    static const char* add_extern_doc();
    static const char* add_family_doc();
    static const char* add_task_doc();
    static const char* suite_doc();
    static const char* add_definition_doc();
    static const char* add_event_doc();
    static const char* add_meter_doc();
    static const char* add_date_doc();
    static const char* add_day_doc();
    static const char* add_today_doc();
    static const char* add_time_doc();
    static const char* add_cron_doc();
    static const char* add_late_doc();
    static const char* add_autocancel_doc();
    static const char* add_autoarchive_doc();
    static const char* add_autorestore_doc();
    static const char* add_verify_doc();
    static const char* add_repeat_date_doc();
    static const char* add_repeat_datetime_doc();
    static const char* add_repeat_date_list_doc();
    static const char* add_repeat_integer_doc();
    static const char* add_repeat_string_doc();
    static const char* add_repeat_enumerated_doc();
    static const char* add_repeat_day_doc();
    static const char* add_defstatus_doc();
    static const char* jobgenctrl_doc();
    static const char* check_job_creation_doc();
    static const char* generate_scripts_doc();
    static const char* check();
    static const char* simulate();
    static const char* get_server_state();
};

#endif /* ecflow_python_DefsDoc_HPP */
