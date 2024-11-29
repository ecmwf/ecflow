/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/base/cts/user/AlterCmd.hpp"

#include <stdexcept>

#include "ecflow/attribute/GenericAttr.hpp"
#include "ecflow/attribute/LateAttr.hpp"
#include "ecflow/attribute/QueueAttr.hpp"
#include "ecflow/base/AbstractClientEnv.hpp"
#include "ecflow/base/AbstractServer.hpp"
#include "ecflow/base/cts/user/CtsApi.hpp"
#include "ecflow/core/Converter.hpp"
#include "ecflow/core/Enumerate.hpp"
#include "ecflow/core/Environment.hpp"
#include "ecflow/core/Extract.hpp"
#include "ecflow/core/Log.hpp"
#include "ecflow/core/Message.hpp"
#include "ecflow/core/Str.hpp"
#include "ecflow/core/exceptions/Exceptions.hpp"
#include "ecflow/node/AvisoAttr.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Expression.hpp"
#include "ecflow/node/Limit.hpp"
#include "ecflow/node/MirrorAttr.hpp"
#include "ecflow/node/Node.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/node/SuiteChanged.hpp"
#include "ecflow/node/parser/AvisoParser.hpp"
#include "ecflow/node/parser/MirrorParser.hpp"

using namespace ecf;
using namespace std;
using namespace boost;
namespace po = boost::program_options;

static std::string dump_args(const std::vector<std::string>& options, const std::vector<std::string>& paths) {
    std::string the_args;
    for (const auto& option : options) {
        the_args += option;
        the_args += " ";
    }
    for (const auto& path : paths) {
        the_args += path;
        the_args += " ";
    }
    return the_args;
}

namespace ecf::detail {

template <>
struct EnumTraits<AlterCmd::Delete_attr_type>
{
    using underlying_t = std::underlying_type_t<AlterCmd::Delete_attr_type>;

    static constexpr std::array map = std::array{
        // clang-format off
        std::make_pair(AlterCmd::DEL_VARIABLE, "variable"),
        std::make_pair(AlterCmd::DEL_TIME, "time"),
        std::make_pair(AlterCmd::DEL_TODAY, "today"),
        std::make_pair(AlterCmd::DEL_DATE, "date"),
        std::make_pair(AlterCmd::DEL_DAY, "day"),
        std::make_pair(AlterCmd::DEL_CRON, "cron"),
        std::make_pair(AlterCmd::DEL_EVENT, "event"),
        std::make_pair(AlterCmd::DEL_METER, "meter"),
        std::make_pair(AlterCmd::DEL_LABEL, "label"),
        std::make_pair(AlterCmd::DEL_TRIGGER, "trigger"),
        std::make_pair(AlterCmd::DEL_COMPLETE, "complete"),
        std::make_pair(AlterCmd::DEL_REPEAT, "repeat"),
        std::make_pair(AlterCmd::DEL_LIMIT, "limit"),
        std::make_pair(AlterCmd::DEL_LIMIT_PATH, "limit_path"),
        std::make_pair(AlterCmd::DEL_INLIMIT, "inlimit"),
        std::make_pair(AlterCmd::DEL_ZOMBIE, "zombie"),
        std::make_pair(AlterCmd::DEL_LATE, "late"),
        std::make_pair(AlterCmd::DEL_QUEUE, "queue"),
        std::make_pair(AlterCmd::DEL_GENERIC, "generic"),
        std::make_pair(AlterCmd::DEL_AVISO, "aviso"),
        std::make_pair(AlterCmd::DEL_MIRROR, "mirror")
        // clang-format on
    };
    static constexpr size_t size = map.size();

    static_assert(EnumTraits<AlterCmd::Delete_attr_type>::size == map.back().first);
};

template <>
struct EnumTraits<AlterCmd::Add_attr_type>
{
    using underlying_t = std::underlying_type_t<AlterCmd::Add_attr_type>;

    static constexpr std::array map = std::array{
        // clang-format off
        std::make_pair(AlterCmd::ADD_TIME, "time"),
        std::make_pair(AlterCmd::ADD_TODAY, "today"),
        std::make_pair(AlterCmd::ADD_DATE, "date"),
        std::make_pair(AlterCmd::ADD_DAY, "day"),
        std::make_pair(AlterCmd::ADD_ZOMBIE, "zombie"),
        std::make_pair(AlterCmd::ADD_VARIABLE, "variable"),
        std::make_pair(AlterCmd::ADD_LATE, "late"),
        std::make_pair(AlterCmd::ADD_LIMIT, "limit"),
        std::make_pair(AlterCmd::ADD_INLIMIT, "inlimit"),
        std::make_pair(AlterCmd::ADD_LABEL, "label"),
        std::make_pair(AlterCmd::ADD_AVISO, "aviso"),
        std::make_pair(AlterCmd::ADD_MIRROR, "mirror")
        // clang-format on
    };
    static constexpr size_t size = map.size();

    static_assert(EnumTraits<AlterCmd::Add_attr_type>::size == map.back().first);
};

template <>
struct EnumTraits<AlterCmd::Change_attr_type>
{
    using underlying_t = std::underlying_type_t<AlterCmd::Change_attr_type>;

    static constexpr std::array map = std::array{
        // clang-format off
        std::make_pair(AlterCmd::VARIABLE, "variable"),
        std::make_pair(AlterCmd::CLOCK_TYPE, "clock_type"),
        std::make_pair(AlterCmd::CLOCK_DATE, "clock_date"),
        std::make_pair(AlterCmd::CLOCK_GAIN, "clock_gain"),
        std::make_pair(AlterCmd::CLOCK_SYNC, "clock_sync"),
        std::make_pair(AlterCmd::EVENT, "event"),
        std::make_pair(AlterCmd::METER, "meter"),
        std::make_pair(AlterCmd::LABEL, "label"),
        std::make_pair(AlterCmd::TRIGGER, "trigger"),
        std::make_pair(AlterCmd::COMPLETE, "complete"),
        std::make_pair(AlterCmd::REPEAT, "repeat"),
        std::make_pair(AlterCmd::LIMIT_MAX, "limit_max"),
        std::make_pair(AlterCmd::LIMIT_VAL, "limit_value"),
        std::make_pair(AlterCmd::DEFSTATUS, "defstatus"),
        std::make_pair(AlterCmd::LATE, "late"),
        std::make_pair(AlterCmd::TIME, "time"),
        std::make_pair(AlterCmd::TODAY, "today"),
        std::make_pair(AlterCmd::AVISO, "aviso"),
        std::make_pair(AlterCmd::MIRROR, "mirror")
        // clang-format on
    };
    static constexpr size_t size = map.size();

    static_assert(EnumTraits<AlterCmd::Change_attr_type>::size == map.back().first);
};

} // namespace ecf::detail

static AlterCmd::Delete_attr_type deleteAttrType(const std::string& s) {
    if (auto found = ecf::Enumerate<AlterCmd::Delete_attr_type>::to_enum(s); found) {
        return found.value();
    }
    return AlterCmd::DELETE_ATTR_ND;
}

static std::string to_string(AlterCmd::Delete_attr_type d) {
    if (auto found = ecf::Enumerate<AlterCmd::Delete_attr_type>::to_string(d); found) {
        return std::string{found.value()};
    }
    return {};
}

static void validDeleteAttr(std::vector<std::string>& vec) {
    vec = ecf::Enumerate<AlterCmd::Delete_attr_type>::designations();
}

static AlterCmd::Add_attr_type addAttrType(const std::string& s) {
    if (auto found = ecf::Enumerate<AlterCmd::Add_attr_type>::to_enum(s); found) {
        return found.value();
    }
    return AlterCmd::ADD_ATTR_ND;
}

static std::string to_string(AlterCmd::Add_attr_type a) {
    if (auto found = ecf::Enumerate<AlterCmd::Add_attr_type>::to_string(a); found) {
        return std::string{found.value()};
    }
    return {};
}

static void validAddAttr(std::vector<std::string>& vec) {
    vec = Enumerate<AlterCmd::Add_attr_type>::designations();
}

static AlterCmd::Change_attr_type changeAttrType(const std::string& s) {
    if (auto found = Enumerate<AlterCmd::Change_attr_type>::to_enum(s); found) {
        return found.value();
    }
    return AlterCmd::CHANGE_ATTR_ND;
}

