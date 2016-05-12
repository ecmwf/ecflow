/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #85 $
//
// Copyright 2009-2016 ECMWF.
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

#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/noncopyable.hpp>

#include "Defs.hpp"
#include "Suite.hpp"
#include "Task.hpp"
#include "Expression.hpp"
#include "JobCreationCtrl.hpp"
#include "BoostPythonUtil.hpp"

#include "DefsDoc.hpp"
#include "NodeAttrDoc.hpp"

using namespace ecf;
using namespace boost::python;
using namespace std;
namespace bp = boost::python;

// See: http://wiki.python.org/moin/boost.python/HowTo#boost.function_objects

Defs* get_defs(node_ptr self) { return self->defs(); }

node_ptr add_variable(node_ptr self,const std::string& name, const std::string& value) { self->add_variable(name,value); return self;}
node_ptr add_variable_int(node_ptr self,const std::string& name, int value) { self->add_variable_int(name,value); return self;}
node_ptr add_variable_var(node_ptr self,const Variable& var) { self->addVariable(var); return self;}
node_ptr add_variable_dict(node_ptr self,const boost::python::dict& dict) {
   std::vector<std::pair<std::string,std::string> > vec;
   BoostPythonUtil::dict_to_str_vec(dict,vec);
   std::vector<std::pair<std::string,std::string> >::iterator i;
   std::vector<std::pair<std::string,std::string> >::iterator vec_end = vec.end();
   for(i = vec.begin(); i != vec_end; ++i) {
      self->add_variable((*i).first,(*i).second);
   }
   return self;
}

node_ptr add_event(node_ptr self,const Event& e)                       { self->addEvent(e); return self; }
node_ptr add_event_1(node_ptr self,int number)                         { self->addEvent(Event(number)); return self; }
node_ptr add_event_2(node_ptr self,int number,const std::string& name) { self->addEvent(Event(number,name)); return self;  }
node_ptr add_event_3(node_ptr self,const std::string& name)            { self->addEvent(Event(name)); return self; }

node_ptr add_meter(node_ptr self,const Meter& m)                                                     { self->addMeter(m); return self;}
node_ptr add_meter_1(node_ptr self,const std::string& meter_name, int min,int max, int color_change) { self->addMeter(Meter(meter_name,min,max,color_change)); return self;}
node_ptr add_meter_2(node_ptr self,const std::string& meter_name, int min,int max)                   { self->addMeter(Meter(meter_name,min,max));return self; }

node_ptr add_label(node_ptr self,const std::string& name, const std::string& value) { self->addLabel(Label(name,value)); return self; }
node_ptr add_label_1(node_ptr self,const Label& label) { self->addLabel(label); return self; }
node_ptr add_limit(node_ptr self,const std::string& name, int limit)                { self->addLimit(Limit(name,limit)); return self;}
node_ptr add_limit_1(node_ptr self,const Limit& limit)                { self->addLimit(limit); return self;}
node_ptr add_in_limit(node_ptr self,const std::string& name,const std::string& pathToNode,int tokens) { self->addInLimit(InLimit(name,pathToNode,tokens)); return self;}
node_ptr add_in_limit_1(node_ptr self,const InLimit& inlimit) { self->addInLimit(inlimit); return self;}
node_ptr add_time(node_ptr self,int hour, int minute)                    { self->addTime(ecf::TimeAttr(hour,minute));return self; }
node_ptr add_time_1(node_ptr self,int hour, int minute, bool relative)   { self->addTime(ecf::TimeAttr(hour,minute,relative)); return self;}
node_ptr add_time_2(node_ptr self,const std::string& ts)                 { self->addTime(ecf::TimeAttr(TimeSeries::create(ts)));return self;}
node_ptr add_time_3(node_ptr self,const ecf::TimeAttr& ts)               { self->addTime(ts);return self;}
node_ptr add_today(node_ptr self,int hour, int minute)                   { self->addToday(ecf::TodayAttr(hour,minute)); return self;}
node_ptr add_today_1(node_ptr self,int hour, int minute, bool relative)  { self->addToday(ecf::TodayAttr(hour,minute,relative)); return self;}
node_ptr add_today_2(node_ptr self,const std::string& ts)                { self->addToday(ecf::TodayAttr(TimeSeries::create(ts)));return self;}
node_ptr add_today_3(node_ptr self,const ecf::TodayAttr& ts)             { self->addToday(ts);return self;}
node_ptr add_date(node_ptr self,int day, int month, int year)            { self->addDate(DateAttr(day,month,year)); return self;}
node_ptr add_date_1(node_ptr self,const DateAttr& d)                     { self->addDate(d); return self;}
node_ptr add_day(node_ptr self,DayAttr::Day_t day )                      { self->addDay(DayAttr(day)); return self;}
node_ptr add_day_1(node_ptr self,const std::string& day )                { self->addDay(DayAttr(DayAttr::getDay(day))); return self;}
node_ptr add_day_2(node_ptr self,const DayAttr& day )                    { self->addDay(day); return self;}
node_ptr add_autocancel(node_ptr self,int days )                         { self->addAutoCancel(ecf::AutoCancelAttr(days)); return self;}
node_ptr add_autocancel_1(node_ptr self,int hour, int min,bool relative) { self->addAutoCancel(ecf::AutoCancelAttr(hour,min,relative)); return self;}
node_ptr add_autocancel_2(node_ptr self,const TimeSlot& ts,bool relative){ self->addAutoCancel(ecf::AutoCancelAttr(ts,relative)); return self;}
node_ptr add_autocancel_3(node_ptr self, const ecf::AutoCancelAttr& attr){ self->addAutoCancel(attr); return self;}
node_ptr add_zombie(node_ptr self, const ZombieAttr& attr){ self->addZombie(attr); return self;}

