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
#include "ecflow/attribute/VerifyAttr.hpp"
#include "ecflow/attribute/Zombie.hpp"
#include "ecflow/attribute/ZombieAttr.hpp"
#include "ecflow/node/Attr.hpp"
#include "ecflow/node/AutoRestoreAttr.hpp"
#include "ecflow/node/AvisoAttr.hpp"
#include "ecflow/node/Expression.hpp"
#include "ecflow/node/Flag.hpp"
#include "ecflow/node/InLimit.hpp"
#include "ecflow/node/JobCreationCtrl.hpp"
#include "ecflow/node/Limit.hpp"
#include "ecflow/node/MirrorAttr.hpp"
#include "ecflow/python/DefsDoc.hpp"
#include "ecflow/python/ExportCollections.hpp"
#include "ecflow/python/NodeAttrDoc.hpp"
#include "ecflow/python/PythonBinding.hpp"
#include "ecflow/python/PythonUtil.hpp"
#include "ecflow/python/Trigger.hpp"

namespace {

// JobCreationCtrl

///
/// @brief Construct a default JobCreationCtrl instance.
///
/// @return The newly created JobCreationCtrl.
///
job_creation_ctrl_ptr JobCreationCtrl_make() {
    return std::make_shared<JobCreationCtrl>();
}

// ZombieAttr

///
/// @brief Construct a ZombieAttr from a zombie type, a list of child command types, and a control action.
///
/// @param zt The zombie type (e.g. user, path, ecf).
/// @param list A Python list of `Child::CmdType` values specifying which child commands the attribute applies to.
/// @param uc The control action to take on matching zombies.
/// @return The newly created ZombieAttr.
///
ZombieAttr ZombieAttr_make(ecf::Child::ZombieType zt, const py::list& list, ecf::ZombieCtrlAction uc) {

    int the_list_size = len(list);

    std::vector<ecf::Child::CmdType> vec;
    vec.reserve(the_list_size);

    for (int i = 0; i < the_list_size; ++i) {
        vec.push_back(list[i].cast<ecf::Child::CmdType>());
    }
    return ZombieAttr(zt, vec, uc);
}

///
/// @brief Construct a ZombieAttr with an explicit server lifetime.
///
/// @param zt The zombie type (e.g. user, path, ecf).
/// @param list A Python list of `Child::CmdType` values specifying which child commands the attribute applies to.
/// @param uc The control action to take on matching zombies.
/// @param life_time_in_server The lifetime of the zombie in the server, in seconds.
/// @return The newly created ZombieAttr.
///
ZombieAttr ZombieAttr_make_lifetime(ecf::Child::ZombieType zt,
                                    const py::list& list,
                                    ecf::ZombieCtrlAction uc,
                                    int life_time_in_server) {

    int the_list_size = len(list);

    std::vector<ecf::Child::CmdType> vec;
    vec.reserve(the_list_size);

    for (int i = 0; i < the_list_size; ++i) {
        vec.push_back(list[i].cast<ecf::Child::CmdType>());
    }
    return ZombieAttr(zt, vec, uc, life_time_in_server);
}

// Limit

///
/// @brief Return the set of node paths currently consuming tokens from the limit.
///
/// @param limit The limit to query.
/// @return A Python list of node path strings.
///
py::list Limit_node_paths(Limit* limit) {
    py::list list;
    const std::set<std::string>& paths = limit->paths();
    for (const auto& path : paths) {
        list.append(path);
    }
    return list;
}

// Queue

///
/// @brief Construct a QueueAttr from a name and a list of step values.
///
/// @param name The queue attribute name.
/// @param list A Python list of string step values for the queue.
/// @return The newly created QueueAttr.
///
QueueAttr QueueAttr_make(const std::string& name, const py::list& list) {
    std::vector<std::string> vec;
    pyutil_list_to_str_vec(list, vec);
    return QueueAttr(name, vec);
}

// Generic

///
/// @brief Construct a GenericAttr from a name and a list of string values.
///
/// @param name The generic attribute name.
/// @param list A Python list of string values.
/// @return The newly created GenericAttr.
///
GenericAttr GenericAttr_make(const std::string& name, const py::list& list) {
    std::vector<std::string> vec;
    pyutil_list_to_str_vec(list, vec);
    return GenericAttr(name, vec);
}

// Late

///
/// @brief Populate a LateAttr from keyword arguments.
///
/// Recognised keywords are `submitted`, `active`, and `complete`; each takes a
/// time-string value of the form `"HH:MM"` or `"+HH:MM"` (relative).
///
/// @param late The LateAttr to populate.
/// @param kwargs Keyword arguments mapping time-slot names to time strings.
///
void LateAttr_extract_from_kwargs(std::shared_ptr<ecf::LateAttr> late, const py::kwargs& kwargs) {

    for (const auto& entry : kwargs) {
        // 1. Extract the keywork argument name
        std::string first;
        if (auto found = py_extract<std::string>(entry.first); found) {
            first = found.value();
        }
        else if (auto found = py_extract<py::str>(entry.first); found) {
            first = found.value();
        }
        else {
            throw std::runtime_error("extract_late_keyword_arguments: keyword argument name expected to be a string");
        }

        // 1.a Ensure the keyword argument is an expected value
        static const std::vector valid_keywords = {"submitted", "active", "complete"};
        if (find(valid_keywords.begin(), valid_keywords.end(), first) == valid_keywords.end()) {
            throw std::runtime_error("extract_late_keyword_arguments: keyword argument expected to be one of "
                                     "[submitted | active | complete]");
        }

        // 2. Extract keyword argument value
        std::string second;
        if (auto found = py_extract<std::string>(entry.second); found) {
            second = found.value();
        }
        else if (auto found = py_extract<py::str>(entry.second); found) {
            second = found.value();
        }
        else {
            throw std::runtime_error("extract_late_keyword_arguments: keyword argument value expected to be a string, "
                                     "ie Late(submitted='00:20',active='15:00',complete='+30:00')");
        }

        // 3. Convert the value to hour/min/relative, and set the Late attributes
        int hour      = 0;
        int min       = 0;
        bool relative = ecf::TimeSeries::getTime(second, hour, min);
        if (first == "submitted") {
            late->add_submitted(hour, min);
        }
        else if (first == "active") {
            late->add_active(hour, min);
        }
        else if (first == "complete") {
            late->add_complete(hour, min, relative);
        }
    }
}

///
/// @brief Construct a default, empty LateAttr.
///
/// @return The newly created LateAttr.
///
std::shared_ptr<ecf::LateAttr> LateAttr_make_default() {
    return std::make_shared<ecf::LateAttr>();
}

///
/// @brief Construct a LateAttr from keyword arguments.
///
/// Only keyword arguments are accepted; positional arguments are rejected.
/// Recognised keywords: `submitted`, `active`, `complete` (time strings, e.g. `"00:20"`, `"+30:00"`).
///
/// @param args Must be empty; positional arguments are not supported.
/// @param kwargs Time-slot keyword arguments (submitted, active, complete).
/// @return The newly created LateAttr.
///
std::shared_ptr<ecf::LateAttr> LateAttr_make(const py::args& args, const py::kwargs& kwargs) {
    int arg_count = len(args);
    if (arg_count > 0) {
        throw std::runtime_error("late_init: Late only expects keyword arguments, i.e. "
                                 "Late(submitted='00:20',active='15:00',complete='+30:00')");
    }
    auto late = std::make_shared<ecf::LateAttr>();
    LateAttr_extract_from_kwargs(late, kwargs);
    return late;
}

// AutoRestoreAttr

///
/// @brief Construct an AutoRestoreAttr from a list of node paths to restore.
///
/// @param list A Python list of absolute node path strings.
/// @return The newly created AutoRestoreAttr.
///
ecf::AutoRestoreAttr AutoRestoreAttr_make(const py::list& list) {
    std::vector<std::string> vec;
    pyutil_list_to_str_vec(list, vec);
    return ecf::AutoRestoreAttr(vec);
}

// RepeatDateList

///
/// @brief Construct a RepeatDateList from a name and a list of date integers.
///
/// @param name The repeat variable name.
/// @param list A Python list of integer dates (in `YYYYMMDD` format).
/// @return The newly created RepeatDateList.
///
RepeatDateList RepeatDateList_make(const std::string& name, const py::list& list) {
    std::vector<int> vec;
    pyutil_list_to_int_vec(list, vec);
    return RepeatDateList(name, vec);
}

// RepeatEnumerated

///
/// @brief Construct a RepeatEnumerated from a name and a list of enumeration values.
///
/// @param name The repeat variable name.
/// @param list A Python list of string enumeration values.
/// @return The newly created RepeatEnumerated.
///
RepeatEnumerated RepeatEnumerated_make(const std::string& name, const py::list& list) {
    std::vector<std::string> vec;
    pyutil_list_to_str_vec(list, vec);
    return RepeatEnumerated(name, vec);
}

// RepeatString

///
/// @brief Construct a RepeatString from a name and a list of string values.
///
/// @param name The repeat variable name.
/// @param list A Python list of string values to iterate over.
/// @return The newly created RepeatString.
///
RepeatString RepeatString_make(const std::string& name, const py::list& list) {
    std::vector<std::string> vec;
    pyutil_list_to_str_vec(list, vec);
    return RepeatString(name, vec);
}

// Cron

///
/// @brief Populate a CronAttr from a dictionary of keyword arguments.
///
/// Recognised keys: `days_of_week`, `days_of_month`, `months`,
/// `last_week_days_of_the_month` (each a list of integers), and
/// `last_day_of_the_month` (a boolean).
///
/// @param cron The CronAttr to populate.
/// @param dict A Python dict mapping keyword names to their values.
///
void CronAttr_extract_kwargs(std::shared_ptr<ecf::CronAttr> cron, py::dict& dict) {
    for (auto entry : dict) {
        if (auto found_key = py_extract<py::str>(entry.first); found_key) {
            std::string key = found_key.value();

            if (auto found_value = py_extract<py::list>(entry.second); found_value) {
                std::vector<int> value;
                pyutil_list_to_int_vec(found_value.value(), value);

                if (key == "days_of_week") {
                    cron->addWeekDays(value);
                }
                else if (key == "days_of_month") {
                    cron->addDaysOfMonth(value);
                }
                else if (key == "months") {
                    cron->addMonths(value);
                }
                else if (key == "last_week_days_of_the_month") {
                    cron->add_last_week_days_of_month(value);
                }
                else {
                    throw std::runtime_error(
                        "CronAttr_extract_kwargs: keyword arguments, expected [days_of_week | "
                        "last_week_days_of_the_month | days_of_month | months | last_day_of_the_month");
                }
            }
            else if (auto found_value = py_extract<py::bool_>(entry.second); found_value) {
                if (key == "last_day_of_the_month" && found_value.value()) {
                    cron->add_last_day_of_month();
                }
                else {
                    throw std::runtime_error(
                        "CronAttr_extract_kwargs: keyword arguments, expected [days_of_week | "
                        "last_week_days_of_the_month | days_of_month | months | last_day_of_the_month]");
                }
            }
            else {
                throw std::runtime_error("CronAttr_extract_kwargs: keyword arguments to be a list");
            }
        }
    }
}

///
/// @brief Construct a default, empty CronAttr.
///
/// @return The newly created CronAttr.
///
std::shared_ptr<ecf::CronAttr> CronAttr_make() {
    return std::make_shared<ecf::CronAttr>();
}

///
/// @brief Construct a CronAttr from a time-series string and optional keyword arguments.
///
/// @param ts A time-series string (e.g. `"10:00"`, `"00:30 23:30 00:30"`).
/// @param kwargs Optional keyword arguments for days/months (see `CronAttr_extract_kwargs`).
/// @return The newly created CronAttr.
///
std::shared_ptr<ecf::CronAttr> CronAttr_make_string_kwargs(const std::string& ts, py::kwargs& kwargs) {
    std::shared_ptr<ecf::CronAttr> cron = std::make_shared<ecf::CronAttr>(ts);
    CronAttr_extract_kwargs(cron, kwargs);
    return cron;
}

///
/// @brief Construct a CronAttr from a TimeSeries object and optional keyword arguments.
///
/// @param ts The time series defining when the cron fires.
/// @param kwargs Optional keyword arguments for days/months (see `CronAttr_extract_kwargs`).
/// @return The newly created CronAttr.
///
std::shared_ptr<ecf::CronAttr> CronAttr_make_timeseries_kwargs(const ecf::TimeSeries& ts, py::kwargs& kwargs) {
    std::shared_ptr<ecf::CronAttr> cron = std::make_shared<ecf::CronAttr>(ts);
    CronAttr_extract_kwargs(cron, kwargs);
    return cron;
}

///
/// @brief Construct a CronAttr from a TimeSeries object.
///
/// @param ts The time series defining when the cron fires.
/// @return The newly created CronAttr.
///
std::shared_ptr<ecf::CronAttr> CronAttr_make_timeseries(const ecf::TimeSeries& ts) {
    return std::make_shared<ecf::CronAttr>(ts);
}

///
/// @brief Set the days-of-week on the cron attribute.
///
/// @param cron The CronAttr to modify.
/// @param list A Python list of integers (0 = Sunday … 6 = Saturday).
///
void CronAttr_set_week_days(ecf::CronAttr* cron, const py::list& list) {
    std::vector<int> int_vec;
    pyutil_list_to_int_vec(list, int_vec);
    cron->addWeekDays(int_vec);
}

///
/// @brief Set the last week-days-of-month on the cron attribute.
///
/// @param cron The CronAttr to modify.
/// @param list A Python list of integers (0 = Sunday … 6 = Saturday).
///
void CronAttr_set_last_week_days_of_month(ecf::CronAttr* cron, const py::list& list) {
    std::vector<int> int_vec;
    pyutil_list_to_int_vec(list, int_vec);
    cron->add_last_week_days_of_month(int_vec);
}

///
/// @brief Set the days-of-month on the cron attribute.
///
/// @param cron The CronAttr to modify.
/// @param list A Python list of integers (1–31).
///
void CronAttr_set_days_of_month(ecf::CronAttr* cron, const py::list& list) {
    std::vector<int> int_vec;
    pyutil_list_to_int_vec(list, int_vec);
    cron->addDaysOfMonth(int_vec);
}

///
/// @brief Set the cron to fire on the last day of the month.
///
/// @param cron The CronAttr to modify.
///
void CronAttr_set_last_day_of_the_month(ecf::CronAttr* cron) {
    cron->add_last_day_of_month();
}

///
/// @brief Set the months on the cron attribute.
///
/// @param cron The CronAttr to modify.
/// @param list A Python list of integers (1 = January … 12 = December).
///
void CronAttr_set_months(ecf::CronAttr* cron, const py::list& list) {
    std::vector<int> int_vec;
    pyutil_list_to_int_vec(list, int_vec);
    cron->addMonths(int_vec);
}

///
/// @brief Set the time series on the cron attribute from a time-series string.
///
/// @param self The CronAttr to modify.
/// @param ts A time-series string (e.g. `"10:00"`, `"00:30 23:30 00:30"`).
///
void CronAttr_set_time_series(ecf::CronAttr* self, const std::string& ts) {
    self->addTimeSeries(ecf::TimeSeries::create(ts));
}

// Aviso

///
/// @brief Construct an AvisoAttr with the given name, listener, and optional connection parameters.
///
/// @param name The attribute name.
/// @param listener The Aviso listener configuration string.
/// @param url The Aviso server URL (defaults to `AvisoAttr::default_url`).
/// @param schema The schema path (defaults to `AvisoAttr::default_schema`).
/// @param polling The polling interval (defaults to `AvisoAttr::default_polling`).
/// @param auth The authentication configuration (defaults to `AvisoAttr::default_auth`).
/// @return The newly created AvisoAttr.
///
ecf::AvisoAttr AvisoAttr_make(const std::string& name,
                              const std::string& listener,
                              const std::string& url     = ecf::AvisoAttr::default_url,
                              const std::string& schema  = ecf::AvisoAttr::default_schema,
                              const std::string& polling = ecf::AvisoAttr::default_polling,
                              const std::string& auth    = ecf::AvisoAttr::default_auth) {
    return ecf::AvisoAttr(nullptr, name, listener, url, schema, polling, 0, auth, "");
}

///
/// @brief Return a string representation of the AvisoAttr.
///
/// @param aviso The attribute to serialise.
/// @return A Python-friendly string representation.
///
std::string AvisoAttr_str(const ecf::AvisoAttr& aviso) {
    return ecf::to_python_string(aviso);
}

// Mirror

///
/// @brief Construct a MirrorAttr with the given name, path, and optional connection parameters.
///
/// @param name The attribute name.
/// @param path The absolute path of the mirrored node on the remote server.
/// @param host The remote server hostname (defaults to `MirrorAttr::default_remote_host`).
/// @param port The remote server port (defaults to `MirrorAttr::default_remote_port`).
/// @param polling The polling interval (defaults to `MirrorAttr::default_polling`).
/// @param ssl Whether to use SSL for the remote connection.
/// @param auth The authentication configuration (defaults to `MirrorAttr::default_remote_auth`).
/// @return The newly created MirrorAttr.
///
ecf::MirrorAttr MirrorAttr_make(const std::string& name,
                                const std::string& path,
                                const std::string& host    = ecf::MirrorAttr::default_remote_host,
                                const std::string& port    = ecf::MirrorAttr::default_remote_port,
                                const std::string& polling = ecf::MirrorAttr::default_polling,
                                bool ssl                   = false,
                                const std::string& auth    = ecf::MirrorAttr::default_remote_auth) {
    return ecf::MirrorAttr(nullptr, name, path, host, port, polling, ssl, auth, "");
}

///
/// @brief Return a string representation of the MirrorAttr.
///
/// @param mirror The attribute to serialise.
/// @return A Python-friendly string representation.
///
std::string MirrorAttr_str(const ecf::MirrorAttr& mirror) {
    return ecf::to_python_string(mirror);
}

} // namespace