static std::string to_string(AlterCmd::Change_attr_type c) {
    if (auto found = Enumerate<AlterCmd::Change_attr_type>::to_string(c); found) {
        return std::string{found.value()};
    }
    return {};
}

static void validChangeAttr(std::vector<std::string>& vec) {
    vec = ecf::Enumerate<AlterCmd::Change_attr_type>::designations();
}

//=======================================================================================

bool AlterCmd::equals(ClientToServerCmd* rhs) const {
    auto* the_rhs = dynamic_cast<AlterCmd*>(rhs);
    if (!the_rhs)
        return false;
    if (paths_ != the_rhs->paths()) {
        return false;
    }
    if (name_ != the_rhs->name()) {
        return false;
    }
    if (value_ != the_rhs->value()) {
        return false;
    }
    if (del_attr_type_ != the_rhs->delete_attr_type()) {
        return false;
    }
    if (change_attr_type_ != the_rhs->change_attr_type()) {
        return false;
    }
    if (add_attr_type_ != the_rhs->add_attr_type()) {
        return false;
    }
    if (flag_type_ != the_rhs->flag_type()) {
        return false;
    }
    if (flag_ != the_rhs->flag()) {
        return false;
    }
    return UserCmd::equals(rhs);
}

void AlterCmd::alter_and_attr_type(std::string& alter_type, std::string& attr_type) const {
    if (del_attr_type_ != AlterCmd::DELETE_ATTR_ND) {
        alter_type = "delete";
        attr_type  = to_string(del_attr_type_);
    }
    else if (change_attr_type_ != AlterCmd::CHANGE_ATTR_ND) {
        alter_type = "change";
        attr_type  = to_string(change_attr_type_);
    }
    else if (add_attr_type_ != AlterCmd::ADD_ATTR_ND) {
        alter_type = "add";
        attr_type  = to_string(add_attr_type_);
    }
    else if (flag_type_ != ecf::Flag::NOT_SET) {
        if (flag_)
            alter_type = "set_flag";
        else
            alter_type = "clear_flag";
        attr_type = ecf::Flag::enum_to_string(flag_type_);
    }
    else {
        alter_type = "sort";
    }
}

void AlterCmd::print_only(std::string& os) const {
    std::string alter_type, attr_type;
    alter_and_attr_type(alter_type, attr_type);
    if (paths_.empty())
        os += CtsApi::to_string(CtsApi::alter(std::vector<std::string>(1, " "), alter_type, attr_type, name_, value_));
    else
        os += CtsApi::to_string(
            CtsApi::alter(std::vector<std::string>(1, paths_[0]), alter_type, attr_type, name_, value_));
}

void AlterCmd::print(std::string& os) const {
    std::string alter_type, attr_type;
    alter_and_attr_type(alter_type, attr_type);
    user_cmd(os, CtsApi::to_string(CtsApi::alter(paths_, alter_type, attr_type, name_, value_)));
}

void AlterCmd::print(std::string& os, const std::string& path) const {
    std::string alter_type, attr_type;
    alter_and_attr_type(alter_type, attr_type);
    user_cmd(os,
             CtsApi::to_string(CtsApi::alter(std::vector<std::string>(1, path), alter_type, attr_type, name_, value_)));
}

STC_Cmd_ptr AlterCmd::alter_server_state(AbstractServer* as) const {
    Defs* defs = as->defs().get();

    if (del_attr_type_ == AlterCmd::DEL_VARIABLE) {
        defs->set_server().delete_user_variable(name_);
    }
    else if (change_attr_type_ == AlterCmd::VARIABLE || add_attr_type_ == AlterCmd::ADD_VARIABLE) {

        // ECFLOW-380: Some variable should be read only
        if (name_ == ecf::environment::ECF_HOST || name_ == ecf::environment::ECF_PORT || name_ == "ECF_PID" ||
            name_ == "ECF_VERSION" || name_ == "ECF_LISTS") {
            std::stringstream ss;
            ss << "AlterCmd:: Cannot add or change read only server variable " << name_;
            throw std::runtime_error(ss.str());
        }
        defs->set_server().add_or_update_user_variables(name_, value_);
    }

    // Update defs flag state
    if (flag_type_ != Flag::NOT_SET) {
        if (flag_)
            defs->flag().set(flag_type_);
        else {
            defs->flag().clear(flag_type_);
            if (flag_type_ == Flag::LOG_ERROR)
                defs->set_server().delete_user_variable("ECF_LOG_ERROR");
            if (flag_type_ == Flag::CHECKPT_ERROR)
                defs->set_server().delete_user_variable("ECF_CHECKPT_ERROR");
        }
    }

    // sort
    ecf::Attr::Type attr = Attr::to_attr(name_);
    if (attr != ecf::Attr::UNKNOWN) {
        bool recursive = (value_ == "recursive") ? true : false;
        defs->sort_attributes(attr, recursive);
    }

    return doJobSubmission(as);
}

