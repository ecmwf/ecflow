/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #53 $ 
//
// Copyright 2009-2012 ECMWF. 
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
#include <boost/make_shared.hpp>

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
#include "NodeAttrDoc.hpp"
#include "BoostPythonUtil.hpp"

using namespace ecf;
using namespace boost::python;
using namespace std;

// See: http://wiki.python.org/moin/boost.python/HowTo#boost.function_objects

void add_time_series_3(CronAttr* self,const std::string& ts) { self->addTimeSeries(TimeSeries::create(ts));}

void set_week_days(CronAttr* cron,const boost::python::list& list)
{
   std::vector<int> int_vec;
   BoostPythonUtil::list_to_int_vec(list,int_vec);
   cron->addWeekDays(int_vec);
}

void set_days_of_month(CronAttr* cron,const boost::python::list& list)
{
   std::vector<int> int_vec;
   BoostPythonUtil::list_to_int_vec(list,int_vec);
   cron->addDaysOfMonth(int_vec);
}

void set_months(CronAttr* cron,const boost::python::list& list)
{
   std::vector<int> int_vec;
   BoostPythonUtil::list_to_int_vec(list,int_vec);
   cron->addMonths(int_vec);
}

// Create as shared because: we want to pass a Python list as part of the constructor,
// *AND* the only way make_constructor works is with a pointer.
// The Node::add function seem to cope with this, some boost python magic,must do a conversion
// from shared_ptr to pass by reference
static boost::shared_ptr<RepeatEnumerated> create_RepeatEnumerated(const std::string& name, const boost::python::list& list)
{
   std::vector<std::string> vec;
   BoostPythonUtil::list_to_str_vec(list,vec);
   return boost::make_shared<RepeatEnumerated>( name,vec );
}
static boost::shared_ptr<RepeatString> create_RepeatString(const std::string& name, const boost::python::list& list)
{
   std::vector<std::string> vec;
   BoostPythonUtil::list_to_str_vec(list,vec);
   return boost::make_shared<RepeatString>( name,vec );
}

static boost::shared_ptr<ZombieAttr> create_ZombieAttr(Child::ZombieType zt,const boost::python::list& list,User::Action uc,int life_time_in_server)
{
   std::vector<Child::CmdType> vec;
   int the_list_size = len(list);
   vec.reserve(the_list_size);
   for (int i = 0; i < the_list_size; ++i) {
      vec.push_back(boost::python::extract<Child::CmdType>(list[i]));
   }

   return boost::make_shared<ZombieAttr>(zt,vec,uc,life_time_in_server );
}

