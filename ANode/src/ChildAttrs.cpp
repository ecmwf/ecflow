/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #285 $
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
#include <assert.h>
#include <boost/bind.hpp>

#include "ChildAttrs.hpp"
#include "Str.hpp"
#include "Ecf.hpp"
#include "Memento.hpp"

using namespace ecf;
using namespace std;

///////////////////////////////////////////////////////////////////////////////////////////

ChildAttrs::ChildAttrs(const ChildAttrs& rhs)
: meters_(rhs.meters_),
  events_(rhs.events_),
  labels_(rhs.labels_),
  node_(NULL)
{
  // users must call set_node() afterwards
}

void ChildAttrs::begin()
{
   for(size_t i = 0; i < meters_.size(); i++)     {   meters_[i].reset(); }
   for(size_t i = 0; i < events_.size(); i++)     {   events_[i].reset(); }
   for(size_t i = 0; i < labels_.size(); i++)     {   labels_[i].reset(); }
}

void ChildAttrs::requeue()
{
   for(size_t i = 0; i < meters_.size(); i++)     {   meters_[i].reset(); }
   for(size_t i = 0; i < events_.size(); i++)     {   events_[i].reset(); }

   // ECFLOW-195, only clear labels, if they are on Suites/Family not tasks(typically only specified on tasks)
   if (node_ && node_->isNodeContainer()) {
      for(size_t i = 0; i < labels_.size(); i++)  {   labels_[i].reset(); }
   }
}

void ChildAttrs::requeue_labels()
{
   // ECFLOW-195, clear labels before a task is run.
   for(size_t i = 0; i < labels_.size(); i++)  {   labels_[i].reset(); }
}

void ChildAttrs::sort_attributes( ecf::Attr::Type attr)
{
   switch ( attr ) {
      case Attr::EVENT:
         sort(events_.begin(),events_.end(),boost::bind(Str::caseInsLess,
                                   boost::bind(&Event::name_or_number,_1),
                                   boost::bind(&Event::name_or_number,_2)));
         break;
      case Attr::METER:
         sort(meters_.begin(),meters_.end(),boost::bind(Str::caseInsLess,
                                   boost::bind(&Meter::name,_1),
                                   boost::bind(&Meter::name,_2)));
         break;
      case Attr::LABEL:
         sort(labels_.begin(),labels_.end(),boost::bind(Str::caseInsLess,
                                   boost::bind(&Label::name,_1),
                                   boost::bind(&Label::name,_2)));
         break;
      case Attr::LIMIT:    break;
      case Attr::VARIABLE: break;
      case Attr::UNKNOWN:  break;
      default:             break;
   }
}

bool ChildAttrs::set_event_used_in_trigger(const std::string& event_name_or_number)
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

bool ChildAttrs::set_meter_used_in_trigger(const std::string& meter_name)
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

bool ChildAttrs::set_event( const std::string& event_name_or_number)  {
   BOOST_FOREACH(Event& e, events_) {
      if (e.name_or_number() == event_name_or_number) {
         e.set_value( true );
         return true;
      }
   }
   return false;
}
bool ChildAttrs::set_event(const std::string& event_name_or_number ,bool value)
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

bool ChildAttrs::set_meter(const std::string& meter_name,int value)
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

bool ChildAttrs::clear_event(const std::string& event_name_or_number ){
   BOOST_FOREACH(Event& e, events_) {
      if (e.name_or_number() == event_name_or_number) {
         e.set_value( false );
         return true;
      }
   }
   return false;
}

void ChildAttrs::changeLabel(const std::string& name,const std::string& value)
{
   size_t theSize = labels_.size();
   for(size_t i = 0; i < theSize; i++) {
      if (labels_[i].name() == name) {
         labels_[i].set_new_value( value );
         return;
      }
   }
   throw std::runtime_error("ChildAttrs::changeLabel: Could not find label " + name);
}