STC_Cmd_ptr AlterCmd::doHandleRequest(AbstractServer* as) const {
    as->update_stats().alter_cmd_++;
    Defs* defs = as->defs().get();

    std::stringstream ss;
    size_t vec_size = paths_.size();
    for (size_t i = 0; i < vec_size; i++) {

        /// For root node user means to change server state
        if (paths_[i] == "/") {
            return alter_server_state(as);
        }

        node_ptr node = find_node_for_edit_no_throw(defs, paths_[i]);
        if (!node.get()) {
            ss << "AlterCmd: Could not find node at path " << paths_[i] << "\n";
            LOG(Log::ERR, "AlterCmd: Failed: Could not find node at path " << paths_[i]);
            continue;
        }

        SuiteChangedPtr changed(node.get());
        try {
            switch (del_attr_type_) {
                case AlterCmd::DEL_VARIABLE:
                    node->deleteVariable(name_);
                    break;
                case AlterCmd::DEL_TIME:
                    node->deleteTime(name_);
                    break;
                case AlterCmd::DEL_TODAY:
                    node->deleteToday(name_);
                    break;
                case AlterCmd::DEL_DATE:
                    node->deleteDate(name_);
                    break;
                case AlterCmd::DEL_DAY:
                    node->deleteDay(name_);
                    break;
                case AlterCmd::DEL_CRON:
                    node->deleteCron(name_);
                    break;
                case AlterCmd::DEL_EVENT:
                    node->deleteEvent(name_);
                    break;
                case AlterCmd::DEL_METER:
                    node->deleteMeter(name_);
                    break;
                case AlterCmd::DEL_LABEL:
                    node->deleteLabel(name_);
                    break;
                case AlterCmd::DEL_TRIGGER:
                    node->deleteTrigger();
                    break;
                case AlterCmd::DEL_COMPLETE:
                    node->deleteComplete();
                    break;
                case AlterCmd::DEL_REPEAT:
                    node->deleteRepeat();
                    break;
                case AlterCmd::DEL_LIMIT:
                    node->deleteLimit(name_);
                    break;
                case AlterCmd::DEL_LIMIT_PATH:
                    node->delete_limit_path(name_, value_);
                    break;
                case AlterCmd::DEL_INLIMIT:
                    node->deleteInlimit(name_);
                    break;
                case AlterCmd::DEL_ZOMBIE:
                    node->deleteZombie(name_);
                    break;
                case AlterCmd::DEL_LATE:
                    node->deleteLate();
                    break;
                case AlterCmd::DEL_QUEUE:
                    node->delete_queue(name_);
                    break;
                case AlterCmd::DEL_GENERIC:
                    node->delete_generic(name_);
                    break;
                case AlterCmd::DELETE_ATTR_ND:
                    break;
                case AlterCmd::DEL_AVISO:
                    node->deleteAviso(name_);
                    break;
                case AlterCmd::DEL_MIRROR:
                    node->deleteMirror(name_);
                    break;
                default:
                    break;
            }
        }
        catch (std::exception& e) {
            ss << "Alter (delete) failed for " << paths_[i] << " : " << e.what() << "\n";
        }

        try {
            switch (change_attr_type_) {
                case AlterCmd::VARIABLE:
                    node->changeVariable(name_, value_);
                    break;
                case AlterCmd::CLOCK_TYPE:
                    node->suite()->changeClockType(name_);
                    break; // node must be a suite, value must [hybrid|real| virtual]
                case AlterCmd::CLOCK_DATE:
                    node->suite()->changeClockDate(name_);
                    break; // Expecting day.month.year: node must be a suite, value must [hybrid|real| virtual]
                case AlterCmd::CLOCK_GAIN:
                    node->suite()->changeClockGain(name_);
                    break; // node must be a suite, value must be int
                case AlterCmd::CLOCK_SYNC:
                    node->suite()->changeClockSync();
                    break; // node must be a suite, sync clock with computer
                case AlterCmd::EVENT:
                    node->changeEvent(name_, value_);
                    break; // if value is empty just set, [1|0] or name [set | clear]
                case AlterCmd::METER:
                    node->changeMeter(name_, value_);
                    break;
                case AlterCmd::LABEL:
                    node->changeLabel(name_, value_);
                    break;
                case AlterCmd::AVISO:
                    node->changeAviso(name_, value_);
                    break;
                case AlterCmd::TRIGGER:
                    node->changeTrigger(name_);
                    break; // expression must parse
                case AlterCmd::COMPLETE:
                    node->changeComplete(name_);
                    break; // expression must parse
                case AlterCmd::REPEAT:
                    node->changeRepeat(name_);
                    break; //
                case AlterCmd::LIMIT_MAX:
                    node->changeLimitMax(name_, value_);
                    break; // value must be convertible to int
                case AlterCmd::LIMIT_VAL:
                    node->changeLimitValue(name_, value_);
                    break; // value < limit max, & value must be convertible to an int
                case AlterCmd::DEFSTATUS:
                    node->changeDefstatus(name_);
                    break; // must be a valid state
                case AlterCmd::LATE:
                    node->changeLate(LateAttr::create(name_));
                    break; // must be a valid late
                case AlterCmd::TIME:
                    node->change_time(name_, value_);
                    break;
                case AlterCmd::TODAY:
                    node->change_today(name_, value_);
                    break;
                case AlterCmd::CHANGE_ATTR_ND:
                    break;
                case AlterCmd::MIRROR:
                    node->changeMirror(name_, value_);
                    break;
                default:
                    break;
            }
        }
        catch (std::exception& e) {
            ss << "Alter (change) failed for " << paths_[i] << " : " << e.what() << "\n";
        }

        try {
            switch (add_attr_type_) {
                case AlterCmd::ADD_TIME:
                    node->addTime(TimeAttr(TimeSeries::create(name_)));
                    break;
                case AlterCmd::ADD_TODAY:
                    node->addToday(TodayAttr(TimeSeries::create(name_)));
                    break;
                case AlterCmd::ADD_DATE:
                    node->addDate(DateAttr::create(name_));
                    break;
                case AlterCmd::ADD_DAY:
                    node->addDay(DayAttr::create(name_));
                    break;
                case AlterCmd::ADD_AVISO: {
                    auto aviso = AvisoParser::parse_aviso_line(value_, name_, node.get());
                    node->addAviso(aviso);
                    break;
                }
                case AlterCmd::ADD_MIRROR: {
                    auto mirror = MirrorParser::parse_mirror_line(value_, name_, node.get());
                    node->addMirror(mirror);
                    break;
                }
                case AlterCmd::ADD_ZOMBIE:
                    node->addZombie(ZombieAttr::create(name_));
                    break;
                case AlterCmd::ADD_VARIABLE:
                    node->add_variable(name_, value_);
                    break;
                case AlterCmd::ADD_LATE:
                    node->addLate(LateAttr::create(name_));
                    break;
                case AlterCmd::ADD_LABEL:
                    node->addLabel(Label(name_, value_));
                    break;
                case AlterCmd::ADD_LIMIT: {
                    int int_value = 0;
                    try {
                        int_value = ecf::convert_to<int>(value_);
                    }
                    catch (const ecf::bad_conversion&) {
                        std::stringstream mss;
                        mss << "AlterCmd: add_limit " << name_ << " " << value_ << " failed. Expected '" << value_
                            << "' to be convertible to an integer";
                        throw std::runtime_error(mss.str());
                    }
                    node->addLimit(Limit(name_, int_value));
                    break;
                }
                case AlterCmd::ADD_INLIMIT: {
                    string path_to_limit; // This can be empty
                    string limitName;
                    if (!Extract::pathAndName(name_, path_to_limit, limitName)) {
                        throw std::runtime_error("AlterCmd::ADD_INLIMIT: Invalid inlimit : " + name_);
                    }
                    int token_value = 1;
                    if (!value_.empty()) {
                        try {
                            token_value = ecf::convert_to<int>(value_);
                        }
                        catch (const ecf::bad_conversion&) {
                            ss << "AlterCmd: add_inlimit expected '" << value_ << "' to be convertible to an integer";
                            throw std::runtime_error(ss.str());
                        }
                    }
                    node->addInLimit(InLimit(limitName, path_to_limit, token_value)); // will throw if not valid
                    break;
                }
                case AlterCmd::ADD_ATTR_ND:
                    break;
            }
        }
        catch (std::exception& e) {
            ss << "Alter (add) failed for " << paths_[i] << ": Could not parse " << name_ << " : " << e.what() << "\n";
        }

        // Change flags
        if (flag_type_ != Flag::NOT_SET) {
            if (flag_)
                node->flag().set(flag_type_);
            else
                node->flag().clear(flag_type_);
        }

        // sort
        ecf::Attr::Type attr = Attr::to_attr(name_);
        if (attr != ecf::Attr::UNKNOWN) {
            bool recursive = (value_ == "recursive") ? true : false;
            node->sort_attributes(attr, recursive);
        }
    }

    std::string error_msg = ss.str();
    if (!error_msg.empty()) {
        throw std::runtime_error(error_msg);
    }

    return doJobSubmission(as);
}

bool AlterCmd::authenticate(AbstractServer* as, STC_Cmd_ptr& cmd) const {
    return do_authenticate(as, cmd, paths_);
}

const char* AlterCmd::arg() {
    return CtsApi::alterArg();
}
const char* AlterCmd::desc() {
    /////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
    return "Alter the node according to the options.\n"
           "To add/delete/change server variables use '/' for the path.\n"
           "arg1 =   [ delete | change | add | set_flag | clear_flag | sort ]\n"
           "         one option must be specified\n"
           "arg2 = For delete:\n"
           "         [ variable | time | today | date  | day | cron | event | meter | late | generic | queue |\n"
           "           label | trigger | complete | repeat | limit | inlimit | limit_path | zombie | aviso | mirror ]\n"
           "       For change:\n"
           "         [ variable | clock_type | clock_gain | clock_date | clock_sync  | event | meter | label |\n"
           "           trigger  | complete   | repeat     | limit_max  | limit_value | defstatus | late | time |\n"
           "           today | aviso | mirror ]\n"
           "         *NOTE* If the clock is changed, then the suite will need to be re-queued in order for\n"
           "         the change to take effect fully.\n"
           "       For add:\n"
           "         [ variable | time | today | date | day | zombie | late | limit | inlimit | label | aviso | mirror ]\n"
           "       For set_flag and clear_flag:\n"
           "         [ force_aborted | user_edit | task_aborted | edit_failed | ecfcmd_failed \n"
           "           statuscmd_failed | killcmd_failed | no_script | killed | status | late | message | \n"
           "           complete | queue_limit | task_waiting | locked | zombie | archived | restored |\n"
           "           threshold | log_error | checkpt_error]\n"
           "       For sort:\n"
           "         [ event | meter | label | variable| limit | all ]\n"
           "arg3 = name/value\n"
           "arg4 = new_value\n"
           "arg5 = paths : At least one node path required.The paths must start with a leading '/' character\n"
           "\n"
           "When adding or updating attributes, such as variable, meter, event, label, limits, or late,\n"
           "  the name (arg3) and value (arg4) must be quoted.\n"
           "\n"
           "When sorting attributes, 'recursive' can be used as the value (arg3)\n"
           "\n"
           "When adding or updating aviso and mirror attributes, the value (arg4) is expected to be a quoted list of\n"
           "  configuration options. For example:\n"
           "   * for aviso, \"--remote_path /s1/f1/t2 --remote_host host --polling 20 --remote_port 3141 --ssl)\"\n"
           "   * for mirror, \"--listener '{ \\\"event\\\": \\\"mars\\\", \\\"request\\\": { \\\"class\\\": \"od\" } }'\n"
           "                  --url http://aviso/ --schema /path/to/schema --polling 60\"\n"
           "\n"
           "For both aviso and mirror, the special value \"reload\" can be used to force reloading the configuration.\n"
           "  n.b. This is typically useful after updating variables used to configure these kind of attributes.\n"
           "\n"
           "Usage:\n\n"
           "   ecflow_client --alter=add variable GLOBAL \"value\" /           # add server variable\n"
           "   ecflow_client --alter=add variable FRED \"value\" /path/to/node # add node variable\n"
           "   ecflow_client --alter=add time \"+00:20\" /path/to/node\n"
           "   ecflow_client --alter=add date \"01.*.*\" /path/to/node\n"
           "   ecflow_client --alter=add day \"sunday\"  /path/to/node\n"
           "   ecflow_client --alter=add label name \"label_value\" /path/to/node\n"
           "   ecflow_client --alter=add late \"-s 00:01 -a 14:30 -c +00:01\" /path/to/node\n"
           "   ecflow_client --alter=add limit mars \"100\" /path/to/node\n"
           "   ecflow_client --alter=add inlimit /path/to/node/withlimit:limit_name \"10\" /path/to/node\n"
           "   # zombie attributes have the following structure:\n"
           "     `zombie_type`:(`client_side_action` | `server_side_action`):`child`:`zombie_life_time`\n"
           "      zombie_type        = \"user\" | \"ecf\" | \"path\" | \"ecf_pid\" | \"ecf_passwd\" | "
           "\"ecf_pid_passwd\"\n"
           "      client_side_action = \"fob\" | \"fail\" | \"block\"\n"
           "      server_side_action = \"adopt\" | \"delete\" | \"kill\"\n"
           "      child              = \"init\" | \"event\" | \"meter\" | \"label\" | \"wait\" | \"abort\" | "
           "\"complete\" | \"queue\"\n"
           "      zombie_life_time   = unsigned integer default: user(300), ecf(3600), path(900)  minimum is 60\n"
           "   ecflow_client --alter=add zombie \"ecf:fail::\" /path/to/node     # ask system zombies to fail\n"
           "   ecflow_client --alter=add zombie \"user:fail::\" /path/to/node    # ask user generated zombies to fail\n"
           "   ecflow_client --alter=add zombie \"path:fail::\" /path/to/node    # ask path zombies to fail\n\n"
           "   ecflow_client --alter=delete variable FRED /path/to/node    # delete variable FRED\n"
           "   ecflow_client --alter=delete variable      /path/to/node    # delete *ALL* variables on the specified "
           "node\n";
}

