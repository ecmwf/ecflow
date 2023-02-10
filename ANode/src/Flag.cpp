/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #12 $
//
// Copyright 2009- ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
// Description :
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8

#include "Flag.hpp"

#include <stdexcept>

#include "Ecf.hpp"
#include "Serialization.hpp"
#include "Str.hpp"

namespace ecf {

void Flag::set(Flag::Type flag) {
    if (!is_set(flag)) {
        // minimize changes to state_change_no_
        flag_ |= (1 << flag);
        state_change_no_ = Ecf::incr_state_change_no();
    }
}

void Flag::clear(Flag::Type flag) {
    if (is_set(flag)) {
        // minimize changes to state_change_no_
        flag_ &= ~(1 << flag);
        state_change_no_ = Ecf::incr_state_change_no();
    }
}

void Flag::reset() {
    flag_            = 0;
    state_change_no_ = Ecf::incr_state_change_no();
}

std::vector<Flag::Type> Flag::list() {
    std::vector<Flag::Type> ret;
    ret.reserve(24);
    ret.push_back(Flag::FORCE_ABORT);
    ret.push_back(Flag::USER_EDIT);
    ret.push_back(Flag::TASK_ABORTED);
    ret.push_back(Flag::EDIT_FAILED);
    ret.push_back(Flag::JOBCMD_FAILED);
    ret.push_back(Flag::KILLCMD_FAILED);
    ret.push_back(Flag::STATUSCMD_FAILED);
    ret.push_back(Flag::NO_SCRIPT);
    ret.push_back(Flag::KILLED);
    ret.push_back(Flag::STATUS);
    ret.push_back(Flag::LATE);
    ret.push_back(Flag::MESSAGE);
    ret.push_back(Flag::BYRULE);
    ret.push_back(Flag::QUEUELIMIT);
    ret.push_back(Flag::WAIT);
    ret.push_back(Flag::LOCKED);
    ret.push_back(Flag::ZOMBIE);
    ret.push_back(Flag::NO_REQUE_IF_SINGLE_TIME_DEP);
    ret.push_back(Flag::ARCHIVED);
    ret.push_back(Flag::RESTORED);
    ret.push_back(Flag::THRESHOLD);
    ret.push_back(Flag::ECF_SIGTERM);
    ret.push_back(Flag::LOG_ERROR);
    ret.push_back(Flag::CHECKPT_ERROR);
    return ret;
}

constexpr std::array<Flag::Type, 24> Flag::array() {
    return std::array<Flag::Type, 24>{Flag::FORCE_ABORT,      Flag::USER_EDIT,     Flag::TASK_ABORTED,
                                      Flag::EDIT_FAILED,      Flag::JOBCMD_FAILED, Flag::KILLCMD_FAILED,
                                      Flag::STATUSCMD_FAILED, Flag::NO_SCRIPT,     Flag::KILLED,
                                      Flag::STATUS,           Flag::LATE,          Flag::MESSAGE,
                                      Flag::BYRULE,           Flag::QUEUELIMIT,    Flag::WAIT,
                                      Flag::LOCKED,           Flag::ZOMBIE,        Flag::NO_REQUE_IF_SINGLE_TIME_DEP,
                                      Flag::ARCHIVED,         Flag::RESTORED,      Flag::THRESHOLD,
                                      Flag::ECF_SIGTERM,      Flag::LOG_ERROR,     Flag::CHECKPT_ERROR};
}

std::string Flag::enum_to_string(Flag::Type flag) {

    switch (flag) {
        case Flag::FORCE_ABORT:
            return "force_aborted";
            break;
        case Flag::USER_EDIT:
            return "user_edit";
            break;
        case Flag::TASK_ABORTED:
            return "task_aborted";
            break;
        case Flag::EDIT_FAILED:
            return "edit_failed";
            break;
        case Flag::JOBCMD_FAILED:
            return "ecfcmd_failed";
            break;
        case Flag::NO_SCRIPT:
            return "no_script";
            break;
        case Flag::KILLED:
            return "killed";
            break;
        case Flag::LATE:
            return "late";
            break;
        case Flag::MESSAGE:
            return "message";
            break;
        case Flag::BYRULE:
            return "by_rule";
            break;
        case Flag::QUEUELIMIT:
            return "queue_limit";
            break;
        case Flag::WAIT:
            return "task_waiting";
            break;
        case Flag::LOCKED:
            return "locked";
            break;
        case Flag::ZOMBIE:
            return "zombie";
            break;
        case Flag::NO_REQUE_IF_SINGLE_TIME_DEP:
            return "no_reque";
            break;
        case Flag::ARCHIVED:
            return "archived";
            break;
        case Flag::RESTORED:
            return "restored";
            break;
        case Flag::THRESHOLD:
            return "threshold";
            break;
        case Flag::ECF_SIGTERM:
            return "sigterm";
            break;
        case Flag::LOG_ERROR:
            return "log_error";
            break;
        case Flag::CHECKPT_ERROR:
            return "checkpt_error";
            break;
        case Flag::KILLCMD_FAILED:
            return "killcmd_failed";
            break;
        case Flag::STATUSCMD_FAILED:
            return "statuscmd_failed";
            break;
        case Flag::STATUS:
            return "status";
            break;
        case Flag::NOT_SET:
            return "not_set";
            break;
        default:
            break;
    };
    return std::string();
}

const char* Flag::enum_to_char_star(Flag::Type flag) {
    switch (flag) {
        case Flag::FORCE_ABORT:
            return "force_aborted";
            break;
        case Flag::USER_EDIT:
            return "user_edit";
            break;
        case Flag::TASK_ABORTED:
            return "task_aborted";
            break;
        case Flag::EDIT_FAILED:
            return "edit_failed";
            break;
        case Flag::JOBCMD_FAILED:
            return "ecfcmd_failed";
            break;
        case Flag::KILLCMD_FAILED:
            return "killcmd_failed";
            break;
        case Flag::STATUSCMD_FAILED:
            return "statuscmd_failed";
            break;
        case Flag::STATUS:
            return "status";
            break;
        case Flag::NO_SCRIPT:
            return "no_script";
            break;
        case Flag::KILLED:
            return "killed";
            break;
        case Flag::LATE:
            return "late";
            break;
        case Flag::MESSAGE:
            return "message";
            break;
        case Flag::BYRULE:
            return "by_rule";
            break;
        case Flag::QUEUELIMIT:
            return "queue_limit";
            break;
        case Flag::WAIT:
            return "task_waiting";
            break;
        case Flag::LOCKED:
            return "locked";
            break;
        case Flag::ZOMBIE:
            return "zombie";
            break;
        case Flag::NO_REQUE_IF_SINGLE_TIME_DEP:
            return "no_reque";
            break;
        case Flag::ARCHIVED:
            return "archived";
            break;
        case Flag::RESTORED:
            return "restored";
            break;
        case Flag::THRESHOLD:
            return "threshold";
            break;
        case Flag::ECF_SIGTERM:
            return "sigterm";
            break;
        case Flag::LOG_ERROR:
            return "log_error";
            break;
        case Flag::CHECKPT_ERROR:
            return "checkpt_error";
            break;
        case Flag::NOT_SET:
            return "not_set";
            break;
        default:
            break;
    };
    assert(false);
    return nullptr;
}

Flag::Type Flag::string_to_flag_type(const std::string& s) {
    if (s == "force_aborted")
        return Flag::FORCE_ABORT;
    if (s == "user_edit")
        return Flag::USER_EDIT;
    if (s == "task_aborted")
        return Flag::TASK_ABORTED;
    if (s == "edit_failed")
        return Flag::EDIT_FAILED;
    if (s == "ecfcmd_failed")
        return Flag::JOBCMD_FAILED;
    if (s == "killcmd_failed")
        return Flag::KILLCMD_FAILED;
    if (s == "statuscmd_failed")
        return Flag::STATUSCMD_FAILED;
    if (s == "status")
        return Flag::STATUS;
    if (s == "no_script")
        return Flag::NO_SCRIPT;
    if (s == "killed")
        return Flag::KILLED;
    if (s == "late")
        return Flag::LATE;
    if (s == "message")
        return Flag::MESSAGE;
    if (s == "by_rule")
        return Flag::BYRULE;
    if (s == "queue_limit")
        return Flag::QUEUELIMIT;
    if (s == "task_waiting")
        return Flag::WAIT;
    if (s == "locked")
        return Flag::LOCKED;
    if (s == "zombie")
        return Flag::ZOMBIE;
    if (s == "no_reque")
        return Flag::NO_REQUE_IF_SINGLE_TIME_DEP;
    if (s == "archived")
        return Flag::ARCHIVED;
    if (s == "restored")
        return Flag::RESTORED;
    if (s == "threshold")
        return Flag::THRESHOLD;
    if (s == "sigterm")
        return Flag::ECF_SIGTERM;
    if (s == "log_error")
        return Flag::LOG_ERROR;
    if (s == "checkpt_error")
        return Flag::CHECKPT_ERROR;
    return Flag::NOT_SET;
}

void Flag::valid_flag_type(std::vector<std::string>& vec) {
    vec.reserve(24);
    vec.emplace_back("force_aborted");
    vec.emplace_back("user_edit");
    vec.emplace_back("task_aborted");
    vec.emplace_back("edit_failed");
    vec.emplace_back("ecfcmd_failed");
    vec.emplace_back("statuscmd_failed");
    vec.emplace_back("killcmd_failed");
    vec.emplace_back("no_script");
    vec.emplace_back("killed");
    vec.emplace_back("status");
    vec.emplace_back("late");
    vec.emplace_back("message");
    vec.emplace_back("by_rule");
    vec.emplace_back("queue_limit");
    vec.emplace_back("task_waiting");
    vec.emplace_back("locked");
    vec.emplace_back("zombie");
    vec.emplace_back("no_reque");
    vec.emplace_back("archived");
    vec.emplace_back("restored");
    vec.emplace_back("threshold");
    vec.emplace_back("sigterm");
    vec.emplace_back("log_error");
    vec.emplace_back("checkpt_error");
}

std::string Flag::to_string() const {
    std::string ret;
    write(ret);
    return ret;
}

void Flag::write(std::string& ret) const {
    bool added                           = false;
    std::array<Flag::Type, 24> flag_list = Flag::array();
    for (auto& i : flag_list) {
        if (is_set(i)) {
            if (added)
                ret += ',';
            ret += enum_to_char_star(i);
            added = true;
        }
    }
}

void Flag::set_flag(const std::string& flags) {
    std::vector<std::string> the_flags_vec;
    Str::split(flags, the_flags_vec, ",");

    for (const auto& i : the_flags_vec) {
        if (i == "migrated")
            continue; // 4.4.x release had migrated ignore. REMOVE when 5.0.0 is default

        Flag::Type ft = string_to_flag_type(i);
        if (ft == Flag::NOT_SET) {
            throw std::runtime_error("Flag::set_flag: Unknown flag types found: " + i);
        }
        set(ft);
    }
}

template <class Archive> void Flag::serialize(Archive& ar, std::uint32_t const version) {
    ar(CEREAL_NVP(flag_));
}
CEREAL_TEMPLATE_SPECIALIZE_V(Flag);

} // namespace ecf
