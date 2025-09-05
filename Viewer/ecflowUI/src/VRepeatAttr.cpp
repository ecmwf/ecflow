/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "VRepeatAttr.hpp"

#include <sstream>

#include "VAttributeType.hpp"
#include "VNode.hpp"
#include "ecflow/attribute/RepeatRange.hpp"
#include "ecflow/core/Calendar.hpp"

std::string VRepeatDateAttr::subType_("date");
std::string VRepeatDateTimeAttr::subType_("datetime");
std::string VRepeatDateListAttr::subType_("datelist");
std::string VRepeatIntAttr::subType_("integer");
std::string VRepeatStringAttr::subType_("string");
std::string VRepeatEnumAttr::subType_("enumerated");
std::string VRepeatDayAttr::subType_("day");

//================================
// VRepeatAttrType
//================================

VRepeatAttrType::VRepeatAttrType() : VAttributeType("repeat") {
    dataCount_                       = 10;
    searchKeyToData_["repeat_name"]  = NameIndex;
    searchKeyToData_["repeat_value"] = ValueIndex;
    searchKeyToData_["name"]         = NameIndex;
    scanProc_                        = VRepeatAttr::scan;
}

QString VRepeatAttrType::toolTip(QStringList d) const {
    QString t = "<b>Type:</b> Repeat";
    if (d.count() == dataCount_) {
        t += " " + d[SubtypeIndex] + "<br>";

        if (d[SubtypeIndex] != "day") {
            t += "<b>Name:</b> " + d[NameIndex] + "<br>";
            t += "<b>Value:</b> " + d[ValueIndex] + "<br>";
            t += "<b>Start:</b> " + d[StartIndex] + "<br>";
            t += "<b>End:</b> " + d[EndIndex];

            if (d[SubtypeIndex] == "integer" || d[SubtypeIndex] == "date") {
                t += "<br><b>Step:</b> " + d[StepIndex];
            }
            t += "<br><b>Complete: </b> " + d[ProgressIndex];
        }
        else {
            t += "<b>Step:</b> " + d[StepIndex];
        }
    }

    return t;
}

QString VRepeatAttrType::definition(QStringList d) const {
    QString t = "repeat";
    if (d.count() == dataCount_) {
        QString subType = d[SubtypeIndex];

        t += " " + subType;

        if (subType == "integer" || subType == "date" || subType == "datetime") {
            t += " " + d[NameIndex];
            t += " " + d[StartIndex];
            t += " " + d[EndIndex];
            t += " " + d[StepIndex];
        }
        else if (subType == "string" || subType == "enumerated" || subType == "datelist") {
            t += " " + d[NameIndex];
            t += " " + d[AllValuesIndex];
        }
        else {
            t += " " + d[StepIndex];
        }
    }
    return t;
}

void VRepeatAttrType::encode(const Repeat& r,
                             const VRepeatAttr* ra,
                             QStringList& data,
                             const std::string& type,
                             QString allValues) const {
    // We try to avoid creating a VRepeat object everytime we are here
    // std::string type=VRepeat::type(r);

    data << qName_;
    data << QString::fromStdString(type);              // TypeIndex
    data << QString::fromStdString(r.name());          // SubtypeIndex
    data << QString::fromStdString(r.valueAsString()); // NameIndex
    data << ra->startValue();                          // StartIndex
    data << ra->endValue();                            // EndIndex
    if (type == "datetime") {
        data << QString::fromStdString(
            ecf::Duration::format(ecf::Duration{std::chrono::seconds{r.step()}})); // StepIndex
    }
    else {
        data << QString::number(r.step()); // StepIndex
    }
    data << allValues;                              // AllValuesIndex
    data << QString::number(ra->currentPosition()); // CurrentPosIndex
    ecf::Limits limits = ecf::limits_of(r.repeatBase());
    data << QString::number(limits.current) + " of " + QString::number(limits.end); // ProgressIndex
}

//=====================================================
//
// VRepeatAttr
//
//=====================================================

VRepeatAttr::VRepeatAttr(VNode* parent) : VAttribute(parent, 0) {
    // name_=e.name_or_number();
}

VAttributeType* VRepeatAttr::type() const {
    static VAttributeType* atype = VAttributeType::find("repeat");
    return atype;
}

int VRepeatAttr::step() const {
    if (node_ptr node = parent_->node()) {
        const Repeat& r = node->repeat();
        return r.step();
    }
    return 0;
}

QStringList VRepeatAttr::data(bool /*firstLine*/) const {
    static auto* atype = static_cast<VRepeatAttrType*>(type());
    QStringList s;
    if (parent_->node_) {
        const Repeat& r = parent_->node_->repeat();
        atype->encode(r, this, s, subType(), allValues());
    }
    return s;
}