void AlterCmd::addOption(boost::program_options::options_description& desc) const {
    // Important: this option is, in practice, multi-token (and thus should use
    // po::value<vector<string>>()->multitoken()). However, because of the special handling
    // necessary to allow positional values, such as "--help", a custom `style_parser` is used
    // instead when parsing the CLI options -- see ClientOptions for details.
    desc.add_options()(AlterCmd::arg(), po::value<vector<string>>(), AlterCmd::desc());
}
void AlterCmd::create(Cmd_ptr& cmd, boost::program_options::variables_map& vm, AbstractClientEnv* ac) const {
    vector<string> args = vm[arg()].as<vector<string>>();

    if (ac->debug())
        dumpVecArgs(AlterCmd::arg(), args);

    std::vector<std::string> options, paths;
    split_args_to_options_and_paths(args, options, paths); // relative order is still preserved
    if (paths.empty()) {
        std::stringstream ss;
        ss << "AlterCmd: No paths specified. Paths must begin with a leading '/' character\n"
           << dump_args(options, paths) << "\n";
        throw std::runtime_error(ss.str());
    }
    if (options.empty()) {
        std::stringstream ss;
        ss << "AlterCmd: Invalid argument list:\n" << dump_args(options, paths) << "\n";
        throw std::runtime_error(ss.str());
    }
    if (options.size() < 2) {
        std::stringstream ss;
        ss << "Alter: At least three arguments expected. Found " << args.size() << "\n"
           << dump_args(options, paths) << "\n";
        throw std::runtime_error(ss.str());
    }

    // arg[0] should one of [ add | delete | change | set_flag | clear_flag | sort ]
    std::string alterType = options[0];

    if (alterType == "add") {

        createAdd(cmd, options, paths);
        return;
    }
    else if (alterType == "change") {

        createChange(cmd, options, paths);
        return;
    }
    else if (alterType == "delete") {

        createDelete(cmd, options, paths);
        return;
    }
    else if (alterType == "set_flag") {

        create_flag(cmd, options, paths, true /*set */);
        return;
    }
    else if (alterType == "clear_flag") {

        create_flag(cmd, options, paths, false /*clear */);
        return;
    }
    else if (alterType == "sort") {

        create_sort_attributes(cmd, options, paths);
        return;
    }

    std::stringstream ss;
    ss << "Alter: The first argument must be one of [ change | delete | add | set_flag | clear_flag | sort ] but found "
          "'"
       << alterType << "'\n"
       << dump_args(options, paths) << "\n";
    throw std::runtime_error(ss.str());
}

AlterCmd::Add_attr_type AlterCmd::get_add_attr_type(const std::string& attr_type) const {
    AlterCmd::Add_attr_type theAttrType = addAttrType(attr_type);
    if (theAttrType == AlterCmd::ADD_ATTR_ND) {
        std::stringstream ss;
        ss << "AlterCmd: add: The second argument must be one of [ ";
        std::vector<std::string> valid;
        validAddAttr(valid);
        for (size_t i = 0; i < valid.size(); ++i) {
            if (i != 0)
                ss << " | ";
            ss << valid[i];
        }
        ss << "] but found " << attr_type << "\n" << AlterCmd::desc();
        throw std::runtime_error(ss.str());
    }
    return theAttrType;
}

void AlterCmd::createAdd(Cmd_ptr& cmd, std::vector<std::string>& options, std::vector<std::string>& paths) const {
    // options[0] - add
    // options[1] - [ time | today | date | day | zombie | variable | late | limit | inlimit | label ]
    // options[2] - [ time_string | date_string | day_string | zombie_string | variable_name | limit_name |
    //                path_to_limit ]
    // options[3] - variable_value

    AlterCmd::Add_attr_type theAttrType = get_add_attr_type(options[1]);

    std::stringstream ss;
    if (options.size() < 3) {
        ss << "AlterCmd: add: At least four arguments expected. Found " << (options.size() + paths.size()) << "\n"
           << dump_args(options, paths) << "\n";
        throw std::runtime_error(ss.str());
    }

    string name, value;
    try {
        extract_name_and_value_for_add(theAttrType, name, value, options, paths);
        check_for_add(theAttrType, name, value);
    }
    catch (std::exception& e) {
        ss << "AlterCmd: add: Could not parse " << name << ". Error: " << e.what()
           << "\n for time,today and date the new value should be a quoted string "
           << "\n for add expected: --alter add variable <name> <value> <paths>\n"
           << dump_args(options, paths) << "\n";
        throw std::runtime_error(ss.str());
    }

    cmd = std::make_shared<AlterCmd>(paths, theAttrType, name, value);
}

