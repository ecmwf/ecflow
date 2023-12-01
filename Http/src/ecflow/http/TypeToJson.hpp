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

void to_json(ecf::ojson&, const ::Meter&);
void to_json(ecf::ojson&, const ::Label&);
void to_json(ecf::ojson&, const ::Variable&);
void to_json(ecf::ojson&, const ::Event&);
void to_json(ecf::ojson&, const ::limit_ptr&);
void to_json(ecf::ojson&, const ::InLimit&);
void to_json(ecf::ojson&, const ::DateAttr&);
void to_json(ecf::ojson&, const ::DayAttr&);
void to_json(ecf::ojson&, const ::RepeatDate&);
void to_json(ecf::ojson&, const ::RepeatDay&);
void to_json(ecf::ojson&, const ::RepeatDateList&);
void to_json(ecf::ojson&, const ::RepeatInteger&);
void to_json(ecf::ojson&, const ::RepeatEnumerated&);
void to_json(ecf::ojson&, const ::RepeatString&);
void to_json(ecf::ojson&, const ::Repeat&);
void to_json(ecf::ojson&, const ::Stats&);
void to_json(ecf::ojson&, const ::Expression*);
void to_json(ecf::ojson&, const ::Expression&);
void to_json(ecf::ojson&, const ::QueueAttr&);
void to_json(ecf::ojson&, const ::ZombieAttr&);
void to_json(ecf::ojson&, const ::GenericAttr&);
namespace ecf {
void to_json(ecf::ojson&, const ecf::TimeAttr&);
void to_json(ecf::ojson&, const ecf::TodayAttr&);
void to_json(ecf::ojson&, const ecf::CronAttr&);
void to_json(ecf::ojson&, const ecf::Flag&);
void to_json(ecf::ojson&, const ecf::LateAttr*);
void to_json(ecf::ojson&, const ecf::LateAttr&);
void to_json(ecf::ojson&, const ecf::TimeSlot&);
void to_json(ecf::ojson&, const ecf::AutoCancelAttr*);
void to_json(ecf::ojson&, const ecf::AutoCancelAttr&);
void to_json(ecf::ojson&, const ecf::AutoArchiveAttr*);
void to_json(ecf::ojson&, const ecf::AutoArchiveAttr&);
void to_json(ecf::ojson&, const ecf::AutoRestoreAttr*);
void to_json(ecf::ojson&, const ecf::AutoRestoreAttr&);
} // namespace ecf

#endif /* ecflow_http_TypeToJson_HPP */
