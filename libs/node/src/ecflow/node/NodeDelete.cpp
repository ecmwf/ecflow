/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include <stdexcept>

#include "ecflow/attribute/AutoArchiveAttr.hpp"
#include "ecflow/attribute/AutoCancelAttr.hpp"
#include "ecflow/attribute/LateAttr.hpp"
#include "ecflow/core/Ecf.hpp"
#include "ecflow/core/Stl.hpp"
#include "ecflow/node/AutoRestoreAttr.hpp"
#include "ecflow/node/AvisoAttr.hpp"
#include "ecflow/node/Expression.hpp"
#include "ecflow/node/Limit.hpp"
#include "ecflow/node/MiscAttrs.hpp"
#include "ecflow/node/Node.hpp"

using namespace ecf;
using namespace std;

void Node::deleteTime(const std::string& name) {
    if (name.empty()) {
        times_.clear(); // delete all
        state_change_no_ = Ecf::incr_state_change_no();
#ifdef DEBUG_STATE_CHANGE_NO
        std::cout << "Node::deleteTime\n";
#endif
        return;
    }
    TimeAttr attr(TimeSeries::create(name)); // can throw if parse fails
    delete_time(attr);                       // can throw if search fails
}

void Node::delete_time(const ecf::TimeAttr& attr) {
    auto found = ecf::algorithm::find_by(times_, [&](const auto& item) { return item.structureEquals(attr); });

    if (found == std::end(times_)) {
        throw std::runtime_error("Node::delete_time: Cannot find time attribute: ");
    }

    times_.erase(found);
    state_change_no_ = Ecf::incr_state_change_no();

#ifdef DEBUG_STATE_CHANGE_NO
    std::cout << "Node::delete_time\n";
#endif
}

void Node::deleteToday(const std::string& name) {
    if (name.empty()) {
        todays_.clear();
        state_change_no_ = Ecf::incr_state_change_no();
#ifdef DEBUG_STATE_CHANGE_NO
        std::cout << "Node::deleteToday\n";
#endif
        return;
    }

    TodayAttr attr(TimeSeries::create(name)); // can throw if parse fails
    delete_today(attr);                       // can throw if search fails
}

void Node::delete_today(const ecf::TodayAttr& attr) {
    // Don't use '==' since that compares additional state like free_
    auto found = ecf::algorithm::find_by(todays_, [&](const auto& item) { return item.structureEquals(attr); });

    if (found == std::end(todays_)) {
        throw std::runtime_error("Node::delete_today: Cannot find today attribute: " + attr.toString());
    }

    todays_.erase(found);
    state_change_no_ = Ecf::incr_state_change_no();

#ifdef DEBUG_STATE_CHANGE_NO
    std::cout << "Node::delete_today\n";
#endif
}

void Node::deleteDate(const std::string& name) {
    if (name.empty()) {
        dates_.clear();
        state_change_no_ = Ecf::incr_state_change_no();
#ifdef DEBUG_STATE_CHANGE_NO
        std::cout << "Node::deleteDate\n";
#endif
        return;
    }

    DateAttr attr(DateAttr::create(name)); // can throw if parse fails
    delete_date(attr);                     // can throw if search fails
}

void Node::delete_date(const DateAttr& attr) {
    // Don't use '==' since that compares additional state like free_
    auto found = ecf::algorithm::find_by(dates_, [&](const auto& item) { return item.structureEquals(attr); });

    if (found == std::end(dates_)) {
        throw std::runtime_error("Node::delete_date: Cannot find date attribute: " + attr.toString());
    }

    dates_.erase(found);
    state_change_no_ = Ecf::incr_state_change_no();
#ifdef DEBUG_STATE_CHANGE_NO
    std::cout << "Node::delete_date\n";
#endif
}

void Node::deleteDay(const std::string& name) {
    if (name.empty()) {
        days_.clear();
        state_change_no_ = Ecf::incr_state_change_no();
#ifdef DEBUG_STATE_CHANGE_NO
        std::cout << "Node::deleteDay\n";
#endif
        return;
    }

    DayAttr attr(DayAttr::create(name)); // can throw if parse fails.
    delete_day(attr);                    // can throw if search fails
}

void Node::delete_day(const DayAttr& attr) {
    // Don't use '==' since that compares additional state like free_
    auto found = ecf::algorithm::find_by(days_, [&](const auto& item) { return item.structureEquals(attr); });

    if (found == std::end(days_)) {
        throw std::runtime_error("Node::delete_day: Cannot find day attribute: " + attr.toString());
    }

    days_.erase(found);
    state_change_no_ = Ecf::incr_state_change_no();
#ifdef DEBUG_STATE_CHANGE_NO
    std::cout << "Node::delete_day\n";
#endif
}

