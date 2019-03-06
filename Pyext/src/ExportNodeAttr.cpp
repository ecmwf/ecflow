/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #53 $ 
//
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description :
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
#include <boost/python.hpp>
#include <boost/python/suite/indexing/vector_indexing_suite.hpp>
#include <boost/python/raw_function.hpp>

#include "NodeAttr.hpp"
#include "Limit.hpp"
#include "InLimit.hpp"
#include "Variable.hpp"
#include "ClockAttr.hpp"
#include "CronAttr.hpp"
#include "DateAttr.hpp"
#include "DayAttr.hpp"
#include "LateAttr.hpp"
#include "RepeatAttr.hpp"
#include "TimeAttr.hpp"
#include "TodayAttr.hpp"
#include "VerifyAttr.hpp"
#include "QueueAttr.hpp"
#include "GenericAttr.hpp"
#include "AutoCancelAttr.hpp"
#include "AutoArchiveAttr.hpp"
#include "AutoRestoreAttr.hpp"
#include "ZombieAttr.hpp"
#include "Zombie.hpp"
#include "NodeAttrDoc.hpp"
#include "BoostPythonUtil.hpp"
#include "Attr.hpp"
#include "Flag.hpp"
#include "JobCreationCtrl.hpp"
#include "DefsDoc.hpp"
#include "Expression.hpp"

#include "Trigger.hpp"

using namespace ecf;
using namespace boost::python;
using namespace std;
namespace bp = boost::python;

// See: http://wiki.python.org/moin/boost.python/HowTo#boost.function_objects
///////////////////////////////////////////////////////////////////////////////////////////////////
object late_raw_constructor(bp::tuple args, bp::dict kw) {
   //cout << "late_raw_constructor len(args):" << len(args) << endl;
   // args[0] is Late(i.e self)
   if (len(args) > 1) throw std::runtime_error("late_raw_constructor: Late only expects keyword arguments, ie. Late(submitted='00:20',active='15:00',complete='+30:00')");
   return args[0].attr("__init__")(kw); // calls -> late_init(dict kw)
}

static void extract_late_keyword_arguments(std::shared_ptr<LateAttr> late, bp::dict& dict) {
   boost::python::list keys = dict.keys();
   const int no_of_keys = len(keys);
   for(int i = 0; i < no_of_keys; ++i) {
      if (extract<string>(keys[i]).check()) {
         std::string first = extract<std::string>(keys[i]);
         if (extract<string>(dict[keys[i]]).check()) {
            std::string second = extract<string>(dict[keys[i]]);
            int hour = 0;
            int min = 0;
            bool relative = TimeSeries::getTime(second,hour,min);
            if (first == "submitted")  late->add_submitted(hour,min);
            else if (first == "active")  late->add_active(hour,min);
            else if (first == "complete")  late->add_complete(hour,min,relative);
            else throw std::runtime_error("extract_late_keyword_arguments: keyword arguments, expected [submitted | active | complete]");
         }
         else throw std::runtime_error("extract_late_keyword_arguments: expected keyword arguments to be a string, ie Late(submitted='00:20',active='15:00',complete='+30:00')");
      }
   }
}

static std::shared_ptr<LateAttr> late_init(bp::dict& dict){
   std::shared_ptr<LateAttr> late = std::make_shared<LateAttr>();
   extract_late_keyword_arguments(late,dict);
   return late;
}

static std::shared_ptr<LateAttr> late_create() { return std::make_shared<LateAttr>();}


///////////////////////////////////////////////////////////////////////////////////////////////////
object cron_raw_constructor(bp::tuple args, bp::dict kw) {
   //cout << "cron_raw_constructor len(args):" << len(args) << endl;
   // args[0] is Cron(i.e self) args[1] is string name
   for (int i = 1; i < len(args) ; ++i) {
      if (extract<string>(args[i]).check())  {
         std::string time_series = extract<string>(args[i]);
         if (time_series.empty()) throw std::runtime_error("cron_raw_constructor: Empty string, please pass a valid time, i.e '12:30'");
         return args[0].attr("__init__")(time_series,kw); // calls -> init(const std::string& ts, dict kw)
      }
      if (extract<TimeSeries>(args[i]).check())  {
         TimeSeries time_series = extract<TimeSeries>(args[i]);
         return args[0].attr("__init__")(time_series,kw); // calls -> init(const TimeSeries& ts, dict kw)
      }
      else throw std::runtime_error("cron_raw_constructor: expects string | TimeSeries and keyword arguments");
   }
   throw std::runtime_error("cron_raw_constructor: expects string | TimeSeries and keyword arguments !!");
   return object();
}

static void extract_cron_keyword_arguments(std::shared_ptr<CronAttr> cron, bp::dict& dict) {
   boost::python::list keys = dict.keys();
   const int no_of_keys = len(keys);
   for(int i = 0; i < no_of_keys; ++i) {

      if (extract<string>(keys[i]).check()) {
         std::string first = extract<std::string>(keys[i]);
         if (extract<bp::list>(dict[keys[i]]).check()) {

            bp::list second = extract<bp::list>(dict[keys[i]]);
            std::vector<int> int_vec;
            BoostPythonUtil::list_to_int_vec(second,int_vec);

            //  expected keywords are: days_of_week,last_week_days_ofThe_month, days_of_month, months
            if (first == "days_of_week") cron->addWeekDays(int_vec);
            else if (first == "days_of_month") cron->addDaysOfMonth(int_vec);
            else if (first == "months") cron->addMonths(int_vec);
            else if (first == "last_week_days_of_the_month") cron->add_last_week_days_of_month(int_vec);
            else throw std::runtime_error("extract_cron_keyword_arguments: keyword arguments, expected [days_of_week | last_week_days_of_the_month | days_of_month | months | last_day_of_the_month");
         }
         else if (extract<bool>(dict[keys[i]]).check()) {
            if (first == "last_day_of_the_month") cron->add_last_day_of_month();
            else throw std::runtime_error("extract_cron_keyword_arguments: keyword arguments, expected [days_of_week | last_week_days_of_the_month | days_of_month | months | last_day_of_the_month]");
        }
         else throw std::runtime_error("extract_cron_keyword_arguments: keyword arguments to be a list");
      }
   }
}

