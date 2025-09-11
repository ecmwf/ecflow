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
#include "ecflow/python/NodeAttrDoc.hpp"
#include "ecflow/python/PythonBinding.hpp"
#include "ecflow/python/PythonUtil.hpp"
#include "ecflow/python/Trigger.hpp"

// See: http://wiki.python.org/moin/boost.python/HowTo#boost.function_objects
///////////////////////////////////////////////////////////////////////////////////////////////////
py::object late_raw_constructor(py::tuple args, py::dict kw) {
    // cout << "late_raw_constructor len(args):" << len(args) << endl;
    //  args[0] is Late(i.e self)
    if (len(args) > 1) {
        throw std::runtime_error("late_raw_constructor: Late only expects keyword arguments, ie. "
                                 "Late(submitted='00:20',active='15:00',complete='+30:00')");
    }
    return args[0].attr("__init__")(kw); // calls -> late_init(dict kw)
}

static void extract_late_keyword_arguments(std::shared_ptr<ecf::LateAttr> late, py::dict& dict) {
    py::list keys        = dict.keys();
    const int no_of_keys = len(keys);
    for (int i = 0; i < no_of_keys; ++i) {
        if (py::extract<std::string>(keys[i]).check()) {
            std::string first = py::extract<std::string>(keys[i]);
            if (py::extract<std::string>(dict[keys[i]]).check()) {
                std::string second = py::extract<std::string>(dict[keys[i]]);
                int hour           = 0;
                int min            = 0;
                bool relative      = ecf::TimeSeries::getTime(second, hour, min);
                if (first == "submitted") {
                    late->add_submitted(hour, min);
                }
                else if (first == "active") {
                    late->add_active(hour, min);
                }
                else if (first == "complete") {
                    late->add_complete(hour, min, relative);
                }
                else {
                    throw std::runtime_error(
                        "extract_late_keyword_arguments: keyword arguments, expected [submitted | active | complete]");
                }
            }
            else {
                throw std::runtime_error("extract_late_keyword_arguments: expected keyword arguments to be a string, "
                                         "ie Late(submitted='00:20',active='15:00',complete='+30:00')");
            }
        }
    }
}

static std::shared_ptr<ecf::LateAttr> late_init(py::dict& dict) {
    std::shared_ptr<ecf::LateAttr> late = std::make_shared<ecf::LateAttr>();
    extract_late_keyword_arguments(late, dict);
    return late;
}