void export_NodeAttr(py::module& m) {

    // Trigger & Complete are a thin wrapper over Expression, allowing to call:
    //
    //   Task("a").add(Trigger("a=1"),Complete("b=1"))
    //
    py::class_<Trigger, std::shared_ptr<Trigger>>(m, "Trigger", DefsDoc::trigger())

        .def(py::init<std::string>())
        .def(py::init<PartExpression>())
        .def(py::init<py::list>())
        .def(py::init<std::string, bool>())
        .def(py::self == py::self)
        .def("__str__", &Trigger::expression)
        .def("get_expression", &Trigger::expression, "returns the trigger expression as a string");

    py::class_<Complete, std::shared_ptr<Complete>>(m, "Complete", DefsDoc::trigger())

        .def(py::init<std::string>())
        .def(py::init<PartExpression>())
        .def(py::init<py::list>())
        .def(py::init<std::string, bool>())
        .def(py::self == py::self)
        .def("__str__", &Complete::expression)
        .def("get_expression", &Complete::expression, "returns the complete expression as a string");

    py::class_<PartExpression>(m, "PartExpression", DefsDoc::part_expression_doc())

        .def(py::init<std::string>())
        .def(py::init<std::string, bool>())
        .def(py::self == py::self)
        .def("get_expression",
             &PartExpression::expression,
             py::return_value_policy::reference,
             "returns the part expression as a string")
        .def("and_expr", &PartExpression::andExpr)
        .def("or_expr", &PartExpression::orExpr);

    py::class_<Expression, std::shared_ptr<Expression>>(m, "Expression", DefsDoc::expression_doc())

        .def(py::init<std::string>())
        .def(py::init<PartExpression>())
        .def(py::self == py::self)
        .def("__str__", &Expression::expression)
        .def("get_expression", &Expression::expression, "returns the complete expression as a string")
        .def("add",
             &Expression::add,
             "Add a part expression, the second and subsequent part expressions must have 'and/or' set")
        .def("parts", &Expression::expr, "Returns a list of PartExpression's");

    py::enum_<ecf::Flag::Type>(m, "FlagType", NodeAttrDoc::flag_type_doc())

        .value("force_abort", ecf::Flag::FORCE_ABORT)
        .value("user_edit", ecf::Flag::USER_EDIT)
        .value("task_aborted", ecf::Flag::TASK_ABORTED)
        .value("edit_failed", ecf::Flag::EDIT_FAILED)
        .value("jobcmd_failed", ecf::Flag::JOBCMD_FAILED)
        .value("killcmd_failed", ecf::Flag::KILLCMD_FAILED)
        .value("statuscmd_failed", ecf::Flag::STATUSCMD_FAILED)
        .value("no_script", ecf::Flag::NO_SCRIPT)
        .value("killed", ecf::Flag::KILLED)
        .value("status", ecf::Flag::STATUS)
        .value("late", ecf::Flag::LATE)
        .value("message", ecf::Flag::MESSAGE)
        .value("byrule", ecf::Flag::BYRULE)
        .value("queuelimit", ecf::Flag::QUEUELIMIT)
        .value("wait", ecf::Flag::WAIT)
        .value("locked", ecf::Flag::LOCKED)
        .value("zombie", ecf::Flag::ZOMBIE)
        .value("no_reque", ecf::Flag::NO_REQUE_IF_SINGLE_TIME_DEP)
        .value("archived", ecf::Flag::ARCHIVED)
        .value("restored", ecf::Flag::RESTORED)
        .value("threshold", ecf::Flag::THRESHOLD)
        .value("sigterm", ecf::Flag::ECF_SIGTERM)
        .value("not_set", ecf::Flag::NOT_SET)
        .value("log_error", ecf::Flag::LOG_ERROR)
        .value("checkpt_error", ecf::Flag::CHECKPT_ERROR)
        .value("remote_error", ecf::Flag::REMOTE_ERROR);

    py::class_<ecf::Flag>(m, "Flag", "Represents additional state associated with a Node.\n\n")

        .def(py::init<>())
        .def("__str__", &ecf::Flag::to_string)
        .def(py::self == py::self)
        .def("is_set", &ecf::Flag::is_set, "Queries if a given flag is set")
        .def("set", &ecf::Flag::set, "Sets the given flag. Used in test only")
        .def("clear", &ecf::Flag::clear, "Clear the given flag. Used in test only")
        .def("reset", &ecf::Flag::reset, "Clears all flags. Used in test only")
        .def_static("list", &ecf::Flag::list, "Returns the list of all flag types. returns FlagTypeVec. Tests only")
        .def_static("type_to_string", &ecf::Flag::enum_to_string, "Convert type to a string. Tests only");

    py::class_<std::vector<ecf::Flag::Type>>(m, "FlagTypeVec", "Hold a list of flag types");

    py::class_<JobCreationCtrl, std::shared_ptr<JobCreationCtrl>>(m, "JobCreationCtrl", DefsDoc::jobgenctrl_doc())

        .def(py::init(&JobCreationCtrl_make), DefsDoc::jobgenctrl_doc())
        .def("set_node_path",
             &JobCreationCtrl::set_node_path,
             "The node we want to check job creation for. If no node specified check all tasks")
        .def("set_dir_for_job_creation",
             &JobCreationCtrl::set_dir_for_job_creation,
             "Specify directory, for job creation")
        .def("set_verbose", &JobCreationCtrl::set_verbose, "Output each task as its being checked.")
        .def("get_dir_for_job_creation",
             &JobCreationCtrl::dir_for_job_creation,
             py::return_value_policy::reference,
             "Returns the directory set for job creation")
        .def("generate_temp_dir",
             &JobCreationCtrl::generate_temp_dir,
             "Automatically generated temporary directory for job creation. Directory written to stdout for "
             "information")
        .def("get_error_msg",
             &JobCreationCtrl::get_error_msg,
             py::return_value_policy::reference,
             "Returns an error message generated during checking of job creation");

    py::enum_<ecf::Child::ZombieType>(m, "ZombieType", NodeAttrDoc::zombie_type_doc())

        .value("ecf", ecf::Child::ECF)
        .value("ecf_pid", ecf::Child::ECF_PID)
        .value("ecf_pid_passwd", ecf::Child::ECF_PID_PASSWD)
        .value("ecf_passwd", ecf::Child::ECF_PASSWD)
        .value("user", ecf::Child::USER)
        .value("path", ecf::Child::PATH);

    py::enum_<ecf::ZombieCtrlAction>(m, "ZombieUserActionType", NodeAttrDoc::zombie_user_action_type_doc())

        .value("fob", ecf::ZombieCtrlAction::FOB)
        .value("fail", ecf::ZombieCtrlAction::FAIL)
        .value("remove", ecf::ZombieCtrlAction::REMOVE)
        .value("adopt", ecf::ZombieCtrlAction::ADOPT)
        .value("block", ecf::ZombieCtrlAction::BLOCK)
        .value("kill", ecf::ZombieCtrlAction::KILL);

    py::enum_<ecf::Child::CmdType>(m, "ChildCmdType", NodeAttrDoc::child_cmd_type_doc())

        .value("init", ecf::Child::INIT)
        .value("event", ecf::Child::EVENT)
        .value("meter", ecf::Child::METER)
        .value("label", ecf::Child::LABEL)
        .value("wait", ecf::Child::WAIT)
        .value("queue", ecf::Child::QUEUE)
        .value("abort", ecf::Child::ABORT)
        .value("complete", ecf::Child::COMPLETE);

    py::enum_<ecf::Attr::Type>(m, "AttrType", NodeAttrDoc::sortable_attribute_type_doc())

        .value("event", ecf::Attr::EVENT)
        .value("meter", ecf::Attr::METER)
        .value("label", ecf::Attr::LABEL)
        .value("limit", ecf::Attr::LIMIT)
        .value("variable", ecf::Attr::VARIABLE)
        .value("all", ecf::Attr::ALL);

    py::class_<ZombieAttr>(m, "ZombieAttr", NodeAttrDoc::zombie_doc())

        .def(py::init(&ZombieAttr_make))
        .def(py::init(&ZombieAttr_make_lifetime))
        .def("__str__", &ZombieAttr::toString)
        .def("__copy__", pyutil_copy_object<ZombieAttr>)
        .def(py::self == py::self)
        .def("empty", &ZombieAttr::empty, "Return true if the attribute is empty")
        .def("zombie_type", &ZombieAttr::zombie_type, "Returns the `zombie type`_")
        .def("user_action", &ZombieAttr::action, "The automated action to invoke, when zombies arise")
        .def("zombie_lifetime",
             &ZombieAttr::zombie_lifetime,
             "Returns the lifetime in seconds of `zombie`_ in the server")
        .def("child_cmds",
             &ZombieAttr::child_cmds,
             "The list of child commands. If empty action applies to all child cmds");

    constexpr const char* zombievec_doc = "Hold a list of zombies";

    py::class_<std::vector<Zombie>>(m, "ZombieVec", zombievec_doc);

    py::class_<Zombie>(m, "Zombie", NodeAttrDoc::plain_zombie_doc())

        .def("__str__", &Zombie::to_string)
        .def("__copy__", pyutil_copy_object<Zombie>)
        .def(py::self == py::self)
        .def("empty", &Zombie::empty)
        .def("manual_user_action", &Zombie::manual_user_action)
        .def("fob", &Zombie::fob)
        .def("fail", &Zombie::fail)
        .def("adopt", &Zombie::adopt)
        .def("block", &Zombie::block)
        .def("remove", &Zombie::remove)
        .def("kill", &Zombie::kill)
        .def("type", &Zombie::type)
        .def("type_str", &Zombie::type_str)
        .def("last_child_cmd", &Zombie::last_child_cmd)
        .def("attr", &Zombie::attr, py::return_value_policy::reference)
        .def("calls", &Zombie::calls)
        .def("jobs_password", &Zombie::jobs_password, py::return_value_policy::reference)
        .def("path_to_task", &Zombie::path_to_task, py::return_value_policy::reference)
        .def("process_or_remote_id", &Zombie::process_or_remote_id, py::return_value_policy::reference)
        .def("user_cmd", &Zombie::user_cmd, py::return_value_policy::reference)
        .def("try_no", &Zombie::try_no)
        .def("duration", &Zombie::duration)
        .def("user_action", &Zombie::user_action)
        .def("user_action_str", &Zombie::user_action_str)
        .def("host", &Zombie::host, py::return_value_policy::reference)
        .def("allowed_age", &Zombie::allowed_age);

    py::class_<Variable>(m, "Variable", NodeAttrDoc::variable_doc())

        .def(py::init<std::string, std::string>())
        .def(py::self == py::self)
        .def(py::self < py::self)
        .def("__str__", &Variable::toString)
        .def("__copy__", pyutil_copy_object<Variable>)
        .def("name", &Variable::name, py::return_value_policy::reference, "Return the variable name as string")
        .def("value", &Variable::theValue, py::return_value_policy::reference, "Return the variable value as a string")
        .def("empty",
             &Variable::empty,
             "Return true if the variable is empty. Used when returning a Null variable, from a find");

    py::class_<Label>(m, "Label", NodeAttrDoc::label_doc())

        .def(py::init<std::string, std::string>())
        .def(py::self == py::self)
        .def(py::self < py::self)
        .def("__str__", &Label::toString)
        .def("__copy__", pyutil_copy_object<Label>)
        .def("name", &Label::name, py::return_value_policy::reference, "Return the `label`_ name as string")
        .def("value", &Label::value, py::return_value_policy::reference, "Return the original `label`_ value as string")
        .def("new_value", &Label::new_value, py::return_value_policy::reference, "Return the new label value as string")
        .def(
            "empty", &Label::empty, "Return true if the Label is empty. Used when returning a NULL Label, from a find");

    py::class_<Limit, std::shared_ptr<Limit>>(m, "Limit", NodeAttrDoc::limit_doc())

        .def(py::init<std::string, int>())
        .def(py::self == py::self)
        .def(py::self < py::self)
        .def("__str__", &Limit::toString)
        .def("__copy__", pyutil_copy_object<Limit>)
        .def("name", &Limit::name, py::return_value_policy::reference, "Return the `limit`_ name as string")
        .def("value", &Limit::value, "The `limit`_ token value as an integer")
        .def("limit", &Limit::theLimit, "The max value of the `limit`_ as an integer")
        .def("increment", &Limit::increment, "used for test only")
        .def("decrement", &Limit::decrement, "used for test only")
        .def("node_paths", &Limit_node_paths, "List of nodes(paths) that have consumed a limit");

    py::class_<InLimit>(m, "InLimit", NodeAttrDoc::inlimit_doc())

        .def(py::init<std::string>())
        .def(py::init<std::string, std::string>())
        .def(py::init<std::string, std::string, int>())
        .def(py::init<std::string, std::string, int, bool>())
        .def(py::init<std::string, std::string, int, bool, bool>())
        .def(py::self == py::self)
        .def(py::self < py::self)
        .def("__str__", &InLimit::toString)
        .def("__copy__", pyutil_copy_object<InLimit>)
        .def("name", &InLimit::name, py::return_value_policy::reference, "Return the `inlimit`_ name as string")
        .def("path_to_node",
             &InLimit::pathToNode,
             py::return_value_policy::reference,
             "Path to the node that holds the limit, can be empty")
        .def("limit_this_node_only",
             &InLimit::limit_this_node_only,
             "Only this node is limited. i.e. typically Family or Suite")
        .def("limit_submission", &InLimit::limit_submission, "Limit submission only")
        .def("tokens", &InLimit::tokens, "The number of token to consume from the Limit");

    py::class_<Event>(m, "Event", NodeAttrDoc::event_doc())

        .def(py::init<std::string>())
        .def(py::init<int, std::string>(), py::arg("number"), py::arg("name") = "")
        .def(py::init<int, std::string, bool>(),
             py::arg("number"),
             py::arg("name")          = "",
             py::arg("initial_state") = false)
        .def(py::init<std::string, bool>(), py::arg("name") = "", py::arg("initial_state") = false)
        .def(py::self == py::self)
        .def(py::self < py::self)
        .def("__str__", &Event::toString)
        .def("__copy__", pyutil_copy_object<Event>)
        .def("name",
             &Event::name,
             py::return_value_policy::reference,
             "Return the Events name as string. If number supplied name may be empty.")
        .def("number", &Event::number, "Return events number as a integer. If not specified return max integer value")
        .def("name_or_number", &Event::name_or_number, "Returns name or number as string")
        .def("value", &Event::value, "Return events current value")
        .def("initial_value",
             &Event::initial_value,
             "Return events initial value, This is value taken for begin/re-queue")
        .def(
            "empty", &Event::empty, "Return true if the Event is empty. Used when returning a NULL Event, from a find");

    py::class_<Meter>(m, "Meter", NodeAttrDoc::meter_doc())

        .def(py::init<std::string, int, int, int>(),
             py::arg("name"),
             py::arg("min"),
             py::arg("color_change") = std::numeric_limits<int>::max(),
             py::arg("value")        = std::numeric_limits<int>::max())
        .def(py::self == py::self)
        .def(py::self < py::self)
        .def("__str__", &Meter::toString)
        .def("__copy__", pyutil_copy_object<Meter>)
        .def("name", &Meter::name, py::return_value_policy::reference, "Return the Meters name as string")
        .def("min", &Meter::min, "Return the Meters minimum value")
        .def("max", &Meter::max, "Return the Meters maximum value")
        .def("value", &Meter::value, "Return meters current value")
        .def("color_change", &Meter::colorChange, "returns the color change")
        .def(
            "empty", &Meter::empty, "Return true if the Meter is empty. Used when returning a NULL Meter, from a find");

    py::class_<QueueAttr>(m, "Queue", NodeAttrDoc::queue_doc())

        .def(py::init(&QueueAttr_make))
        .def(py::self == py::self)
        .def(py::self < py::self)
        .def("__str__", &QueueAttr::toString)
        .def("__copy__", pyutil_copy_object<QueueAttr>)
        .def("name", &QueueAttr::name, py::return_value_policy::reference, "Return the queue name as string")
        .def("value", &QueueAttr::value, "Return the queue current value as string")
        .def("index", &QueueAttr::index, "Return the queue current index as a integer")
        .def("empty",
             &QueueAttr::empty,
             "Return true if the Queue is empty. Used when returning a NULL Queue, from a find");

    py::class_<GenericAttr>(m, "Generic", NodeAttrDoc::generic_doc())

        .def(py::init(&GenericAttr_make))
        .def(py::self == py::self)
        .def(py::self < py::self)
        .def("__str__", &GenericAttr::to_string)
        .def("__copy__", pyutil_copy_object<GenericAttr>)
        .def("name", &GenericAttr::name, py::return_value_policy::reference, "Return the generic name as string")
        .def("empty",
             &GenericAttr::empty,
             "Return true if the Generic is empty. Used when returning a NULL Generic, from a find")
        .def("values", &GenericAttr::values, "The list of values for the generic");

    py::class_<DateAttr>(m, "Date", NodeAttrDoc::date_doc())

        .def(py::init<int, int, int>()) // day,month,year
        .def(py::init<std::string>())
        .def(py::self == py::self)
        .def(py::self < py::self)
        .def("__str__", &DateAttr::toString)
        .def("__copy__", pyutil_copy_object<DateAttr>)
        .def("day", &DateAttr::day, "Return the day. The range is 0-31, 0 means its wild-carded")
        .def("month", &DateAttr::month, "Return the month. The range is 0-12, 0 means its wild-carded")
        .def("year", &DateAttr::year, "Return the year, 0 means its wild-carded");

    py::enum_<DayAttr::Day_t>(m, "Days", NodeAttrDoc::days_enum_doc())

        .value("sunday", DayAttr::SUNDAY)
        .value("monday", DayAttr::MONDAY)
        .value("tuesday", DayAttr::TUESDAY)
        .value("wednesday", DayAttr::WEDNESDAY)
        .value("thursday", DayAttr::THURSDAY)
        .value("friday", DayAttr::FRIDAY)
        .value("saturday", DayAttr::SATURDAY);

    py::class_<DayAttr>(m, "Day", NodeAttrDoc::day_doc())

        .def(py::init<DayAttr::Day_t>())
        .def(py::init<std::string>()) // constructor
        .def(py::self == py::self)
        .def(py::self < py::self)
        .def("__str__", &DayAttr::toString)
        .def("__copy__", pyutil_copy_object<DayAttr>)
        .def("day", &DayAttr::day, "Return the day as enumerator");

    py::class_<ecf::TimeAttr>(m, "Time", NodeAttrDoc::time_doc())

        .def(py::init<ecf::TimeSlot, bool>(),
             py::arg("ts"),
             py::arg("relative") = false,
             "Initialize with a iTimeSlot and an optional relative flag")
        .def(py::init<int, int, bool>(),
             py::arg("hour"),
             py::arg("minute"),
             py::arg("relative") = false) // hour, minute, relative
        .def(py::init<ecf::TimeSeries>())
        .def(py::init<ecf::TimeSlot, ecf::TimeSlot, ecf::TimeSlot, bool>(),
             py::arg("start"),
             py::arg("finish"),
             py::arg("increment"),
             py::arg("relative") = false)
        .def(py::init<std::string>())
        .def(py::self == py::self)
        .def("__str__", &ecf::TimeAttr::toString)
        .def("__copy__", pyutil_copy_object<ecf::TimeAttr>)
        .def("time_series",
             &ecf::TimeAttr::time_series,
             py::return_value_policy::reference,
             "Return the Time attributes time series");

    py::class_<ecf::TodayAttr>(m, "Today", NodeAttrDoc::today_doc())

        .def(py::init<ecf::TimeSlot, bool>(),
             py::arg("ts"),
             py::arg("relative") = false,
             "Initialize with a iTimeSlot and an optional relative flag")
        .def(py::init<int, int, bool>(),
             py::arg("hour"),
             py::arg("minute"),
             py::arg("relative") = false) // hour, minute, relative
        .def(py::init<ecf::TimeSeries>())
        .def(py::init<ecf::TimeSlot, ecf::TimeSlot, ecf::TimeSlot, bool>(),
             py::arg("start"),
             py::arg("finish"),
             py::arg("increment"),
             py::arg("relative") = false)
        .def(py::init<std::string>())
        .def(py::self == py::self)
        .def("__str__", &ecf::TodayAttr::toString)
        .def("__copy__", pyutil_copy_object<ecf::TodayAttr>)
        .def("time_series",
             &ecf::TodayAttr::time_series,
             py::return_value_policy::reference,
             "Return the Today's time series");

    py::class_<ecf::LateAttr, std::shared_ptr<ecf::LateAttr>>(m, "Late", NodeAttrDoc::late_doc())

        .def(py::init(&LateAttr_make_default))
        .def(py::init(&LateAttr_make))
        .def("submitted",
             &ecf::LateAttr::addSubmitted,
             "submitted(TimeSlot):The time node can stay `submitted`_. Submitted is always relative. If the node "
             "stays\n"
             "submitted longer than the time specified, the `late`_ flag is set\n")
        .def("submitted",
             &ecf::LateAttr::add_submitted,
             "submitted(hour,minute) The time node can stay submitted. Submitted is always relative. If the node "
             "stays\n"
             "submitted longer than the time specified, the late flag is set\n")
        .def("active",
             &ecf::LateAttr::add_active,
             "active(hour,minute): The time the node must become `active`_. If the node is still `queued`_ or "
             "`submitted`_\n"
             "by the time specified, the late flag is set")
        .def("active",
             &ecf::LateAttr::addActive,
             "active(TimeSlot):The time the node must become `active`_. If the node is still `queued`_ or "
             "`submitted`_\n"
             "by the time specified, the late flag is set")
        .def("complete",
             &ecf::LateAttr::add_complete,
             "complete(hour,minute):The time the node must become `complete`_. If relative, time is taken from the "
             "time\n"
             "the node became `active`_, otherwise node must be `complete`_ by the time given")
        .def("complete",
             &ecf::LateAttr::addComplete,
             "complete(TimeSlot): The time the node must become `complete`_. If relative, time is taken from the "
             "time\n"
             "the node became `active`_, otherwise node must be `complete`_ by the time given")
        .def(py::self == py::self)
        .def("__str__", &ecf::LateAttr::toString)
        .def("__copy__", pyutil_copy_object<ecf::LateAttr>)
        .def("submitted",
             &ecf::LateAttr::submitted,
             py::return_value_policy::reference,
             "Return the submitted time as a TimeSlot")
        .def("active",
             &ecf::LateAttr::active,
             py::return_value_policy::reference,
             "Return the active time as a TimeSlot")
        .def("complete",
             &ecf::LateAttr::complete,
             py::return_value_policy::reference,
             "Return the complete time as a TimeSlot")
        .def("complete_is_relative",
             &ecf::LateAttr::complete_is_relative,
             "Returns a boolean where true means that complete is relative")
        .def("is_late", &ecf::LateAttr::isLate, "Return True if late");

    py::class_<ecf::AutoCancelAttr, std::shared_ptr<ecf::AutoCancelAttr>>(
        m, "Autocancel", NodeAttrDoc::autocancel_doc())

        .def(py::init<int, int, bool>()) // hour, minute, relative
        .def(py::init<int>())            // days
        .def(py::init<ecf::TimeSlot, bool>())
        .def(py::self == py::self)
        .def(py::self < py::self)
        .def("__str__", &ecf::AutoCancelAttr::toString)
        .def("__copy__", pyutil_copy_object<ecf::AutoCancelAttr>)
        .def(
            "time", &ecf::AutoCancelAttr::time, py::return_value_policy::reference, "returns cancel time as a TimeSlot")
        .def("relative", &ecf::AutoCancelAttr::relative, "Returns a boolean where true means the time is relative")
        .def("days", &ecf::AutoCancelAttr::days, "Returns a boolean true if time was specified in days");

    py::class_<ecf::AutoArchiveAttr, std::shared_ptr<ecf::AutoArchiveAttr>>(
        m, "Autoarchive", NodeAttrDoc::autoarchive_doc())

        .def(py::init<int, int, bool, bool>())
        // hour, minute, relative, idle (idle: true is queued, aborted, or complete; false is completed)
        .def(py::init<int, bool>())
        // days, idle
        .def(py::init<ecf::TimeSlot, bool, bool>())
        // TimeSlot,relative,idle
        .def(py::self == py::self)
        .def(py::self < py::self)
        .def("__str__", &ecf::AutoArchiveAttr::toString)
        .def("__copy__", pyutil_copy_object<ecf::AutoArchiveAttr>)
        .def("time",
             &ecf::AutoArchiveAttr::time,
             py::return_value_policy::reference,
             "returns archive time as a TimeSlot")
        .def("relative", &ecf::AutoArchiveAttr::relative, "Returns a boolean where true means the time is relative")
        .def("days", &ecf::AutoArchiveAttr::days, "Returns a boolean true if time was specified in days")
        .def("idle",
             &ecf::AutoArchiveAttr::idle,
             "Returns a boolean true if archiving when idle, i.e queued,aborted,complete and time elapsed");

    py::class_<ecf::AutoRestoreAttr, std::shared_ptr<ecf::AutoRestoreAttr>>(
        m, "Autorestore", NodeAttrDoc::autorestore_doc())

        .def(py::init(&AutoRestoreAttr_make))
        .def(py::self == py::self)
        .def("__str__", &ecf::AutoRestoreAttr::toString)
        .def("__copy__", pyutil_copy_object<ecf::AutoRestoreAttr>)
        .def("nodes_to_restore",
             &ecf::AutoRestoreAttr::nodes_to_restore,
             py::return_value_policy::reference,
             "returns a list of nodes to be restored");

    py::class_<RepeatDate>(m, "RepeatDate", NodeAttrDoc::repeat_date_doc())

        .def(py::init<std::string, int, int, int>(),
             py::arg("name"),
             py::arg("start"),
             py::arg("end"),
             py::arg("delta") = 1)
        .def(py::self == py::self)
        .def("__str__", &RepeatDate::toString)
        .def("__copy__", pyutil_copy_object<RepeatDate>)
        .def("name", &RepeatDate::name, py::return_value_policy::reference, "Return the name of the repeat.")
        .def("start", &RepeatDate::start, "Return the start date as an integer in yyyymmdd format")
        .def("end", &RepeatDate::end, "Return the end date as an integer in yyyymmdd format")
        .def("step",
             &RepeatDate::step,
             "Return the step increment. This is used to update the repeat, until end date is reached");

    py::class_<RepeatDateTime>(m, "RepeatDateTime", NodeAttrDoc::repeat_datetime_doc())

        .def(py::init([](const std::string& name, int start, int end, int delta) {
                 return RepeatDateTime(name, std::to_string(start), std::to_string(end), std::to_string(delta));
             }),
             py::arg("name"),
             py::arg("start"),
             py::arg("end"),
             py::arg("delta") = 1)
        .def(py::init([](const std::string& name,
                         const std::string& start,
                         const std::string& end,
                         const std::string& delta) { return RepeatDateTime(name, start, end, delta); }),
             py::arg("name"),
             py::arg("start"),
             py::arg("end"),
             py::arg("delta") = "24:00:00")
        .def(py::self == py::self)
        .def("__str__", &RepeatDateTime::toString)
        .def("__copy__", pyutil_copy_object<RepeatDateTime>)
        .def("name", &RepeatDateTime::name, py::return_value_policy::reference, "Return the name of the repeat.")
        .def(
            "start", &RepeatDateTime::start, "Return the start date as an integer (i.e. seconds since 19700101T000000)")
        .def("end", &RepeatDateTime::end, "Return the end date as an integer (i.e. seconds since 19700101T000000)")
        .def("step",
             &RepeatDateTime::step,
             "Return the step increment (in seconds). This is used to update the repeat, until end instant is "
             "reached");

    py::class_<RepeatDateList>(m, "RepeatDateList", NodeAttrDoc::repeat_date_list_doc())

        .def(py::init(&RepeatDateList_make))
        .def(py::self == py::self)
        .def("__str__", &RepeatDateList::toString)
        .def("__copy__", pyutil_copy_object<RepeatDateList>)
        .def("name", &RepeatDateList::name, py::return_value_policy::reference, "Return the name of the repeat.")
        .def("start", &RepeatDateList::start, "Return the start date as an integer in yyyymmdd format")
        .def("end", &RepeatDateList::end, "Return the end date as an integer in yyyymmdd format");

    py::class_<RepeatInteger>(m, "RepeatInteger", NodeAttrDoc::repeat_integer_doc())

        .def(py::init<std::string, int, int, int>(),
             py::arg("name"),
             py::arg("start"),
             py::arg("end"),
             py::arg("delta") = 1)
        .def(py::self == py::self)
        .def("__str__", &RepeatInteger::toString)
        .def("__copy__", pyutil_copy_object<RepeatInteger>)
        .def("name", &RepeatInteger::name, py::return_value_policy::reference, "Return the name of the repeat.")
        .def("start", &RepeatInteger::start)
        .def("end", &RepeatInteger::end)
        .def("step", &RepeatInteger::step);

    // Create as shared because: we want to pass a Python list as part of the constructor,
    // and the only way make_constructor works is with a pointer.
    py::class_<RepeatEnumerated, std::shared_ptr<RepeatEnumerated>>(
        m, "RepeatEnumerated", NodeAttrDoc::repeat_enumerated_doc())

        .def(py::init(&RepeatEnumerated_make))
        .def(py::self == py::self)
        .def("__str__", &RepeatEnumerated::toString)
        .def("__copy__", pyutil_copy_object<RepeatEnumerated>)
        .def("name", &RepeatEnumerated::name, py::return_value_policy::reference, "Return the name of the `repeat`_.")
        .def("start", &RepeatEnumerated::start)
        .def("end", &RepeatEnumerated::end)
        .def("step", &RepeatEnumerated::step);

    py::class_<RepeatString, std::shared_ptr<RepeatString>>(m, "RepeatString", NodeAttrDoc::repeat_string_doc())

        .def(py::init(&RepeatString_make))
        .def(py::self == py::self)
        .def("__str__", &RepeatString::toString)
        .def("__copy__", pyutil_copy_object<RepeatString>)
        .def("name", &RepeatString::name, py::return_value_policy::reference, "Return the name of the `repeat`_.")
        .def("start", &RepeatString::start)
        .def("end", &RepeatString::end)
        .def("step", &RepeatString::step);

    py::class_<RepeatDay>(m, "RepeatDay", NodeAttrDoc::repeat_day_doc())

        .def(py::init<int>(), py::arg("day") = 1)
        .def(py::self == py::self)
        .def("__str__", &RepeatDay::toString)
        .def("__copy__", pyutil_copy_object<RepeatDay>);

    py::class_<Repeat>(m, "Repeat", NodeAttrDoc::repeat_doc())

        .def(py::init<int>())
        .def(py::self == py::self)
        .def("__str__", &Repeat::toString)
        .def("__copy__", pyutil_copy_object<Repeat>)
        .def("empty", &Repeat::empty, "Return true if the repeat is empty.")
        .def("name",
             &Repeat::name,
             py::return_value_policy::reference,
             "The `repeat`_ name, can be referenced in `trigger`_ expressions")
        .def("start", &Repeat::start, "The start value of the repeat, as an integer")
        .def("end", &Repeat::end, "The last value of the repeat, as an integer")
        .def("step", &Repeat::step, "The increment for the repeat, as an integer")
        .def("value", &Repeat::last_valid_value, "The current value of the repeat as an integer");

    void (ecf::CronAttr::*CronAttr_set_time_series_timeseries)(const ecf::TimeSeries&) = &ecf::CronAttr::addTimeSeries;
    void (ecf::CronAttr::*CronAttr_set_time_series_timeslot_timeslot_timeslot)(
        const ecf::TimeSlot& s, const ecf::TimeSlot& f, const ecf::TimeSlot& i) = &ecf::CronAttr::addTimeSeries;

    py::class_<ecf::CronAttr, std::shared_ptr<ecf::CronAttr>>(m, "Cron", NodeAttrDoc::cron_doc())

        .def(py::init(&CronAttr_make))
        .def(py::init(&CronAttr_make_string_kwargs))
        .def(py::init(&CronAttr_make_timeseries_kwargs))
        .def(py::init(&CronAttr_make_timeseries))
        .def(py::self == py::self)
        .def("__str__", &ecf::CronAttr::toString)
        .def("__copy__", pyutil_copy_object<ecf::CronAttr>)
        .def("set_week_days",
             &CronAttr_set_week_days,
             "Specifies days of week. Expects a list of integers, with integer range 0==Sun to 6==Sat")
        .def("set_last_week_days_of_the_month",
             &CronAttr_set_last_week_days_of_month,
             "Specifies last week days of the month. Expects a list of integers, with integer range 0==Sun to "
             "6==Sat")
        .def("set_days_of_month",
             &CronAttr_set_days_of_month,
             "Specifies days of the month. Expects a list of integers with integer range 1-31")
        .def("set_last_day_of_the_month", &CronAttr_set_last_day_of_the_month, "Set cron for the last day of the month")
        .def(
            "set_months", &CronAttr_set_months, "Specifies months. Expects a list of integers, with integer range 1-12")
        .def("set_time_series",
             &ecf::CronAttr::add_time_series,
             py::arg("hour"),
             py::arg("minute"),
             py::arg("relative") = false,
             "time_series(hour(int),minute(int),relative to suite start(bool=false)), Add a time slot")
        .def("set_time_series", CronAttr_set_time_series_timeseries, "Add a time series. This will never complete")
        .def("set_time_series",
             CronAttr_set_time_series_timeslot_timeslot_timeslot,
             "Add a time series. This will never complete")
        .def("set_time_series", &CronAttr_set_time_series, "Add a time series. This will never complete")
        .def("time", &ecf::CronAttr::time, py::return_value_policy::reference, "return cron time as a TimeSeries")
        .def("last_day_of_the_month",
             &ecf::CronAttr::last_day_of_the_month,
             "Return true if last day of month is enabled")
        .def("week_days", &ecf::CronAttr::week_days, "returns a integer list of week days")
        .def("last_week_days_of_the_month",
             &ecf::CronAttr::last_week_days_of_month,
             "returns a integer list of last week days of the month")
        .def("days_of_month", &ecf::CronAttr::days_of_month, "returns a integer list of days of the month")
        .def("months", &ecf::CronAttr::months, "returns a integer list of months of the year");

    py::class_<VerifyAttr>(m, "Verify")

        .def(py::init<NState::State, int>()) // state, expected
        .def(py::self == py::self)
        .def("__str__", &VerifyAttr::toString)
        .def("__copy__", pyutil_copy_object<VerifyAttr>);

    py::class_<ClockAttr, std::shared_ptr<ClockAttr>>(m, "Clock", NodeAttrDoc::clock_doc())

        .def(py::init<int, int, int, bool>(),
             py::arg("day"),
             py::arg("month"),
             py::arg("year"),
             py::arg("hybrid") = false) // day, month, year, hybrid
        .def(py::init<bool>(), py::arg("hybrid") = false)
        .def(py::self == py::self)
        .def("__str__", &ClockAttr::toString)
        .def("__copy__", pyutil_copy_object<ClockAttr>)
        .def("set_gain_in_seconds", &ClockAttr::set_gain_in_seconds, "Set the gain in seconds")
        .def("set_gain", &ClockAttr::set_gain, "Set the gain in hours and minutes")
        .def("day", &ClockAttr::day, "Returns the day as an integer, range 1-31")
        .def("month", &ClockAttr::month, "Returns the month as an integer, range 1-12")
        .def("year", &ClockAttr::year, "Returns the year as an integer, > 1400")
        .def("gain", &ClockAttr::gain, "Returns the gain as an long. This represents seconds")
        .def("positive_gain",
             &ClockAttr::positive_gain,
             "Returns a boolean, where true means that the gain is positive");

    py::class_<ecf::AvisoAttr>(m, "AvisoAttr", NodeAttrDoc::aviso_doc())

        .def(py::init(&AvisoAttr_make),
             py::arg("name"),
             py::arg("listener"),
             py::arg("url")     = "%ECF_AVISO_URL%",
             py::arg("schema")  = "%ECF_AVISO_SCHEMA%",
             py::arg("polling") = "%ECF_AVISO_POLLING%",
             py::arg("auth")    = "%ECF_AVISO_AUTH%")
        .def(py::self == py::self)
        .def("__str__", &AvisoAttr_str)
        .def("__copy__", pyutil_copy_object<ecf::AvisoAttr>)
        .def("name",
             &ecf::AvisoAttr::name,
             py::return_value_policy::reference,
             "Returns the name of the Aviso attribute")
        .def("listener",
             &ecf::AvisoAttr::listener,
             py::return_value_policy::reference,
             "Returns the Aviso listener configuration")
        .def("url",
             &ecf::AvisoAttr::url,
             py::return_value_policy::reference,
             "Returns the URL used to contact the Aviso server")
        .def("schema",
             &ecf::AvisoAttr::schema,
             py::return_value_policy::reference,
             "Returns the path to the schema used to contact the Aviso server")
        .def("polling", &ecf::AvisoAttr::polling, "Returns polling interval used to contact the Aviso server")
        .def("auth",
             &ecf::AvisoAttr::auth,
             py::return_value_policy::reference,
             "Returns the path to Authentication credentials used to contact the Aviso server");

    py::class_<ecf::MirrorAttr>(m, "MirrorAttr", NodeAttrDoc::mirror_doc())

        .def(py::init(&MirrorAttr_make),
             py::arg("name"),
             py::arg("remote_path") = "%ECF_MIRROR_REMOTE_PATH%",
             py::arg("remote_host") = "%ECF_MIRROR_REMOTE_HOST%",
             py::arg("remote_port") = "%ECF_MIRROR_REMOTE_PORT%",
             py::arg("polling")     = "%ECF_MIRROR_REMOTE_POLLING%",
             py::arg("ssl")         = false,
             py::arg("auth")        = "%ECF_MIRROR_REMOTE_AUTH%")
        .def(py::self == py::self)
        .def("__str__", &MirrorAttr_str)
        .def("__copy__", pyutil_copy_object<ecf::MirrorAttr>)
        .def("name",
             &ecf::MirrorAttr::name,
             py::return_value_policy::reference,
             "Returns the name of the Mirror attribute")
        .def("remote_path",
             &ecf::MirrorAttr::remote_path,
             py::return_value_policy::reference,
             "Returns the path on the remote ecFlow server")
        .def("remote_host",
             &ecf::MirrorAttr::remote_host,
             py::return_value_policy::reference,
             "Returns the host of the remote ecFlow server")
        .def("remote_port",
             &ecf::MirrorAttr::remote_port,
             py::return_value_policy::reference,
             "Returns the port of the remote ecFlow server")
        .def("ssl", &ecf::MirrorAttr::ssl, "Returns a boolean, where true means that SSL is enabled")
        .def("polling",
             &ecf::MirrorAttr::polling,
             "Returns the polling interval used to contact the remove ecFlow server")
        .def("auth",
             &ecf::MirrorAttr::auth,
             py::return_value_policy::reference,
             "Returns the path to Authentication credentials used to contact the remote ecFlow server");
}
