/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_node_Aspect_HPP
#define ecflow_node_Aspect_HPP

///
/// \brief This class is used to provide observers with more info regarding
/// whats changed. used as a part of the observer pattern
/// s

namespace ecf {

class Aspect {
public:
    enum Type {
        NOT_DEFINED,
        ORDER,
        ADD_REMOVE_NODE,
        ADD_REMOVE_ATTR,
        METER,
        EVENT,
        LABEL,
        LIMIT,
        STATE,
        DEFSTATUS,
        SUSPENDED,
        SERVER_STATE,
        SERVER_VARIABLE,
        EXPR_TRIGGER,
        EXPR_COMPLETE,
        REPEAT,
        REPEAT_INDEX,
        NODE_VARIABLE,
        LATE,
        TODAY,
        TIME,
        DAY,
        CRON,
        DATE,
        FLAG,
        SUBMITTABLE,
        SUITE_CLOCK,
        SUITE_BEGIN,
        SUITE_CALENDAR,
        ALIAS_NUMBER,
        QUEUE,
        QUEUE_INDEX,
        GENERIC
    };

    // Disable default construction
    Aspect() = delete;
    // Disable copy (and move) semantics
    Aspect(const Aspect&)                  = delete;
    const Aspect& operator=(const Aspect&) = delete;
};

} // namespace ecf

#endif /* ecflow_node_Aspect_HPP */