void AlterCmd::extract_name_and_value_for_add(AlterCmd::Add_attr_type theAttrType,
                                              std::string& name,
                                              std::string& value,
                                              std::vector<std::string>& options,
                                              std::vector<std::string>& paths) const {
    // **** parse and check format, expect this argument to be single or double tick quoted ****
    // **** for time,date,day or zombie
    std::stringstream ss;
    name = options[2];
    switch (theAttrType) {
        case AlterCmd::ADD_TIME:
            break;
        case AlterCmd::ADD_TODAY:
            break;
        case AlterCmd::ADD_DATE:
            break;
        case AlterCmd::ADD_DAY:
            break;
        case AlterCmd::ADD_ZOMBIE:
            break;
        case AlterCmd::ADD_LATE:
            break;
        case AlterCmd::ADD_VARIABLE: {
            if (options.size() == 3 && paths.size() > 1) {
                // variable value may be a path, hence it will be in the paths parameter
                options.push_back(paths[0]);
                paths.erase(paths.begin());
            }
            if (options.size() < 4) {
                ss << "AlterCmd: add: Expected 'add variable <name> <value> <paths>. Not enough arguments\n"
                   << dump_args(options, paths) << "\n";
                throw std::runtime_error(ss.str());
            }
            value = options[3];
            break;
        }
        case AlterCmd::ADD_LABEL: {
            if (options.size() == 3 && paths.size() > 1) {
                // label value may be a path, hence it will be in the paths parameter
                options.push_back(paths[0]);
                paths.erase(paths.begin());
            }
            if (options.size() < 4) {
                ss << "AlterCmd: add: Expected 'add label <name> <value> <paths>. Not enough arguments\n"
                   << dump_args(options, paths) << "\n";
                throw std::runtime_error(ss.str());
            }
            value = options[3];
            break;
        }
        case AlterCmd::ADD_AVISO: {
            if (options.size() != 4 || paths.size() < 1) {
                ss << "AlterCmd: add: Expected 'add aviso <name> <cfg> <path> [<path> [...]]. Not enough arguments\n"
                   << dump_args(options, paths) << "\n";
                throw std::runtime_error(ss.str());
            }
            value = options[3];
            break;
        }
        case AlterCmd::ADD_MIRROR: {
            if (options.size() != 4 || paths.size() < 1) {
                ss << "AlterCmd: add: Expected 'add mirror <name> <cfg> <path> [<path> [...]]. Not enough arguments\n"
                   << dump_args(options, paths) << "\n";
                throw std::runtime_error(ss.str());
            }
            value = options[3];
            break;
        }

        case AlterCmd::ADD_LIMIT: {
            if (options.size() < 4) {
                ss << "AlterCmd: add: Expected 'add limit <name> int. Not enough arguments\n"
                   << dump_args(options, paths) << "\n";
                throw std::runtime_error(ss.str());
            }
            value = options[3];
            break;
        }
        case AlterCmd::ADD_INLIMIT: { // inlimit /obs/limits:hpcd 2 name=hpcd, path=/obs/limits, tokens=2(optional)
            // options[0]  - add
            // options[1]  - [ inlimit ]
            // options[2]  - [ path_to_limit:limit_name ]   --> name
            // options[3]  - integer (optional)             --> value
            if (options.size() < 3) {
                ss << "AlterCmd: add: Expected 'add inlimit <path-to-limit:limit_name> <int>(optional) <paths>. Not "
                      "enough arguments\n"
                   << dump_args(options, paths) << "\n";
                throw std::runtime_error(ss.str());
            }
            if (options.size() == 4) {
                value = options[3];
            }
            break;
        }
        case AlterCmd::ADD_ATTR_ND:
            break;
    }
}

void AlterCmd::check_for_add(AlterCmd::Add_attr_type theAttrType,
                             const std::string& name,
                             const std::string& value) const {
    // **** parse and check format, expect this argument to be single or double tick quoted ****
    // **** for time,date,day or zombie
    if (name.empty())
        throw std::runtime_error("Alter: check_for_add : name is empty ?");

    std::stringstream ss;
    switch (theAttrType) {
        case AlterCmd::ADD_TIME:
            (void)TimeSeries::create(name);
            break;
        case AlterCmd::ADD_TODAY:
            (void)TimeSeries::create(name);
            break;
        case AlterCmd::ADD_DATE:
            (void)DateAttr::create(name);
            break;
        case AlterCmd::ADD_DAY:
            (void)DayAttr::create(name);
            break;
        case AlterCmd::ADD_AVISO: {
            AvisoParser::parse_aviso_line(value, name);
            break;
        }
        case AlterCmd::ADD_MIRROR: {
            MirrorParser::parse_mirror_line(value, name);
            break;
        }
        case AlterCmd::ADD_ZOMBIE:
            (void)ZombieAttr::create(name);
            break;
        case AlterCmd::ADD_LATE:
            (void)LateAttr::create(name);
            break;
        case AlterCmd::ADD_VARIABLE: {
            // Create a Variable to check valid names
            Variable check(name, value);
            break;
        }
        case AlterCmd::ADD_LABEL: {
            // Create a Label to check valid names
            Label check(name, value);
            break;
        }
        case AlterCmd::ADD_LIMIT: {
            int int_value = 0;
            try {
                int_value = ecf::convert_to<int>(value);
            }
            catch (const ecf::bad_conversion&) {
                ss << "AlterCmd add_limit expected value(" << value << ") to be convertible to an integer\n";
                throw std::runtime_error(ss.str());
            }
            Limit check(name, int_value); // will throw if not valid
            break;
        }
        case AlterCmd::ADD_INLIMIT: { // inlimit /obs/limits:hpcd 2 name=hpcd, path=/obs/limits, tokens=2(optional)
            // options[0]  - add
            // options[1]  - [ inlimit ]
            // options[2]  - [ path_to_limit:limit_name ]  ---> name
            // options[3]  - integer (optional)            ---> value
            string path_to_limit; // This can be empty
            string limitName;
            if (!Extract::pathAndName(name, path_to_limit, limitName)) {
                throw std::runtime_error("AlterCmd add inlimit Invalid inlimit : " + name);
            }
            int token_value = 1;
            if (!value.empty()) {
                try {
                    token_value = ecf::convert_to<int>(value);
                }
                catch (const ecf::bad_conversion&) {
                    ss << "AlterCmd add inlimit expected optional limit token '" << value
                       << "' to be convertible to an integer\n";
                    throw std::runtime_error(ss.str());
                }
            }
            InLimit inlimit(limitName, path_to_limit, token_value); // will throw if not valid
            break;
        }
        case AlterCmd::ADD_ATTR_ND:
            break;
    }
}

AlterCmd::Delete_attr_type AlterCmd::get_delete_attr_type(const std::string& attr_type) const {
    AlterCmd::Delete_attr_type theAttrType = deleteAttrType(attr_type);
    if (theAttrType == AlterCmd::DELETE_ATTR_ND) {
        std::stringstream ss;
        ss << "Alter: delete: The second argument must be one of [ ";
        std::vector<std::string> valid;
        validDeleteAttr(valid);
        for (size_t i = 0; i < valid.size(); ++i) {
            if (i != 0)
                ss << " | ";
            ss << valid[i];
        }
        ss << "] but found " << attr_type << "\n" << AlterCmd::desc();
        throw std::runtime_error(ss.str());
    }
    return theAttrType;
}

void AlterCmd::createDelete(Cmd_ptr& cmd,
                            const std::vector<std::string>& options,
                            const std::vector<std::string>& paths) const {
    // options[0] = delete
    // options[1] = [ variable | time | today | date | day | cron | event | meter | label | trigger | complete |
    //                repeat | limit | limit_path | inlimit | zombie | late ]
    // options[2] = name ( of object to be deleted ) optional
    // options[3] = limit_path (optional *ONLY* applicable for limit_path), specifies the path to be deleted

    AlterCmd::Delete_attr_type theAttrType = get_delete_attr_type(options[1]);

    // Generally an empty third argument means delete all attributes, otherwise delete the specific one.
    std::string name, value;
    try {
        extract_name_and_value_for_delete(theAttrType, name, value, options, paths);
        check_for_delete(theAttrType, name, value);
    }
    catch (std::exception& e) {
        std::stringstream ss;
        ss << "AlterCmd: delete: Could not parse " << name << ". Error: " << e.what()
           << "\n for time,today and date the new value should be a quoted string\n"
           << dump_args(options, paths) << "\n";
        throw std::runtime_error(ss.str());
    }
    cmd = std::make_shared<AlterCmd>(paths, theAttrType, name, value);
}

void AlterCmd::extract_name_and_value_for_delete(AlterCmd::Delete_attr_type theAttrType,
                                                 std::string& name,
                                                 std::string& value,
                                                 const std::vector<std::string>& options,
                                                 const std::vector<std::string>& paths) const {
    // Generally an empty third argument means delete all attributes, otherwise delete the specific one.
    if (options.size() >= 3)
        name = options[2];

    // Deleting the limit path requires an additional arg
    std::string path_value;

    // if specified make sure its parses
    if (theAttrType == AlterCmd::DEL_LIMIT_PATH) {
        if (name.empty()) {
            std::stringstream ss;
            ss << "Delete limit_path failed. No limit name provided. Expected 5 args: delete limit_path <limit_name> "
                  "<path-to-limit> <path_to_node>\n";
            ss << dump_args(options, paths) << "\n";
            throw std::runtime_error(ss.str());
        }

        std::vector<std::string> altered_path = paths;
        if (options.size() == 4) {
            // User has provided a limit path which does not start with '/'. Go with flow
            path_value = options[3];
        }
        else {
            // Since we have a limit path(i.e begins with'/') it will appear in the paths, as the first path
            if (paths.size() <= 1) {
                std::stringstream ss;
                ss << "Delete limit_path failed: No path to limit provided. Expected 5 args: delete limit_path "
                      "<limit_name> <path-to-limit> <path_to_node>\n"
                   << dump_args(options, paths) << "\n";
                throw std::runtime_error(ss.str());
            }
            path_value = paths[0];

            // Change paths to remove the limit path.
            altered_path.erase(altered_path.begin());
        }
        value = path_value;
    }
}