void ChildAttrs::changeEvent(const std::string& event_name_or_number,const std::string& setOrClear)
{
   bool value;
   if (!setOrClear.empty()) {
      if (setOrClear != Event::SET() && setOrClear != Event::CLEAR() ) {
         throw std::runtime_error("ChildAttrs::changeEvent: Expected empty string, 'set' or 'clear' but found " + setOrClear + " for event " + event_name_or_number);
      }
      value = (setOrClear == Event::SET());
   }
   else  value = true;

   changeEvent(event_name_or_number,value);
}
void ChildAttrs::changeEvent(const std::string& event_name_or_number,bool value)
{
   if (set_event(event_name_or_number,value))  return;
   throw std::runtime_error("ChildAttrs::changeEvent: Could not find event " + event_name_or_number);
}

void ChildAttrs::changeMeter(const std::string& meter_name,const std::string& value)
{
   int theValue = 0;
   try {
      theValue = boost::lexical_cast< int >( value );
   }
   catch ( boost::bad_lexical_cast& ) {
      throw std::runtime_error( "ChildAttrs::changeMeter expected integer value but found " + value);
   }
   changeMeter(meter_name,theValue);
}
void ChildAttrs::changeMeter(const std::string& meter_name,int value)
{
   if (set_meter(meter_name,value)) return;
   throw std::runtime_error("ChildAttrs::changeMeter: Could not find meter " + meter_name);
}

std::ostream& ChildAttrs::print(std::ostream& os) const
{
   BOOST_FOREACH(const Label& la, labels_ )  { la.print(os); }
   BOOST_FOREACH(const Meter& m, meters_ )   { m.print(os); }
   BOOST_FOREACH(const Event& e, events_ )   { e.print(os); }
   return os;
}

bool ChildAttrs::operator==(const ChildAttrs& rhs) const
{
   if (labels_.size() != rhs.labels_.size()) {
#ifdef DEBUG
      if (Ecf::debug_equality()) {
         std::cout << "ChildAttrs::operator==  (labels_.size() != rhs.labels_.size()) " << node_->debugNodePath() << "\n";
      }
#endif
      return false;
   }
   for(unsigned i = 0; i < labels_.size(); ++i) {
      if (labels_[i] != rhs.labels_[i]) {
#ifdef DEBUG
         if (Ecf::debug_equality()) {
            std::cout << "ChildAttrs::operator==  (labels_[i] != rhs.labels_[i]) " << node_->debugNodePath() << "\n";
            std::cout << "   lhs = " << labels_[i].dump() << "\n";
            std::cout << "   rhs = " << rhs.labels_[i].dump() << "\n";
         }
#endif
         return false;
      }
   }

   if (meters_.size() != rhs.meters_.size()) {
#ifdef DEBUG
      if (Ecf::debug_equality()) {
         std::cout << "ChildAttrs::operator==  (meters_.size() != rhs.meters_.size()) " << node_->debugNodePath() << "\n";
      }
#endif
      return false;
   }
   for(size_t i = 0; i < meters_.size(); ++i) {
      if (!(meters_[i] == rhs.meters_[i] )) {
#ifdef DEBUG
         if (Ecf::debug_equality()) {
            std::cout << "ChildAttrs::operator==   (!(meters_[i] == rhs.meters_[i] )) " << node_->debugNodePath() << "\n";
         }
#endif
         return false;
      }
   }
   if (events_.size() != rhs.events_.size()) {
#ifdef DEBUG
      if (Ecf::debug_equality()) {
         std::cout << "ChildAttrs::operator==   (events_.size() != rhs.events_.size()) " << node_->debugNodePath() << "\n";
      }
#endif
      return false;
   }
   for(size_t i = 0; i < events_.size(); ++i) {
      if (!(events_[i] == rhs.events_[i] )) {
#ifdef DEBUG
         if (Ecf::debug_equality()) {
            std::cout << "ChildAttrs::operator==   (!(events_[i] == rhs.events_[i] )) " << node_->debugNodePath() << "\n";
         }
#endif
         return false;
      }
   }
   return true;
}

