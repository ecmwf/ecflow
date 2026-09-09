/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
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
        GENERIC,
        AVISO,
        MIRROR
    };

    // Disable default construction
    Aspect() = delete;
};

} // namespace ecf

#endif /* ecflow_node_Aspect_HPP */