void Node::deleteCron(const std::string& name) {
    if (name.empty()) {
        crons_.clear();
        state_change_no_ = Ecf::incr_state_change_no();
#ifdef DEBUG_STATE_CHANGE_NO
        std::cout << "Node::deleteCron\n";
#endif
        return;
    }

    CronAttr attr = CronAttr::create(name); // can throw if parse fails
    delete_cron(attr);                      // can throw if search fails
}

void Node::delete_cron(const ecf::CronAttr& attr) {
    // Don't use '==' since that compares additional state like free_
    auto found = ecf::algorithm::find_by(crons_, [&](const auto& item) { return item.structureEquals(attr); });

    if (found == std::end(crons_)) {
        throw std::runtime_error("Node::delete_cron: Cannot find cron attribute: " + attr.toString());
    }

    crons_.erase(found);
    state_change_no_ = Ecf::incr_state_change_no();
#ifdef DEBUG_STATE_CHANGE_NO
    std::cout << "Node::delete_cron\n";
#endif
}

void Node::deleteVariable(const std::string& name) {
    if (name.empty()) {
        vars_.clear(); // delete all
        state_change_no_ = Ecf::incr_state_change_no();

#ifdef DEBUG_STATE_CHANGE_NO
        std::cout << "Node::deleteVariable\n";
#endif
        return;
    }

    auto found = ecf::algorithm::find_by_name(vars_, name);

    if (found == std::end(vars_)) {
        throw std::runtime_error("Node::deleteVariable: Cannot find 'user' variable of name " + name);
    }

    vars_.erase(found);
    state_change_no_ = Ecf::incr_state_change_no();
#ifdef DEBUG_STATE_CHANGE_NO
    std::cout << "Node::deleteVariable\n";
#endif
}

void Node::delete_variable_no_error(const std::string& name) {
    if (name.empty()) {
#ifdef DEBUG_STATE_CHANGE_NO
        std::cout << "Node::delete_variable_no_error\n";
#endif
        return;
    }

    auto found = ecf::algorithm::find_by_name(vars_, name);

    if (found != std::end(vars_)) {
        vars_.erase(found);
        state_change_no_ = Ecf::incr_state_change_no();
#ifdef DEBUG_STATE_CHANGE_NO
        std::cout << "Node::delete_variable_no_error\n";
#endif
    }
}

void Node::deleteEvent(const std::string& name) {
    if (name.empty()) {
        events_.clear();
        state_change_no_ = Ecf::incr_state_change_no();
#ifdef DEBUG_STATE_CHANGE_NO
        std::cout << "Node::deleteEvent\n";
#endif
        return;
    }

    auto found = ecf::algorithm::find_by(events_, [&](const auto& item) { return item.name_or_number() == name; });

    if (found == std::end(events_)) {
        throw std::runtime_error("Node::deleteEvent: Cannot find event: " + name);
    }

    events_.erase(found);
    state_change_no_ = Ecf::incr_state_change_no();
#ifdef DEBUG_STATE_CHANGE_NO
    std::cout << "Node::deleteEvent\n";
#endif
}

void Node::deleteMeter(const std::string& name) {
    if (name.empty()) {
        meters_.clear();
        state_change_no_ = Ecf::incr_state_change_no();
#ifdef DEBUG_STATE_CHANGE_NO
        std::cout << "Expression::clearFree()\n";
#endif
        return;
    }

    auto found = ecf::algorithm::find_by_name(meters_, name);

    if (found == std::end(meters_)) {
        throw std::runtime_error("Node::deleteMeter: Cannot find meter: " + name);
    }

    meters_.erase(found);
    state_change_no_ = Ecf::incr_state_change_no();
#ifdef DEBUG_STATE_CHANGE_NO
    std::cout << "Expression::clearFree()\n";
#endif
}

void Node::deleteLabel(const std::string& name) {
    if (name.empty()) {
        labels_.clear();
        state_change_no_ = Ecf::incr_state_change_no();
#ifdef DEBUG_STATE_CHANGE_NO
        std::cout << "Node::deleteLabel\n";
#endif
        return;
    }

    auto found = ecf::algorithm::find_by_name(labels_, name);

    if (found == std::end(labels_)) {
        throw std::runtime_error("Node::deleteLabel: Cannot find label: " + name);
    }

    labels_.erase(found);
    state_change_no_ = Ecf::incr_state_change_no();
#ifdef DEBUG_STATE_CHANGE_NO
    std::cout << "Node::deleteLabel\n";
#endif
}