void AlterCmd::check_for_delete(AlterCmd::Delete_attr_type theAttrType,
                                const std::string& name,
                                const std::string& value) const {
    switch (theAttrType) {
        case AlterCmd::DEL_VARIABLE: {
            if (!name.empty())
                Variable check(name, ""); // Create a Variable to check valid names
            break;
        }
        case AlterCmd::DEL_TIME: {
            if (!name.empty())
                (void)TimeSeries::create(name); // will throw if not valid
            break;
        }
        case AlterCmd::DEL_TODAY: {
            if (!name.empty())
                (void)TimeSeries::create(name); // will throw if not valid
            break;
        }
        case AlterCmd::DEL_DATE: {
            if (!name.empty())
                (void)DateAttr::create(name); // will throw if not valid
            break;
        }
        case AlterCmd::DEL_DAY: {
            if (!name.empty())
                (void)DayAttr::create(name); // will throw if not valid
            break;
        }
        case AlterCmd::DEL_CRON: {
            if (!name.empty()) {
                CronAttr parsedCron = CronAttr::create(name); // will throw if not valid

                // additional check since parsing is very forgiving. if parsed string is same as default
                // then no cron was specified.
                CronAttr emptyCron;
                if (emptyCron.structureEquals(parsedCron)) {
                    throw std::runtime_error("Delete cron Attribute failed. Check cron " + name);
                }
            }
            break;
        }
        case AlterCmd::DEL_AVISO: {
            if (!AvisoAttr::is_valid_name(name)) {
                throw ecf::InvalidArgument(ecf::Message("Invalid AvisoAttr name :", name_));
            }
            break;
        }
        case AlterCmd::DEL_MIRROR: {
            if (!MirrorAttr::is_valid_name(name)) {
                throw ecf::InvalidArgument(ecf::Message("Invalid MirrorAttr name :", name_));
            }
            break;
        }
        case AlterCmd::DEL_EVENT: {
            if (!name.empty()) {
                Event check(name); // will throw if not valid
            }
            break;
        }
        case AlterCmd::DEL_METER: {
            if (!name.empty())
                Meter check(name, 0, 100); // will throw if not valid
            break;
        }
        case AlterCmd::DEL_LABEL: {
            if (!name.empty())
                Label check(name, "value"); // will throw if not valid
            break;
        }
        case AlterCmd::DEL_TRIGGER:
            break; // there can only be one trigger per node, so we delete by path
        case AlterCmd::DEL_COMPLETE:
            break; // there can only be one complete per node, so we delete by path
        case AlterCmd::DEL_REPEAT:
            break; // there can only be one repeat per node, so we delete by path
        case AlterCmd::DEL_LATE:
            break; // there can only be one late per node, so we delete by path
        case AlterCmd::DEL_QUEUE: {
            if (!name.empty()) {
                std::vector<std::string> vec;
                vec.emplace_back("a");
                QueueAttr check(name, vec); // will throw if not valid
            }
            break;
        }
        case AlterCmd::DEL_GENERIC: {
            if (!name.empty())
                GenericAttr check(name); // will throw if not valid
            break;
        }
        case AlterCmd::DEL_LIMIT: {
            if (!name.empty())
                Limit check(name, 10); // will throw if not valid
            break;
        }
        case AlterCmd::DEL_INLIMIT: {
            if (!name.empty()) {
                // name can be:
                //    limit_name
                //    /path/to/limit:limit_name
                string path_to_limit; // This can be empty
                string limitName;
                if (!Extract::pathAndName(name, path_to_limit, limitName)) {
                    throw std::runtime_error("AlterCmd::DEL_INLIMIT : Invalid inlimit : " + name);
                }
                InLimit check(limitName, path_to_limit); // will throw if not valid
            }
            break;
        }
        case AlterCmd::DEL_ZOMBIE: {
            if (!Child::valid_zombie_type(name)) {
                throw std::runtime_error(
                    "Delete Zombie Attribute failed. Expected one of [ ecf | path | user ] but found " + name);
            }
            break;
        }
        case AlterCmd::DEL_LIMIT_PATH: {
            if (name.empty()) {
                throw std::runtime_error("Delete limit_path failed. No limit name provided");
            }
            return;
        }
        case AlterCmd::DELETE_ATTR_ND:
            break;
    }
}

// =====================================================================================

AlterCmd::Change_attr_type AlterCmd::get_change_attr_type(const std::string& attr_type) const {
    AlterCmd::Change_attr_type theAttrType = changeAttrType(attr_type);
    if (theAttrType == AlterCmd::CHANGE_ATTR_ND) {
        std::stringstream ss;
        ss << "AlterCmd: change: The third argument(" << attr_type << ") must be one of [ ";
        std::vector<std::string> valid;
        validChangeAttr(valid);
        for (size_t i = 0; i < valid.size(); ++i) {
            if (i != 0)
                ss << " | ";
            ss << valid[i];
        }
        ss << "]\n" << AlterCmd::desc();
        throw std::runtime_error(ss.str());
    }
    return theAttrType;
}

void AlterCmd::createChange(Cmd_ptr& cmd, std::vector<std::string>& options, std::vector<std::string>& paths) const {
    // options[0] = change
    // options[1] = [ variable | clock_type | clock_gain | clock_date | clock_sync | event | meter | label | trigger |
    //                complete | repeat | limit_max | limit_value | defstatus | late ]
    // options[2] = name
    // options[3] = value

    AlterCmd::Change_attr_type theAttrType = get_change_attr_type(options[1]);

    std::string name, value;
    extract_name_and_value_for_change(theAttrType, name, value, options, paths);
    cmd = std::make_shared<AlterCmd>(paths, theAttrType, name, value);
}

