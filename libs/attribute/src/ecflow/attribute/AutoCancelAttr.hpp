/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_attribute_AutoCancelAttr_HPP
#define ecflow_attribute_AutoCancelAttr_HPP

#include <cstdint>

#include "ecflow/core/TimeSlot.hpp"

namespace ecf {
class Calendar;
} // namespace ecf

namespace ecf {

// Use compiler ,  destructor, assignment, copy constructor
class AutoCancelAttr {
public:
    AutoCancelAttr() = default;
    AutoCancelAttr(int hour, int minute, bool relative) : time_(hour, minute), relative_(relative) {}
    AutoCancelAttr(const TimeSlot& ts, bool relative) : time_(ts), relative_(relative) {}
    explicit AutoCancelAttr(int days) : time_(TimeSlot(days * 24, 0)), days_(true) {}

    bool operator==(const AutoCancelAttr& rhs) const;
    bool operator<(const AutoCancelAttr& rhs) const { return time_ < rhs.time(); }
    bool isFree(const ecf::Calendar&, const boost::posix_time::time_duration& suiteDurationAtComplete) const;

    std::string toString() const;

    const TimeSlot& time() const { return time_; }
    bool relative() const { return relative_; }
    bool days() const { return days_; }

public:
    void write(std::string& ret) const;

private:
    TimeSlot time_;
    bool relative_{true};
    bool days_{false};

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar, std::uint32_t const /*version*/);
};

} // namespace ecf

#endif /* ecflow_attribute_AutoCancelAttr_HPP */
