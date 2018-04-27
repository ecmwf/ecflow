#ifndef SUITE_HPP_
#define SUITE_HPP_
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #73 $ 
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

#include "NodeContainer.hpp"
#include "Calendar.hpp"
#include "ClockAttr.hpp"
class SuiteGenVariables;
namespace ecf { class CalendarUpdateParams;  } // forward declare


class Suite : public NodeContainer {
public:
   explicit Suite( const std::string& name )
   : NodeContainer(name),
     defs_(NULL),
     begun_(false),
     state_change_no_(0),
     modify_change_no_(0),
     begun_change_no_(0),
     calendar_change_no_(0),
     suite_gen_variables_(NULL)
     {}

   Suite()
   : defs_(NULL),
     begun_(false),
     state_change_no_(0),
     modify_change_no_(0),
     begun_change_no_(0),
     calendar_change_no_(0),
     suite_gen_variables_(NULL)
     {}

   Suite( const Suite&);
   Suite& operator=(const Suite&);

   virtual ~Suite();

   static suite_ptr create(const std::string& name);

   virtual Suite* suite() const { return const_cast<Suite*>(this); }
   virtual Defs* defs() const { return defs_;}
   void set_defs(Defs* d) { defs_ = d;}
   virtual Suite* isSuite() const  {  return const_cast<Suite*>(this); }
   virtual NodeContainer* isNodeContainer() const { return const_cast<Suite*>(this); }

   /// Overridden to take into account begin()
   virtual bool resolveDependencies(JobsParam& );

   virtual void accept(ecf::NodeTreeVisitor&);
   virtual void acceptVisitTraversor(ecf::NodeTreeVisitor& v);
   virtual void reset();
   virtual void begin();
   virtual void requeue(Requeue_args& args);
   bool begun() const { return begun_; }
   void reset_begin();
   virtual void update_generated_variables() const;

   virtual const Variable& findGenVariable(const std::string& name) const;
   virtual void gen_variables(std::vector<Variable>&) const;

   void updateCalendar( const ecf::CalendarUpdateParams &, std::vector<node_ptr>& auto_cancelled_nodes);

   virtual const std::string& debugType() const;

   bool operator==(const Suite& rhs) const;
   std::ostream& print(std::ostream&) const;

   void addClock( const ClockAttr& , bool initialize_calendar = true); // throw std::run_time if more than one clock is added
   void add_end_clock( const ClockAttr& );
   void changeClock( const ClockAttr& );
   void changeClockType(const std::string& theType);
   void changeClockDate(const std::string& theDate);
   void changeClockGain(const std::string& theIntGain);
   void changeClockSync();

   /// return the suites calendar
   const ecf::Calendar& calendar() const { return calendar_;}
   ecf::Calendar& set_calendar() { return calendar_;}
   clock_ptr clockAttr() const { return clockAttr_;}
   clock_ptr clock_end_attr() const { return clock_end_attr_;}

   virtual bool checkInvariants(std::string& errorMsg) const;

   // Memento functions
   virtual void collateChanges(DefsDelta&) const;
   void set_memento(const SuiteClockMemento*,std::vector<ecf::Aspect::Type>& aspects,bool);
   void set_memento(const SuiteBeginDeltaMemento*,std::vector<ecf::Aspect::Type>& aspects,bool);
   void set_memento(const SuiteCalendarMemento*,std::vector<ecf::Aspect::Type>& aspects,bool);
   void set_memento(const OrderMemento* m,std::vector<ecf::Aspect::Type>& aspects,bool f) { NodeContainer::set_memento(m,aspects,f); }
   void set_memento(const ChildrenMemento* m,std::vector<ecf::Aspect::Type>& aspects,bool f) { NodeContainer::set_memento(m,aspects,f); }

   void set_state_change_no( unsigned int x )  { state_change_no_ = x;}
   unsigned int state_change_no() const        { return state_change_no_; }
   void set_modify_change_no( unsigned int x ) { modify_change_no_ = x;}
   unsigned int modify_change_no() const       { return modify_change_no_; }

   virtual void read_state(const std::string& line,const std::vector<std::string>& lineTokens);

private:
   void reset_begin_only();
   void begin_calendar();
   void requeue_calendar();
   void handle_clock_attribute_change();
   virtual std::string write_state() const;

   friend class boost::serialization::access;
   template<class Archive>
   void serialize(Archive & ar, const unsigned int /*version*/)
   {
      // serialise base class information
      ar & boost::serialization::base_object<NodeContainer>(*this);
      ar & begun_;
      ar & clockAttr_;
      ar & calendar_;

      // The calendar does not persist the clock type or start stop with server since
      // that is persisted with the clock attribute
      if (Archive::is_loading::value) {
         if (clockAttr_.get()) clockAttr_->init_calendar(calendar_);
      }
   }

private:
   Defs*                      defs_;                // *NOT* persisted, set by parent Defs
   bool                       begun_;
   clock_ptr                  clockAttr_;
   clock_ptr                  clock_end_attr_;      // *NOT* persisted, used by simulator only
   ecf::Calendar              calendar_;            // *Only* persisted since used by the why() on client side
   unsigned int               state_change_no_;     // no need to persist
   unsigned int               modify_change_no_;    // no need to persist
   unsigned int               begun_change_no_;     // no need to persist,  record changes to begun_. Needed for SSyncCmd
   unsigned int               calendar_change_no_;  // no need to persist,
   mutable SuiteGenVariables* suite_gen_variables_; // NOT persisted can be generated by calling update_generated_variables()
   friend class SuiteGenVariables;
};

std::ostream& operator<<(std::ostream& os, const Suite&);


// This class helps in avoiding the creation of generated variables until required.
// This improves client->server down load times by avoiding thousands of string constructions
class SuiteGenVariables : private boost::noncopyable {
public:
   explicit SuiteGenVariables(const Suite*);

   void force_update() { force_update_ = true;}
   void update_generated_variables() const;
   const Variable& findGenVariable(const std::string& name) const;
   void gen_variables(std::vector<Variable>& vec) const;

private:
   const Suite* suite_;
   mutable Variable genvar_suite_;    // *NOT* persisted, can be generated by calling update_generated_variables()
   mutable Variable genvar_ecf_time_; // *NOT* persisted, can be generated by calling update_generated_variables()
   mutable Variable genvar_time_;     // *NOT* persisted, can be generated by calling update_generated_variables()
   mutable Variable genvar_yyyy_;     // *NOT* persisted, can be generated by calling update_generated_variables()
   mutable Variable genvar_dow_;      // *NOT* persisted, can be generated by calling update_generated_variables()
   mutable Variable genvar_doy_;      // *NOT* persisted, can be generated by calling update_generated_variables()
   mutable Variable genvar_date_;     // *NOT* persisted, can be generated by calling update_generated_variables()
   mutable Variable genvar_day_;      // *NOT* persisted, can be generated by calling update_generated_variables()
   mutable Variable genvar_dd_;       // *NOT* persisted, can be generated by calling update_generated_variables()
   mutable Variable genvar_mm_;       // *NOT* persisted, can be generated by calling update_generated_variables()
   mutable Variable genvar_month_;    // *NOT* persisted, can be generated by calling update_generated_variables()
   mutable Variable genvar_ecf_date_; // *NOT* persisted, can be generated by calling update_generated_variables()
   mutable Variable genvar_ecf_clock_;// *NOT* persisted, can be generated by calling update_generated_variables()
   mutable Variable genvar_ecf_julian_;// *NOT* persisted, can be generated by calling update_generated_variables()
   mutable bool force_update_;
};

#endif