void AlterCmd::extract_name_and_value_for_change(AlterCmd::Change_attr_type theAttrType,
                                                 std::string& name,
                                                 std::string& value,
                                                 std::vector<std::string>& options,
                                                 std::vector<std::string>& paths) const {
    std::stringstream ss;
    switch (theAttrType) {
        case AlterCmd::VARIABLE: {
            if (options.size() == 3 && paths.size() > 1) {
                // The variable value may be a path, and hence it will be paths and not options parameter
                options.push_back(paths[0]);
                paths.erase(paths.begin()); // remove first path, since it has been added to options
            }
            if (options.size() < 3 || options.size() > 4) {
                ss << "AlterCmd: change: expected 5 args : change variable <variable_name> <new_value> <path_to_node>";
                ss << " but found only " << (options.size() + paths.size())
                   << " arguments.\nThe value should be quoted if there are spaces\n";
                ss << dump_args(options, paths) << "\n";
                throw std::runtime_error(ss.str());
            }
            name = options[2];
            if (options.size() == 4)
                value = options[3];
            break;
        }

        case AlterCmd::CLOCK_TYPE: {
            if (options.size() != 3) {
                ss << "AlterCmd: change: expected at least four args i.e. change clock_type [ hybrid | real ] "
                      "<path_to_suite>";
                ss << " but found only " << (options.size() + paths.size()) << " arguments\n"
                   << dump_args(options, paths) << "\n";
                throw std::runtime_error(ss.str());
            }
            name = options[2];
            break;
        }

        case AlterCmd::CLOCK_DATE: {
            if (options.size() != 3) {
                ss << "AlterCmd: change clock_date : expected at least four args :  change clock_date day.month.year "
                      "<path_to_suite>";
                ss << " but found only " << (options.size() + paths.size()) << " arguments\n"
                   << dump_args(options, paths) << "\n";
                throw std::runtime_error(ss.str());
            }
            name = options[2];
            break;
        }

        case AlterCmd::CLOCK_GAIN: {
            if (options.size() != 3) {
                ss << "AlterCmd: change clock_gain : expected four args i.e. change clock_gain <int> <path_to_suite> ";
                ss << " but found " << (options.size() + paths.size())
                   << " arguments. The actual gain must be convertible to an integer\n";
                ss << dump_args(options, paths) << "\n";
                throw std::runtime_error(ss.str());
            }
            name = options[2];
            break;
        }

        case AlterCmd::CLOCK_SYNC: {
            if (options.size() != 2) {
                ss << "AlterCmd: change clock_sync : expected three args i.e. change clock_sync  <path_to_suite> ";
                ss << " but found " << (options.size() + paths.size()) << " arguments.\n";
                ss << dump_args(options, paths) << "\n";
                throw std::runtime_error(ss.str());
            }
            break;
        }

        case AlterCmd::EVENT: {
            if (options.size() != 3 && options.size() != 4) {
                ss << "AlterCmd: Change event : expected four/five args:  change event <name_or_number> <[set | clear "
                      "| <nothing>]> <path_to_node>";
                ss << " but found only " << (options.size() + paths.size()) << " arguments\n";
                ss << dump_args(options, paths) << "\n";
                throw std::runtime_error(ss.str());
            }
            name = options[2];
            if (options.size() == 4) {
                value = options[3];
            }
            break;
        }

        case AlterCmd::METER: {
            if (options.size() != 4) {
                ss << "AlterCmd: change: expected five args: change meter meter_name meter_value  <path_to_node>";
                ss << " but found only " << (options.size() + paths.size())
                   << " arguments. The meter value must be convertible to an integer\n";
                ss << dump_args(options, paths) << "\n";
                throw std::runtime_error(ss.str());
            }
            name  = options[2];
            value = options[3];
            break;
        }

        case AlterCmd::LABEL: {
            if (options.size() == 3 && paths.size() == 1) {
                // ECFLOW-648 allow label value to be empty
                // HOWEVER , we cannot cope multiple paths, and setting value to empty.
                // since empty quotes are removed by boost program options, hence if we have a label value which is
                // path, and multiple paths
                value.clear();
            }
            else {
                // ECFLOW-480 take into account label values that is a path, adding quotes around the value does not
                // help: Note boost program options will remove the quotes around the value
                //      hence it's difficult to say what is an option and what is a path.
                //      However since we expect 4(change,label,<label_name>,<label_value>) options, work around the
                //      problem
                if (options.size() == 3 && paths.size() > 1) {
                    options.push_back(paths[0]);
                    paths.erase(paths.begin()); // remove first path, since it has been added to options
                }
                if (options.size() != 4) {
                    ss << "AlterCmd: change label expected at least five args : change label <label_name> "
                          "<label_value> <path_to_node> ";
                    ss << " but found  " << (options.size() + paths.size())
                       << " arguments. the label value should be quoted\n";
                    ss << dump_args(options, paths) << "\n";
                    throw std::runtime_error(ss.str());
                }
                value = options[3];
                if (value.find("\\n") != std::string::npos) {
                    Str::replaceall(value, "\\n", "\n");
                }
            }
            name = options[2];
            break;
        }

        case AlterCmd::AVISO: {
            if (options.size() != 4 || paths.size() < 1) {
                ss << "AlterCmd: change: Expected 'change aviso <name> <cfg> <path> [<path>] [...]."
                   << "Incorrect number of arguments.\n"
                   << dump_args(options, paths) << "\n";
                throw std::runtime_error(ss.str());
            }
            name  = options[2];
            value = options[3];
            break;
        }

        case AlterCmd::MIRROR: {
            if (options.size() != 4 || paths.size() < 1) {
                ss << "AlterCmd: change: Expected 'change mirror <name> <cfg> <path> [<path>] [...]."
                   << "Incorrect number of arguments.\n"
                   << dump_args(options, paths) << "\n";
                throw std::runtime_error(ss.str());
            }
            name  = options[2];
            value = options[3];
            break;
        }

        case AlterCmd::LATE: {
            if (options.size() != 3) {
                ss << "AlterCmd: change: expected three args: change late \"late -s +00:15  -a  20:00  -c +02:00\" "
                      "<path_to_node>";
                ss << " but found only " << (options.size() + paths.size()) << " arguments\n";
                ss << dump_args(options, paths) << "\n";
                throw std::runtime_error(ss.str());
            }
            name = options[2];
            break;
        }

        case AlterCmd::TRIGGER: {
            if (options.size() != 3) {
                ss << "AlterCmd: change: expected four args : change trigger 'expression' <path_to_node>";
                ss << " but found " << (options.size() + paths.size())
                   << " arguments. The trigger expression must be quoted\n";
                ss << dump_args(options, paths) << "\n";
                throw std::runtime_error(ss.str());
            }
            name = options[2];
            break;
        }

        case AlterCmd::COMPLETE: {
            if (options.size() != 3) {
                ss << "AlterCmd: change complete: expected four args: change complete 'expression'  <path_to_node> ";
                ss << " but found " << (options.size() + paths.size()) << " arguments. The expression must be quoted\n";
                ss << dump_args(options, paths) << "\n";
                throw std::runtime_error(ss.str());
            }
            name = options[2];
            break;
        }

        case AlterCmd::REPEAT: {
            // *NOTE* a Node can only have *ONE* repeat, hence no need to provide name
            if (options.size() != 3) {
                ss << "AlterCmd: change repeat: expected four arg's : change repeat [ integer | string ] "
                      "<path_to_node>";
                ss << " but found only " << (options.size() + paths.size()) << " arguments.\n";
                ss << dump_args(options, paths) << "\n";
                throw std::runtime_error(ss.str());
            }
            name = options[2];
            break;
        }

        case AlterCmd::LIMIT_MAX: {
            if (options.size() != 4) {
                ss << "AlterCmd: change: limit_max: : expected five arguments : change limit_max <limit_name> <int> "
                      "<path_to_node>";
                ss << " but found  " << (options.size() + paths.size()) << " arguments.\n";
                ss << dump_args(options, paths) << "\n";
                throw std::runtime_error(ss.str());
            }
            name  = options[2];
            value = options[3];
            break;
        }

        case AlterCmd::LIMIT_VAL: {
            if (options.size() != 4) {
                ss << "AlterCmd: change: limit-value: expected five arguments : change limit_value <limit_name> <int> "
                      "<path_to_node>";
                ss << " but found  " << (options.size() + paths.size()) << " arguments.\n";
                ss << dump_args(options, paths) << "\n";
                throw std::runtime_error(ss.str());
            }
            name  = options[2];
            value = options[3];
            break;
        }

        case AlterCmd::TIME: {
            if (options.size() != 4) {
                ss << "AlterCmd: change: time: expected five arguments : change time old_time new_time <path_to_node>";
                ss << " but found  " << (options.size() + paths.size()) << " arguments.\n";
                ss << dump_args(options, paths) << "\n";
                throw std::runtime_error(ss.str());
            }
            name  = options[2];
            value = options[3];
            break;
        }

        case AlterCmd::TODAY: {
            if (options.size() != 4) {
                ss << "AlterCmd: change: today: expected five arguments : change time old_today new_today "
                      "<path_to_node>";
                ss << " but found  " << (options.size() + paths.size()) << " arguments.\n";
                ss << dump_args(options, paths) << "\n";
                throw std::runtime_error(ss.str());
            }
            name  = options[2];
            value = options[3];
            break;
        }

        case AlterCmd::DEFSTATUS: {
            if (options.size() != 3) {
                ss << "AlterCmd: change defstatus expected four args : change defstatus [ queued | complete | unknown "
                      "| aborted | suspended ] <path_to_node>";
                ss << " but found  " << (options.size() + paths.size()) << " arguments.\n";
                ss << dump_args(options, paths) << "\n";
                throw std::runtime_error(ss.str());
            }
            name = options[2];
            break;
        }

        case AlterCmd::CHANGE_ATTR_ND:
            break;
        default:
            break;
    }
}