bool ChildAttrs::getLabelValue(const std::string& labelName, std::string& value) const
{
   size_t theSize = labels_.size();
   for(size_t i = 0; i < theSize; i++) {
      if (labels_[i].name() == labelName) {
         if (!(labels_[i].new_value().empty())) value = labels_[i].new_value();
         else                                   value = labels_[i].value();
         return true;
      }
   }
   return false;
}

bool ChildAttrs::getLabelNewValue(const std::string& labelName, std::string& value) const
{
   size_t theSize = labels_.size();
   for(size_t i = 0; i < theSize; i++) {
      if (labels_[i].name() == labelName) {
         value = labels_[i].new_value();
         return true;
      }
   }
   return false;
}

void ChildAttrs::addLabel( const Label& l)
{
   if (findLabel(l.name())) {
      std::stringstream ss;
      ss << "Add Label failed: Duplicate label of name '" << l.name() << "' already exist for node " << node_->debugNodePath();
      throw std::runtime_error( ss.str() );
   }
   labels_.push_back( l );
   node_->state_change_no_ = Ecf::incr_state_change_no();
}

void ChildAttrs::addMeter( const Meter& m)
{
   const Meter& meter = findMeter(m.name());
   if (!meter.empty()) {
      std::stringstream ss;
      ss << "Add Meter failed: Duplicate Meter of name '" << m.name() << "' already exist for node " << node_->debugNodePath();
      throw std::runtime_error( ss.str() );
   }
   meters_.push_back( m );
   node_->state_change_no_ = Ecf::incr_state_change_no();
}

void ChildAttrs::addEvent( const Event& e)
{
   const Event& event = findEvent(e);
   if (!event.empty()) {
      std::stringstream ss;
      ss << "Add Event failed: Duplicate Event of name '" << e.name_or_number() << "' already exist for node " << node_->debugNodePath();
      throw std::runtime_error( ss.str() );
   }
   events_.push_back( e );
   node_->state_change_no_ = Ecf::incr_state_change_no();
}


void ChildAttrs::deleteEvent(const std::string& name)
{
   if (name.empty()) {
      events_.clear();
      node_->state_change_no_ = Ecf::incr_state_change_no();
#ifdef DEBUG_STATE_CHANGE_NO
      std::cout << "ChildAttrs::deleteEvent\n";
#endif
      return;
   }

   size_t theSize = events_.size();
   for(size_t i = 0; i < theSize; i++) {
      if (events_[i].name_or_number() == name) {
         events_.erase( events_.begin() + i );
         node_->state_change_no_ = Ecf::incr_state_change_no();
#ifdef DEBUG_STATE_CHANGE_NO
         std::cout << "ChildAttrs::deleteEvent\n";
#endif
         return;
      }
   }
   throw std::runtime_error("ChildAttrs::deleteEvent: Can not find event: " + name);
}

void ChildAttrs::deleteMeter(const std::string& name)
{
   if (name.empty()) {
      meters_.clear();
      node_->state_change_no_ = Ecf::incr_state_change_no();
#ifdef DEBUG_STATE_CHANGE_NO
      std::cout << "Expression::clearFree()\n";
#endif
      return;
   }

   size_t theSize = meters_.size();
   for(size_t i = 0; i < theSize; i++) {
      if (meters_[i].name() == name) {
         meters_.erase( meters_.begin() + i );
         node_->state_change_no_ = Ecf::incr_state_change_no();
#ifdef DEBUG_STATE_CHANGE_NO
         std::cout << "Expression::clearFree()\n";
#endif
         return;
      }
   }
   throw std::runtime_error("ChildAttrs::deleteMeter: Can not find meter: " + name);
}

void ChildAttrs::deleteLabel(const std::string& name)
{
   if (name.empty()) {
      labels_.clear();
      node_->state_change_no_ = Ecf::incr_state_change_no();
#ifdef DEBUG_STATE_CHANGE_NO
      std::cout << "ChildAttrs::deleteLabel\n";
#endif
      return;
   }

   size_t theSize = labels_.size();
   for(size_t i = 0; i < theSize; i++) {
      if (labels_[i].name() == name) {
         labels_.erase( labels_.begin() + i );
         node_->state_change_no_ = Ecf::incr_state_change_no();
#ifdef DEBUG_STATE_CHANGE_NO
         std::cout << "ChildAttrs::deleteLabel\n";
#endif
         return;
      }
   }
   throw std::runtime_error("ChildAttrs::deleteLabel: Can not find label: " + name);
}


