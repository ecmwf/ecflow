/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_python_NodeAttrDoc_HPP
#define ecflow_python_NodeAttrDoc_HPP

#include <boost/core/noncopyable.hpp>

// ===========================================================================
// IMPORTANT: These appear as python doc strings.
//            Additionally they are auto documented using sphinx-poco
//            Hence the doc strings use reStructuredText markup
// ===========================================================================
class NodeAttrDoc {
public:
    NodeAttrDoc()                              = delete;
    NodeAttrDoc(const NodeAttrDoc&)            = delete;
    NodeAttrDoc& operator=(const NodeAttrDoc&) = delete;

    static const char* variable_doc();
    static const char* zombie_doc();
    static const char* zombie_type_doc();
    static const char* zombie_user_action_type_doc();
    static const char* child_cmd_type_doc();
    static const char* label_doc();
    static const char* limit_doc();
    static const char* inlimit_doc();
    static const char* event_doc();
    static const char* meter_doc();
    static const char* queue_doc();
    static const char* date_doc();
    static const char* day_doc();
    static const char* days_enum_doc();
    static const char* time_doc();
    static const char* today_doc();
    static const char* late_doc();
    static const char* autocancel_doc();
    static const char* autoarchive_doc();
    static const char* autorestore_doc();
    static const char* repeat_doc();
    static const char* repeat_date_doc();
    static const char* repeat_datetime_doc();
    static const char* repeat_date_list_doc();
    static const char* repeat_integer_doc();
    static const char* repeat_enumerated_doc();
    static const char* repeat_string_doc();
    static const char* repeat_day_doc();
    static const char* cron_doc();
    static const char* clock_doc();
    static const char* aviso_doc();
    static const char* mirror_doc();
};

#endif /* ecflow_python_NodeAttrDoc_HPP */
