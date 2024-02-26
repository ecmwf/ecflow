/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_http_TypeToJson_HPP
#define ecflow_http_TypeToJson_HPP

#include "ecflow/attribute/AutoArchiveAttr.hpp"
#include "ecflow/attribute/AutoCancelAttr.hpp"
#include "ecflow/attribute/ClockAttr.hpp"
#include "ecflow/attribute/CronAttr.hpp"
#include "ecflow/attribute/DateAttr.hpp"
#include "ecflow/attribute/DayAttr.hpp"
#include "ecflow/attribute/GenericAttr.hpp"
#include "ecflow/attribute/LateAttr.hpp"
#include "ecflow/attribute/NodeAttr.hpp"
#include "ecflow/attribute/QueueAttr.hpp"
#include "ecflow/attribute/RepeatAttr.hpp"
#include "ecflow/attribute/TimeAttr.hpp"
#include "ecflow/attribute/TodayAttr.hpp"
#include "ecflow/attribute/Variable.hpp"
#include "ecflow/attribute/ZombieAttr.hpp"
#include "ecflow/base/Stats.hpp"
#include "ecflow/core/TimeSlot.hpp"
#include "ecflow/http/JSON.hpp"
#include "ecflow/node/AutoRestoreAttr.hpp"
#include "ecflow/node/Expression.hpp"
#include "ecflow/node/Flag.hpp"
#include "ecflow/node/InLimit.hpp"
#include "ecflow/node/Limit.hpp"
#include "ecflow/node/LimitFwd.hpp"

/**
 * IMPORTANT
 *
 * The following `to_json` functions are used by nlohmann::json library to convert the given object to a JSON object.
 *
 * These functions are effectively customization points, found using ADL (Argument-Dependent Lookup), and so need to be
 * declared in the same namespace as the object.
 */

void to_json(ecf::http::ojson&, const ::Meter&);
void to_json(ecf::http::ojson&, const ::Label&);
void to_json(ecf::http::ojson&, const ::Variable&);
void to_json(ecf::http::ojson&, const ::Event&);
void to_json(ecf::http::ojson&, const ::limit_ptr&);
void to_json(ecf::http::ojson&, const ::Limit&);
void to_json(ecf::http::ojson&, const ::InLimit&);
void to_json(ecf::http::ojson&, const ::DateAttr&);
void to_json(ecf::http::ojson&, const ::DayAttr&);
void to_json(ecf::http::ojson&, const ::RepeatDate&);
void to_json(ecf::http::ojson&, const ::RepeatDateTime&);
void to_json(ecf::http::ojson&, const ::RepeatDay&);
void to_json(ecf::http::ojson&, const ::RepeatDateList&);
void to_json(ecf::http::ojson&, const ::RepeatInteger&);
void to_json(ecf::http::ojson&, const ::RepeatEnumerated&);
void to_json(ecf::http::ojson&, const ::RepeatString&);
void to_json(ecf::http::ojson&, const ::Repeat&);
void to_json(ecf::http::ojson&, const ::Stats&);
void to_json(ecf::http::ojson&, const ::Expression*);
void to_json(ecf::http::ojson&, const ::Expression&);
void to_json(ecf::http::ojson&, const ::QueueAttr&);
void to_json(ecf::http::ojson&, const ::ZombieAttr&);
void to_json(ecf::http::ojson&, const ::GenericAttr&);
namespace ecf {
void to_json(ecf::http::ojson&, const ecf::TimeAttr&);
void to_json(ecf::http::ojson&, const ecf::TodayAttr&);
void to_json(ecf::http::ojson&, const ecf::CronAttr&);
void to_json(ecf::http::ojson&, const ecf::Flag&);
void to_json(ecf::http::ojson&, const ecf::LateAttr*);
void to_json(ecf::http::ojson&, const ecf::LateAttr&);
void to_json(ecf::http::ojson&, const ecf::TimeSlot&);
void to_json(ecf::http::ojson&, const ecf::AutoCancelAttr*);
void to_json(ecf::http::ojson&, const ecf::AutoCancelAttr&);
void to_json(ecf::http::ojson&, const ecf::AutoArchiveAttr*);
void to_json(ecf::http::ojson&, const ecf::AutoArchiveAttr&);
void to_json(ecf::http::ojson&, const ecf::AutoRestoreAttr*);
void to_json(ecf::http::ojson&, const ecf::AutoRestoreAttr&);
} // namespace ecf

#endif /* ecflow_http_TypeToJson_HPP */