const Event& ChildAttrs::findEvent(const Event& theEvent) const
{
   size_t theSize = events_.size();
   for(size_t i = 0; i < theSize; i++)   {
      if (events_[i] == theEvent) {
         return events_[i];
      }
   }
   return Event::EMPTY();
}

const Event& ChildAttrs::findEventByNumber(int number) const
{
   size_t theSize = events_.size();
   for(size_t i = 0; i < theSize; i++)   {
      if (events_[i].number() == number) {
         return events_[i];
      }
   }
   return Event::EMPTY();
}

const Event& ChildAttrs::findEventByName( const std::string& event_name) const
{
   size_t theSize = events_.size();
   for(size_t i = 0; i < theSize; i++)   {
      if (events_[i].name() == event_name) {
         return events_[i];
      }
   }
   return Event::EMPTY();
}

const Event& ChildAttrs::findEventByNameOrNumber( const std::string& theName) const
{
   const Event& event = findEventByName(theName);
   if (!event.empty()) {
      return event;
   }

   // Test for numeric, and then casting, is ****faster***** than relying on exception alone
   if ( theName.find_first_of( Str::NUMERIC(), 0 ) != std::string::npos ) {
      try {
         int eventNumber = boost::lexical_cast< int >( theName );
         return findEventByNumber(eventNumber);
      }
      catch ( boost::bad_lexical_cast&) {}
   }
   return Event::EMPTY();
}

const Meter& ChildAttrs::findMeter(const std::string& name) const
{
   size_t theSize = meters_.size();
   for(size_t i = 0; i < theSize; i++) {
      if (meters_[i].name() == name) {
         return meters_[i];
      }
   }
   return Meter::EMPTY();
}

Meter& ChildAttrs::find_meter(const std::string& name)
{
   size_t theSize = meters_.size();
   for(size_t i = 0; i < theSize; i++) {
      if (meters_[i].name() == name) {
         return meters_[i];
      }
   }
   return const_cast<Meter&>(Meter::EMPTY());
}

bool ChildAttrs::findLabel(const std::string& name) const
{
   size_t theSize = labels_.size();
   for(size_t i = 0; i < theSize; i++) {
      if (labels_[i].name() == name) {
         return true;
      }
   }
   return false;
}

const Label& ChildAttrs::find_label(const std::string& name) const
{
   size_t theSize = labels_.size();
   for(size_t i = 0; i < theSize; i++) {
      if (labels_[i].name() == name) {
          return labels_[i];
      }
   }
   return Label::EMPTY();
}


void ChildAttrs::set_memento( const NodeEventMemento* memento) {

#ifdef DEBUG_MEMENTO
   std::cout << "ChildAttrs::set_memento(const NodeEventMemento* memento) " << node_->debugNodePath() << "\n";
#endif

   if (set_event(memento->event_.name_or_number(),  memento->event_.value())) {
      return;
   }
   addEvent( memento->event_);
}

void ChildAttrs::set_memento( const NodeMeterMemento* memento) {

#ifdef DEBUG_MEMENTO
   std::cout << "ChildAttrs::set_memento(const NodeMeterMemento* memento) " << node_->debugNodePath() << "\n";
#endif

   if (set_meter(memento->meter_.name(), memento->meter_.value())) {
      return;
   }
   addMeter(memento->meter_);
}

void ChildAttrs::set_memento( const NodeLabelMemento* memento) {

#ifdef DEBUG_MEMENTO
   std::cout << "ChildAttrs::set_memento(const NodeLabelMemento* memento) " << node_->debugNodePath() << "\n";
#endif

   size_t theSize = labels_.size();
   for(size_t i = 0; i < theSize; i++) {
      if (labels_[i].name() == memento->label_.name()) {
         labels_[i] = memento->label_;
         return;
      }
   }
   addLabel(memento->label_);
}