std::string VRepeatAttr::strName() const {
    if (parent_->node_) {
        const Repeat& r = parent_->node_->repeat();
        if (r.empty() == false) {
            return r.name();
        }
    }
    return {};
}

void VRepeatAttr::scan(VNode* vnode, std::vector<VAttribute*>& vec) {
    if (vnode->node_) {
        const Repeat& r = vnode->node_->repeat();
        if (r.empty() == false && r.repeatBase()) {
            VRepeatAttr* a = nullptr;

            if (r.repeatBase()->isDate()) {
                a = new VRepeatDateAttr(vnode);
            }
            else if (r.repeatBase()->isDateTime()) {
                a = new VRepeatDateTimeAttr(vnode);
            }
            else if (r.repeatBase()->isDateList()) {
                a = new VRepeatDateListAttr(vnode);
            }
            else if (r.repeatBase()->isInteger()) {
                a = new VRepeatIntAttr(vnode);
            }
            else if (r.repeatBase()->isString()) {
                a = new VRepeatStringAttr(vnode);
            }
            else if (r.repeatBase()->isEnumerated()) {
                a = new VRepeatEnumAttr(vnode);
            }
            else if (r.repeatBase()->isDay()) {
                a = new VRepeatDayAttr(vnode);
            }

            if (a) {
                vec.push_back(a);
            }
        }
    }
}

QString VRepeatAttr::allValues() const {
    return {};
}

//=====================================================
//
// VRepeatDateAttr
//
//=====================================================

int VRepeatDateAttr::endIndex() const {
    if (node_ptr node = parent_->node()) {
        const Repeat& r = node->repeat();
        if (r.step() > 0) {
            long jStart = ecf::CalendarDate(r.start()).as_julian_day().value();
            long jEnd   = ecf::CalendarDate(r.end()).as_julian_day().value();

            int index = (jEnd - jStart) / r.step();
            long val  = jStart + index * r.step();
            while (val > jEnd && index >= 1) {
                index--;
                val = jStart + index * r.step();
            }
            return index;
        }
    }
    return 0;
}

int VRepeatDateAttr::currentIndex() const {
    if (node_ptr node = parent_->node()) {
        const Repeat& r = node->repeat();
        int cur         = (ecf::CalendarDate(r.index_or_value()).as_julian_day().value() -
                   ecf::CalendarDate(r.start()).as_julian_day().value()) /
                  r.step();
        return cur;
    }
    return 0;
}

QString VRepeatDateAttr::startValue() const {
    if (node_ptr node = parent_->node()) {
        const Repeat& r = node->repeat();
        return QString::number(r.start());
    }
    return {};
}

QString VRepeatDateAttr::endValue() const {
    if (node_ptr node = parent_->node()) {
        const Repeat& r = node->repeat();
        return QString::number(r.end());
    }
    return {};
}

std::string VRepeatDateAttr::value(int index) const {
    std::stringstream ss;
    if (node_ptr node = parent_->node()) {
        const Repeat& r = node->repeat();
        auto date       = ecf::CalendarDate(r.start()) + (index * r.step());
        ss << date.value();
    }

    return ss.str();
}

int VRepeatDateAttr::currentPosition() const {
    if (node_ptr node = parent_->node()) {
        const Repeat& r = node->repeat();
        if (r.start() == r.end()) {
            return -1;
        }
        else if (r.value() == r.start()) {
            return 0;
        }
        else if (r.value() == r.end() || ecf::CalendarDate(r.value()) + r.step() > ecf::CalendarDate(r.end())) {
            return 2;
        }
        else {
            return 1;
        }
    }

    return -1;
}

//=====================================================
//
// VRepeatDateTimeAttr
//
//=====================================================

int VRepeatDateTimeAttr::endIndex() const {
    if (node_ptr node = parent_->node()) {
        auto& r  = node->repeat();
        auto rng = ecf::make_range<RepeatDateTime>(r);
        auto idx = rng.end();
        idx      = std::min(idx, rng.size() - 1); // ensure idx is within range [0, size-1]
        return idx;
    }
    return 0;
}

int VRepeatDateTimeAttr::currentIndex() const {
    if (node_ptr node = parent_->node()) {
        auto& r  = node->repeat();
        auto rng = ecf::make_range<RepeatDateTime>(r);
        auto idx = rng.current_index();
        return idx;
    }
    return 0;
}

QString VRepeatDateTimeAttr::startValue() const {
    if (node_ptr node = parent_->node()) {
        auto& r  = node->repeat();
        auto rng = ecf::make_range<RepeatDateTime>(r);
        auto val = ecf::front(rng); // front(rng) returns the first value in the range;
        auto fmt = ecf::Instant::format(val);
        return QString::fromStdString(fmt);
    }
    return {};
}