static std::shared_ptr<CronAttr> cron_init(const std::string& ts, bp::dict& dict){
   std::shared_ptr<CronAttr> cron = std::make_shared<CronAttr>(ts);
   extract_cron_keyword_arguments(cron,dict);
   return cron;
}

static std::shared_ptr<CronAttr> cron_init1(const TimeSeries& ts, bp::dict& dict) {
   std::shared_ptr<CronAttr> cron = std::make_shared<CronAttr>(ts);
   extract_cron_keyword_arguments(cron,dict);
   return cron;
}

static std::shared_ptr<CronAttr> cron_create() { return std::make_shared<CronAttr>();}
static std::shared_ptr<CronAttr> cron_create2(const TimeSeries& ts) { return std::make_shared<CronAttr>(ts);}
///////////////////////////////////////////////////////////////////////////////////////////////////


void add_time_series_3(CronAttr* self,const std::string& ts) { self->addTimeSeries(TimeSeries::create(ts));}

void set_week_days(CronAttr* cron,const bp::list& list)
{
   std::vector<int> int_vec;
   BoostPythonUtil::list_to_int_vec(list,int_vec);
   cron->addWeekDays(int_vec);
}
void set_last_week_days_of_month(CronAttr* cron,const bp::list& list)
{
   std::vector<int> int_vec;
   BoostPythonUtil::list_to_int_vec(list,int_vec);
   cron->add_last_week_days_of_month(int_vec);
}

void set_days_of_month(CronAttr* cron,const bp::list& list)
{
   std::vector<int> int_vec;
   BoostPythonUtil::list_to_int_vec(list,int_vec);
   cron->addDaysOfMonth(int_vec);
}
void set_last_day_of_the_month(CronAttr* cron)
{
   cron->add_last_day_of_month();
}

void set_months(CronAttr* cron,const bp::list& list)
{
   std::vector<int> int_vec;
   BoostPythonUtil::list_to_int_vec(list,int_vec);
   cron->addMonths(int_vec);
}

// Create as shared because: we want to pass a Python list as part of the constructor,
// *AND* the only way make_constructor works is with a pointer.
// The Node::add function seem to cope with this, some boost python magic,must do a conversion
// from shared_ptr to pass by reference
static std::shared_ptr<RepeatEnumerated> create_RepeatEnumerated(const std::string& name, const bp::list& list)
{
   std::vector<std::string> vec;
   BoostPythonUtil::list_to_str_vec(list,vec);
   return std::make_shared<RepeatEnumerated>( name,vec );
}
static std::shared_ptr<RepeatString> create_RepeatString(const std::string& name, const bp::list& list)
{
   std::vector<std::string> vec;
   BoostPythonUtil::list_to_str_vec(list,vec);
   return std::make_shared<RepeatString>( name,vec );
}
static std::shared_ptr<AutoRestoreAttr> create_AutoRestoreAttr(const boost::python::list& list)
{
   std::vector<std::string> vec;
   BoostPythonUtil::list_to_str_vec(list,vec);
   return std::make_shared<AutoRestoreAttr>( vec );
}

static std::shared_ptr<QueueAttr> create_queue(const std::string& name, const boost::python::list& list)
{
   std::vector<std::string> vec;
   BoostPythonUtil::list_to_str_vec(list,vec);
   return std::make_shared<QueueAttr>( name, vec );
}

static std::shared_ptr<GenericAttr> create_generic(const std::string& name, const boost::python::list& list)
{
   std::vector<std::string> vec;
   BoostPythonUtil::list_to_str_vec(list,vec);
   return std::make_shared<GenericAttr>( name, vec );
}

static std::shared_ptr<ZombieAttr> create_ZombieAttr(
      Child::ZombieType zt,const bp::list& list,User::Action uc,int life_time_in_server)
{
   std::vector<Child::CmdType> vec;
   int the_list_size = len(list);
   vec.reserve(the_list_size);
   for (int i = 0; i < the_list_size; ++i) {
      vec.push_back(extract<Child::CmdType>(list[i]));
   }
   return std::make_shared<ZombieAttr>(zt,vec,uc,life_time_in_server );
}

static std::shared_ptr<ZombieAttr> create_ZombieAttr1(
      Child::ZombieType zt,const bp::list& list,User::Action uc)
{
   std::vector<Child::CmdType> vec;
   int the_list_size = len(list);
   vec.reserve(the_list_size);
   for (int i = 0; i < the_list_size; ++i) {
      vec.push_back(extract<Child::CmdType>(list[i]));
   }
   return std::make_shared<ZombieAttr>(zt,vec,uc);
}

static bp::list wrap_set_of_strings(Limit* limit)
{
   bp::list list;
   const std::set<std::string>& paths = limit->paths();
   BOOST_FOREACH(std::string path, paths) { list.append(path); }
   return list;
}

static job_creation_ctrl_ptr makeJobCreationCtrl() { return std::make_shared<JobCreationCtrl>();}

