/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #18 $
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

#include "BoostPythonUtil.hpp"
#include "NodeUtil.hpp"
#include "Trigger.hpp"

#include "Defs.hpp"
#include "Suite.hpp"
#include "Task.hpp"
#include "Str.hpp"

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
#include "AutoCancelAttr.hpp"
#include "ZombieAttr.hpp"
#include "Attr.hpp"
#include "Flag.hpp"
#include "Edit.hpp"

using namespace boost::python;
using namespace std;
using namespace ecf;
namespace bp = boost::python;


object NodeUtil::node_raw_constructor(bp::tuple args, bp::dict kw) {
   // cout << "node_raw_constructor len(args):" << len(args) << endl;
   // args[0] is Task(i.e self) args[1] is string name
   bp::list the_list;
   std::string name;
   for (int i = 1; i < len(args) ; ++i) {
      if (extract<string>(args[i]).check()) name = extract<string>(args[i]);
      else the_list.append(args[i]);
   }
   if (name.empty()) throw std::runtime_error("node_raw_constructor: first argument must be a string");
   return args[0].attr("__init__")(name,the_list,kw); // calls -> init(const std::string& name, list attr, dict kw)
}

node_ptr NodeUtil::add_variable_dict(node_ptr self,const bp::dict& dict)
{
   std::vector<Variable> vec;
   BoostPythonUtil::dict_to_str_vec(dict,vec);
   for(const auto & i : vec) self->addVariable(i);
   return self;
}

object NodeUtil::node_iadd(node_ptr self, const bp::list& list) {
   // std::cout << "node_iadd list " << self->name() << "\n";
   int the_list_size = len(list);
   for(int i = 0; i < the_list_size; ++i) (void)do_add(self,list[i]);
   return object(self); // return node_ptr as python object, relies class_<Node>... for type registration
}

object NodeUtil::do_add(node_ptr self, const bp::object& arg){
   //std::cout << "do_add " << self->name() << "\n";
   if (arg.ptr() == object().ptr())  return object(self); // *IGNORE* None
   if (extract<Edit>(arg).check()) {
      Edit edit = extract<Edit>(arg);
      const std::vector<Variable>& vec = edit.variables();
      for(const auto & i : vec) self->addVariable(i);
   }
   else if (extract<node_ptr>(arg).check()) {
      // std::cout << "  do_add node_ptr\n";
      NodeContainer* nc = self->isNodeContainer();
      if (!nc) throw std::runtime_error("ExportNode::add() : Can only add a child to Suite or Family");
      node_ptr child = extract<node_ptr>(arg);
      nc->addChild(child);
   }
   else if (extract<Event>(arg).check())     self->addEvent(extract<Event>(arg));
   else if (extract<Meter>(arg).check())     self->addMeter(extract<Meter>(arg));
   else if (extract<Label>(arg).check())     self->addLabel(extract<Label>(arg));
   else if (extract<Trigger>(arg).check()){Trigger t = extract<Trigger>(arg); self->py_add_trigger_expr(t.expr());}
   else if (extract<Complete>(arg).check()){Complete t = extract<Complete>(arg);self->py_add_complete_expr(t.expr());}
   else if (extract<Limit>(arg).check())     self->addLimit(extract<Limit>(arg));
   else if (extract<InLimit>(arg).check())   self->addInLimit(extract<InLimit>(arg));
   else if (extract<DayAttr>(arg).check())   self->addDay(extract<DayAttr>(arg));
   else if (extract<DateAttr>(arg).check())  self->addDate(extract<DateAttr>(arg));
   else if (extract<TodayAttr>(arg).check()) self->addToday(extract<TodayAttr>(arg));
   else if (extract<TimeAttr>(arg).check())  self->addTime(extract<TimeAttr>(arg));
   else if (extract<CronAttr>(arg).check())  self->addCron(extract<CronAttr>(arg));
   else if (extract<LateAttr>(arg).check())  self->addLate(extract<LateAttr>(arg));
   else if (extract<ZombieAttr>(arg).check())self->addZombie(extract<ZombieAttr>(arg));
   else if (extract<RepeatDate>(arg).check())self->addRepeat(Repeat(extract<RepeatDate>(arg)  ));
   else if (extract<RepeatInteger>(arg).check())self->addRepeat(Repeat(extract<RepeatInteger>(arg)  ));
   else if (extract<RepeatEnumerated>(arg).check())self->addRepeat(Repeat(extract<RepeatEnumerated>(arg)  ));
   else if (extract<RepeatString>(arg).check())self->addRepeat(Repeat(extract<RepeatString>(arg)  ));
   else if (extract<RepeatDay>(arg).check())self->addRepeat(Repeat(extract<RepeatDay>(arg)  ));
   else if (extract<AutoCancelAttr>(arg).check())self->addAutoCancel(extract<AutoCancelAttr>(arg));
   else if (extract<VerifyAttr>(arg).check())self->addVerify(extract<VerifyAttr>(arg));
   else if (extract<Defstatus>(arg).check()){Defstatus t = extract<Defstatus>(arg);self->addDefStatus(t.state());}
   else if (extract<bp::list>(arg).check()){
      //std::cout << "  do_add list\n";
      bp::list the_list  = extract<bp::list>(arg);
      int the_list_size = len(the_list);
      for(int i = 0; i < the_list_size; ++i) (void) do_add(self,the_list[i]); // recursive
   }
   else if (extract<ClockAttr>(arg).check()) {
      if (!self->isSuite() ) throw std::runtime_error("ExportNode::add() : Can only add a clock to a suite");
      self->isSuite()->addClock( extract<ClockAttr>(arg));
   }
   else if (extract<Variable>(arg).check())       self->addVariable(extract<Variable>(arg) );
   else if (extract<dict>(arg).check()){dict d = extract<dict>(arg); add_variable_dict(self,d);}
   else throw std::runtime_error("ExportNode::add : Unknown type ");
   return object(self);
}