QString VRepeatDateTimeAttr::endValue() const {
    if (node_ptr node = parent_->node()) {
        auto& r  = node->repeat();
        auto rng = ecf::make_range<RepeatDateTime>(r);
        auto val = ecf::back(rng); // back(rng) returns the last value in the range;
        auto fmt = ecf::Instant::format(val);
        return QString::fromStdString(fmt);
    }
    return {};
}

std::string VRepeatDateTimeAttr::value(int index) const {
    std::stringstream ss;
    if (node_ptr node = parent_->node()) {
        auto& r  = node->repeat();
        auto rng = ecf::make_range<RepeatDateTime>(r);
        auto val = rng.at(index);
        auto fmt = ecf::Instant::format(val);
        ss << fmt;
    }
    return ss.str();
}

int VRepeatDateTimeAttr::currentPosition() const {
    if (node_ptr node = parent_->node()) {
        auto& r = node->repeat();
        if (r.start() == r.end()) {
            return -1;
        }
        else if (r.value() == r.start()) {
            return 0;
        }
        else if (r.value() == r.end() || ecf::coerce_from_seconds_into_instant(r.value() + r.step()) >
                                             ecf::coerce_from_seconds_into_instant(r.end())) {
            return 2;
        }
        else {
            return 1;
        }
    }

    return -1;
}

//=====================================================
//
// VRepeatDateListAttr
//
//=====================================================

std::string VRepeatDateListAttr::value(int index) const {
    if (node_ptr node = parent_->node()) {
        const Repeat& r = node->repeat();
        return r.value_as_string(index);
    }
    return {};
}

int VRepeatDateListAttr::endIndex() const {
    if (node_ptr node = parent_->node()) {
        const Repeat& r = node->repeat();
        if (auto* rdl = static_cast<const RepeatDateList*>(r.repeatBase())) {
            return rdl->indexNum() - 1;
        }
    }
    return 0;
}

int VRepeatDateListAttr::currentIndex() const {
    if (node_ptr node = parent_->node()) {
        const Repeat& r = node->repeat();
        return r.index_or_value();
    }
    return 0;
}

QString VRepeatDateListAttr::startValue() const {
    return QString::fromStdString(value(0));
}

QString VRepeatDateListAttr::endValue() const {
    return QString::fromStdString(value(endIndex()));
}

QString VRepeatDateListAttr::allValues() const {
    QString vals;

    if (node_ptr node = parent_->node()) {
        const Repeat& r = node->repeat();

        int start = 0;
        int end   = endIndex();

        if (end <= start) {
            return {};
        }

        for (int i = start; i <= end; i++) {
            if (!vals.isEmpty()) {
                vals += " ";
            }
            vals += QString::fromStdString(r.value_as_string(i));
        }
        return vals;
    }
    return vals;
}

namespace {

/**
 * This helper function determines the position of the repeat value in the list,
 * indicating if the value is the first, middle, or last in the list.
 *
 * @tparam REPEAT
 * @param repeat
 * @return 0 == First, 1 == Middle, 2 == Last, -1 == Invalid
 */
template <typename REPEAT>
int get_repeat_position(const REPEAT& repeat) {
    if (repeat.indexNum() < 2) {
        return -1; // == Invalid
    }

    auto limits = ecf::limits_of(&repeat);

    if (limits.is_first()) {
        return 0; // == First
    }

    if (limits.is_last()) {
        return 2; // == Last
    }

    return 1; // == Middle
}

} // namespace

int VRepeatDateListAttr::currentPosition() const {
    if (node_ptr node = parent_->node()) {
        const Repeat& r = node->repeat();
        if (auto* rdl = static_cast<const RepeatDateList*>(r.repeatBase())) {
            return get_repeat_position(*rdl);
        }
    }
    return -1;
}

//=====================================================
//
// VRepeatIntAttr
//
//=====================================================

int VRepeatIntAttr::endIndex() const {
    if (node_ptr node = parent_->node()) {
        const Repeat& r = node->repeat();
        if (r.step() > 0) {
            int index = (r.end() - r.start()) / r.step();
            int val   = r.start() + index * r.step();
            while (val > r.end() && index >= 1) {
                index--;
                val = r.start() + index * r.step();
            }
            return index;
        }
    }
    return 0;
}

int VRepeatIntAttr::currentIndex() const {
    if (node_ptr node = parent_->node()) {
        const Repeat& r = node->repeat();
        if (r.step() > 0) {
            return (r.index_or_value() - r.start()) / r.step();
        }
    }
    return 0;
}

std::string VRepeatIntAttr::value(int index) const {
    std::stringstream ss;
    if (node_ptr node = parent_->node()) {
        const Repeat& r = node->repeat();
        ss << r.start() + index * r.step();
    }
    return ss.str();
}