void export_NodeAttr()
{
   // Trigger & Complete thin wrapper over Expression, allows us to call: Task("a").add(Trigger("a=1"),Complete("b=1"))
   class_<Trigger,std::shared_ptr<Trigger> >("Trigger",DefsDoc::trigger(), init<std::string>() )
   .def(init<PartExpression>())
   .def(init<bp::list>())
   .def(init<std::string,bool>())
   .def(self == self )                            // __eq__
   .def("__str__",        &Trigger::expression)   // __str__
   .def("get_expression", &Trigger::expression, "returns the trigger expression as a string")
   ;

   class_<Complete,std::shared_ptr<Complete> >("Complete",DefsDoc::trigger(), init<std::string>() )
   .def(init<PartExpression>())
   .def(init<bp::list>())
   .def(init<std::string,bool>())
   .def(self == self )                             // __eq__
   .def("__str__",        &Complete::expression)   // __str__
   .def("get_expression", &Complete::expression, "returns the complete expression as a string")
   ;

   // mimic PartExpression(const std::string& expression  )
   // mimic PartExpression(const std::string& expression, bool andExpr /* true means AND , false means OR */ )
   // Use to adding large trigger and complete expressions
   class_<PartExpression>("PartExpression",DefsDoc::part_expression_doc(), init<std::string>())
   .def(init<std::string,bool>())
   .def(self == self )                 // __eq__
   .def("get_expression", &PartExpression::expression, return_value_policy<copy_const_reference>(), "returns the part expression as a string")
   .def("and_expr",       &PartExpression::andExpr)
   .def("or_expr",        &PartExpression::orExpr)
   ;

   class_<Expression,  std::shared_ptr<Expression> >("Expression",DefsDoc::expression_doc(), init<std::string>() )
   .def(init<PartExpression>())
   .def(self == self )                               // __eq__
   .def("__str__",        &Expression::expression)   // __str__
   .def("get_expression", &Expression::expression, "returns the complete expression as a string")
   .def("add",            &Expression::add,"Add a part expression, the second and subsequent part expressions must have 'and/or' set")
   .add_property("parts", bp::range( &Expression::part_begin, &Expression::part_end),"Returns a list of PartExpression's" )
   ;

   enum_<Flag::Type>("FlagType",
         "Flags store state associated with a node\n\n"
         "- FORCE_ABORT   - Node* do not run when try_no > ECF_TRIES, and task aborted by user\n"
         "- USER_EDIT     - task\n"
         "- TASK_ABORTED  - task*\n"
         "- EDIT_FAILED   - task*\n"
         "- JOBCMD_FAILED - task*\n"
         "- NO_SCRIPT     - task*\n"
         "- KILLED        - task* do not run when try_no > ECF_TRIES, and task killed by user\n"
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
         "- NOT_SET\n"
   )
         .value("force_abort",  Flag::FORCE_ABORT)
         .value("user_edit",    Flag::USER_EDIT)
         .value("task_aborted", Flag::TASK_ABORTED)
         .value("edit_failed",  Flag::EDIT_FAILED)
         .value("jobcmd_failed",Flag::JOBCMD_FAILED)
         .value("no_script",    Flag::NO_SCRIPT)
         .value("killed",       Flag::KILLED)
         .value("late",         Flag::LATE)
         .value("message",      Flag::MESSAGE)
         .value("byrule",       Flag::BYRULE)
         .value("queuelimit",   Flag::QUEUELIMIT)
         .value("wait",         Flag::WAIT)
         .value("locked",       Flag::LOCKED)
         .value("zombie",       Flag::ZOMBIE)
         .value("no_reque",     Flag::NO_REQUE_IF_SINGLE_TIME_DEP)
         .value("archived",     Flag::ARCHIVED)
         .value("restored",     Flag::RESTORED)
         .value("threshold",    Flag::THRESHOLD)
         .value("sigterm",      Flag::ECF_SIGTERM)
         .value("not_set",      Flag::NOT_SET)
         ;

   class_<Flag>("Flag",
         "Represents additional state associated with a Node.\n\n" ,
         init<>())
   .def("__str__",       &Flag::to_string) // __str__
   .def(self == self )                     // __eq__
   .def("is_set",        &Flag::is_set,"Queries if a given flag is set")
   .def("set",           &Flag::set,   "Sets the given flag. Used in test only")
   .def("clear",         &Flag::clear, "Clear the given flag. Used in test only")
   .def("reset",         &Flag::reset, "Clears all flags. Used in test only")
   .def("list",          &Flag::list,  "Returns the list of all flag types. returns FlagTypeVec. Used in test only").staticmethod("list")
   .def("type_to_string",&Flag::enum_to_string, "Convert type to a string. Used in test only").staticmethod("type_to_string")
   ;

   class_<std::vector<Flag::Type> >("FlagTypeVec", "Hold a list of flag types")
   .def(vector_indexing_suite<std::vector<Flag::Type> , true >()) ;


   class_<JobCreationCtrl, boost::noncopyable, job_creation_ctrl_ptr >("JobCreationCtrl",  DefsDoc::jobgenctrl_doc())
   .def("__init__",make_constructor(makeJobCreationCtrl), DefsDoc::jobgenctrl_doc())
   .def("set_node_path", &JobCreationCtrl::set_node_path, "The node we want to check job creation for. If no node specified check all tasks")
   .def("set_dir_for_job_creation", &JobCreationCtrl::set_dir_for_job_creation, "Specify directory, for job creation")
   .def("set_verbose", &JobCreationCtrl::set_verbose, "Output each task as its being checked.")
   .def("get_dir_for_job_creation", &JobCreationCtrl::dir_for_job_creation, return_value_policy<copy_const_reference>(), "Returns the directory set for job creation")
   .def("generate_temp_dir", &JobCreationCtrl::generate_temp_dir, "Automatically generated temporary directory for job creation. Directory written to stdout for information")
   .def("get_error_msg", &JobCreationCtrl::get_error_msg, return_value_policy<copy_const_reference>(),"Returns an error message generated during checking of job creation")
   ;

	enum_<Child::ZombieType>("ZombieType", NodeAttrDoc::zombie_type_doc())
         .value("ecf",            Child::ECF)
         .value("ecf_pid",        Child::ECF_PID)
         .value("ecf_pid_passwd", Child::ECF_PID_PASSWD)
         .value("ecf_passwd",     Child::ECF_PASSWD)
         .value("user",           Child::USER)
         .value("path",           Child::PATH)
	;

	enum_<User::Action>("ZombieUserActionType",NodeAttrDoc::zombie_user_action_type_doc())
	.value("fob",     User::FOB)
	.value("fail",    User::FAIL)
	.value("remove",  User::REMOVE)
	.value("adopt",   User::ADOPT)
   .value("block",   User::BLOCK)
   .value("kill",    User::KILL)
	;

	enum_<Child::CmdType>("ChildCmdType",NodeAttrDoc::child_cmd_type_doc())
	.value("init",    Child::INIT)
	.value("event",   Child::EVENT)
	.value("meter",   Child::METER)
	.value("label",   Child::LABEL)
   .value("wait",    Child::WAIT)
   .value("queue",   Child::QUEUE)
	.value("abort",   Child::ABORT)
	.value("complete",Child::COMPLETE)
 	;

   enum_<Attr::Type>("AttrType", "Sortable attribute type, currently [event | meter | label | limit | variable | all ]")
   .value("event",   Attr::EVENT)
   .value("meter",   Attr::METER)
   .value("label",   Attr::LABEL)
   .value("limit",   Attr::LIMIT)
   .value("variable",Attr::VARIABLE)
   .value("all",     Attr::ALL)
   ;

	// 	ZombieAttr(ecf::Child::ZombieType t, const std::vector<ecf::Child::CmdType>& c, ecf::User::Action a, int zombie_lifetime);
 	class_<ZombieAttr>("ZombieAttr",NodeAttrDoc::zombie_doc())
   .def("__init__",make_constructor(&create_ZombieAttr) )
   .def("__init__",make_constructor(&create_ZombieAttr1) )
   .def("__str__",    &ZombieAttr::toString)              // __str__
   .def("__copy__",   copyObject<ZombieAttr>)             // __copy__ uses copy constructor
 	.def(self == self )                                    // __eq__
 	.def("empty",          &ZombieAttr::empty,          "Return true if the attribute is empty")
 	.def("zombie_type",    &ZombieAttr::zombie_type,    "Returns the `zombie type`_")
 	.def("user_action",    &ZombieAttr::action,         "The automated action to invoke, when zombies arise")
 	.def("zombie_lifetime",&ZombieAttr::zombie_lifetime,"Returns the lifetime in seconds of `zombie`_ in the server")
   .add_property( "child_cmds",bp::range(&ZombieAttr::child_begin,&ZombieAttr::child_end),"The list of child commands. If empty action applies to all child cmds")
   ;

   class_<std::vector<Zombie> >("ZombieVec", "Hold a list of zombies")
   .def(vector_indexing_suite<std::vector<Zombie> , true >()) ;

   class_<Zombie>("Zombie","Represent a zombie process stored by the server")
   .def("__str__",    &Zombie::to_string)             // __str__
   .def("__copy__",   copyObject<Zombie>)             // __copy__ uses copy constructor
   .def(self == self )                                // __eq__
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
   .def("attr", &Zombie::attr,return_value_policy<copy_const_reference>())
   .def("calls", &Zombie::calls)
   .def("jobs_password", &Zombie::jobs_password,return_value_policy<copy_const_reference>())
   .def("path_to_task", &Zombie::path_to_task,return_value_policy<copy_const_reference>())
   .def("process_or_remote_id", &Zombie::process_or_remote_id,return_value_policy<copy_const_reference>())
   .def("user_cmd", &Zombie::user_cmd,return_value_policy<copy_const_reference>())
   .def("try_no", &Zombie::try_no)
   .def("duration", &Zombie::duration)
   .def("user_action", &Zombie::user_action)
   .def("user_action_str", &Zombie::user_action_str)
   .def("host", &Zombie::host, return_value_policy<copy_const_reference>())
   .def("allowed_age", &Zombie::allowed_age)
   ;


 	class_<Variable>("Variable",NodeAttrDoc::variable_doc(),init<std::string, std::string>())
 	.def("__str__",    &Variable::toString)                // __str__
 	.def("__copy__",   copyObject<Variable>)               // __copy__ uses copy constructor
 	.def(self == self )                                    // __eq__
 	.def("name",     &Variable::name,     return_value_policy<copy_const_reference>(), "Return the variable name as string")
 	.def("value",    &Variable::theValue, return_value_policy<copy_const_reference>(), "Return the variable value as a string")
 	.def("empty",    &Variable::empty,   "Return true if the variable is empty. Used when returning a Null variable, from a find")
 	;
 	// .add_property("value", make_function(&Variable::theValue ,return_value_policy<copy_const_reference>()),
 	//                       &Variable::set_value,  "get/set the value as a property")
 	// The following works v = Variable('name','fred')
 	//   print v.value
 	// But it will then break v.value()

   // We need to return pass a list of Variable as arguments, to retrieve the generated variables
   class_<std::vector<Variable> >("VariableList", "Hold a list of Variables")
               .def(vector_indexing_suite<std::vector<Variable> >()) ;


	class_<Label>("Label",NodeAttrDoc::label_doc(),init<std::string, std::string>())
	.def(self == self )                                    // __eq__
	.def("__str__",   &Label::toString)                    // __str__
   .def("__copy__",   copyObject<Label>)                  // __copy__ uses copy constructor
	.def("name",      &Label::name,      return_value_policy<copy_const_reference>(), "Return the `label`_ name as string")
	.def("value",     &Label::value,     return_value_policy<copy_const_reference>(), "Return the original `label`_ value as string")
	.def("new_value", &Label::new_value, return_value_policy<copy_const_reference>(), "Return the new label value as string")
   .def("empty",     &Label::empty,     "Return true if the Label is empty. Used when returning a NULL Label, from a find")
  	;

	// This will not work, because paths_begin
   //.add_property("node_paths", bp::range(&Limit::paths_begin,&Limit::paths_begin),"List of nodes(paths) that have consumed a limit")

	class_<Limit, std::shared_ptr<Limit> >("Limit",NodeAttrDoc::limit_doc(),init<std::string, int>())
	.def(self == self )                               // __eq__
	.def("__str__",  &Limit::toString)                // __str__
   .def("__copy__",   copyObject<Limit>)             // __copy__ uses copy constructor
	.def("name",     &Limit::name, return_value_policy<copy_const_reference>(), "Return the `limit`_ name as string")
   .def("value",    &Limit::value,    "The `limit`_ token value as an integer")
   .def("limit",    &Limit::theLimit, "The max value of the `limit`_ as an integer")
   .def("increment",&Limit::increment, "used for test only")
   .def("decrement",&Limit::decrement, "used for test only")
   .def("node_paths",&wrap_set_of_strings,"List of nodes(paths) that have consumed a limit")
 	;
#if ECF_ENABLE_PYTHON_PTR_REGISTER
   bp::register_ptr_to_python< std::shared_ptr<Limit> >(); // needed for mac and boost 1.6
#endif

   class_<InLimit>("InLimit",NodeAttrDoc::inlimit_doc())
			.def( init<std::string>() )
			.def( init<std::string,std::string>() )
			.def( init<std::string,std::string,int>() )
			.def( init<std::string,std::string,int,bool>() )
			.def(self == self )                                  // __eq__
			.def("__str__",     &InLimit::toString)              // __str__
			.def("__copy__",   copyObject<InLimit>)              // __copy__ uses copy constructor
			.def("name",        &InLimit::name,       return_value_policy<copy_const_reference>(), "Return the `inlimit`_ name as string")
			.def("path_to_node",&InLimit::pathToNode, return_value_policy<copy_const_reference>(), "Path to the node that holds the limit, can be empty")
			.def("limit_this_node_only",&InLimit::limit_this_node_only ,                           "Only this node is limited. i.e. typically Family or Suite")
			.def("tokens",      &InLimit::tokens,                                                  "The number of token to consume from the Limit")
			;

	class_<Event>("Event",NodeAttrDoc::event_doc(), init<int, optional<std::string> >())
   .def( init<std::string> () )
	.def(self == self )                                  // __eq__
	.def("__str__",     &Event::toString)                // __str__
   .def("__copy__",   copyObject<Event>)                // __copy__ uses copy constructor
	.def("name",        &Event::name,       return_value_policy<copy_const_reference>(), "Return the Events name as string. If number supplied name may be empty.")
	.def("number",      &Event::number,     "Return events number as a integer. If not specified return max integer value")
   .def("name_or_number",&Event::name_or_number,"returns name or number as an string")
   .def("value",       &Event::value,      "Return events current value")
   .def("empty",       &Event::empty,      "Return true if the Event is empty. Used when returning a NULL Event, from a find")
	;

	class_<Meter>("Meter",NodeAttrDoc::meter_doc(),init<std::string,int,int,optional<int> >())
 	.def(self == self )                                  // __eq__
	.def("__str__",     &Meter::toString)                // __str__
   .def("__copy__",   copyObject<Meter>)                // __copy__ uses copy constructor
	.def("name",        &Meter::name,       return_value_policy<copy_const_reference>(), "Return the Meters name as string")
	.def("min",         &Meter::min,                                                     "Return the Meters minimum value")
	.def("max",         &Meter::max,                                                     "Return the Meters maximum value")
	.def("value",       &Meter::value,                                                   "Return meters current value")
	.def("color_change",&Meter::colorChange,                                             "returns the color change")
   .def("empty",       &Meter::empty,       "Return true if the Meter is empty. Used when returning a NULL Meter, from a find")
	;

   class_<QueueAttr>("Queue",NodeAttrDoc::queue_doc())
   .def("__init__", make_constructor(&create_queue) )
   .def(self == self )                                  // __eq__
   .def("__str__",     &QueueAttr::toString)            // __str__
   .def("__copy__",   copyObject<QueueAttr>)           // __copy__ uses copy constructor
   .def("name",        &QueueAttr::name,       return_value_policy<copy_const_reference>(), "Return the queue name as string")
   .def("value",       &QueueAttr::value,                                                   "Return the queue current value as string")
   .def("index",       &QueueAttr::index,                                                   "Return the queue current index as a integer")
   .def("empty",       &QueueAttr::empty,       "Return true if the Queue is empty. Used when returning a NULL Queue, from a find")
   ;

   class_<GenericAttr>("Generic","A generic attribute, used to add new attributes for the future, without requiring a API change")
   .def("__init__", make_constructor(&create_generic) )
   .def(self == self )                              // __eq__
   .def("__str__",  &GenericAttr::to_string)         // __str__
   .def("__copy__", copyObject<GenericAttr>)        // __copy__ uses copy constructor
   .def("name",     &GenericAttr::name,       return_value_policy<copy_const_reference>(), "Return the generic name as string")
   .def("empty",    &GenericAttr::empty,       "Return true if the Generic is empty. Used when returning a NULL Generic, from a find")
   .add_property( "values",boost::python::range(&GenericAttr::values_begin,&GenericAttr::values_end),"The list of values for the generic")
   ;

	class_<DateAttr>("Date",NodeAttrDoc::date_doc() ,init<int,int,int>())  // day,month,year
	.def(init<std::string>())
	.def(self == self )                                     // __eq__
	.def("__str__",     &DateAttr::toString)                // __str__
   .def("__copy__",    copyObject<DateAttr>)               // __copy__ uses copy constructor
	.def("day",         &DateAttr::day,      "Return the day. The range is 0-31, 0 means its wild-carded")
	.def("month",       &DateAttr::month,    "Return the month. The range is 0-12, 0 means its wild-carded")
   .def("year",        &DateAttr::year,     "Return the year, 0 means its wild-carded")
	;

	enum_<DayAttr::Day_t>("Days",NodeAttrDoc::days_enum_doc())
	.value("sunday",   DayAttr::SUNDAY)
	.value("monday",   DayAttr::MONDAY)
	.value("tuesday",  DayAttr::TUESDAY)
	.value("wednesday",DayAttr::WEDNESDAY)
	.value("thursday", DayAttr::THURSDAY)
	.value("friday",   DayAttr::FRIDAY)
	.value("saturday", DayAttr::SATURDAY);

	class_<DayAttr>("Day",NodeAttrDoc::day_doc(),init<DayAttr::Day_t>() )
   .def(init<std::string>())                              // constructor
   .def(self == self )                                    // __eq__
	.def("__str__",     &DayAttr::toString)                // __str__
   .def("__copy__",    copyObject<DayAttr>)               // __copy__ uses copy constructor
	.def("day",         &DayAttr::day,      "Return the day as enumerator")
	;

	class_<TimeAttr>("Time",NodeAttrDoc::time_doc(),init<TimeSlot, optional<bool> >())
	.def( init<int,int,optional<bool> >())                  // hour, minute, relative
 	.def( init<TimeSeries>())
    .def( init<TimeSlot,TimeSlot,TimeSlot,bool>())
    .def( init<std::string>())
	.def(self == self )                           // __eq__
	.def("__str__",    &TimeAttr::toString)       // __str__
    .def("__copy__",    copyObject<TimeAttr>)     // __copy__ uses copy constructor
	.def("time_series",&TimeAttr::time_series,return_value_policy<copy_const_reference>(), "Return the Time attributes time series")
	;

	class_<TodayAttr>("Today",NodeAttrDoc::today_doc() ,init<TimeSlot, optional<bool> >())
	.def( init<int,int,optional<bool> >())                  // hour, minute, relative
 	.def( init<TimeSeries>())
	.def( init<TimeSlot,TimeSlot,TimeSlot,bool>())
    .def( init<std::string>())
	.def(self == self )                                     // __eq__
	.def("__str__",    &TodayAttr::toString)                // __str__
    .def("__copy__",   copyObject<TodayAttr>)               // __copy__ uses copy constructor
	.def("time_series",&TodayAttr::time_series,return_value_policy<copy_const_reference>(), "Return the Todays time series")
	;


	class_<LateAttr, std::shared_ptr<LateAttr>  >("Late",NodeAttrDoc::late_doc())
    .def("__init__",raw_function(&late_raw_constructor,1))  // will call -> late_init
    .def("__init__",make_constructor(&late_init))
    .def("__init__",make_constructor(&late_create))
    .def( "submitted", &LateAttr::addSubmitted,
          "submitted(TimeSlot):The time node can stay `submitted`_. Submitted is always relative. If the node stays\n"
          "submitted longer than the time specified, the `late`_ flag is set\n"
    )
    .def( "submitted", &LateAttr::add_submitted,
          "submitted(hour,minute) The time node can stay submitted. Submitted is always relative. If the node stays\n"
          "submitted longer than the time specified, the late flag is set\n"
    )
    .def( "active",    &LateAttr::add_active,
          "active(hour,minute): The time the node must become `active`_. If the node is still `queued`_ or `submitted`_\n"
          "by the time specified, the late flag is set"
    )
    .def( "active",   &LateAttr::addActive,
          "active(TimeSlot):The time the node must become `active`_. If the node is still `queued`_ or `submitted`_\n"
          "by the time specified, the late flag is set"
    )
    .def( "complete",  &LateAttr::add_complete,
          "complete(hour,minute):The time the node must become `complete`_. If relative, time is taken from the time\n"
          "the node became `active`_, otherwise node must be `complete`_ by the time given"
    )
    .def( "complete", &LateAttr::addComplete,
          "complete(TimeSlot): The time the node must become `complete`_. If relative, time is taken from the time\n"
          "the node became `active`_, otherwise node must be `complete`_ by the time given"
    )
    .def(self == self )                                  // __eq__
    .def("__str__",   &LateAttr::toString)               // __str__
    .def("__copy__",   copyObject<LateAttr>)             // __copy__ uses copy constructor
    .def("submitted", &LateAttr::submitted,return_value_policy<copy_const_reference>(), "Return the submitted time as a TimeSlot")
    .def("active",    &LateAttr::active,   return_value_policy<copy_const_reference>(), "Return the active time as a TimeSlot")
    .def("complete",  &LateAttr::complete, return_value_policy<copy_const_reference>(), "Return the complete time as a TimeSlot")
    .def("complete_is_relative",  &LateAttr::complete_is_relative, "Returns a boolean where true means that complete is relative")
    .def("is_late",   &LateAttr::isLate, "Return True if late")
    ;
#if ECF_ENABLE_PYTHON_PTR_REGISTER
   bp::register_ptr_to_python< std::shared_ptr<LateAttr> >(); // needed for mac and boost 1.6
#endif

	class_<AutoCancelAttr, std::shared_ptr<AutoCancelAttr> >(
			"Autocancel",NodeAttrDoc::autocancel_doc() ,
			init<int,int,bool >()                             // hour, minute, relative
	)
	.def( init<int>())                                        // days
	.def( init<TimeSlot, bool>())
	.def(self == self )                                       // __eq__
	.def("__str__", &AutoCancelAttr::toString)                // __str__
   .def("__copy__",copyObject<AutoCancelAttr>)               // __copy__ uses copy constructor
	.def("time",    &AutoCancelAttr::time, return_value_policy<copy_const_reference>(), "returns cancel time as a TimeSlot")
	.def("relative",&AutoCancelAttr::relative, "Returns a boolean where true means the time is relative")
	.def("days",    &AutoCancelAttr::days,     "Returns a boolean true if time was specified in days")
  	;
#if ECF_ENABLE_PYTHON_PTR_REGISTER
   bp::register_ptr_to_python< std::shared_ptr<AutoCancelAttr> >(); // needed for mac and boost 1.6
#endif


   class_<AutoArchiveAttr, std::shared_ptr<AutoArchiveAttr> >(
         "Autoarchive",NodeAttrDoc::autocancel_doc() ,
         init<int,int,bool >()                             // hour, minute, relative
   )
   .def( init<int>())                                        // days
   .def( init<TimeSlot, bool>())
   .def(self == self )                                       // __eq__
   .def("__str__", &AutoArchiveAttr::toString)               // __str__
   .def("__copy__",copyObject<AutoArchiveAttr>)              // __copy__ uses copy constructor
   .def("time",    &AutoArchiveAttr::time, return_value_policy<copy_const_reference>(), "returns archive time as a TimeSlot")
   .def("relative",&AutoArchiveAttr::relative, "Returns a boolean where true means the time is relative")
   .def("days",    &AutoArchiveAttr::days,     "Returns a boolean true if time was specified in days")
   ;
#if defined(__clang__)
   boost::python::register_ptr_to_python< std::shared_ptr<AutoArchiveAttr> >(); // needed for mac and boost 1.6
#endif


   class_<AutoRestoreAttr, std::shared_ptr<AutoRestoreAttr> >( "Autorestore",NodeAttrDoc::autocancel_doc())
   .def("__init__",make_constructor(&create_AutoRestoreAttr) )
   .def(self == self )                                       // __eq__
   .def("__str__", &AutoRestoreAttr::toString)               // __str__
   .def("__copy__",copyObject<AutoRestoreAttr>)              // __copy__ uses copy constructor
   .def("nodes_to_restore",&AutoRestoreAttr::nodes_to_restore, return_value_policy<copy_const_reference>(), "returns a list of nodes to be restored")
   ;
#if defined(__clang__)
   boost::python::register_ptr_to_python< std::shared_ptr<AutoRestoreAttr> >(); // needed for mac and boost 1.6
#endif


	class_<RepeatDate >("RepeatDate",NodeAttrDoc::repeat_date_doc() ,init< std::string, int, int, optional<int> >()) // name, start, end , delta
 	.def(self == self )                              // __eq__
	.def("__str__",        &RepeatDate::toString)    // __str__
   .def("__copy__",       copyObject<RepeatDate>)   // __copy__ uses copy constructor
	.def("name",           &RepeatDate::name, return_value_policy<copy_const_reference>(),"Return the name of the repeat.")
	.def("start",          &RepeatDate::start ,"Return the start date as an integer in yyyymmdd format")
	.def("end",            &RepeatDate::end,   "Return the end date as an integer in yyyymmdd format")
	.def("step",           &RepeatDate::step,  "Return the step increment. This is used to update the repeat, until end date is reached")
	;

	class_<RepeatInteger>("RepeatInteger",NodeAttrDoc::repeat_integer_doc(),init< std::string, int, int, optional<int> >()) // name, start, end , delta = 1
 	.def(self == self )                                  // __eq__
	.def("__str__",        &RepeatInteger::toString)     // __str__
   .def("__copy__",       copyObject<RepeatInteger>)    // __copy__ uses copy constructor
	.def("name",           &RepeatInteger::name, return_value_policy<copy_const_reference>(),"Return the name of the repeat.")
	.def("start",          &RepeatInteger::start)
	.def("end",            &RepeatInteger::end)
	.def("step",           &RepeatInteger::step)
	;

	// Create as shared because: we want to pass a Python list as part of the constructor,
	// and the only way make_constructor works is with a pointer.
	class_<RepeatEnumerated, std::shared_ptr<RepeatEnumerated> >("RepeatEnumerated",NodeAttrDoc::repeat_enumerated_doc())
   .def("__init__",make_constructor(&create_RepeatEnumerated) )
	.def(self == self )                                     // __eq__
	.def("__str__",        &RepeatEnumerated::toString)     // __str__
    .def("__copy__",       copyObject<RepeatEnumerated>)    // __copy__ uses copy constructor
	.def("name",           &RepeatEnumerated::name, return_value_policy<copy_const_reference>(),"Return the name of the `repeat`_.")
	.def("start",          &RepeatEnumerated::start)
	.def("end",            &RepeatEnumerated::end)
	.def("step",           &RepeatEnumerated::step)
	;
#if ECF_ENABLE_PYTHON_PTR_REGISTER
   bp::register_ptr_to_python< std::shared_ptr<RepeatEnumerated> >(); // needed for mac and boost 1.6
#endif

	class_<RepeatString,std::shared_ptr<RepeatString> >("RepeatString", NodeAttrDoc::repeat_string_doc())
   .def("__init__",make_constructor(&create_RepeatString) )
	.def(self == self )                                 // __eq__
	.def("__str__",        &RepeatString::toString)     // __str__
   .def("__copy__",       copyObject<RepeatString>)    // __copy__ uses copy constructor
	.def("name",           &RepeatString::name, return_value_policy<copy_const_reference>(),"Return the name of the `repeat`_.")
	.def("start",          &RepeatString::start)
	.def("end",            &RepeatString::end)
	.def("step",           &RepeatString::step)
	;
#if ECF_ENABLE_PYTHON_PTR_REGISTER
   bp::register_ptr_to_python< std::shared_ptr<RepeatString> >(); // needed for mac and boost 1.6
#endif

	class_<RepeatDay>("RepeatDay",NodeAttrDoc::repeat_day_doc(),init< optional<int> >())
 	.def(self == self )                              // __eq__
	.def("__str__",        &RepeatDay::toString)     // __str__
   .def("__copy__",       copyObject<RepeatDay>)    // __copy__ uses copy constructor
	;

	class_<Repeat>("Repeat",NodeAttrDoc::repeat_doc() ,init< int >())
	.def(self == self )                    // __eq__
	.def("__str__", &Repeat::toString)     // __str__
	.def("__copy__",copyObject<Repeat>)    // __copy__ uses copy constructor
	.def("empty",   &Repeat::empty ,"Return true if the repeat is empty.")
	.def("name",    &Repeat::name, return_value_policy<copy_const_reference>(), "The `repeat`_ name, can be referenced in `trigger`_ expressions")
	.def("start",   &Repeat::start,"The start value of the repeat, as an integer")
	.def("end",     &Repeat::end,  "The last value of the repeat, as an integer")
	.def("step",    &Repeat::step, "The increment for the repeat, as an integer")
	.def("value",   &Repeat::last_valid_value,"The current value of the repeat as an integer")
	;


	void (CronAttr::*add_time_series)(const TimeSeries&) = &CronAttr::addTimeSeries;
	void (CronAttr::*add_time_series_2)( const TimeSlot& s, const TimeSlot& f, const TimeSlot& i) = &CronAttr::addTimeSeries;
	class_<CronAttr, std::shared_ptr<CronAttr> >("Cron",NodeAttrDoc::cron_doc() )
   .def("__init__",raw_function(&cron_raw_constructor,1))  // will call -> cron_init or cron_init1
   .def("__init__",make_constructor(&cron_init))
   .def("__init__",make_constructor(&cron_init1))
   .def("__init__",make_constructor(&cron_create2))
   .def("__init__",make_constructor(&cron_create))
	.def(self == self )                                // __eq__
	.def("__str__",            &CronAttr::toString)    // __str__
	.def("__copy__",copyObject<CronAttr>)              // __copy__ uses copy constructor
   .def( "set_week_days",     &set_week_days ,   "Specifies days of week. Expects a list of integers, with integer range 0==Sun to 6==Sat")
   .def( "set_last_week_days_of_the_month",&set_last_week_days_of_month,"Specifies last week days of the month. Expects a list of integers, with integer range 0==Sun to 6==Sat")
   .def( "set_days_of_month", &set_days_of_month,"Specifies days of the month. Expects a list of integers with integer range 1-31" )
   .def( "set_last_day_of_the_month",&set_last_day_of_the_month,"Set cron for the last day of the month" )
	.def( "set_months",        &set_months  ,     "Specifies months. Expects a list of integers, with integer range 1-12")
	.def( "set_time_series",   &CronAttr::add_time_series,(bp::arg("hour"),bp::arg("minute"),bp::arg("relative")=false),"time_series(hour(int),minute(int),relative to suite start(bool=false)), Add a time slot")
	.def( "set_time_series",   add_time_series,   "Add a time series. This will never complete")
	.def( "set_time_series",   add_time_series_2, "Add a time series. This will never complete")
	.def( "set_time_series",   &add_time_series_3,"Add a time series. This will never complete")
   .def( "time",              &CronAttr::time, return_value_policy<copy_const_reference>(), "return cron time as a TimeSeries")
   .def( "last_day_of_the_month", &CronAttr::last_day_of_the_month, "Return true if last day of month is enabled")
   .add_property( "week_days",    bp::range(&CronAttr::week_days_begin,    &CronAttr::week_days_end),     "returns a integer list of week days")
   .add_property( "last_week_days_of_the_month",bp::range(&CronAttr::last_week_days_of_month_begin,&CronAttr::last_week_days_end_of_month_end ),"returns a integer list of last week days of the month")
	.add_property( "days_of_month",bp::range(&CronAttr::days_of_month_begin,&CronAttr::days_of_month_end), "returns a integer list of days of the month")
	.add_property( "months",       bp::range(&CronAttr::months_begin,       &CronAttr::months_end),        "returns a integer list of months of the year")
 	;


	class_<VerifyAttr>("Verify", init<NState::State,int>())  // state, expected
	.def(self == self )                               // __eq__
	.def("__str__",        &VerifyAttr::toString)     // __str__
   .def("__copy__",copyObject<VerifyAttr>)           // __copy__ uses copy constructor
	;


	void (ClockAttr::*start_stop_with_server)(bool) = &ClockAttr::startStopWithServer;
	class_<ClockAttr, std::shared_ptr<ClockAttr> >("Clock",NodeAttrDoc::clock_doc() ,init<int,int,int,optional<bool> > ())  // day, month, year, hybrid
    .def( init<int,int,int,bool>())
    .def( init<bool>())
	.def(self == self )                                   // __eq__
	.def("__str__",             &ClockAttr::toString)     // __str__
    .def("__copy__",copyObject<ClockAttr>)                // __copy__ uses copy constructor
	.def( "set_gain_in_seconds",&ClockAttr::set_gain_in_seconds, "Set the gain in seconds")
	.def( "set_gain",     &ClockAttr::set_gain,                  "Set the gain in hours and minutes")
 	.def( "set_virtual",  start_stop_with_server,   "Sets/unsets the clock as being virtual")
	.def( "day",          &ClockAttr::day,          "Returns the day as an integer, range 1-31")
	.def( "month",        &ClockAttr::month,        "Returns the month as an integer, range 1-12")
	.def( "year",         &ClockAttr::year,         "Returns the year as an integer, > 1400")
	.def( "gain",         &ClockAttr::gain,         "Returns the gain as an long. This represents seconds")
	.def( "positive_gain",&ClockAttr::positive_gain,"Returns a boolean, where true means that the gain is positive")
	.def( "virtual"      ,&ClockAttr::is_virtual,   "Returns a boolean, where true means that clock is virtual")
	;
#if ECF_ENABLE_PYTHON_PTR_REGISTER
   bp::register_ptr_to_python< std::shared_ptr<ClockAttr> >(); // needed for mac and boost 1.6
#endif
}