node_ptr add_cron(node_ptr self,const ecf::CronAttr& attr)      { self->addCron(attr); return self; }
node_ptr add_late(node_ptr self,const ecf::LateAttr& attr)      { self->addLate(attr); return self; }
std::string get_state_change_time(node_ptr self,const std::string& format)
{
   if (format == "iso_extended") return to_iso_extended_string(self->state_change_time());
   else if (format == "iso") return to_iso_string(self->state_change_time());
   return to_simple_string(self->state_change_time());
}

node_ptr add_defstatus(node_ptr self,DState::State s)      { self->addDefStatus(s); return self; }
node_ptr add_repeat_date(node_ptr self,const RepeatDate& d)       { self->addRepeat(d); return self; }
node_ptr add_repeat_integer(node_ptr self,const RepeatInteger& d) { self->addRepeat(d); return self; }
node_ptr add_repeat_string(node_ptr self,const RepeatString& d)   { self->addRepeat(d); return self; }
node_ptr add_repeat_enum(node_ptr self,const RepeatEnumerated& d) { self->addRepeat(d); return self; }
node_ptr add_repeat_day(node_ptr self,const RepeatDay& d)         { self->addRepeat(d); return self; }
node_ptr add_trigger(node_ptr self,const std::string& expr)      { self->add_trigger(expr); return self; }
node_ptr add_trigger_expr(node_ptr self,const Expression& expr)  { self->add_trigger_expr(expr); return self; }
node_ptr add_complete(node_ptr self,const std::string& expr)     { self->add_complete(expr); return self; }
node_ptr add_complete_expr(node_ptr self,const Expression& expr) { self->add_complete_expr(expr); return self; }
node_ptr add_part_trigger(node_ptr self,const PartExpression& expr)  { self->add_part_trigger(PartExpression(expr)); return self; }
node_ptr add_part_trigger_1(node_ptr self,const std::string& expression)                { self->add_part_trigger(PartExpression(expression)); return self;}
node_ptr add_part_trigger_2(node_ptr self,const std::string& expression, bool and_expr) { self->add_part_trigger(PartExpression(expression,and_expr)); return self;}
node_ptr add_part_complete(node_ptr self,const PartExpression& expr)  { self->add_part_complete(PartExpression(expr)); return self; }
node_ptr add_part_complete_1(node_ptr self,const std::string& expression)                 { self->add_part_complete(PartExpression(expression)); return self;}
node_ptr add_part_complete_2(node_ptr self,const std::string& expression, bool and_expr){ self->add_part_complete(PartExpression(expression,and_expr)); return self;}
bool evaluate_trigger(node_ptr self) { Ast* t = self->triggerAst(); if (t) return t->evaluate();return false;}
bool evaluate_complete(node_ptr self) { Ast* t = self->completeAst(); if (t) return t->evaluate();return false;}

static job_creation_ctrl_ptr makeJobCreationCtrl() { return boost::make_shared<JobCreationCtrl>();}

std::vector<node_ptr> get_all_nodes(node_ptr self){ std::vector<node_ptr> nodes; self->get_all_nodes(nodes); return nodes; }

