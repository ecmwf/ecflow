/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #85 $
//
// Copyright 2009-2017 ECMWF.
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

#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/noncopyable.hpp>

#include "Defs.hpp"
#include "Suite.hpp"
#include "Task.hpp"
#include "JobCreationCtrl.hpp"
#include "Str.hpp"
#include "ClientInvoker.hpp"

#include "NodeUtil.hpp"
#include "Edit.hpp"
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

node_ptr add_repeat_date(node_ptr self,const RepeatDate& d)       { self->addRepeat(d); return self; }
node_ptr add_repeat_integer(node_ptr self,const RepeatInteger& d) { self->addRepeat(d); return self; }
node_ptr add_repeat_string(node_ptr self,const RepeatString& d)   { self->addRepeat(d); return self; }
node_ptr add_repeat_enum(node_ptr self,const RepeatEnumerated& d) { self->addRepeat(d); return self; }
node_ptr add_repeat_day(node_ptr self,const RepeatDay& d)         { self->addRepeat(d); return self; }

void sort_attributes(node_ptr self,const std::string& attribute_name, bool recursive){
   std::string attribute = attribute_name; boost::algorithm::to_lower(attribute);
   ecf::Attr::Type attr = Attr::to_attr(attribute_name);
   if (attr == ecf::Attr::UNKNOWN) {
      std::stringstream ss;  ss << "sort_attributes: the attribute " << attribute_name << " is not valid";
      throw std::runtime_error(ss.str());
   }
   self->sort_attributes(attr,recursive);
}

std::vector<node_ptr> get_all_nodes(node_ptr self){ std::vector<node_ptr> nodes; self->get_all_nodes(nodes); return nodes; }

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

node_ptr add_defstatus(node_ptr self,DState::State s)             { self->addDefStatus(s); return self; }
node_ptr add_defstatus1(node_ptr self,const Defstatus& ds)         { self->addDefStatus(ds.state()); return self; }

/////////////////////////////////////////////////////////////////////////////////////////

static object do_rshift(node_ptr self, const bp::object& arg){
   //std::cout << "do_rshift\n";
   (void)NodeUtil::do_add(self,arg);

   if (extract<node_ptr>(arg).check()) {
      NodeContainer* nc = self->isNodeContainer();
      if (!nc) throw std::runtime_error("ExportNode::do_rshift() : Can only add a child to Suite or Family");
      node_ptr child = extract<node_ptr>(arg);

      std::vector<node_ptr> children;
      nc->immediateChildren(children);
      node_ptr previous_child;
      for(size_t i =0; i < children.size(); i++) {
         if (previous_child && children[i] == child) {
            // if existing trigger, add new trigger as AND
            if (child->get_trigger()) child->add_part_trigger( PartExpression( previous_child->name() + " == complete", PartExpression::AND) );
            else child->add_trigger_expr( previous_child->name() + " == complete");
         }
         if (children[i]->defStatus() != DState::COMPLETE)  previous_child = children[i];
      }
   }
   return object(self);
}
static object do_lshift(node_ptr self, const bp::object& arg){
   //std::cout << "do_lshift : " << self->name() << "\n"; cout << flush;
   (void)NodeUtil::do_add(self,arg);

   if (extract<node_ptr>(arg).check()) {

      NodeContainer* nc = self->isNodeContainer();
      if (!nc) throw std::runtime_error("ExportNode::do_lshift() : Can only add a child to Suite or Family");
      node_ptr child = extract<node_ptr>(arg);

      std::vector<node_ptr> children;
      nc->immediateChildren(children);
      node_ptr previous_child;
      for(size_t i =0; i < children.size(); i++) {
         if (i == 0) continue;
         if (children[i-1]->defStatus() != DState::COMPLETE)  previous_child = children[i-1] ;

         if (previous_child &&  previous_child != child && children[i] == child) {
            // if existing trigger, add new trigger as AND
            if (previous_child->get_trigger()) previous_child->add_part_trigger( PartExpression( child->name() + " == complete", PartExpression::AND) );
            else previous_child->add_trigger_expr( child->name() + " == complete");
         }
      }
   }
   return object(self);
}

static object add(bp::tuple args, bp::dict kwargs)
{
   int the_list_size = len(args);
   node_ptr self = extract<node_ptr>(args[0]); // self
   if (!self) throw std::runtime_error("ExportNode::add() : first argument is not a node");