void export_NodeAttr()
{
	enum_<Child::ZombieType>("ZombieType", NodeAttrDoc::zombie_type_doc())
	.value("ecf",   Child::ECF)
	.value("user",  Child::USER)
	.value("path",  Child::PATH)
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
	.value("abort",   Child::ABORT)
	.value("complete",Child::COMPLETE)
 	;

	// 	ZombieAttr(ecf::Child::ZombieType t, const std::vector<ecf::Child::CmdType>& c, ecf::User::Action a, int zombie_lifetime);
 	class_<ZombieAttr>("ZombieAttr",NodeAttrDoc::zombie_doc())
   .def("__init__",make_constructor(&create_ZombieAttr) )
 	.def("__str__",    &ZombieAttr::toString)              // __str__
 	.def(self == self )                                    // __eq__
 	.def("empty",          &ZombieAttr::empty,          "Return true if the attribute is empty")
 	.def("zombie_type",    &ZombieAttr::zombie_type,    "Returns the :term:`zombie type`")
 	.def("user_action",    &ZombieAttr::action,         "The automated action to invoke, when zombies arise")
 	.def("zombie_lifetime",&ZombieAttr::zombie_lifetime,"Returns the lifetime in seconds of :term:`zombie` in the server")
   .add_property( "child_cmds",boost::python::range(&ZombieAttr::child_begin,&ZombieAttr::child_end),"The list of child commands. If empty action applies to all child cmds")
   ;

 	class_<Variable>("Variable",NodeAttrDoc::variable_doc(),init<std::string, std::string>())
 	.def("__str__",    &Variable::toString)                // __str__
 	.def(self == self )                                    // __eq__
	.def("name",     &Variable::name,     return_value_policy<copy_const_reference>(), "Return the variable name as string")
   .def("value",    &Variable::theValue, return_value_policy<copy_const_reference>(), "Return the variable value as a string")
   .def("empty",    &Variable::empty,   "Return true if the variable is empty. Used when returning a Null variable, from a find")
 	;

   // We need to return pass a list of Variable as arguments, to retrieve the generated variables
   class_<std::vector<Variable> >("VariableList", "Hold a list of Variables")
               .def(vector_indexing_suite<std::vector<Variable> >()) ;


	class_<Label>("Label",NodeAttrDoc::label_doc(),init<std::string, std::string>())
	.def(self == self )                                    // __eq__
	.def("__str__",   &Label::toString)                    // __str__
	.def("name",      &Label::name,      return_value_policy<copy_const_reference>(), "Return the :term:`label` name as string")
	.def("value",     &Label::value,     return_value_policy<copy_const_reference>(), "Return the original :term:`label` value as string")
	.def("new_value", &Label::new_value, return_value_policy<copy_const_reference>(), "Return the new label value as string")
   .def("empty",     &Label::empty,     "Return true if the Label is empty. Used when returning a NULL Label, from a find")
  	;

	class_<Limit,  boost::shared_ptr<Limit> >("Limit",NodeAttrDoc::limit_doc(),init<std::string, int>())
	.def(self == self )                               // __eq__
	.def("__str__",  &Limit::toString)                // __str__
	.def("name",     &Limit::name, return_value_policy<copy_const_reference>(), "Return the :term:`limit` name as string")
   .def("value",    &Limit::value,    "The :term:`limit` token value as an integer")
   .def("limit",    &Limit::theLimit, "The max value of the :term:`limit` as an integer")
   .add_property("node_paths", boost::python::range(&Limit::paths_begin,&Limit::paths_begin),"List of nodes(paths) that have consumed a limit")
	;

	class_<InLimit>("InLimit",NodeAttrDoc::inlimit_doc(),init<std::string,  std::string, optional<int> >())
	.def( init<std::string,std::string> () )
	.def( init<std::string> () )
	.def(self == self )                                  // __eq__
	.def("__str__",     &InLimit::toString)              // __str__
	.def("name",        &InLimit::name,       return_value_policy<copy_const_reference>(), "Return the :term:`inlimit` name as string")
	.def("path_to_node",&InLimit::pathToNode, return_value_policy<copy_const_reference>(), "Path to the node that holds the limit, can be empty")
	.def("tokens",      &InLimit::tokens,                                                  "The number of token to consume from the Limit")
	;

	class_<Event>("Event",NodeAttrDoc::event_doc(), init<int, optional<std::string> >())
   .def( init<std::string> () )
	.def(self == self )                                  // __eq__
	.def("__str__",     &Event::toString)                // __str__
	.def("name",        &Event::name,       return_value_policy<copy_const_reference>(), "Return the Events name as string. If number supplied name may be empty.")
	.def("number",      &Event::number,     "Return events number as a integer. if not specified return max integer value")
	.def("value",       &Event::value,      "Return events current value")
   .def("empty",       &Event::empty,      "Return true if the Event is empty. Used when returning a NULL Event, from a find")
	;

	class_<Meter>("Meter",NodeAttrDoc::meter_doc(),init<std::string,int,int,optional<int> >())
 	.def(self == self )                                  // __eq__
	.def("__str__",     &Meter::toString)                // __str__
	.def("name",        &Meter::name,       return_value_policy<copy_const_reference>(), "Return the Meters name as string")
	.def("min",         &Meter::min,                                                     "Return the Meters minimum value")
	.def("max",         &Meter::max,                                                     "Return the Meters maximum value")
	.def("value",       &Meter::value,                                                   "Return meters current value")
	.def("color_change",&Meter::colorChange,                                             "returns the color change")
   .def("empty",       &Meter::empty,       "Return true if the Meter is empty. Used when returning a NULL Meter, from a find")
	;

	class_<DateAttr>("Date",NodeAttrDoc::date_doc() ,init<int,int,int>())  // day,month,year
	.def(self == self )                                     // __eq__
	.def("__str__",     &DateAttr::toString)                // __str__
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
	.def(self == self )                                    // __eq__
	.def("__str__",     &DayAttr::toString)                // __str__
	.def("day",         &DayAttr::day,      "Return the day as enumerator")
	;

	class_<TimeAttr>("Time",NodeAttrDoc::time_doc() ,init<TimeSlot, optional<bool> >())
	.def( init<int,int,optional<bool> >())                  // hour, minute, relative
 	.def( init<TimeSeries>())
	.def( init<TimeSlot,TimeSlot,TimeSlot,bool>())
	.def(self == self )                           // __eq__
	.def("__str__",    &TimeAttr::toString)       // __str__
	.def("time_series",&TimeAttr::time_series,return_value_policy<copy_const_reference>(), "Return the Time attributes time series")
	;

	class_<TodayAttr>("Today",NodeAttrDoc::today_doc() ,init<TimeSlot, optional<bool> >())
	.def( init<int,int,optional<bool> >())                  // hour, minute, relative
 	.def( init<TimeSeries>())
	.def( init<TimeSlot,TimeSlot,TimeSlot,bool>())
	.def(self == self )                                     // __eq__
	.def("__str__",    &TodayAttr::toString)                // __str__
	.def("time_series",&TodayAttr::time_series,return_value_policy<copy_const_reference>(), "Return the Todays time series")
	;


	class_<LateAttr, boost::shared_ptr<LateAttr>  >("Late",NodeAttrDoc::late_doc())
 	.def( "submitted", &LateAttr::addSubmitted,
 	      "submitted(TimeSlot):The time node can stay :term:`submitted`. Submitted is always relative. If the node stays\n"
 	      "submitted longer than the time specified, the :term:`late` flag is set\n"
 	)
	.def( "submitted", &LateAttr::add_submitted,
	      "submitted(hour,minute) The time node can stay submitted. Submitted is always relative. If the node stays\n"
	      "submitted longer than the time specified, the late flag is set\n"
	 )
	.def( "active",    &LateAttr::add_active,
	      "active(hour,minute): The time the node must become :term:`active`. If the node is still :term:`queued` or :term:`submitted`\n"
	      "by the time specified, the late flag is set"
	 )
	 .def( "active",   &LateAttr::addActive,
	       "active(TimeSlot):The time the node must become :term:`active`. If the node is still :term:`queued` or :term:`submitted`\n"
	       "by the time specified, the late flag is set"
	 )
	.def( "complete",  &LateAttr::add_complete,
	      "complete(hour,minute):The time the node must become :term:`complete`. If relative, time is taken from the time\n"
	      "the node became :term:`active`, otherwise node must be :term:`complete` by the time given"
	 )
	 .def( "complete", &LateAttr::addComplete,
	       "complete(TimeSlot): The time the node must become :term:`complete`. If relative, time is taken from the time\n"
	       "the node became :term:`active`, otherwise node must be :term:`complete` by the time given"
	 )
	.def(self == self )                                  // __eq__
	.def("__str__",   &LateAttr::toString)               // __str__
	.def("submitted", &LateAttr::submitted,return_value_policy<copy_const_reference>(), "Return the submitted time as a TimeSlot")
	.def("active",    &LateAttr::active,   return_value_policy<copy_const_reference>(), "Return the active time as a TimeSlot")
	.def("complete",  &LateAttr::complete, return_value_policy<copy_const_reference>(), "Return the complete time as a TimeSlot")
   .def("complete_is_relative",  &LateAttr::complete_is_relative, "Returns a boolean where true means that complete is relative")
   .def("is_late",   &LateAttr::isLate, "Return True if late")
 	;


	class_<AutoCancelAttr, boost::shared_ptr<AutoCancelAttr> >(
			"Autocancel",NodeAttrDoc::autocancel_doc() ,
			init<int,int,bool >()                             // hour, minute, relative
	)
	.def( init<int>())                                        // days
	.def( init<TimeSlot, bool>())
	.def(self == self )                                       // __eq__
	.def("__str__", &AutoCancelAttr::toString)                // __str__
	.def("time",    &AutoCancelAttr::time, return_value_policy<copy_const_reference>(), "returns cancel time as a TimeSlot")
	.def("relative",&AutoCancelAttr::relative, "Returns a boolean where true means the time is relative")
	.def("days",    &AutoCancelAttr::days,     "Returns a boolean true if time was specified in days")
  	;


	class_<RepeatDate >("RepeatDate",NodeAttrDoc::repeat_date_doc() ,init< std::string, int, int, optional<int> >()) // name, start, end , delta
 	.def(self == self )                              // __eq__
	.def("__str__",        &RepeatDate::toString)    // __str__
	.def("name",           &RepeatDate::name, return_value_policy<copy_const_reference>(),"Return the name of the repeat.")
	.def("start",          &RepeatDate::start ,"Return the start date as an integer in yyyymmdd format")
	.def("end",            &RepeatDate::end,   "Return the end date as an integer in yyyymmdd format")
	.def("step",           &RepeatDate::step,  "Return the step increment. This is used to update the repeat, until end date is reached")
	;

	class_<RepeatInteger>("RepeatInteger",NodeAttrDoc::repeat_integer_doc(),init< std::string, int, int, optional<int> >()) // name, start, end , delta = 1
 	.def(self == self )                                  // __eq__
	.def("__str__",        &RepeatInteger::toString)     // __str__
	.def("name",           &RepeatInteger::name, return_value_policy<copy_const_reference>(),"Return the name of the repeat.")
	.def("start",          &RepeatInteger::start)
	.def("end",            &RepeatInteger::end)
	.def("step",           &RepeatInteger::step)
	;

	// Create as shared because: we want to pass a Python list as part of the constructor,
	// and the only way make_constructor works is with a pointer.
	class_<RepeatEnumerated, boost::shared_ptr<RepeatEnumerated> >("RepeatEnumerated",NodeAttrDoc::repeat_enumerated_doc())
   .def("__init__",make_constructor(&create_RepeatEnumerated) )
	.def(self == self )                                     // __eq__
	.def("__str__",        &RepeatEnumerated::toString)     // __str__
	.def("name",           &RepeatEnumerated::name, return_value_policy<copy_const_reference>(),"Return the name of the :term:`repeat`.")
	.def("start",          &RepeatEnumerated::start)
	.def("end",            &RepeatEnumerated::end)
	.def("step",           &RepeatEnumerated::step)
	;

	class_<RepeatString,boost::shared_ptr<RepeatString> >("RepeatString", NodeAttrDoc::repeat_string_doc())
   .def("__init__",make_constructor(&create_RepeatString) )
	.def(self == self )                                 // __eq__
	.def("__str__",        &RepeatString::toString)     // __str__
	.def("name",           &RepeatString::name, return_value_policy<copy_const_reference>(),"Return the name of the :term:`repeat`.")
	.def("start",          &RepeatString::start)
	.def("end",            &RepeatString::end)
	.def("step",           &RepeatString::step)
	;

	class_<RepeatDay>("RepeatDay",NodeAttrDoc::repeat_day_doc(),init< optional<int> >())
 	.def(self == self )                              // __eq__
	.def("__str__",        &RepeatDay::toString)     // __str__
	;

	class_<Repeat>("Repeat",NodeAttrDoc::repeat_doc() ,init< int >())
	.def(self == self )                    // __eq__
	.def("__str__", &Repeat::toString)     // __str__
	.def("empty",   &Repeat::empty ,"Return true if the repeat is empty.")
	.def("name",    &Repeat::name, return_value_policy<copy_const_reference>(), "The :term:`repeat` name, can be referenced in :term:`trigger` expressions")
	.def("start",   &Repeat::start,"The start value of the repeat, as an integer")
	.def("end",     &Repeat::end,  "The last value of the repeat, as an integer")
   .def("step",    &Repeat::step, "The increment for the repeat, as an integer")
   .def("value",   &Repeat::last_valid_value,"The current value of the repeat as an integer")
	;


	void (CronAttr::*add_time_series)(const TimeSeries&) = &CronAttr::addTimeSeries;
	void (CronAttr::*add_time_series_2)( const TimeSlot& s, const TimeSlot& f, const TimeSlot& i) = &CronAttr::addTimeSeries;
	class_<CronAttr>("Cron",NodeAttrDoc::cron_doc() )
	.def(self == self )                                // __eq__
	.def("__str__",            &CronAttr::toString)    // __str__
   .def( "set_week_days",     &set_week_days ,   "Specifies days of week. Expects a list of integers, with integer range 0==Sun to 6==Sat")
	.def( "set_days_of_month", &set_days_of_month,"Specifies days of the month. Expects a list of integers with integer range 1-31" )
	.def( "set_months",        &set_months  ,     "Specifies months. Expects a list of integers, with integer range 1-12")
	.def( "set_time_series",   &CronAttr::add_time_series,"time_series(hour(int),minute(int),relative to suite start(bool=false)), Add a time slot")
	.def( "set_time_series",   add_time_series,   "Add a time series. This will never complete")
   .def( "set_time_series",   add_time_series_2, "Add a time series. This will never complete")
   .def( "set_time_series",   &add_time_series_3,"Add a time series. This will never complete")
	.def( "time",              &CronAttr::time, return_value_policy<copy_const_reference>(), "return cron time as a TimeSeries")
	.add_property( "week_days",    boost::python::range(&CronAttr::week_days_begin,    &CronAttr::week_days_end),     "returns a integer list of week days")
	.add_property( "days_of_month",boost::python::range(&CronAttr::days_of_month_begin,&CronAttr::days_of_month_end), "returns a integer list of days of the month")
	.add_property( "months",       boost::python::range(&CronAttr::months_begin,       &CronAttr::months_end),        "returns a integer list of months of the year")
 	;


	class_<VerifyAttr>("Verify", init<NState::State,int>())  // state, expected
	.def(self == self )                               // __eq__
	.def("__str__",        &VerifyAttr::toString)     // __str__
	;


	void (ClockAttr::*start_stop_with_server)(bool) = &ClockAttr::startStopWithServer;
	class_<ClockAttr, boost::shared_ptr<ClockAttr> >("Clock",NodeAttrDoc::clock_doc() ,init<int,int,int,optional<bool> > ())  // day, month, year, hybrid
   .def( init<int,int,int,bool>())
   .def( init<bool>())
	.def(self == self )                                   // __eq__
	.def("__str__",             &ClockAttr::toString)     // __str__
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
}
