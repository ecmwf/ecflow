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

#include "AutoArchiveAttr.hpp"
#include "AutoCancelAttr.hpp"
#include "AutoRestoreAttr.hpp"
#include "ClockAttr.hpp"
#include "CronAttr.hpp"
#include "DateAttr.hpp"
#include "DayAttr.hpp"
#include "Expression.hpp"
#include "Flag.hpp"
#include "GenericAttr.hpp"
#include "InLimit.hpp"
#include "JSON.hpp"
#include "LateAttr.hpp"
#include "Limit.hpp"
#include "LimitFwd.hpp"
#include "NodeAttr.hpp"
#include "QueueAttr.hpp"
#include "RepeatAttr.hpp"
#include "Stats.hpp"
#include "TimeAttr.hpp"
#include "TodayAttr.hpp"
#include "Variable.hpp"
#include "ZombieAttr.hpp"
#include "ecflow/core/TimeSlot.hpp"

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
