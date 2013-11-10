//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #33 $ 
//
// Copyright 2009-2012 ECMWF. 
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description :
//============================================================================
#include <boost/lexical_cast.hpp>

#include "Suite.hpp"
#include "ExprAst.hpp"
#include "Stl.hpp"
#include "NState.hpp"
#include "Ecf.hpp"
#include "Str.hpp"

using namespace ecf;
using namespace std;

void Node::changeVariable(const std::string& name,const std::string& value)
{
	size_t theSize = varVec_.size();
	for(size_t i = 0; i < theSize; i++) {
		if (varVec_[i].name() == name) {
			varVec_[i].set_value( value );
			variable_change_no_ = Ecf::incr_state_change_no();
			return;
		}
	}
 	throw std::runtime_error("Node::changeVariable: Could not find variable " + name);
}

void Node::changeClockType(const std::string& clockType)
{
   // ISSUES:
   // If we have a clock, then changing the date, can only have an effect, is suite is re-queued
   // This then initialises the calendar with the clock attribute
   if (clockType != "hybrid" && clockType != "real") {
		throw std::runtime_error("Node::changeClockType: expected clock type to be 'hybrid' or 'real'  but found " + clockType);
	}

   clock_ptr clock = suite()->clockAttr();
	if (!clock.get()) {
	   suite()->addClock( ClockAttr( clockType == "hybrid") ); // will update state change_no
	   return;
	}

	clock->hybrid( clockType == "hybrid" ); // will update state change_no

#ifdef DEBUG_STATE_CHANGE_NO
	std::cout << "Node::changeClockType\n";
#endif
}

void Node::changeClockDate(const std::string& theDate)
{
   // See ISSUES: Node::changeClockType
   int day,month,year;
   DateAttr::getDate(theDate,day,month,year);
   if (day == 0 || month == 0 || year == 0)  throw std::runtime_error("Node::changeClockDate Invalid clock date:" + theDate );

   clock_ptr clock = suite()->clockAttr();
   if (!clock.get())  {
      suite()->addClock( ClockAttr(day,month,year) ); // will update state change_no
      return;
   }

   clock->date(day,month,year); // this will check the date and update state change_no

#ifdef DEBUG_STATE_CHANGE_NO
   std::cout << "Node::changeClockType\n";
#endif
}

void Node::changeClockGain(const std::string& gain)
{
   // See: ISSUES on Node::changeClockDate
	long theGain = 0;
 	try {
 		theGain = boost::lexical_cast< long >( gain );
	}
	catch ( boost::bad_lexical_cast& ) {
 		throw std::runtime_error( "Node::changeClockGain: value '" + gain + "' is not convertible to an long, for suite " + suite()->name());
	}

	clock_ptr clock = suite()->clockAttr();
   if (!clock.get())  {
      suite()->addClock( ClockAttr() ); // will update state change_no
      clock = suite()->clockAttr();
   }

	if (theGain > 0) {
		clock->set_gain_in_seconds( theGain, true);  // will update state change_no
	}
	else {
		clock->set_gain_in_seconds( theGain, false); // will update state change_no
	}

#ifdef DEBUG_STATE_CHANGE_NO
	std::cout << "Node::changeClockGain\n";
#endif
}

void Node::changeClockSync()
{
   clock_ptr clock = suite()->clockAttr();
   if (!clock.get())  {
      suite()->addClock( ClockAttr() ); // will update state change_no
   }
   else {
      // clear so that on re-queue we sync with computer
      clock->sync();
   }

#ifdef DEBUG_STATE_CHANGE_NO
   std::cout << "Node::changeClockSync\n";
#endif
}

bool Node::set_event(const std::string& event_name_or_number ,bool value)
{
   if (child_attrs_) return child_attrs_->set_event(event_name_or_number,value);
	return false;
}

bool Node::set_event_used_in_trigger(const std::string& event_name_or_number)
{
   if (child_attrs_) return child_attrs_->set_event_used_in_trigger(event_name_or_number);
	return false;
}
void Node::changeEvent(const std::string& event_name_or_number,const std::string& setOrClear)
{
   if (child_attrs_) return child_attrs_->changeEvent(event_name_or_number,setOrClear);
}
void Node::changeEvent(const std::string& event_name_or_number,bool value)
{
   if (child_attrs_) {
      child_attrs_->changeEvent(event_name_or_number,value);
      return;
   }
 	throw std::runtime_error("Node::changeEvent: Could not find event " + event_name_or_number);
}