void AlterCmd::check_for_change(AlterCmd::Change_attr_type theAttrType,
                                const std::string& name,
                                const std::string& value) const {
    std::stringstream ss;
    switch (theAttrType) {
        case AlterCmd::VARIABLE:
            break;
        case AlterCmd::CLOCK_TYPE: {
            if (name != "hybrid" && name != "real") {
                ss << "AlterCmd: change clock_type: expected third argument to be one of [ hybrid | real ] but found "
                   << name << "\n";
                throw std::runtime_error(ss.str());
            }
            break;
        }
        case AlterCmd::CLOCK_DATE: {
            // Check date is in correct format:
            try {
                int day, month, year;
                DateAttr::getDate(name, day, month, year);
                DateAttr::checkDate(day, month, year, false /* for clocks, we don't allow wild carding */);
            }
            catch (std::exception& e) {
                ss << "AlterCmd:change  clock_date " << name << " is not valid. " << e.what();
                throw std::runtime_error(ss.str());
            }
            break;
        }

        case AlterCmd::CLOCK_GAIN: {
            try {
                ecf::convert_to<int>(name);
            }
            catch (const ecf::bad_conversion&) {
                ss << "AlterCmd:change  clock_gain expected '" << name << "' to be convertible to an integer\n";
                throw std::runtime_error(ss.str());
            }
            break;
        }

        case AlterCmd::CLOCK_SYNC:
            break;

        case AlterCmd::EVENT: {
            if (!value.empty() && value != Event::SET() && value != Event::CLEAR()) {
                ss << "AlterCmd: Change event : expected  <[set | clear | <nothing>]> for the value";
                throw std::runtime_error(ss.str());
            }
            // The name could be an integer
            try {
                ecf::convert_to<int>(name);
            }
            catch (const ecf::bad_conversion&) {
                // name is not an integer, check name is valid
                Event check_name(name); // will throw if name is not valid
            }
            break;
        }

        case AlterCmd::METER: {
            Meter check(name, 0, 100); // Check meter name , by creating a meter
            try {
                ecf::convert_to<int>(value);
            }
            catch (const ecf::bad_conversion&) {
                ss << "AlterCmd change meter : " << value << " to be convertible to an integer\n";
                throw std::runtime_error(ss.str());
            }
            break;
        }

        case AlterCmd::LABEL: {
            Label check(name, value); // Check name , by creating
            break;
        }

        case AlterCmd::LATE: {
            (void)LateAttr::create(name); // Check we can create the late
            break;
        }

        case AlterCmd::TIME: {
            (void)TimeSeries::create(name);
            (void)TimeSeries::create(value);
            break;
        }

        case AlterCmd::TODAY: {
            (void)TimeSeries::create(name);
            (void)TimeSeries::create(value);
            break;
        }

        case AlterCmd::TRIGGER: {
            std::string error_msg       = "AlterCmd: change trigger:";
            std::unique_ptr<AstTop> ast = Expression::parse_no_throw(name, error_msg);
            if (!ast.get()) {
                ss << error_msg << "\n";
                throw std::runtime_error(ss.str());
            }
            break;
        }

        case AlterCmd::COMPLETE: {
            std::string error_msg       = "AlterCmd: change complete:";
            std::unique_ptr<AstTop> ast = Expression::parse_no_throw(name, error_msg);
            if (!ast.get()) {
                ss << error_msg << "\n";
                throw std::runtime_error(ss.str());
            }
            break;
        }

        case AlterCmd::REPEAT: {
            // *NOTE* a Node can only have *ONE* repeat, hence no need to provide name
            break;
        }

        case AlterCmd::LIMIT_MAX: {
            int limit = 0;
            try {
                limit = ecf::convert_to<int>(value);
            }
            catch (const ecf::bad_conversion&) {
                ss << "AlterCmd: change: limit-max: expected " << value << " to be convertible to an integer\n";
                throw std::runtime_error(ss.str());
            }
            Limit check(name, limit); // Check name , by creating
            break;
        }

        case AlterCmd::LIMIT_VAL: {
            try {
                ecf::convert_to<int>(value);
            }
            catch (const ecf::bad_conversion&) {
                ss << "AlterCmd: change: limit_value: expected " << value << " to be convertible to an integer\n";
                throw std::runtime_error(ss.str());
            }
            Limit check(name, 10); // Check name, by creating
            break;
        }

        case AlterCmd::DEFSTATUS: {
            if (!DState::isValid(name)) {
                ss << "AlterCmd change defstatus : expected " << name
                   << " to be a valid state,  i.e one of [ queued | complete | unknown | aborted | suspended ]\n";
                throw std::runtime_error(ss.str());
            }
            break;
        }

        case AlterCmd::CHANGE_ATTR_ND:
            break;
        default:
            break;
    }
}

ecf::Flag::Type AlterCmd::get_flag_type(const std::string& flag_type) const {
    Flag::Type theFlagType = Flag::string_to_flag_type(flag_type);
    if (theFlagType == Flag::NOT_SET) {
        std::stringstream ss;
        ss << "AlterCmd: set/clear_flag: The second argument(" << flag_type << ") must be one of [ ";
        std::vector<std::string> valid;
        Flag::valid_flag_type(valid);
        for (size_t i = 0; i < valid.size(); ++i) {
            if (i != 0)
                ss << " | ";
            ss << valid[i];
        }
        ss << "]\n" << AlterCmd::desc();
        throw std::runtime_error(ss.str());
    }
    return theFlagType;
}

void AlterCmd::create_flag(Cmd_ptr& cmd,
                           const std::vector<std::string>& options,
                           const std::vector<std::string>& paths,
                           bool flag) const {
    // options[0] = set_flag | clear_flag
    // options[1] = [ force_aborted | user_edit | task_aborted | edit_failed | ecfcmd_failed | no_script | killed |
    //                late | message | complete | queue_limit | task_waiting | locked | zombie ]

    Flag::Type theFlagType = get_flag_type(options[1]);
    cmd                    = std::make_shared<AlterCmd>(paths, theFlagType, flag);
}

void AlterCmd::check_sort_attr_type(const std::string& attr_type) const {
    ecf::Attr::Type theAttrType = Attr::to_attr(attr_type);
    if (theAttrType == Attr::UNKNOWN) {
        std::stringstream ss;
        ss << "AlterCmd: sort: The second argument must be one of [ ";
        std::vector<std::string> valid = Attr::all_attrs();
        for (size_t i = 0; i < valid.size(); ++i) {
            if (i != 0)
                ss << " | ";
            ss << valid[i];
        }
        ss << "] but found " << attr_type << "\n" << AlterCmd::desc();
        throw std::runtime_error(ss.str());
    }
}

void AlterCmd::create_sort_attributes(Cmd_ptr& cmd,
                                      const std::vector<std::string>& options,
                                      const std::vector<std::string>& paths) const {
    // options[0] - sort
    // options[1] - [ event | meter | label | limit | variable | all ]
    // options[2] - recursive
    std::stringstream ss;
    if (options.size() < 2) {
        ss << "AlterCmd: add: At least three arguments expected. Found " << (options.size() + paths.size()) << "\n"
           << dump_args(options, paths) << "\n";
        throw std::runtime_error(ss.str());
    }

    check_sort_attr_type(options[1]);
    std::string name = options[1];

    std::string value;
    if (options.size() == 3) {
        if (options[2] != "recursive") {
            ss << "AlterCmd: sort: Expected third argument to be 'recursive' but found '" << options[2] << "\n"
               << AlterCmd::desc();
            throw std::runtime_error(ss.str());
        }
        value = "recursive";
    }

    cmd = std::make_shared<AlterCmd>(paths, name, value);
}

AlterCmd::AlterCmd(const std::vector<std::string>& paths,
                   const std::string& alterType, /* one of [ add | change | delete | set_flag | clear_flag ] */
                   const std::string& attrType,
                   const std::string& name,
                   const std::string& value)
    : paths_(paths),
      name_(name),
      value_(value) {
    if (alterType == "add") {

        add_attr_type_ = get_add_attr_type(attrType);
        check_for_add(add_attr_type_, name, value);
        return;
    }
    else if (alterType == "change") {

        change_attr_type_ = get_change_attr_type(attrType);
        check_for_change(change_attr_type_, name, value);
        return;
    }
    else if (alterType == "delete") {

        del_attr_type_ = get_delete_attr_type(attrType);
        check_for_delete(del_attr_type_, name, value);
        return;
    }
    else if (alterType == "set_flag") {

        flag_type_ = get_flag_type(attrType);
        flag_      = true;
        return;
    }
    else if (alterType == "clear_flag") {

        flag_type_ = get_flag_type(attrType);
        return;
    }

    std::stringstream ss;
    ss << "AlterCmd constructor: The alterType argument must be one of [ change | delete | add | set_flag | clear_flag "
          "| sort ] but found '"
       << alterType << "'\n";
    throw std::runtime_error(ss.str());
}

std::ostream& operator<<(std::ostream& os, const AlterCmd& c) {
    std::string ret;
    c.print(ret);
    os << ret;
    return os;
}

CEREAL_REGISTER_TYPE(AlterCmd)
CEREAL_REGISTER_DYNAMIC_INIT(AlterCmd)