void export_Node()
{
   enum_<Flag::Type>("FlagType",
         "Flags store state associated with a node\n\n"
         "FORCE_ABORT   - Node* do not run when try_no > ECF_TRIES, and task aborted by user\n"
         "USER_EDIT     - task\n"
         "TASK_ABORTED  - task*\n"
         "EDIT_FAILED   - task*\n"
         "JOBCMD_FAILED - task*\n"
         "NO_SCRIPT     - task*\n"
         "KILLED        - task* do not run when try_no > ECF_TRIES, and task killed by user\n"
         "MIGRATED      - Node\n"
         "LATE          - Node attribute, Task is late, or Defs checkpt takes to long\n"
         "MESSAGE       - Node\n"
         "BYRULE        - Node*, set if node is set to complete by complete trigger expression\n"
         "QUEUELIMIT    - Node\n"
         "WAIT          - task* \n"
         "LOCKED        - Server\n"
         "ZOMBIE        - task*\n"
         "NO_REQUE      - task\n"
         "NOT_SET\n"
   )
         .value("force_abort",  Flag::FORCE_ABORT)
         .value("user_edit",    Flag::USER_EDIT)
         .value("task_aborted", Flag::TASK_ABORTED)
         .value("edit_failed",  Flag::EDIT_FAILED)
         .value("jobcmd_failed",Flag::JOBCMD_FAILED)
         .value("no_script",    Flag::NO_SCRIPT)
         .value("killed",       Flag::KILLED)
         .value("migrated",     Flag::MIGRATED)
         .value("late",         Flag::LATE)
         .value("message",      Flag::MESSAGE)
         .value("byrule",       Flag::BYRULE)
         .value("queuelimit",   Flag::QUEUELIMIT)
         .value("wait",         Flag::WAIT)
         .value("locked",       Flag::LOCKED)
         .value("zombie",       Flag::ZOMBIE)
         .value("no_reque",     Flag::NO_REQUE_IF_SINGLE_TIME_DEP)
         .value("not_set",      Flag::NOT_SET)
         ;

   class_<Flag>("Flag",
         "Represents additional state associated with a Node.\n\n"
         ,
         init<>()
      )
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
   .def("get_dir_for_job_creation", &JobCreationCtrl::dir_for_job_creation, return_value_policy<copy_const_reference>(), "Returns the directory set for job creation")
   .def("generate_temp_dir", &JobCreationCtrl::generate_temp_dir, "Automatically generated temporary directory for job creation. Directory written to stdout for information")
   .def("get_error_msg", &JobCreationCtrl::get_error_msg, return_value_policy<copy_const_reference>(),"Returns an error message generated during checking of job creation")
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

   class_<Expression,  boost::shared_ptr<Expression> >("Expression",DefsDoc::expression_doc(), init<std::string>() )
   .def(init<PartExpression>())
   .def(self == self )                               // __eq__
   .def("__str__",        &Expression::expression)   // __str__
   .def("get_expression", &Expression::expression, "returns the complete expression as a string")
   .def("add",            &Expression::add,"Add a part expression, the second and subsequent part expressions must have 'and/or' set")
   .add_property("parts", boost::python::range( &Expression::part_begin, &Expression::part_end),"Returns a list of PartExpression's" )
   ;

   // Turn off proxies by passing true as the NoProxy template parameter.
   // shared_ptrs don't need proxies because calls on one a copy of the
   // shared_ptr will affect all of them (duh!).
   class_<std::vector<node_ptr> >("NodeVec", "Hold a list of Nodes (i.e :term:`suite`, :term:`family` or :term:`task` s)")
   .def(vector_indexing_suite<std::vector<node_ptr> , true >()) ;

   // Note: we have have not added __setattr__, as it seems to interfere with
   // classes derived from Node. i.e calling self.fred = bill in the derived class
   // expects self to be of type Node.
   class_<Node, boost::noncopyable, node_ptr >("Node", DefsDoc::node_doc(), no_init)
   .def("name",&Node::name, return_value_policy<copy_const_reference>() )
   .def("add_trigger",      &add_trigger,             DefsDoc::add_trigger_doc())
   .def("add_trigger",      &add_trigger_expr)
   .def("add_complete",     &add_complete,            DefsDoc::add_trigger_doc())
   .def("add_complete",     &add_complete_expr)
   .def("add_part_trigger" ,&add_part_trigger,        DefsDoc::add_trigger_doc())
   .def("add_part_trigger" ,&add_part_trigger_1 )
   .def("add_part_trigger" ,&add_part_trigger_2 )
   .def("add_part_complete",&add_part_complete,          DefsDoc::add_trigger_doc())
   .def("add_part_complete",&add_part_complete_1 )
   .def("add_part_complete",&add_part_complete_2 )
   .def("evaluate_trigger", &evaluate_trigger ,"evaluate trigger expression")
   .def("evaluate_complete",&evaluate_complete ,"evaluate complete expression")
   .def("add_variable",     &add_variable,               DefsDoc::add_variable_doc())
   .def("add_variable",     &add_variable_int)
   .def("add_variable",     &add_variable_var)
   .def("add_variable",     &add_variable_dict)
   .def("add_label",        &add_label,                  DefsDoc::add_label_doc())
   .def("add_label",        &add_label_1)
   .def("add_limit",        &add_limit,                  DefsDoc::add_limit_doc())
   .def("add_limit",        &add_limit_1)
   .def("add_inlimit",      &add_in_limit,(bp::arg("limit_name"),bp::arg("path_to_node_containing_limit")="",bp::arg("tokens")=1),DefsDoc::add_inlimit_doc())
   .def("add_inlimit",      &add_in_limit_1)
   .def("add_event",        &add_event,                  DefsDoc::add_event_doc())
   .def("add_event",        &add_event_1)
   .def("add_event",        &add_event_2)
   .def("add_event",        &add_event_3)
   .def("add_meter",        &add_meter,                  DefsDoc::add_meter_doc())
   .def("add_meter",        add_meter_1)
   .def("add_meter",        add_meter_2)
   .def("add_date",         &add_date,                   DefsDoc::add_date_doc() )
   .def("add_date",         &add_date_1)
   .def("add_day",          &add_day,                    DefsDoc::add_day_doc())
   .def("add_day",          &add_day_1)
   .def("add_day",          &add_day_2)
   .def("add_today",        &add_today,                  DefsDoc::add_today_doc())
   .def("add_today",        &add_today_1)
   .def("add_today",        &add_today_2)
   .def("add_today",        &add_today_3)
   .def("add_time",         &add_time,                   DefsDoc::add_time_doc())
   .def("add_time",         &add_time_1 )
   .def("add_time",         &add_time_2 )
   .def("add_time",         &add_time_3 )
   .def("add_cron",         &add_cron,                   DefsDoc::add_cron_doc())
   .def("add_late",         &add_late,                   DefsDoc::add_late_doc())
   .def("add_autocancel",   &add_autocancel,             DefsDoc::add_autocancel_doc())
   .def("add_autocancel",   &add_autocancel_1)
   .def("add_autocancel",   &add_autocancel_2)
   .def("add_autocancel",   &add_autocancel_3)
   .def("add_verify",       &Node::addVerify,            DefsDoc::add_verify_doc())
   .def("add_repeat",       &add_repeat_date,            DefsDoc::add_repeat_date_doc())
   .def("add_repeat",       &add_repeat_integer,         DefsDoc::add_repeat_integer_doc())
   .def("add_repeat",       &add_repeat_string,          DefsDoc::add_repeat_string_doc())
   .def("add_repeat",       &add_repeat_enum,            DefsDoc::add_repeat_enumerated_doc() )
   .def("add_repeat",       &add_repeat_day,             DefsDoc::add_repeat_day_doc() )
   .def("add_defstatus",    &add_defstatus,              DefsDoc::add_defstatus_doc())
   .def("add_zombie",       &add_zombie,                 NodeAttrDoc::zombie_doc())
   .def("delete_variable",  &Node::deleteVariable       )
   .def("delete_event",     &Node::deleteEvent          )
   .def("delete_meter",     &Node::deleteMeter          )
   .def("delete_label",     &Node::deleteLabel          )
   .def("delete_trigger",   &Node::deleteTrigger        )
   .def("delete_complete",  &Node::deleteComplete       )
   .def("delete_repeat",    &Node::deleteRepeat         )
   .def("delete_limit",     &Node::deleteLimit          )
   .def("delete_inlimit",   &Node::deleteInlimit        )
   .def("delete_time",      &Node::deleteTime           )
   .def("delete_time",      &Node::delete_time          )
   .def("delete_today",     &Node::deleteToday          )
   .def("delete_today",     &Node::delete_today         )
   .def("delete_date",      &Node::deleteDate           )
   .def("delete_date",      &Node::delete_date          )
   .def("delete_day",       &Node::deleteDay            )
   .def("delete_day",       &Node::delete_day           )
   .def("delete_cron",      &Node::deleteCron           )
   .def("delete_cron",      &Node::delete_cron          )
   .def("delete_zombie",    &Node::deleteZombie         )
   .def("delete_zombie",    &Node::delete_zombie        )
   .def("change_trigger",   &Node::changeTrigger        )
   .def("change_complete",  &Node::changeComplete       )
   .def("get_abs_node_path",    &Node::absNodePath, DefsDoc::abs_node_path_doc())
   .def("has_time_dependencies",      &Node::hasTimeDependencies)
   .def("update_generated_variables", &Node::update_generated_variables)
   .def("get_generated_variables", &Node::gen_variables, "returns a list of generated variables. Use ecflow.VariableList as return argument")
   .def("is_suspended",     &Node::isSuspended, "Returns true if the :term:`node` is in a :term:`suspended` state")
   .def("find_variable",    &Node::findVariable,           return_value_policy<copy_const_reference>(), "Find user variable on the node only.  Returns a object")
   .def("find_parent_variable",&Node::find_parent_variable,return_value_policy<copy_const_reference>(), "Find user variable variable up the parent hierarchy.  Returns a object")
   .def("find_meter",       &Node::findMeter,              return_value_policy<copy_const_reference>(), "Find the :term:`meter` on the node only. Returns a object")
   .def("find_event",       &Node::findEventByNameOrNumber,return_value_policy<copy_const_reference>(), "Find the :term:`event` on the node only. Returns a object")
   .def("find_label",       &Node::find_label,             return_value_policy<copy_const_reference>(), "Find the :term:`label` on the node only. Returns a object")
   .def("find_limit",       &Node::find_limit  ,           "Find the :term:`limit` on the node only. returns a limit ptr" )
   .def("find_node_up_the_tree",&Node::find_node_up_the_tree  , "Search immediate node, then up the node hierarchy" )
   .def("get_state",        &Node::state , "Returns the state of the node. This excludes the suspended state")
   .def("get_state_change_time",&get_state_change_time, (bp::arg("format")="iso_extended"), "Returns the time of the last state change as a string. Default format is iso_extended, (iso_extended, iso, simple)")
   .def("get_dstate",       &Node::dstate, "Returns the state of node. This will include suspended state")
   .def("get_defstatus",    &Node::defStatus )
   .def("get_repeat",       &Node::repeat, return_value_policy<copy_const_reference>() )
   .def("get_late",         &Node::get_late,return_internal_reference<>() )
   .def("get_autocancel",   &Node::get_autocancel, return_internal_reference<>())
   .def("get_trigger",      &Node::get_trigger, return_internal_reference<>() )
   .def("get_complete",     &Node::get_complete, return_internal_reference<>() )
   .def("get_defs",         get_defs,   return_internal_reference<>() )
   .def("get_parent",       &Node::parent, return_internal_reference<>() )
   .def("get_all_nodes",    &get_all_nodes,"Returns all the child nodes")
   .def("get_flag",         &Node::get_flag,return_value_policy<copy_const_reference>(),"Return additional state associated with a node.")
   .add_property("meters",    boost::python::range( &Node::meter_begin,    &Node::meter_end) ,  "Returns a list of :term:`meter` s")
   .add_property("events",    boost::python::range( &Node::event_begin,    &Node::event_end) ,  "Returns a list of :term:`event` s")
   .add_property("variables", boost::python::range( &Node::variable_begin, &Node::variable_end),"Returns a list of user defined :term:`variable` s" )
   .add_property("labels",    boost::python::range( &Node::label_begin,    &Node::label_end) ,  "Returns a list of :term:`label` s")
   .add_property("limits",    boost::python::range( &Node::limit_begin,    &Node::limit_end),   "Returns a list of :term:`limit` s" )
   .add_property("inlimits",  boost::python::range( &Node::inlimit_begin,  &Node::inlimit_end), "Returns a list of :term:`inlimit` s" )
   .add_property("verifies",  boost::python::range( &Node::verify_begin,   &Node::verify_end),  "Returns a list of Verify's" )
   .add_property("times",     boost::python::range( &Node::time_begin,     &Node::time_end),    "Returns a list of :term:`time` s" )
   .add_property("todays",    boost::python::range( &Node::today_begin,    &Node::today_end),   "Returns a list of :term:`today` s" )
   .add_property("dates",     boost::python::range( &Node::date_begin,     &Node::date_end),    "Returns a list of :term:`date` s" )
   .add_property("days",      boost::python::range( &Node::day_begin,      &Node::day_end),     "Returns a list of :term:`day` s")
   .add_property("crons",     boost::python::range( &Node::cron_begin,     &Node::cron_end),    "Returns a list of :term:`cron` s" )
   .add_property("zombies",   boost::python::range( &Node::zombie_begin,   &Node::zombie_end),  "Returns a list of :term:`zombie` s" )
   ;
}