void Node::deleteAviso(const std::string& name) {
    if (name.empty()) {
        avisos_.clear();
        state_change_no_ = Ecf::incr_state_change_no();
#ifdef DEBUG_STATE_CHANGE_NO
        std::cout << "Node::deleteAviso\n";
#endif
        return;
    }

    auto found = ecf::algorithm::find_by_name(avisos_, name);

    if (found == std::end(avisos_)) {
        throw std::runtime_error("Node::deleteAviso: Cannot find aviso: " + name);
    }

    avisos_.erase(found);
    state_change_no_ = Ecf::incr_state_change_no();
#ifdef DEBUG_STATE_CHANGE_NO
    std::cout << "Node::deleteAviso\n";
#endif
}

void Node::delete_queue(const std::string& name) {
    if (misc_attrs_) {
        misc_attrs_->delete_queue(name);
        return;
    }
    throw std::runtime_error("Node::delete_queue: Cannot find queue: " + name);
}

void Node::delete_generic(const std::string& name) {
    if (misc_attrs_) {
        misc_attrs_->delete_generic(name);
        return;
    }
    throw std::runtime_error("Node::delete_generic : Cannot find generic: " + name);
}

void Node::deleteTrigger() {
    if (t_expr_) {
        t_expr_.reset(nullptr);
        state_change_no_ = Ecf::incr_state_change_no();
#ifdef DEBUG_STATE_CHANGE_NO
        std::cout << "Node::deleteTrigger()\n";
#endif
    }
}

void Node::deleteComplete() {
    if (c_expr_) {
        c_expr_.reset(nullptr);
        state_change_no_ = Ecf::incr_state_change_no();
#ifdef DEBUG_STATE_CHANGE_NO
        std::cout << "Node::deleteComplete()\n";
#endif
    }
}

void Node::deleteRepeat() {
    if (!repeat_.empty()) {
        repeat_.clear(); // will delete the pimple
        state_change_no_ = Ecf::incr_state_change_no();
#ifdef DEBUG_STATE_CHANGE_NO
        std::cout << "Node::deleteRepeat())\n";
#endif
    }
}

void Node::deleteLimit(const std::string& name) {
    if (name.empty()) {
        limits_.clear();
        state_change_no_ = Ecf::incr_state_change_no();
#ifdef DEBUG_STATE_CHANGE_NO
        std::cout << "Node::deleteLimit\n";
#endif
        return;
    }

    auto found = ecf::algorithm::find_by_name(limits_, name);

    if (found == std::end(limits_)) {
        throw std::runtime_error("Node::deleteLimit: Cannot find limit: " + name);
    }

    limits_.erase(found);
    state_change_no_ = Ecf::incr_state_change_no();
#ifdef DEBUG_STATE_CHANGE_NO
    std::cout << "Node::deleteLimit\n";
#endif
}

void Node::delete_limit_path(const std::string& name, const std::string& path) {
    if (name.empty()) {
        throw std::runtime_error("Node::delete_limit_path: the limit name must be provided");
    }
    if (path.empty()) {
        throw std::runtime_error("Node::delete_limit_path: the limit path must be provided");
    }

    auto found = ecf::algorithm::find_by_name(limits_, name);

    if (found == std::end(limits_)) {
        throw std::runtime_error("Node::delete_limit_path: Cannot find limit: " + name);
    }

    (*found)->delete_path(path);
}

void Node::deleteInlimit(const std::string& name) {
    // if name exists but no corresponding in limit found raises an exception
    if (inLimitMgr_.deleteInlimit(name)) {
        state_change_no_ = Ecf::incr_state_change_no();
#ifdef DEBUG_STATE_CHANGE_NO
        std::cout << "Node::deleteInlimit\n";
#endif
    }
}

void Node::delete_misc_attrs_if_empty() {
    if (misc_attrs_ && misc_attrs_->empty()) {
        misc_attrs_.reset(nullptr);
    }
}

void Node::deleteZombie(const std::string& zombie_type) {
    if (misc_attrs_) {
        misc_attrs_->deleteZombie(zombie_type);
        delete_misc_attrs_if_empty();
    }
}

void Node::delete_zombie(Child::ZombieType zt) {
    if (misc_attrs_) {
        misc_attrs_->delete_zombie(zt);
        delete_misc_attrs_if_empty();
    }
}

void Node::deleteLate() {
    late_.reset(nullptr);
    state_change_no_ = Ecf::incr_state_change_no();
}

void Node::deleteAutoCancel() {
    auto_cancel_.reset(nullptr);
    state_change_no_ = Ecf::incr_state_change_no();
}

void Node::deleteAutoArchive() {
    auto_archive_.reset(nullptr);
    state_change_no_ = Ecf::incr_state_change_no();
}

void Node::deleteAutoRestore() {
    auto_restore_.reset(nullptr);
    state_change_no_ = Ecf::incr_state_change_no();
}