static std::shared_ptr<ecf::LateAttr> late_create() {
    return std::make_shared<ecf::LateAttr>();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
py::object cron_raw_constructor(py::tuple args, py::dict kw) {
    // cout << "cron_raw_constructor len(args):" << len(args) << endl;
    //  args[0] is Cron(i.e self) args[1] is string name
    for (int i = 1; i < len(args); ++i) {
        if (py::extract<std::string>(args[i]).check()) {
            std::string time_series = py::extract<std::string>(args[i]);
            if (time_series.empty()) {
                throw std::runtime_error("cron_raw_constructor: Empty string, please pass a valid time, i.e '12:30'");
            }
            return args[0].attr("__init__")(time_series, kw); // calls -> init(const std::string& ts, dict kw)
        }
        if (py::extract<ecf::TimeSeries>(args[i]).check()) {
            ecf::TimeSeries time_series = py::extract<ecf::TimeSeries>(args[i]);
            return args[0].attr("__init__")(time_series, kw); // calls -> init(const ecf::TimeSeries& ts, dict kw)
        }
        else {
            throw std::runtime_error("cron_raw_constructor: expects string | TimeSeries and keyword arguments");
        }
    }
    throw std::runtime_error("cron_raw_constructor: expects string | TimeSeries and keyword arguments !!");
    return py::object();
}

static void extract_cron_keyword_arguments(std::shared_ptr<ecf::CronAttr> cron, py::dict& dict) {
    py::list keys        = dict.keys();
    const int no_of_keys = len(keys);
    for (int i = 0; i < no_of_keys; ++i) {

        if (py::extract<std::string>(keys[i]).check()) {
            std::string first = py::extract<std::string>(keys[i]);
            if (py::extract<py::list>(dict[keys[i]]).check()) {

                py::list second = py::extract<py::list>(dict[keys[i]]);
                std::vector<int> int_vec;
                pyutil_list_to_int_vec(second, int_vec);

                //  expected keywords are: days_of_week,last_week_days_ofThe_month, days_of_month, months
                if (first == "days_of_week") {
                    cron->addWeekDays(int_vec);
                }
                else if (first == "days_of_month") {
                    cron->addDaysOfMonth(int_vec);
                }
                else if (first == "months") {
                    cron->addMonths(int_vec);
                }
                else if (first == "last_week_days_of_the_month") {
                    cron->add_last_week_days_of_month(int_vec);
                }
                else {
                    throw std::runtime_error(
                        "extract_cron_keyword_arguments: keyword arguments, expected [days_of_week | "
                        "last_week_days_of_the_month | days_of_month | months | last_day_of_the_month");
                }
            }
            else if (py::extract<bool>(dict[keys[i]]).check()) {
                if (first == "last_day_of_the_month") {
                    cron->add_last_day_of_month();
                }
                else {
                    throw std::runtime_error(
                        "extract_cron_keyword_arguments: keyword arguments, expected [days_of_week | "
                        "last_week_days_of_the_month | days_of_month | months | last_day_of_the_month]");
                }
            }
            else {
                throw std::runtime_error("extract_cron_keyword_arguments: keyword arguments to be a list");
            }
        }
    }
}

static std::shared_ptr<ecf::CronAttr> cron_init(const std::string& ts, py::dict& dict) {
    std::shared_ptr<ecf::CronAttr> cron = std::make_shared<ecf::CronAttr>(ts);
    extract_cron_keyword_arguments(cron, dict);
    return cron;
}

static std::shared_ptr<ecf::CronAttr> cron_init1(const ecf::TimeSeries& ts, py::dict& dict) {
    std::shared_ptr<ecf::CronAttr> cron = std::make_shared<ecf::CronAttr>(ts);
    extract_cron_keyword_arguments(cron, dict);
    return cron;
}

static std::shared_ptr<ecf::CronAttr> cron_create() {
    return std::make_shared<ecf::CronAttr>();
}
static std::shared_ptr<ecf::CronAttr> cron_create2(const ecf::TimeSeries& ts) {
    return std::make_shared<ecf::CronAttr>(ts);
}
///////////////////////////////////////////////////////////////////////////////////////////////////

void add_time_series_3(ecf::CronAttr* self, const std::string& ts) {
    self->addTimeSeries(ecf::TimeSeries::create(ts));
}

void set_week_days(ecf::CronAttr* cron, const py::list& list) {
    std::vector<int> int_vec;
    pyutil_list_to_int_vec(list, int_vec);
    cron->addWeekDays(int_vec);
}
void set_last_week_days_of_month(ecf::CronAttr* cron, const py::list& list) {
    std::vector<int> int_vec;
    pyutil_list_to_int_vec(list, int_vec);
    cron->add_last_week_days_of_month(int_vec);
}

void set_days_of_month(ecf::CronAttr* cron, const py::list& list) {
    std::vector<int> int_vec;
    pyutil_list_to_int_vec(list, int_vec);
    cron->addDaysOfMonth(int_vec);
}
void set_last_day_of_the_month(ecf::CronAttr* cron) {
    cron->add_last_day_of_month();
}

void set_months(ecf::CronAttr* cron, const py::list& list) {
    std::vector<int> int_vec;
    pyutil_list_to_int_vec(list, int_vec);
    cron->addMonths(int_vec);
}

// Create as shared because: we want to pass a Python list as part of the constructor,
// *AND* the only way make_constructor works is with a pointer.
// The Node::add function seem to cope with this, some boost python magic,must do a conversion
// from shared_ptr to pass by reference
static std::shared_ptr<RepeatEnumerated> create_RepeatEnumerated(const std::string& name, const py::list& list) {
    std::vector<std::string> vec;
    pyutil_list_to_str_vec(list, vec);
    return std::make_shared<RepeatEnumerated>(name, vec);
}
static std::shared_ptr<RepeatDateList> create_RepeatDateList(const std::string& name, const py::list& list) {
    std::vector<int> vec;
    pyutil_list_to_int_vec(list, vec);
    return std::make_shared<RepeatDateList>(name, vec);
}
static std::shared_ptr<RepeatString> create_RepeatString(const std::string& name, const py::list& list) {
    std::vector<std::string> vec;
    pyutil_list_to_str_vec(list, vec);
    return std::make_shared<RepeatString>(name, vec);
}
static std::shared_ptr<ecf::AutoRestoreAttr> create_AutoRestoreAttr(const py::list& list) {
    std::vector<std::string> vec;
    pyutil_list_to_str_vec(list, vec);
    return std::make_shared<ecf::AutoRestoreAttr>(vec);
}

static std::shared_ptr<QueueAttr> create_queue(const std::string& name, const py::list& list) {
    std::vector<std::string> vec;
    pyutil_list_to_str_vec(list, vec);
    return std::make_shared<QueueAttr>(name, vec);
}

static std::shared_ptr<GenericAttr> create_generic(const std::string& name, const py::list& list) {
    std::vector<std::string> vec;
    pyutil_list_to_str_vec(list, vec);
    return std::make_shared<GenericAttr>(name, vec);
}

static std::shared_ptr<ZombieAttr>
create_ZombieAttr(ecf::Child::ZombieType zt, const py::list& list, ecf::ZombieCtrlAction uc, int life_time_in_server) {
    std::vector<ecf::Child::CmdType> vec;
    int the_list_size = len(list);
    vec.reserve(the_list_size);
    for (int i = 0; i < the_list_size; ++i) {
        vec.push_back(py::extract<ecf::Child::CmdType>(list[i]));
    }
    return std::make_shared<ZombieAttr>(zt, vec, uc, life_time_in_server);
}

static std::shared_ptr<ZombieAttr>
create_ZombieAttr1(ecf::Child::ZombieType zt, const py::list& list, ecf::ZombieCtrlAction uc) {
    std::vector<ecf::Child::CmdType> vec;
    int the_list_size = len(list);
    vec.reserve(the_list_size);
    for (int i = 0; i < the_list_size; ++i) {
        vec.push_back(py::extract<ecf::Child::CmdType>(list[i]));
    }
    return std::make_shared<ZombieAttr>(zt, vec, uc);
}

static py::list wrap_set_of_strings(Limit* limit) {
    py::list list;
    const std::set<std::string>& paths = limit->paths();
    for (std::string path : paths) {
        list.append(path);
    }
    return list;
}

static job_creation_ctrl_ptr makeJobCreationCtrl() {
    return std::make_shared<JobCreationCtrl>();
}

static std::shared_ptr<ecf::AvisoAttr> aviso_init(const std::string& name,
                                                  const std::string& listener,
                                                  const std::string& url     = ecf::AvisoAttr::default_url,
                                                  const std::string& schema  = ecf::AvisoAttr::default_schema,
                                                  const std::string& polling = ecf::AvisoAttr::default_polling,
                                                  const std::string& auth    = ecf::AvisoAttr::default_auth) {
    auto attr = std::make_shared<ecf::AvisoAttr>(nullptr, name, listener, url, schema, polling, 0, auth, "");
    return attr;
}

static std::shared_ptr<ecf::AvisoAttr> aviso_init_defaults_0(const std::string& name, const std::string& listener) {
    return aviso_init(name, listener);
}

static std::shared_ptr<ecf::AvisoAttr>
aviso_init_defaults_1(const std::string& name, const std::string& listener, const std::string& url) {
    return aviso_init(name, listener, url);
}

static std::shared_ptr<ecf::AvisoAttr> aviso_init_defaults_2(const std::string& name,
                                                             const std::string& listener,
                                                             const std::string& url,
                                                             const std::string& schema) {
    return aviso_init(name, listener, url, schema);
}

static std::shared_ptr<ecf::AvisoAttr> aviso_init_defaults_3(const std::string& name,
                                                             const std::string& listener,
                                                             const std::string& url,
                                                             const std::string& schema,
                                                             const std::string& polling) {
    return aviso_init(name, listener, url, schema, polling);
}

static std::shared_ptr<ecf::MirrorAttr> mirror_init(const std::string& name,
                                                    const std::string& path,
                                                    const std::string& host    = ecf::MirrorAttr::default_remote_host,
                                                    const std::string& port    = ecf::MirrorAttr::default_remote_port,
                                                    const std::string& polling = ecf::MirrorAttr::default_polling,
                                                    bool ssl                   = false,
                                                    const std::string& auth    = ecf::MirrorAttr::default_remote_auth) {
    auto attr = std::make_shared<ecf::MirrorAttr>(nullptr, name, path, host, port, polling, ssl, auth, "");
    return attr;
}

static std::shared_ptr<ecf::MirrorAttr> mirror_init_defaults_0(const std::string& name, const std::string& path) {
    return mirror_init(name, path);
}

static std::shared_ptr<ecf::MirrorAttr>
mirror_init_defaults_1(const std::string& name, const std::string& path, const std::string& host) {
    return mirror_init(name, path, host);
}

static std::shared_ptr<ecf::MirrorAttr> mirror_init_defaults_2(const std::string& name,
                                                               const std::string& path,
                                                               const std::string& host,
                                                               const std::string& port) {
    return mirror_init(name, path, host, port);
}

static std::shared_ptr<ecf::MirrorAttr> mirror_init_defaults_3(const std::string& name,
                                                               const std::string& path,
                                                               const std::string& host,
                                                               const std::string& port,
                                                               const std::string& polling) {
    return mirror_init(name, path, host, port, polling);
}

static std::shared_ptr<ecf::MirrorAttr> mirror_init_defaults_4(const std::string& name,
                                                               const std::string& path,
                                                               const std::string& host,
                                                               const std::string& port,
                                                               const std::string& polling,
                                                               bool ssl) {
    return mirror_init(name, path, host, port, polling, ssl);
}

void export_NodeAttr() {
    // Trigger & Complete thin wrapper over Expression, allows us to call: Task("a").add(Trigger("a=1"),Complete("b=1"))
    py::class_<Trigger, std::shared_ptr<Trigger>>("Trigger", DefsDoc::trigger(), py::init<std::string>())
        .def(py::init<PartExpression>())
        .def(py::init<py::list>())
        .def(py::init<std::string, bool>())
        .def(py::self == py::self)            // __eq__
        .def("__str__", &Trigger::expression) // __str__
        .def("get_expression", &Trigger::expression, "returns the trigger expression as a string");

    py::class_<Complete, std::shared_ptr<Complete>>("Complete", DefsDoc::trigger(), py::init<std::string>())
        .def(py::init<PartExpression>())
        .def(py::init<py::list>())
        .def(py::init<std::string, bool>())
        .def(py::self == py::self)             // __eq__
        .def("__str__", &Complete::expression) // __str__
        .def("get_expression", &Complete::expression, "returns the complete expression as a string");

    // mimic PartExpression(const std::string& expression  )
    // mimic PartExpression(const std::string& expression, bool andExpr /* true means AND , false means OR */ )
    // Use to adding large trigger and complete expressions
    py::class_<PartExpression>("PartExpression", DefsDoc::part_expression_doc(), py::init<std::string>())
        .def(py::init<std::string, bool>())
        .def(py::self == py::self) // __eq__
        .def("get_expression",
             &PartExpression::expression,
             py::return_value_policy<py::copy_const_reference>(),
             "returns the part expression as a string")
        .def("and_expr", &PartExpression::andExpr)
        .def("or_expr", &PartExpression::orExpr);

    py::class_<Expression, std::shared_ptr<Expression>>(
        "Expression", DefsDoc::expression_doc(), py::init<std::string>())
        .def(py::init<PartExpression>())
        .def(py::self == py::self)               // __eq__
        .def("__str__", &Expression::expression) // __str__
        .def("get_expression", &Expression::expression, "returns the complete expression as a string")
        .def("add",
             &Expression::add,
             "Add a part expression, the second and subsequent part expressions must have 'and/or' set")
        .add_property(
            "parts", py::range(&Expression::part_begin, &Expression::part_end), "Returns a list of PartExpression's");

    py::enum_<ecf::Flag::Type>(
        "FlagType",
        "Flags store state associated with a node\n\n"
        "- FORCE_ABORT   - Node* do not run when try_no > ECF_TRIES, and task aborted by user\n"
        "- USER_EDIT     - task\n"
        "- TASK_ABORTED  - task*\n"
        "- EDIT_FAILED   - task*\n"
        "- JOBCMD_FAILED - task*\n"
        "- KILLCMD_FAILED   - task*\n"
        "- STATUSCMD_FAILED - task*\n"
        "- NO_SCRIPT     - task*\n"
        "- KILLED        - task* do not run when try_no > ECF_TRIES, and task killed by user\n"
        "- STATUS        - task* indicates that the status command has been run\n"
        "- LATE          - Node attribute, Task is late, or Defs checkpt takes to long\n"
        "- MESSAGE       - Node\n"
        "- BYRULE        - Node*, set if node is set to complete by complete trigger expression\n"
        "- QUEUELIMIT    - Node\n"
        "- WAIT          - task* \n"
        "- LOCKED        - Server\n"
        "- ZOMBIE        - task*\n"
        "- NO_REQUE      - task\n"
        "- ARCHIVED      - Suite/Family\n"
        "- RESTORED      - Family/Family\n"
        "- THRESHOLD     - task\n"
        "- SIGTERM       - Defs, records that server received a SIGTERM signal\n"
        "- LOG_ERROR     - Error in opening or writing to log file\n"
        "- CHECKPT_ERROR - Error in opening or writing to checkpt file \n"
        "- NOT_SET\n")
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

    py::class_<ecf::Flag>("Flag", "Represents additional state associated with a Node.\n\n", py::init<>())
        .def("__str__", &ecf::Flag::to_string) // __str__
        .def(py::self == py::self)             // __eq__
        .def("is_set", &ecf::Flag::is_set, "Queries if a given flag is set")
        .def("set", &ecf::Flag::set, "Sets the given flag. Used in test only")
        .def("clear", &ecf::Flag::clear, "Clear the given flag. Used in test only")
        .def("reset", &ecf::Flag::reset, "Clears all flags. Used in test only")
        .def("list", &ecf::Flag::list, "Returns the list of all flag types. returns FlagTypeVec. Used in test only")
        .staticmethod("list")
        .def("type_to_string", &ecf::Flag::enum_to_string, "Convert type to a string. Used in test only")
        .staticmethod("type_to_string");

    py::class_<std::vector<ecf::Flag::Type>>("FlagTypeVec", "Hold a list of flag types")
        .def(py::vector_indexing_suite<std::vector<ecf::Flag::Type>, true>());

    py::class_<JobCreationCtrl, boost::noncopyable, job_creation_ctrl_ptr>("JobCreationCtrl", DefsDoc::jobgenctrl_doc())
        .def("__init__", py::make_constructor(makeJobCreationCtrl), DefsDoc::jobgenctrl_doc())
        .def("set_node_path",
             &JobCreationCtrl::set_node_path,
             "The node we want to check job creation for. If no node specified check all tasks")
        .def("set_dir_for_job_creation",
             &JobCreationCtrl::set_dir_for_job_creation,
             "Specify directory, for job creation")
        .def("set_verbose", &JobCreationCtrl::set_verbose, "Output each task as its being checked.")
        .def("get_dir_for_job_creation",
             &JobCreationCtrl::dir_for_job_creation,
             py::return_value_policy<py::copy_const_reference>(),
             "Returns the directory set for job creation")
        .def(
            "generate_temp_dir",
            &JobCreationCtrl::generate_temp_dir,
            "Automatically generated temporary directory for job creation. Directory written to stdout for information")
        .def("get_error_msg",
             &JobCreationCtrl::get_error_msg,
             py::return_value_policy<py::copy_const_reference>(),
             "Returns an error message generated during checking of job creation");

    py::enum_<ecf::Child::ZombieType>("ZombieType", NodeAttrDoc::zombie_type_doc())
        .value("ecf", ecf::Child::ECF)
        .value("ecf_pid", ecf::Child::ECF_PID)
        .value("ecf_pid_passwd", ecf::Child::ECF_PID_PASSWD)
        .value("ecf_passwd", ecf::Child::ECF_PASSWD)
        .value("user", ecf::Child::USER)
        .value("path", ecf::Child::PATH);

    py::enum_<ecf::ZombieCtrlAction>("ZombieUserActionType", NodeAttrDoc::zombie_user_action_type_doc())
        .value("fob", ecf::ZombieCtrlAction::FOB)
        .value("fail", ecf::ZombieCtrlAction::FAIL)
        .value("remove", ecf::ZombieCtrlAction::REMOVE)
        .value("adopt", ecf::ZombieCtrlAction::ADOPT)
        .value("block", ecf::ZombieCtrlAction::BLOCK)
        .value("kill", ecf::ZombieCtrlAction::KILL);

    py::enum_<ecf::Child::CmdType>("ChildCmdType", NodeAttrDoc::child_cmd_type_doc())
        .value("init", ecf::Child::INIT)
        .value("event", ecf::Child::EVENT)
        .value("meter", ecf::Child::METER)
        .value("label", ecf::Child::LABEL)
        .value("wait", ecf::Child::WAIT)
        .value("queue", ecf::Child::QUEUE)
        .value("abort", ecf::Child::ABORT)
        .value("complete", ecf::Child::COMPLETE);

    py::enum_<ecf::Attr::Type>("AttrType",
                               "Sortable attribute type, currently [event | meter | label | limit | variable | all ]")
        .value("event", ecf::Attr::EVENT)
        .value("meter", ecf::Attr::METER)
        .value("label", ecf::Attr::LABEL)
        .value("limit", ecf::Attr::LIMIT)
        .value("variable", ecf::Attr::VARIABLE)
        .value("all", ecf::Attr::ALL);

    py::class_<ZombieAttr>("ZombieAttr", NodeAttrDoc::zombie_doc())
        .def("__init__", make_constructor(&create_ZombieAttr))
        .def("__init__", make_constructor(&create_ZombieAttr1))
        .def("__str__", &ZombieAttr::toString)           // __str__
        .def("__copy__", pyutil_copy_object<ZombieAttr>) // __copy__ uses copy constructor
        .def(py::self == py::self)                       // __eq__
        .def("empty", &ZombieAttr::empty, "Return true if the attribute is empty")
        .def("zombie_type", &ZombieAttr::zombie_type, "Returns the `zombie type`_")
        .def("user_action", &ZombieAttr::action, "The automated action to invoke, when zombies arise")
        .def("zombie_lifetime",
             &ZombieAttr::zombie_lifetime,
             "Returns the lifetime in seconds of `zombie`_ in the server")
        .add_property("child_cmds",
                      py::range(&ZombieAttr::child_begin, &ZombieAttr::child_end),
                      "The list of child commands. If empty action applies to all child cmds");

    py::class_<std::vector<Zombie>>("ZombieVec", "Hold a list of zombies")
        .def(py::vector_indexing_suite<std::vector<Zombie>, true>());

    py::class_<Zombie>("Zombie", "Represent a zombie process stored by the server")
        .def("__str__", &Zombie::to_string)          // __str__
        .def("__copy__", pyutil_copy_object<Zombie>) // __copy__ uses copy constructor
        .def(py::self == py::self)                   // __eq__
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
        .def("attr", &Zombie::attr, py::return_value_policy<py::copy_const_reference>())
        .def("calls", &Zombie::calls)
        .def("jobs_password", &Zombie::jobs_password, py::return_value_policy<py::copy_const_reference>())
        .def("path_to_task", &Zombie::path_to_task, py::return_value_policy<py::copy_const_reference>())
        .def("process_or_remote_id", &Zombie::process_or_remote_id, py::return_value_policy<py::copy_const_reference>())
        .def("user_cmd", &Zombie::user_cmd, py::return_value_policy<py::copy_const_reference>())
        .def("try_no", &Zombie::try_no)
        .def("duration", &Zombie::duration)
        .def("user_action", &Zombie::user_action)
        .def("user_action_str", &Zombie::user_action_str)
        .def("host", &Zombie::host, py::return_value_policy<py::copy_const_reference>())
        .def("allowed_age", &Zombie::allowed_age);

    py::class_<Variable>("Variable", NodeAttrDoc::variable_doc(), py::init<std::string, std::string>())
        .def("__str__", &Variable::toString)           // __str__
        .def("__copy__", pyutil_copy_object<Variable>) // __copy__ uses copy constructor
        .def(py::self < py::self)                      // __lt__
        .def(py::self == py::self)                     // __eq__
        .def("name",
             &Variable::name,
             py::return_value_policy<py::copy_const_reference>(),
             "Return the variable name as string")
        .def("value",
             &Variable::theValue,
             py::return_value_policy<py::copy_const_reference>(),
             "Return the variable value as a string")
        .def("empty",
             &Variable::empty,
             "Return true if the variable is empty. Used when returning a Null variable, from a find");
    // .add_property("value", make_function(&Variable::theValue ,py::return_value_policy<py::copy_const_reference>()),
    //                       &Variable::set_value,  "get/set the value as a property")
    // The following works v = Variable('name','fred')
    //   print v.value
    // But it will then break v.value()

    // We need to return pass a list of Variable as arguments, to retrieve the generated variables
    py::class_<std::vector<Variable>>("VariableList", "Hold a list of Variables")
        .def(py::vector_indexing_suite<std::vector<Variable>>());

    py::class_<Label>("Label", NodeAttrDoc::label_doc(), py::init<std::string, std::string>())
        .def(py::self == py::self)                  // __eq__
        .def("__str__", &Label::toString)           // __str__
        .def("__copy__", pyutil_copy_object<Label>) // __copy__ uses copy constructor
        .def(py::self < py::self)                   // __lt__
        .def("name",
             &Label::name,
             py::return_value_policy<py::copy_const_reference>(),
             "Return the `label`_ name as string")
        .def("value",
             &Label::value,
             py::return_value_policy<py::copy_const_reference>(),
             "Return the original `label`_ value as string")
        .def("new_value",
             &Label::new_value,
             py::return_value_policy<py::copy_const_reference>(),
             "Return the new label value as string")
        .def(
            "empty", &Label::empty, "Return true if the Label is empty. Used when returning a NULL Label, from a find");

    // This will not work, because paths_begin
    //.add_property("node_paths", py::range(&Limit::paths_begin,&Limit::paths_begin),"List of nodes(paths) that have
    // consumed a limit")

    py::class_<Limit, std::shared_ptr<Limit>>("Limit", NodeAttrDoc::limit_doc(), py::init<std::string, int>())
        .def(py::self == py::self)                  // __eq__
        .def("__str__", &Limit::toString)           // __str__
        .def("__copy__", pyutil_copy_object<Limit>) // __copy__ uses copy constructor
        .def(py::self < py::self)                   // __lt__
        .def("name",
             &Limit::name,
             py::return_value_policy<py::copy_const_reference>(),
             "Return the `limit`_ name as string")
        .def("value", &Limit::value, "The `limit`_ token value as an integer")
        .def("limit", &Limit::theLimit, "The max value of the `limit`_ as an integer")
        .def("increment", &Limit::increment, "used for test only")
        .def("decrement", &Limit::decrement, "used for test only")
        .def("node_paths", &wrap_set_of_strings, "List of nodes(paths) that have consumed a limit");
#if ECF_ENABLE_PYTHON_PTR_REGISTER
    py::register_ptr_to_python<std::shared_ptr<Limit>>(); // needed for mac and boost 1.6
#endif

    py::class_<InLimit>("InLimit", NodeAttrDoc::inlimit_doc())
        .def(py::init<std::string>())
        .def(py::init<std::string, std::string>())
        .def(py::init<std::string, std::string, int>())
        .def(py::init<std::string, std::string, int, bool>())
        .def(py::init<std::string, std::string, int, bool, bool>())
        .def(py::self == py::self)                    // __eq__
        .def("__str__", &InLimit::toString)           // __str__
        .def("__copy__", pyutil_copy_object<InLimit>) // __copy__ uses copy constructor
        .def(py::self < py::self)                     // __lt__
        .def("name",
             &InLimit::name,
             py::return_value_policy<py::copy_const_reference>(),
             "Return the `inlimit`_ name as string")
        .def("path_to_node",
             &InLimit::pathToNode,
             py::return_value_policy<py::copy_const_reference>(),
             "Path to the node that holds the limit, can be empty")
        .def("limit_this_node_only",
             &InLimit::limit_this_node_only,
             "Only this node is limited. i.e. typically Family or Suite")
        .def("limit_submission", &InLimit::limit_submission, "Limit submission only")
        .def("tokens", &InLimit::tokens, "The number of token to consume from the Limit");

    py::class_<Event>("Event", NodeAttrDoc::event_doc(), py::init<int, py::optional<std::string>>())
        .def(py::init<int, std::string, bool>()) // here bool is the initial value, by default is false/clear. The value
                                                 // taken by begin/re-queue
        .def(py::init<std::string, bool>())      // here bool is the initial value, by default is false/clear. The value
                                                 // taken by begin/re-queue
        .def(py::init<std::string>())
        .def(py::self == py::self)                  // __eq__
        .def("__str__", &Event::toString)           // __str__
        .def("__copy__", pyutil_copy_object<Event>) // __copy__ uses copy constructor
        .def(py::self < py::self)                   // __lt__
        .def("name",
             &Event::name,
             py::return_value_policy<py::copy_const_reference>(),
             "Return the Events name as string. If number supplied name may be empty.")
        .def("number", &Event::number, "Return events number as a integer. If not specified return max integer value")
        .def("name_or_number", &Event::name_or_number, "Returns name or number as string")
        .def("value", &Event::value, "Return events current value")
        .def("initial_value",
             &Event::initial_value,
             "Return events initial value, This is value taken for begin/re-queue")
        .def(
            "empty", &Event::empty, "Return true if the Event is empty. Used when returning a NULL Event, from a find");

    py::class_<Meter>("Meter", NodeAttrDoc::meter_doc(), py::init<std::string, int, int, py::optional<int>>())
        .def(py::self == py::self)                  // __eq__
        .def("__str__", &Meter::toString)           // __str__
        .def("__copy__", pyutil_copy_object<Meter>) // __copy__ uses copy constructor
        .def(py::self < py::self)                   // __lt__
        .def("name",
             &Meter::name,
             py::return_value_policy<py::copy_const_reference>(),
             "Return the Meters name as string")
        .def("min", &Meter::min, "Return the Meters minimum value")
        .def("max", &Meter::max, "Return the Meters maximum value")
        .def("value", &Meter::value, "Return meters current value")
        .def("color_change", &Meter::colorChange, "returns the color change")
        .def(
            "empty", &Meter::empty, "Return true if the Meter is empty. Used when returning a NULL Meter, from a find");

    py::class_<QueueAttr>("Queue", NodeAttrDoc::queue_doc())
        .def("__init__", make_constructor(&create_queue))
        .def(py::self == py::self)                      // __eq__
        .def("__str__", &QueueAttr::toString)           // __str__
        .def("__copy__", pyutil_copy_object<QueueAttr>) // __copy__ uses copy constructor
        .def(py::self < py::self)                       // __lt__
        .def("name",
             &QueueAttr::name,
             py::return_value_policy<py::copy_const_reference>(),
             "Return the queue name as string")
        .def("value", &QueueAttr::value, "Return the queue current value as string")
        .def("index", &QueueAttr::index, "Return the queue current index as a integer")
        .def("empty",
             &QueueAttr::empty,
             "Return true if the Queue is empty. Used when returning a NULL Queue, from a find");

    py::class_<GenericAttr>(
        "Generic", "A generic attribute, used to add new attributes for the future, without requiring a API change")
        .def("__init__", make_constructor(&create_generic))
        .def(py::self == py::self)                        // __eq__
        .def("__str__", &GenericAttr::to_string)          // __str__
        .def("__copy__", pyutil_copy_object<GenericAttr>) // __copy__ uses copy constructor
        .def(py::self < py::self)                         // __lt__
        .def("name",
             &GenericAttr::name,
             py::return_value_policy<py::copy_const_reference>(),
             "Return the generic name as string")
        .def("empty",
             &GenericAttr::empty,
             "Return true if the Generic is empty. Used when returning a NULL Generic, from a find")
        .add_property("values",
                      py::range(&GenericAttr::values_begin, &GenericAttr::values_end),
                      "The list of values for the generic");

    py::class_<DateAttr>("Date", NodeAttrDoc::date_doc(), py::init<int, int, int>()) // day,month,year
        .def(py::init<std::string>())
        .def(py::self == py::self)                     // __eq__
        .def("__str__", &DateAttr::toString)           // __str__
        .def("__copy__", pyutil_copy_object<DateAttr>) // __copy__ uses copy constructor
        .def(py::self < py::self)                      // __lt__
        .def("day", &DateAttr::day, "Return the day. The range is 0-31, 0 means its wild-carded")
        .def("month", &DateAttr::month, "Return the month. The range is 0-12, 0 means its wild-carded")
        .def("year", &DateAttr::year, "Return the year, 0 means its wild-carded");

    py::enum_<DayAttr::Day_t>("Days", NodeAttrDoc::days_enum_doc())
        .value("sunday", DayAttr::SUNDAY)
        .value("monday", DayAttr::MONDAY)
        .value("tuesday", DayAttr::TUESDAY)
        .value("wednesday", DayAttr::WEDNESDAY)
        .value("thursday", DayAttr::THURSDAY)
        .value("friday", DayAttr::FRIDAY)
        .value("saturday", DayAttr::SATURDAY);

    py::class_<DayAttr>("Day", NodeAttrDoc::day_doc(), py::init<DayAttr::Day_t>())
        .def(py::init<std::string>())                 // constructor
        .def(py::self == py::self)                    // __eq__
        .def("__str__", &DayAttr::toString)           // __str__
        .def("__copy__", pyutil_copy_object<DayAttr>) // __copy__ uses copy constructor
        .def(py::self < py::self)                     // __lt__
        .def("day", &DayAttr::day, "Return the day as enumerator");

    py::class_<ecf::TimeAttr>("Time", NodeAttrDoc::time_doc(), py::init<ecf::TimeSlot, py::optional<bool>>())
        .def(py::init<int, int, py::optional<bool>>()) // hour, minute, relative
        .def(py::init<ecf::TimeSeries>())
        .def(py::init<ecf::TimeSlot, ecf::TimeSlot, ecf::TimeSlot, bool>())
        .def(py::init<std::string>())
        .def(py::self == py::self)                          // __eq__
        .def("__str__", &ecf::TimeAttr::toString)           // __str__
        .def("__copy__", pyutil_copy_object<ecf::TimeAttr>) // __copy__ uses copy constructor
        .def("time_series",
             &ecf::TimeAttr::time_series,
             py::return_value_policy<py::copy_const_reference>(),
             "Return the Time attributes time series");

    py::class_<ecf::TodayAttr>("Today", NodeAttrDoc::today_doc(), py::init<ecf::TimeSlot, py::optional<bool>>())
        .def(py::init<int, int, py::optional<bool>>()) // hour, minute, relative
        .def(py::init<ecf::TimeSeries>())
        .def(py::init<ecf::TimeSlot, ecf::TimeSlot, ecf::TimeSlot, bool>())
        .def(py::init<std::string>())
        .def(py::self == py::self)                           // __eq__
        .def("__str__", &ecf::TodayAttr::toString)           // __str__
        .def("__copy__", pyutil_copy_object<ecf::TodayAttr>) // __copy__ uses copy constructor
        .def("time_series",
             &ecf::TodayAttr::time_series,
             py::return_value_policy<py::copy_const_reference>(),
             "Return the Todays time series");

    py::class_<ecf::LateAttr, std::shared_ptr<ecf::LateAttr>>("Late", NodeAttrDoc::late_doc())
        .def("__init__", py::raw_function(&late_raw_constructor, 1)) // will call -> late_init
        .def("__init__", py::make_constructor(&late_init))
        .def("__init__", py::make_constructor(&late_create))
        .def(
            "submitted",
            &ecf::LateAttr::addSubmitted,
            "submitted(TimeSlot):The time node can stay `submitted`_. Submitted is always relative. If the node stays\n"
            "submitted longer than the time specified, the `late`_ flag is set\n")
        .def(
            "submitted",
            &ecf::LateAttr::add_submitted,
            "submitted(hour,minute) The time node can stay submitted. Submitted is always relative. If the node stays\n"
            "submitted longer than the time specified, the late flag is set\n")
        .def("active",
             &ecf::LateAttr::add_active,
             "active(hour,minute): The time the node must become `active`_. If the node is still `queued`_ or "
             "`submitted`_\n"
             "by the time specified, the late flag is set")
        .def(
            "active",
            &ecf::LateAttr::addActive,
            "active(TimeSlot):The time the node must become `active`_. If the node is still `queued`_ or `submitted`_\n"
            "by the time specified, the late flag is set")
        .def("complete",
             &ecf::LateAttr::add_complete,
             "complete(hour,minute):The time the node must become `complete`_. If relative, time is taken from the "
             "time\n"
             "the node became `active`_, otherwise node must be `complete`_ by the time given")
        .def("complete",
             &ecf::LateAttr::addComplete,
             "complete(TimeSlot): The time the node must become `complete`_. If relative, time is taken from the time\n"
             "the node became `active`_, otherwise node must be `complete`_ by the time given")
        .def(py::self == py::self)                          // __eq__
        .def("__str__", &ecf::LateAttr::toString)           // __str__
        .def("__copy__", pyutil_copy_object<ecf::LateAttr>) // __copy__ uses copy constructor
        .def("submitted",
             &ecf::LateAttr::submitted,
             py::return_value_policy<py::copy_const_reference>(),
             "Return the submitted time as a TimeSlot")
        .def("active",
             &ecf::LateAttr::active,
             py::return_value_policy<py::copy_const_reference>(),
             "Return the active time as a TimeSlot")
        .def("complete",
             &ecf::LateAttr::complete,
             py::return_value_policy<py::copy_const_reference>(),
             "Return the complete time as a TimeSlot")
        .def("complete_is_relative",
             &ecf::LateAttr::complete_is_relative,
             "Returns a boolean where true means that complete is relative")
        .def("is_late", &ecf::LateAttr::isLate, "Return True if late");
#if ECF_ENABLE_PYTHON_PTR_REGISTER
    py::register_ptr_to_python<std::shared_ptr<ecf::LateAttr>>(); // needed for mac and boost 1.6
#endif

    py::class_<ecf::AutoCancelAttr, std::shared_ptr<ecf::AutoCancelAttr>>(
        "Autocancel", NodeAttrDoc::autocancel_doc(), py::init<int, int, bool>() // hour, minute, relative
        )
        .def(py::init<int>()) // days
        .def(py::init<ecf::TimeSlot, bool>())
        .def(py::self == py::self)                                // __eq__
        .def("__str__", &ecf::AutoCancelAttr::toString)           // __str__
        .def("__copy__", pyutil_copy_object<ecf::AutoCancelAttr>) // __copy__ uses copy constructor
        .def(py::self < py::self)                                 // __lt__
        .def("time",
             &ecf::AutoCancelAttr::time,
             py::return_value_policy<py::copy_const_reference>(),
             "returns cancel time as a TimeSlot")
        .def("relative", &ecf::AutoCancelAttr::relative, "Returns a boolean where true means the time is relative")
        .def("days", &ecf::AutoCancelAttr::days, "Returns a boolean true if time was specified in days");
#if ECF_ENABLE_PYTHON_PTR_REGISTER
    py::register_ptr_to_python<std::shared_ptr<ecf::AutoCancelAttr>>(); // needed for mac and boost 1.6
#endif

    py::class_<ecf::AutoArchiveAttr, std::shared_ptr<ecf::AutoArchiveAttr>>(
        "Autoarchive",
        NodeAttrDoc::autoarchive_doc(),
        py::init<int, int, bool, bool>() // hour, minute,relative,idle(true means queued,aborted,complete, false means
                                         // completed only)
        )
        .def(py::init<int, bool>())                                // days, idle
        .def(py::init<ecf::TimeSlot, bool, bool>())                // TimeSlot,relative,idle
        .def(py::self == py::self)                                 // __eq__
        .def("__str__", &ecf::AutoArchiveAttr::toString)           // __str__
        .def("__copy__", pyutil_copy_object<ecf::AutoArchiveAttr>) // __copy__ uses copy constructor
        .def(py::self < py::self)                                  // __lt__
        .def("time",
             &ecf::AutoArchiveAttr::time,
             py::return_value_policy<py::copy_const_reference>(),
             "returns archive time as a TimeSlot")
        .def("relative", &ecf::AutoArchiveAttr::relative, "Returns a boolean where true means the time is relative")
        .def("days", &ecf::AutoArchiveAttr::days, "Returns a boolean true if time was specified in days")
        .def("idle",
             &ecf::AutoArchiveAttr::idle,
             "Returns a boolean true if archiving when idle, i.e queued,aborted,complete and time elapsed");
#if ECF_ENABLE_PYTHON_PTR_REGISTER
    py::register_ptr_to_python<std::shared_ptr<ecf::AutoArchiveAttr>>(); // needed for mac and boost 1.6
#endif

    py::class_<ecf::AutoRestoreAttr, std::shared_ptr<ecf::AutoRestoreAttr>>("Autorestore",
                                                                            NodeAttrDoc::autorestore_doc())
        .def("__init__", make_constructor(&create_AutoRestoreAttr))
        .def(py::self == py::self)                                 // __eq__
        .def("__str__", &ecf::AutoRestoreAttr::toString)           // __str__
        .def("__copy__", pyutil_copy_object<ecf::AutoRestoreAttr>) // __copy__ uses copy constructor
        .def("nodes_to_restore",
             &ecf::AutoRestoreAttr::nodes_to_restore,
             py::return_value_policy<py::copy_const_reference>(),
             "returns a list of nodes to be restored");
#if ECF_ENABLE_PYTHON_PTR_REGISTER
    py::register_ptr_to_python<std::shared_ptr<AutoRestoreAttr>>(); // needed for mac and boost 1.6
#endif

    py::class_<RepeatDate>("RepeatDate",
                           NodeAttrDoc::repeat_date_doc(),
                           py::init<std::string, int, int, py::optional<int>>()) // name, start, end , delta
        .def(py::self == py::self)                                               // __eq__
        .def("__str__", &RepeatDate::toString)                                   // __str__
        .def("__copy__", pyutil_copy_object<RepeatDate>)                         // __copy__ uses copy constructor
        .def("name",
             &RepeatDate::name,
             py::return_value_policy<py::copy_const_reference>(),
             "Return the name of the repeat.")
        .def("start", &RepeatDate::start, "Return the start date as an integer in yyyymmdd format")
        .def("end", &RepeatDate::end, "Return the end date as an integer in yyyymmdd format")
        .def("step",
             &RepeatDate::step,
             "Return the step increment. This is used to update the repeat, until end date is reached");

    py::class_<RepeatDateTime>(
        "RepeatDateTime",
        NodeAttrDoc::repeat_datetime_doc(),
        py::init<std::string, std::string, std::string, py::optional<std::string>>()) // name, start, end , delta
        .def(py::self == py::self)                                                    // __eq__
        .def("__str__", &RepeatDateTime::toString)                                    // __str__
        .def("__copy__", pyutil_copy_object<RepeatDateTime>)                          // __copy__ uses copy constructor
        .def("name",
             &RepeatDateTime::name,
             py::return_value_policy<py::copy_const_reference>(),
             "Return the name of the repeat.")
        .def(
            "start", &RepeatDateTime::start, "Return the start date as an integer (i.e. seconds since 19700101T000000)")
        .def("end", &RepeatDateTime::end, "Return the end date as an integer (i.e. seconds since 19700101T000000)")
        .def("step",
             &RepeatDateTime::step,
             "Return the step increment (in seconds). This is used to update the repeat, until end instant is reached");

    py::class_<RepeatDateList>("RepeatDateList", NodeAttrDoc::repeat_date_list_doc())
        .def("__init__", make_constructor(&create_RepeatDateList))
        .def(py::self == py::self)                           // __eq__
        .def("__str__", &RepeatDateList::toString)           // __str__
        .def("__copy__", pyutil_copy_object<RepeatDateList>) // __copy__ uses copy constructor
        .def("name",
             &RepeatDateList::name,
             py::return_value_policy<py::copy_const_reference>(),
             "Return the name of the repeat.")
        .def("start", &RepeatDateList::start, "Return the start date as an integer in yyyymmdd format")
        .def("end", &RepeatDateList::end, "Return the end date as an integer in yyyymmdd format");

    py::class_<RepeatInteger>("RepeatInteger",
                              NodeAttrDoc::repeat_integer_doc(),
                              py::init<std::string, int, int, py::optional<int>>()) // name, start, end , delta = 1
        .def(py::self == py::self)                                                  // __eq__
        .def("__str__", &RepeatInteger::toString)                                   // __str__
        .def("__copy__", pyutil_copy_object<RepeatInteger>)                         // __copy__ uses copy constructor
        .def("name",
             &RepeatInteger::name,
             py::return_value_policy<py::copy_const_reference>(),
             "Return the name of the repeat.")
        .def("start", &RepeatInteger::start)
        .def("end", &RepeatInteger::end)
        .def("step", &RepeatInteger::step);

    // Create as shared because: we want to pass a Python list as part of the constructor,
    // and the only way make_constructor works is with a pointer.
    py::class_<RepeatEnumerated, std::shared_ptr<RepeatEnumerated>>("RepeatEnumerated",
                                                                    NodeAttrDoc::repeat_enumerated_doc())
        .def("__init__", make_constructor(&create_RepeatEnumerated))
        .def(py::self == py::self)                             // __eq__
        .def("__str__", &RepeatEnumerated::toString)           // __str__
        .def("__copy__", pyutil_copy_object<RepeatEnumerated>) // __copy__ uses copy constructor
        .def("name",
             &RepeatEnumerated::name,
             py::return_value_policy<py::copy_const_reference>(),
             "Return the name of the `repeat`_.")
        .def("start", &RepeatEnumerated::start)
        .def("end", &RepeatEnumerated::end)
        .def("step", &RepeatEnumerated::step);
#if ECF_ENABLE_PYTHON_PTR_REGISTER
    py::register_ptr_to_python<std::shared_ptr<RepeatEnumerated>>(); // needed for mac and boost 1.6
#endif

    py::class_<RepeatString, std::shared_ptr<RepeatString>>("RepeatString", NodeAttrDoc::repeat_string_doc())
        .def("__init__", make_constructor(&create_RepeatString))
        .def(py::self == py::self)                         // __eq__
        .def("__str__", &RepeatString::toString)           // __str__
        .def("__copy__", pyutil_copy_object<RepeatString>) // __copy__ uses copy constructor
        .def("name",
             &RepeatString::name,
             py::return_value_policy<py::copy_const_reference>(),
             "Return the name of the `repeat`_.")
        .def("start", &RepeatString::start)
        .def("end", &RepeatString::end)
        .def("step", &RepeatString::step);
#if ECF_ENABLE_PYTHON_PTR_REGISTER
    py::register_ptr_to_python<std::shared_ptr<RepeatString>>(); // needed for mac and boost 1.6
#endif

    py::class_<RepeatDay>("RepeatDay", NodeAttrDoc::repeat_day_doc(), py::init<py::optional<int>>())
        .def(py::self == py::self)                      // __eq__
        .def("__str__", &RepeatDay::toString)           // __str__
        .def("__copy__", pyutil_copy_object<RepeatDay>) // __copy__ uses copy constructor
        ;

    py::class_<Repeat>("Repeat", NodeAttrDoc::repeat_doc(), py::init<int>())
        .def(py::self == py::self)                   // __eq__
        .def("__str__", &Repeat::toString)           // __str__
        .def("__copy__", pyutil_copy_object<Repeat>) // __copy__ uses copy constructor
        .def("empty", &Repeat::empty, "Return true if the repeat is empty.")
        .def("name",
             &Repeat::name,
             py::return_value_policy<py::copy_const_reference>(),
             "The `repeat`_ name, can be referenced in `trigger`_ expressions")
        .def("start", &Repeat::start, "The start value of the repeat, as an integer")
        .def("end", &Repeat::end, "The last value of the repeat, as an integer")
        .def("step", &Repeat::step, "The increment for the repeat, as an integer")
        .def("value", &Repeat::last_valid_value, "The current value of the repeat as an integer");

    void (ecf::CronAttr::*add_time_series)(const ecf::TimeSeries&) = &ecf::CronAttr::addTimeSeries;
    void (ecf::CronAttr::*add_time_series_2)(const ecf::TimeSlot& s, const ecf::TimeSlot& f, const ecf::TimeSlot& i) =
        &ecf::CronAttr::addTimeSeries;
    py::class_<ecf::CronAttr, std::shared_ptr<ecf::CronAttr>>("Cron", NodeAttrDoc::cron_doc())
        .def("__init__", py::raw_function(&cron_raw_constructor, 1)) // will call -> cron_init or cron_init1
        .def("__init__", py::make_constructor(&cron_init))
        .def("__init__", py::make_constructor(&cron_init1))
        .def("__init__", py::make_constructor(&cron_create2))
        .def("__init__", py::make_constructor(&cron_create))
        .def(py::self == py::self)                          // __eq__
        .def("__str__", &ecf::CronAttr::toString)           // __str__
        .def("__copy__", pyutil_copy_object<ecf::CronAttr>) // __copy__ uses copy constructor
        .def("set_week_days",
             &set_week_days,
             "Specifies days of week. Expects a list of integers, with integer range 0==Sun to 6==Sat")
        .def("set_last_week_days_of_the_month",
             &set_last_week_days_of_month,
             "Specifies last week days of the month. Expects a list of integers, with integer range 0==Sun to 6==Sat")
        .def("set_days_of_month",
             &set_days_of_month,
             "Specifies days of the month. Expects a list of integers with integer range 1-31")
        .def("set_last_day_of_the_month", &set_last_day_of_the_month, "Set cron for the last day of the month")
        .def("set_months", &set_months, "Specifies months. Expects a list of integers, with integer range 1-12")
        .def("set_time_series",
             &ecf::CronAttr::add_time_series,
             (py::arg("hour"), py::arg("minute"), py::arg("relative") = false),
             "time_series(hour(int),minute(int),relative to suite start(bool=false)), Add a time slot")
        .def("set_time_series", add_time_series, "Add a time series. This will never complete")
        .def("set_time_series", add_time_series_2, "Add a time series. This will never complete")
        .def("set_time_series", &add_time_series_3, "Add a time series. This will never complete")
        .def("time",
             &ecf::CronAttr::time,
             py::return_value_policy<py::copy_const_reference>(),
             "return cron time as a TimeSeries")
        .def("last_day_of_the_month",
             &ecf::CronAttr::last_day_of_the_month,
             "Return true if last day of month is enabled")
        .add_property("week_days",
                      py::range(&ecf::CronAttr::week_days_begin, &ecf::CronAttr::week_days_end),
                      "returns a integer list of week days")
        .add_property(
            "last_week_days_of_the_month",
            py::range(&ecf::CronAttr::last_week_days_of_month_begin, &ecf::CronAttr::last_week_days_end_of_month_end),
            "returns a integer list of last week days of the month")
        .add_property("days_of_month",
                      py::range(&ecf::CronAttr::days_of_month_begin, &ecf::CronAttr::days_of_month_end),
                      "returns a integer list of days of the month")
        .add_property("months",
                      py::range(&ecf::CronAttr::months_begin, &ecf::CronAttr::months_end),
                      "returns a integer list of months of the year");

    py::class_<VerifyAttr>("Verify", py::init<NState::State, int>()) // state, expected
        .def(py::self == py::self)                                   // __eq__
        .def("__str__", &VerifyAttr::toString)                       // __str__
        .def("__copy__", pyutil_copy_object<VerifyAttr>)             // __copy__ uses copy constructor
        ;

    py::class_<ClockAttr, std::shared_ptr<ClockAttr>>(
        "Clock", NodeAttrDoc::clock_doc(), py::init<int, int, int, py::optional<bool>>()) // day, month, year, hybrid
        .def(py::init<int, int, int, bool>())
        .def(py::init<bool>())
        .def(py::self == py::self)                      // __eq__
        .def("__str__", &ClockAttr::toString)           // __str__
        .def("__copy__", pyutil_copy_object<ClockAttr>) // __copy__ uses copy constructor
        .def("set_gain_in_seconds", &ClockAttr::set_gain_in_seconds, "Set the gain in seconds")
        .def("set_gain", &ClockAttr::set_gain, "Set the gain in hours and minutes")
        .def("day", &ClockAttr::day, "Returns the day as an integer, range 1-31")
        .def("month", &ClockAttr::month, "Returns the month as an integer, range 1-12")
        .def("year", &ClockAttr::year, "Returns the year as an integer, > 1400")
        .def("gain", &ClockAttr::gain, "Returns the gain as an long. This represents seconds")
        .def("positive_gain",
             &ClockAttr::positive_gain,
             "Returns a boolean, where true means that the gain is positive");
#if ECF_ENABLE_PYTHON_PTR_REGISTER
    py::register_ptr_to_python<std::shared_ptr<ClockAttr>>(); // needed for mac and boost 1.6
#endif

    py::class_<ecf::AvisoAttr>("AvisoAttr", NodeAttrDoc::aviso_doc())
        .def("__init__", py::make_constructor(&aviso_init))
        .def("__init__", py::make_constructor(&aviso_init_defaults_0))
        .def("__init__", py::make_constructor(&aviso_init_defaults_1))
        .def("__init__", py::make_constructor(&aviso_init_defaults_2))
        .def("__init__", py::make_constructor(&aviso_init_defaults_3))
        .def(py::self == py::self)                           // __eq__
        .def("__str__", &ecf::to_python_string)              // __str__
        .def("__copy__", pyutil_copy_object<ecf::AvisoAttr>) // __copy__ uses copy constructor
        .def("name",
             &ecf::AvisoAttr::name,
             py::return_value_policy<py::copy_const_reference>(),
             "Returns the name of the Aviso attribute")
        .def("listener",
             &ecf::AvisoAttr::listener,
             py::return_value_policy<py::copy_const_reference>(),
             "Returns the Aviso listener configuration")
        .def("url",
             &ecf::AvisoAttr::url,
             py::return_value_policy<py::copy_const_reference>(),
             "Returns the URL used to contact the Aviso server")
        .def("schema",
             &ecf::AvisoAttr::schema,
             py::return_value_policy<py::copy_const_reference>(),
             "Returns the path to the schema used to contact the Aviso server")
        .def("polling", &ecf::AvisoAttr::polling, "Returns polling interval used to contact the Aviso server")
        .def("auth",
             &ecf::AvisoAttr::auth,
             py::return_value_policy<py::copy_const_reference>(),
             "Returns the path to Authentication credentials used to contact the Aviso server");

    py::class_<ecf::MirrorAttr>("MirrorAttr", NodeAttrDoc::mirror_doc())
        .def("__init__", py::make_constructor(&mirror_init))
        .def("__init__", py::make_constructor(&mirror_init_defaults_0))
        .def("__init__", py::make_constructor(&mirror_init_defaults_1))
        .def("__init__", py::make_constructor(&mirror_init_defaults_2))
        .def("__init__", py::make_constructor(&mirror_init_defaults_3))
        .def("__init__", py::make_constructor(&mirror_init_defaults_4))
        .def(py::self == py::self)                            // __eq__
        .def("__str__", &ecf::to_python_string)               // __str__
        .def("__copy__", pyutil_copy_object<ecf::MirrorAttr>) // __copy__ uses copy constructor
        .def("name",
             &ecf::MirrorAttr::name,
             py::return_value_policy<py::copy_const_reference>(),
             "Returns the name of the Mirror attribute")
        .def("remote_path",
             &ecf::MirrorAttr::remote_path,
             py::return_value_policy<py::copy_const_reference>(),
             "Returns the path on the remote ecFlow server")
        .def("remote_host",
             &ecf::MirrorAttr::remote_host,
             py::return_value_policy<py::copy_const_reference>(),
             "Returns the host of the remote ecFlow server")
        .def("remote_port",
             &ecf::MirrorAttr::remote_port,
             py::return_value_policy<py::copy_const_reference>(),
             "Returns the port of the remote ecFlow server")
        .def("ssl", &ecf::MirrorAttr::ssl, "Returns a boolean, where true means that SSL is enabled")
        .def("polling",
             &ecf::MirrorAttr::polling,
             "Returns the polling interval used to contact the remove ecFlow server")
        .def("auth",
             &ecf::MirrorAttr::auth,
             py::return_value_policy<py::copy_const_reference>(),
             "Returns the path to Authentication credentials used to contact the remote ecFlow server");
}