bool Node::set_meter(const std::string& meter_name,int value)
{
   if (child_attrs_) return child_attrs_->set_meter(meter_name,value);
	return false;
}
bool Node::set_meter_used_in_trigger(const std::string& meter_name)
{
   if (child_attrs_) return child_attrs_->set_meter_used_in_trigger(meter_name);
	return false;
}
void Node::changeMeter(const std::string& meter_name,const std::string& value)
{
   if (child_attrs_) {
      child_attrs_->changeMeter(meter_name,value);
      return;
   }
   throw std::runtime_error("Node::changeMeter: Could not find meter " + meter_name);
}
void Node::changeMeter(const std::string& meter_name,int value)
{
   if (child_attrs_) {
      child_attrs_->changeMeter(meter_name,value);
      return;
   }
 	throw std::runtime_error("Node::changeMeter: Could not find meter " + meter_name);
}

void Node::changeLabel(const std::string& name,const std::string& value)
{
   if (child_attrs_) {
      child_attrs_->changeLabel(name,value);
      return;
   }
	throw std::runtime_error("Node::changeLabel: Could not find label " + name);
}

void Node::changeTrigger(const std::string& expression)
{
	PartExpression part(expression);
 	string parseErrorMsg;
	std::auto_ptr<AstTop> ast = part.parseExpressions( parseErrorMsg );
	if (!ast.get()) {
		std::stringstream ss;
		ss << "Node::changeTrigger: Failed to parse expression '" << expression << "'.  " << parseErrorMsg;
		throw std::runtime_error( ss.str() );
	}

	deleteTrigger();
	add_trigger( expression );
}

void Node::changeComplete(const std::string& expression)
{
	PartExpression part(expression);
 	string parseErrorMsg;
	std::auto_ptr<AstTop> ast = part.parseExpressions( parseErrorMsg );
	if (!ast.get()) {
		std::stringstream ss;
		ss << "Node::changeComplete: Failed to parse expression '" << expression << "'.  " << parseErrorMsg;
		throw std::runtime_error( ss.str() );
	}

	deleteComplete();
	add_complete( expression );
}

void Node::changeRepeat(const std::string& value)
{
	if (repeat_.empty())  throw std::runtime_error("Node::changeRepeat: Could not find repeat on " + absNodePath());
 	repeat_.change( value ); // this can throw std::runtime_error
}

void Node::increment_repeat()
{
   if (repeat_.empty())  throw std::runtime_error("Node::increment_repeat: Could not find repeat on " + absNodePath());
   repeat_.increment();
}

void Node::changeLimitMax(const std::string& name,const std::string& maxValue)
{
	int theValue = 0;
 	try {
 		theValue = boost::lexical_cast< int >( maxValue );
	}
	catch ( boost::bad_lexical_cast& ) {
		throw std::runtime_error( "Node::changeLimitMax expected integer value but found " + maxValue);
	}
	changeLimitMax(name, theValue);
}

void Node::changeLimitMax(const std::string& name,int maxValue)
{
	limit_ptr limit = find_limit(name);
	if (!limit.get())  throw std::runtime_error("Node::changeLimitMax: Could not find limit " + name);
	limit->setLimit( maxValue );
}

void Node::changeLimitValue(const std::string& name,const std::string& value)
{
	int theValue = 0;
 	try {
 		theValue = boost::lexical_cast< int >( value );
	}
	catch ( boost::bad_lexical_cast& ) {
		throw std::runtime_error( "Node::changeLimitValue expected integer value but found " + value);
	}
	changeLimitValue(name,theValue);
}

void Node::changeLimitValue(const std::string& name,int value)
{
	limit_ptr limit = find_limit(name);
	if (!limit.get())  throw std::runtime_error("Node::changeLimitValue: Could not find limit " + name);
	limit->setValue( value );
}

void Node::changeDefstatus(const std::string& theState)
{
	if (!DState::isValid(theState)) {
		throw std::runtime_error( "Node::changeDefstatus expected a state but found " + theState);
	}

	// Updates state_change_no on the defStatus
	defStatus_.setState( DState::toState(theState) );
}
