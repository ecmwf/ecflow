//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #33 $ 
//
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description :
//============================================================================
#include <boost/lexical_cast.hpp>

#include "Node.hpp"
#include "ExprAst.hpp"
#include "Stl.hpp"
#include "NState.hpp"
#include "Ecf.hpp"
#include "Str.hpp"

using namespace ecf;
using namespace std;

void Node::changeVariable(const std::string& name,const std::string& value)
{
	size_t theSize = vars_.size();
	for(size_t i = 0; i < theSize; i++) {
		if (vars_[i].name() == name) {
			vars_[i].set_value( value );
			variable_change_no_ = Ecf::incr_state_change_no();
			return;
		}
	}
 	throw std::runtime_error("Node::changeVariable: Could not find variable " + name);
}

bool Node::set_event(const std::string& event_name_or_number ,bool value)
{
   if (events_.empty()) {
      return false;
   }

   // find by name first
   size_t theSize = events_.size();
   for(size_t i = 0; i < theSize; i++)   {
      if (events_[i].name() == event_name_or_number) {
         events_[i].set_value( value );
         return true;
      }
   }

   // Test for numeric, and then casting, is ****faster***** than relying on exception alone
   if ( event_name_or_number.find_first_of( Str::NUMERIC(), 0 ) != std::string::npos ) {
      try {
         int eventNumber = boost::lexical_cast< int >( event_name_or_number );
         for(size_t i = 0; i < theSize; i++)   {
            if (events_[i].number() == eventNumber) {
               events_[i].set_value( value );
               return true;;
            }
         }
      }
      catch ( boost::bad_lexical_cast&) {}
   }
   return false;
}

bool Node::set_event_used_in_trigger(const std::string& event_name_or_number)
{
   if (events_.empty()) {
      return false;
   }

   // find by name first
   size_t theSize = events_.size();
   for(size_t i = 0; i < theSize; i++)   {
      if (events_[i].name() == event_name_or_number) {
         events_[i].usedInTrigger( true );
         return true;
      }
   }

   // Test for numeric, and then casting, is ****faster***** than relying on exception alone
   if ( event_name_or_number.find_first_of( Str::NUMERIC(), 0 ) != std::string::npos ) {
      try {
         int eventNumber = boost::lexical_cast< int >( event_name_or_number );
         for(size_t i = 0; i < theSize; i++)   {
            if (events_[i].number() == eventNumber) {
               events_[i].usedInTrigger( true );
               return true;;
            }
         }
      }
      catch ( boost::bad_lexical_cast&) {}
   }
   return false;
}
void Node::changeEvent(const std::string& event_name_or_number,const std::string& setOrClear)
{
   bool value;
   if (!setOrClear.empty()) {
      if (setOrClear != Event::SET() && setOrClear != Event::CLEAR() ) {
         throw std::runtime_error("Node::changeEvent: Expected empty string, 'set' or 'clear' but found " + setOrClear + " for event " + event_name_or_number);
      }
      value = (setOrClear == Event::SET());
   }
   else  value = true;

   changeEvent(event_name_or_number,value);
}

void Node::changeEvent(const std::string& event_name_or_number,bool value)
{
   if (set_event(event_name_or_number,value))  return;
 	throw std::runtime_error("Node::changeEvent: Could not find event " + event_name_or_number);
}

bool Node::set_meter(const std::string& meter_name,int value)
{
   size_t the_meter_size = meters_.size();
   for(size_t i = 0; i < the_meter_size ; ++i) {
      if (meters_[i].name() == meter_name) {
         meters_[i].set_value( value);
         return true;
      }
   }
   return false;
}
bool Node::set_meter_used_in_trigger(const std::string& meter_name)
{
   size_t the_meter_size = meters_.size();
    for(size_t i = 0; i < the_meter_size ; ++i) {
       if (meters_[i].name() == meter_name ) {
          meters_[i].usedInTrigger( true );
          return true;
       }
    }
    return false;
}
void Node::changeMeter(const std::string& meter_name,const std::string& value)
{
   int theValue = 0;
   try {
      theValue = boost::lexical_cast< int >( value );
   }
   catch ( boost::bad_lexical_cast& ) {
      throw std::runtime_error( "Node::changeMeter expected integer value but found " + value);
   }
   changeMeter(meter_name,theValue);
}
void Node::changeMeter(const std::string& meter_name,int value)
{
   if (set_meter(meter_name,value)) return;
 	throw std::runtime_error("Node::changeMeter: Could not find meter " + meter_name);
}

void Node::changeLabel(const std::string& name,const std::string& value)
{
   size_t theSize = labels_.size();
   for(size_t i = 0; i < theSize; i++) {
      if (labels_[i].name() == name) {
         labels_[i].set_new_value( value );
         return;
      }
   }
   throw std::runtime_error("Node::changeLabel: Could not find label " + name);
}

void Node::changeTrigger(const std::string& expression)
{
   (void)parse_and_check_expressions(expression,true/*trigger*/,"Node::changeTrigger:" ); // will throw for errors
	deleteTrigger();
	add_trigger( expression );
}

void Node::changeComplete(const std::string& expression)
{
   (void)parse_and_check_expressions(expression,false/*complete*/,"Node::changeComplete:" ); // will throw for errors
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
	d_st_.setState( DState::toState(theState) );
}

void Node::changeLate(const ecf::LateAttr& late)
{
   late_ = std::make_unique<ecf::LateAttr>(late);
   state_change_no_ = Ecf::incr_state_change_no();
}
