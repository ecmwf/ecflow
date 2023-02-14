#ifndef TYPE_TO_JSON_HPP
#define TYPE_TO_JSON_HPP

/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        : TypeToJson
// Author      : partio
// Revision    : $Revision$
//
// Copyright 2009- ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
// Description :
//
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8

#include <AutoArchiveAttr.hpp>
#include <AutoCancelAttr.hpp>
#include <AutoRestoreAttr.hpp>
#include <ClockAttr.hpp>
#include <CronAttr.hpp>
#include <DateAttr.hpp>
#include <DayAttr.hpp>
#include <Expression.hpp>
#include <Flag.hpp>
#include <GenericAttr.hpp>
#include <InLimit.hpp>
#include <LateAttr.hpp>
#include <Limit.hpp>
#include <LimitFwd.hpp>
#include <NodeAttr.hpp>
#include <QueueAttr.hpp>
#include <RepeatAttr.hpp>
#include <Stats.hpp>
#include <TimeAttr.hpp>
#include <TimeSlot.hpp>
#include <TodayAttr.hpp>
#include <Variable.hpp>
#include <ZombieAttr.hpp>
#include <nlohmann/json.hpp>

void to_json(nlohmann::json&, const Meter&);
void to_json(nlohmann::json&, const Label&);
void to_json(nlohmann::json&, const Variable&);
void to_json(nlohmann::json&, const Event&);
void to_json(nlohmann::json&, const limit_ptr&);
void to_json(nlohmann::json&, const InLimit&);
void to_json(nlohmann::json&, const DateAttr&);
void to_json(nlohmann::json&, const DayAttr&);
void to_json(nlohmann::json&, const RepeatDate&);
void to_json(nlohmann::json&, const RepeatDay&);
void to_json(nlohmann::json&, const RepeatDateList&);
void to_json(nlohmann::json&, const RepeatInteger&);
void to_json(nlohmann::json&, const RepeatEnumerated&);
void to_json(nlohmann::json&, const RepeatString&);
void to_json(nlohmann::json&, const Repeat&);
void to_json(nlohmann::json&, const Stats&);
void to_json(nlohmann::json&, const Expression*);
void to_json(nlohmann::json&, const Expression&);
void to_json(nlohmann::json&, const QueueAttr&);
void to_json(nlohmann::json&, const ZombieAttr&);
void to_json(nlohmann::json&, const GenericAttr&);

namespace ecf {
void to_json(nlohmann::json&, const TimeAttr&);
void to_json(nlohmann::json&, const TodayAttr&);
void to_json(nlohmann::json&, const CronAttr&);
void to_json(nlohmann::json&, const Flag&);
void to_json(nlohmann::json&, const LateAttr*);
void to_json(nlohmann::json&, const LateAttr&);
void to_json(nlohmann::json&, const TimeSlot&);
void to_json(nlohmann::json&, const AutoCancelAttr*);
void to_json(nlohmann::json&, const AutoCancelAttr&);
void to_json(nlohmann::json&, const AutoArchiveAttr*);
void to_json(nlohmann::json&, const AutoArchiveAttr&);
void to_json(nlohmann::json&, const AutoRestoreAttr*);
void to_json(nlohmann::json&, const AutoRestoreAttr&);
} // namespace ecf

#endif
