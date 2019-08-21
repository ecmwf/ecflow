#ifndef SUITE_HPP_
#define SUITE_HPP_
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #73 $ 
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

#include "NodeContainer.hpp"
#include "Calendar.hpp"
#include "ClockAttr.hpp"
#include "NodeFwd.hpp"

class SuiteGenVariables;
namespace ecf { class CalendarUpdateParams;  } // forward declare

class Suite : public NodeContainer {
public:
   explicit Suite( const std::string& name,bool check = true) : NodeContainer(name,check) {}
   Suite()= default;

   Suite( const Suite&);
   Suite& operator=(const Suite&);
   node_ptr clone() const override;

   ~Suite() override;

   static suite_ptr create(const std::string& name, bool check = true);

   bool check_defaults() const override;

   Suite* suite() const override { return const_cast<Suite*>(this); }
   Defs* defs() const override { return defs_;}
   void set_defs(Defs* d) { defs_ = d;}
   Suite* isSuite() const override  {  return const_cast<Suite*>(this); }
   NodeContainer* isNodeContainer() const override { return const_cast<Suite*>(this); }

   /// Overridden to take into account begin()
   bool resolveDependencies(JobsParam& ) override;

   void accept(ecf::NodeTreeVisitor&) override;
   void acceptVisitTraversor(ecf::NodeTreeVisitor& v) override;
   void reset() override;
   void begin() override;
   void requeue(Requeue_args& args) override;
   bool begun() const { return begun_; }
   void reset_begin();
   void update_generated_variables() const override;

   const Variable& findGenVariable(const std::string& name) const override;
   void gen_variables(std::vector<Variable>&) const override;

   void updateCalendar( const ecf::CalendarUpdateParams &,Node::Calendar_args&);

   const std::string& debugType() const override;

   bool operator==(const Suite& rhs) const;
   void print(std::string&) const override;

   void addClock( const ClockAttr& , bool initialize_calendar = true); // throw std::run_time if more than one clock is added
   void add_end_clock( const ClockAttr& );
   void changeClock( const ClockAttr& );
   void changeClockType(const std::string& theType);
   void changeClockDate(const std::string& theDate);
   void changeClockGain(const std::string& theIntGain);
   void changeClockSync();

   /// return the suites calendar
   const ecf::Calendar& calendar() const { return cal_;}
   ecf::Calendar& set_calendar() { return cal_;}
   clock_ptr clockAttr() const { return clockAttr_;}
   clock_ptr clock_end_attr() const { return clock_end_attr_;}

   bool checkInvariants(std::string& errorMsg) const override;

   // Memento functions
   void collateChanges(DefsDelta&) const override;
   void set_memento(const SuiteClockMemento*,std::vector<ecf::Aspect::Type>& aspects,bool);
   void set_memento(const SuiteBeginDeltaMemento*,std::vector<ecf::Aspect::Type>& aspects,bool);
   void set_memento(const SuiteCalendarMemento*,std::vector<ecf::Aspect::Type>& aspects,bool);
   void set_memento(const OrderMemento* m,std::vector<ecf::Aspect::Type>& aspects,bool f) { NodeContainer::set_memento(m,aspects,f); }
   void set_memento(const ChildrenMemento* m,std::vector<ecf::Aspect::Type>& aspects,bool f) { NodeContainer::set_memento(m,aspects,f); }

   void set_state_change_no( unsigned int x )  { state_change_no_ = x;}
   unsigned int state_change_no() const        { return state_change_no_; }
   void set_modify_change_no( unsigned int x ) { modify_change_no_ = x;}
   unsigned int modify_change_no() const       { return modify_change_no_; }

   void read_state(const std::string& line,const std::vector<std::string>& lineTokens) override;

private:
   void reset_begin_only();
   void begin_calendar();
   void requeue_calendar();
   void handle_clock_attribute_change();
   void write_state(std::string&, bool&) const override;

   friend class cereal::access;
   template<class Archive>
   void serialize(Archive & ar, std::uint32_t const version );

private:
   Defs*                      defs_{nullptr};          // *NOT* persisted, set by parent Defs
   clock_ptr                  clockAttr_;
   clock_ptr                  clock_end_attr_;         // *NOT* persisted, used by simulator only
   ecf::Calendar              cal_;                    // *Only* persisted since used by the why() on client side
   unsigned int               state_change_no_{0};     // no need to persist
   unsigned int               modify_change_no_{0};    // no need to persist
   unsigned int               begun_change_no_{0};     // no need to persist,  record changes to begun_. Needed for SSyncCmd
   unsigned int               calendar_change_no_{0};  // no need to persist,
   mutable SuiteGenVariables* suite_gen_variables_{nullptr}; // NOT persisted can be generated by calling update_generated_variables()
   bool                       begun_{false};
   friend class SuiteGenVariables;
};

std::ostream& operator<<(std::ostream& os, const Suite&);


// This class helps in avoiding the creation of generated variables until required.
// This improves client->server down load times by avoiding thousands of string constructions
class SuiteGenVariables {
private:
  SuiteGenVariables(const SuiteGenVariables&) = delete;
  const SuiteGenVariables& operator=(const SuiteGenVariables&) = delete;
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