QString VRepeatIntAttr::startValue() const {
    if (node_ptr node = parent_->node()) {
        const Repeat& r = node->repeat();
        return QString::number(r.start());
    }
    return {};
}

QString VRepeatIntAttr::endValue() const {
    if (node_ptr node = parent_->node()) {
        const Repeat& r = node->repeat();
        return QString::number(r.end());
    }
    return {};
}

int VRepeatIntAttr::currentPosition() const {
    if (node_ptr node = parent_->node()) {
        const Repeat& r = node->repeat();
        if (r.start() == r.end()) {
            return -1; // == Invalid
        }
        else if (r.value() == r.start()) {
            return 0; // == First
        }
        else if (r.value() == r.end() || r.value() + r.step() > r.end()) {
            return 2; // == Last
        }
        else {
            return 1; // == Middle
        }
    }
    return -1;
}

//=====================================================
//
// VRepeatDayAttr
//
//=====================================================

std::string VRepeatDayAttr::value(int /*index*/) const {
    std::stringstream ss;
    if (node_ptr node = parent_->node()) {
        const Repeat& r = node->repeat();
        ss << r.step();
    }
    return ss.str();
}

QString VRepeatDayAttr::startValue() const {
    return {};
}

QString VRepeatDayAttr::endValue() const {
    return {};
}

//=====================================================
//
// VRepeatEnumAttr
//
//=====================================================

std::string VRepeatEnumAttr::value(int index) const {
    if (node_ptr node = parent_->node()) {
        const Repeat& r = node->repeat();
        return r.value_as_string(index);
    }
    return {};
}

int VRepeatEnumAttr::endIndex() const {
    if (node_ptr node = parent_->node()) {
        const Repeat& r = node->repeat();
        return r.end();
    }
    return 0;
}

int VRepeatEnumAttr::currentIndex() const {
    if (node_ptr node = parent_->node()) {
        const Repeat& r = node->repeat();
        return r.index_or_value();
    }
    return 0;
}

QString VRepeatEnumAttr::startValue() const {
    return QString::fromStdString(value(startIndex()));
}

QString VRepeatEnumAttr::endValue() const {
    return QString::fromStdString(value(endIndex()));
}

QString VRepeatEnumAttr::allValues() const {
    QString vals;

    if (node_ptr node = parent_->node()) {
        const Repeat& r = node->repeat();
        int start       = r.start();
        int end         = r.end();

        if (end <= start) {
            return {};
        }

        if (end - start > 1) {
            for (int i = start; i <= end; i++) {
                if (!vals.isEmpty()) {
                    vals += " ";
                }
                vals += "\"" + QString::fromStdString(r.value_as_string(i)) + "\"";
            }
            return vals;
        }
    }
    return vals;
}

int VRepeatEnumAttr::currentPosition() const {
    if (node_ptr node = parent_->node()) {
        const Repeat& r = node->repeat();
        if (auto* repeat = static_cast<const RepeatEnumerated*>(r.repeatBase()); repeat) {
            return get_repeat_position(*repeat);
        }
    }
    return -1;
}

//=====================================================
//
// VRepeatStringAttr
//
//=====================================================

std::string VRepeatStringAttr::value(int index) const {
    if (node_ptr node = parent_->node()) {
        const Repeat& r = node->repeat();
        return r.value_as_string(index);
    }
    return {};
}

int VRepeatStringAttr::endIndex() const {
    if (node_ptr node = parent_->node()) {
        const Repeat& r = node->repeat();
        return r.end();
    }
    return 0;
}

int VRepeatStringAttr::currentIndex() const {
    if (node_ptr node = parent_->node()) {
        const Repeat& r = node->repeat();
        return r.index_or_value();
    }
    return 0;
}

QString VRepeatStringAttr::startValue() const {
    return QString::fromStdString(value(startIndex()));
}

QString VRepeatStringAttr::endValue() const {
    return QString::fromStdString(value(endIndex()));
}

QString VRepeatStringAttr::allValues() const {
    QString vals;

    if (node_ptr node = parent_->node()) {
        const Repeat& r = node->repeat();
        int start       = r.start();
        int end         = r.end();

        if (end <= start) {
            return {};
        }

        if (end - start > 1) {
            for (int i = start; i <= end; i++) {
                if (!vals.isEmpty()) {
                    vals += " ";
                }
                vals += "\"" + QString::fromStdString(r.value_as_string(i)) + "\"";
            }
            return vals;
        }
    }
    return vals;
}

int VRepeatStringAttr::currentPosition() const {
    if (node_ptr node = parent_->node()) {
        const Repeat& r = node->repeat();
        if (auto* rdl = static_cast<const RepeatString*>(r.repeatBase())) {
            return get_repeat_position(*rdl);
        }
    }
    return -1;
}