   for (int i = 1; i < the_list_size; ++i) (void)NodeUtil::do_add(self,args[i]);

   // key word arguments are use for adding variable only
   (void)NodeUtil::add_variable_dict(self,kwargs);

   return object(self); // return node_ptr as python object, relies class_<Node>... for type registration
}

static object node_getattr(node_ptr self, const std::string& attr) {
   // cout << " node_getattr  self.name() : " << self->name() << "  attr " << attr << "\n";
   size_t pos = 0;
   node_ptr child = self->findImmediateChild(attr,pos);
   if (child) { return object(child);}

   const Variable& var = self->findVariable(attr);
   if (!var.empty()) return object(var);

   const Variable& gvar = self->findGenVariable(attr);
   if (!gvar.empty()) return object(gvar);

   const Event& event = self->findEventByNameOrNumber(attr);
   if (!event.empty()) return object(event);

   const Meter& meter = self->findMeter(attr);
   if (!meter.empty()) return object(meter);

   limit_ptr limit = self->find_limit( attr );
   if (limit.get()) return object(limit);

   std::stringstream ss; ss << "ExportNode::node_getattr: function of name '" << attr << "' does not exist *OR* child node,variable,meter,event or limit on node " << self->absNodePath();
   throw std::runtime_error(ss.str());
   return object();
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct null_deleter {
    void operator()(void const *) const{}
};
void do_replace_on_server(node_ptr self,ClientInvoker& theClient,bool suspend_node_first, bool force_replace)
{
   // Need to make a defs_ptr from a Defs*  to avoid double delete use null_deletor
   defs_ptr defs = defs_ptr( self->defs(),null_deleter());
   bool create_parents_as_required = true;
   if (suspend_node_first) theClient.suspend(self->absNodePath());
   theClient.replace_1(self->absNodePath(),defs,create_parents_as_required, force_replace); // this can throw
}
void replace_on_server(node_ptr self, bool suspend_node_first,bool force_replace){
   ClientInvoker theClient; // assume HOST and PORT found from environment
   do_replace_on_server(self,theClient,suspend_node_first,force_replace);
}
void replace_on_server1(node_ptr self, const std::string& host, const std::string& port,bool suspend_node_first,bool force_replace){
   ClientInvoker theClient(host,port);
   do_replace_on_server(self,theClient,suspend_node_first,force_replace);
}
void replace_on_server2(node_ptr self, const std::string& host_port,bool suspend_node_first,bool force_replace){
   ClientInvoker theClient(host_port);
   do_replace_on_server(self,theClient,suspend_node_first,force_replace);
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////


void export_Node()
{
   // Turn off proxies by passing true as the NoProxy template parameter.
   // shared_ptrs don't need proxies because calls on one a copy of the
   // shared_ptr will affect all of them (duh!).
   class_<std::vector<node_ptr> >("NodeVec", "Hold a list of Nodes (i.e `suite`_, `family`_ or `task`_ s)")
   .def(vector_indexing_suite<std::vector<node_ptr> , true >()) ;

   class_<Node, boost::noncopyable, node_ptr >("Node", DefsDoc::node_doc(), no_init)
   .def("name",&Node::name, return_value_policy<copy_const_reference>() )
   .def("add", raw_function(add,1),           DefsDoc::add())  // a.add(b) & a.add([b])
   .def("__add__",  &NodeUtil::do_add,                  DefsDoc::add())  // a + b
   .def("__rshift__",  &do_rshift)                             // nc >> a >> b >> c     a + (b.add(Trigger('a==complete')) + (c.add(Trigger('b==complete')))
   .def("__lshift__",  &do_lshift)                             // nc << a << b << c     (a.add(Trigger('b==complete')) + (b.add(Trigger('c==complete'))) + c
   .def("__iadd__", &NodeUtil::do_add)                         // a += b
   .def("__iadd__", &NodeUtil::node_iadd)                      // a += [ b ]
   .def("__getattr__",      &node_getattr) /* Any attempt to resolve a property, method, or field name that doesn't actually exist on the object itself will be passed to __getattr__*/
   .def("remove",           &Node::remove,           "Remove the node from its parent. and returns it")
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
   .def("add_variable",     &NodeUtil::add_variable_dict)
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
   .def("add_defstatus",    &add_defstatus1,             DefsDoc::add_defstatus_doc())
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
   .def("sort_attributes",  &Node::sort_attributes,(bp::arg("attribute_type"),bp::arg("recursive")=true))
   .def("sort_attributes",  &sort_attributes,(bp::arg("attribute_type"),bp::arg("recursive")=true))
   .def("get_abs_node_path",    &Node::absNodePath, DefsDoc::abs_node_path_doc())
   .def("has_time_dependencies",      &Node::hasTimeDependencies)
   .def("update_generated_variables", &Node::update_generated_variables)
   .def("get_generated_variables", &Node::gen_variables, "returns a list of generated variables. Use ecflow.VariableList as return argument")
   .def("is_suspended",     &Node::isSuspended, "Returns true if the `node`_ is in a `suspended`_ state")
   .def("find_variable",    &Node::findVariable,           return_value_policy<copy_const_reference>(), "Find user variable on the node only.  Returns a object")
   .def("find_gen_variable",&Node::findGenVariable,        return_value_policy<copy_const_reference>(), "Find generated variable on the node only.  Returns a object")
   .def("find_parent_variable",&Node::find_parent_variable,return_value_policy<copy_const_reference>(), "Find user variable variable up the parent hierarchy.  Returns a object")
   .def("find_meter",       &Node::findMeter,              return_value_policy<copy_const_reference>(), "Find the `meter`_ on the node only. Returns a object")
   .def("find_event",       &Node::findEventByNameOrNumber,return_value_policy<copy_const_reference>(), "Find the `event`_ on the node only. Returns a object")
   .def("find_label",       &Node::find_label,             return_value_policy<copy_const_reference>(), "Find the `label`_ on the node only. Returns a object")
   .def("find_limit",       &Node::find_limit  ,           "Find the `limit`_ on the node only. returns a limit ptr" )
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
   .def("replace_on_server",&replace_on_server,(bp::arg("suspend_node_first")=true,bp::arg("force")=true),"replace node on the server.")
   .def("replace_on_server",&replace_on_server1,(bp::arg("suspend_node_first")=true,bp::arg("force")=true),"replace node on the server.")
   .def("replace_on_server",&replace_on_server2,(bp::arg("suspend_node_first")=true,bp::arg("force")=true),"replace node on the server.")
   .add_property("meters",    bp::range( &Node::meter_begin,    &Node::meter_end) ,  "Returns a list of `meter`_ s")
   .add_property("events",    bp::range( &Node::event_begin,    &Node::event_end) ,  "Returns a list of `event`_ s")
   .add_property("variables", bp::range( &Node::variable_begin, &Node::variable_end),"Returns a list of user defined `variable`_ s" )
   .add_property("labels",    bp::range( &Node::label_begin,    &Node::label_end) ,  "Returns a list of `label`_ s")
   .add_property("limits",    bp::range( &Node::limit_begin,    &Node::limit_end),   "Returns a list of `limit`_ s" )
   .add_property("inlimits",  bp::range( &Node::inlimit_begin,  &Node::inlimit_end), "Returns a list of `inlimit`_ s" )
   .add_property("verifies",  bp::range( &Node::verify_begin,   &Node::verify_end),  "Returns a list of Verify's" )
   .add_property("times",     bp::range( &Node::time_begin,     &Node::time_end),    "Returns a list of `time`_ s" )
   .add_property("todays",    bp::range( &Node::today_begin,    &Node::today_end),   "Returns a list of `today`_ s" )
   .add_property("dates",     bp::range( &Node::date_begin,     &Node::date_end),    "Returns a list of `date`_ s" )
   .add_property("days",      bp::range( &Node::day_begin,      &Node::day_end),     "Returns a list of `day`_ s")
   .add_property("crons",     bp::range( &Node::cron_begin,     &Node::cron_end),    "Returns a list of `cron`_ s" )
   .add_property("zombies",   bp::range( &Node::zombie_begin,   &Node::zombie_end),  "Returns a list of `zombie`_ s" )
   ;
#if ECF_ENABLE_PYTHON_PTR_REGISTER
   bp::register_ptr_to_python<node_ptr>(); // needed for mac and boost 1.6
#endif
}
