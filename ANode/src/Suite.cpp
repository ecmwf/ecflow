//============================================================================
// Name        : NodeTree.cpp
// Author      : Avi
// Revision    : $Revision: #128 $ 
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

#include <assert.h>
#include <sstream>
#include <boost/lexical_cast.hpp>
#include <boost/make_shared.hpp>

#include "Suite.hpp"
#include "Defs.hpp"
#include "PrintStyle.hpp"
#include "NodeTreeVisitor.hpp"
#include "DefsDelta.hpp"

#include "Stl.hpp"
#include "Str.hpp"
#include "Ecf.hpp"
#include "Indentor.hpp"
#include "ExprAst.hpp"
#include "Log.hpp"
#include "CalendarUpdateParams.hpp"
#include "SuiteChanged.hpp"
#include "JobsParam.hpp"

using namespace ecf;
using namespace std;
using namespace boost::posix_time;
using namespace boost::gregorian;

///////////////////////////////////////////////////////////////////////////////////////////
//#define DEBUG_FIND_NODE 1

// Create the generated variable up-front. This allows them to be referenced
// is abstract syntax tree during the post process call


Suite::Suite(const Suite& rhs)
: NodeContainer(rhs),
  defs_(NULL),
  begun_(rhs.begun_),
  state_change_no_(0),
  modify_change_no_(0),
  begun_change_no_(0),
  calendar_change_no_(0),
  suite_gen_variables_(NULL)
{
   if (rhs.clockAttr_.get())
      clockAttr_ = boost::make_shared<ClockAttr>( *rhs.clockAttr_ );

   if (rhs.clock_end_attr_.get())
      clock_end_attr_ = boost::make_shared<ClockAttr>( *rhs.clock_end_attr_ );

   calendar_ = rhs.calendar_;
}

node_ptr Suite::clone() const
{
   return boost::make_shared<Suite>(*this );
}

Suite& Suite::operator=(const Suite& rhs)
{
   // defs_ not set
   if (this != &rhs) {
      NodeContainer::operator=(rhs);
      begun_ = rhs.begun_;
      if (rhs.clockAttr_.get()) clockAttr_ = boost::make_shared<ClockAttr>( *rhs.clockAttr_ );
      if (rhs.clock_end_attr_.get()) clock_end_attr_ = boost::make_shared<ClockAttr>( *rhs.clock_end_attr_ );
      calendar_ = rhs.calendar_;

      state_change_no_ = 0;
      modify_change_no_ = Ecf::incr_modify_change_no();
      begun_change_no_ = 0;
      calendar_change_no_ = 0;

      delete suite_gen_variables_;
      suite_gen_variables_ = NULL;
   }
   return *this;
}

Suite::~Suite()
{
//	std::cout << "Suite::~Suite() " << debugNodePath() << "\n";
   if (!Ecf::server()) {
      notify_delete();
   }
   delete suite_gen_variables_;
}

suite_ptr Suite::create(const std::string& name)
{
	return boost::make_shared<Suite>( name );
}

void Suite::accept(ecf::NodeTreeVisitor& v)
{
	SuiteChanged1 changed(this);
	v.visitSuite(this);
	NodeContainer::accept(v);
}

void Suite::acceptVisitTraversor(ecf::NodeTreeVisitor& v)
{
	SuiteChanged1 changed(this);
	v.visitSuite(this);
}

void Suite::begin()
{
   if (false == begun_ ) {

      // begin will change all the states of all child nodes, reset all attributes
      SuiteChanged1 changed(this);

      // begin can cause thousands of mementos to be created, to avoid this we
      // update the modify change number.
      Ecf::incr_modify_change_no();

      begun_ = true;
      begun_change_no_ = Ecf::incr_state_change_no();

      begin_calendar();

      NodeContainer::begin();

      update_generated_variables();
   }
}

void Suite::requeue(Requeue_args& args)
{
   if (false == begun_) {
      std::stringstream ss; ss << "Suite::requeue: The suite " << name() << " must be 'begun' first\n";
      throw std::runtime_error( ss.str() ) ;
   }

   // This is more efficient than: since no locking is required
   //    SuiteChanged changed(boost::dynamic_pointer_cast<Suite>(shared_from_this()));
   // since no locking is required in SuiteChanged
   SuiteChanged1 changed(this); //

   // requeue can cause thousands of mementos to be created, to avoid this we
   // update the modify change number, this will force a full sync in client
   Ecf::incr_modify_change_no();

   requeue_calendar();

   NodeContainer::requeue(args);

   update_generated_variables();
}

void Suite::reset()
{
   // reset will change all the states of all child nodes, reset all attributes
   SuiteChanged1 changed(this);

   // reset can cause thousands of mementos to be created, to avoid this we
   // update the modify change number.
   Ecf::incr_modify_change_no();

   reset_begin_only();

   requeue_calendar();

   NodeContainer::reset();
}

void Suite::reset_begin() {
   SuiteChanged1 changed(this);
   reset_begin_only();
}

void Suite::reset_begin_only()  // private
{
   begun_ = false;
   begun_change_no_ = Ecf::incr_state_change_no();
}

void Suite::begin_calendar()
{
	// Begin the calendar, from the clock attribute _ELSE_
	// Get the local time, second level resolution, based on the
	// time zone settings of the computer.
	if (clockAttr_.get())  {
	   clockAttr_->init_calendar(calendar_);  // *IF* AlterCmd was used, wait till Suite is requed
	   clockAttr_->begin_calendar(calendar_);
	}
	else {
	   calendar_.begin(Calendar::second_clock_time());
	}
}

void Suite::requeue_calendar()
{
   // ECFLOW-417
   // Need special handling for a Suite with hybrid clock and repeat day variable.
   // Basically on re-queue, we need to update calendar date by the repeat day variable
   if ( clockAttr_.get() && clockAttr_->hybrid() && repeat().is_repeat_day()) {

      // Get the current time, but with the *existing* date of the suite
      boost::gregorian::date suite_date = calendar_.suiteTime().date();
      suite_date += date_duration(repeat().step());

      ptime suiteTime = ptime(suite_date, Calendar::second_clock_time().time_of_day() );
      calendar_.begin( suiteTime );

      // make sure update variable regenerates all suite variables, i.e like ECF_DATE, etc
      // Needed since we have changed calendar date
      if (suite_gen_variables_)  suite_gen_variables_->force_update();

      return;
   }

   // Carry on same as before
   begin_calendar();
}


void Suite::updateCalendar(
      const ecf::CalendarUpdateParams & calParams,
      std::vector<node_ptr>& auto_cancelled_nodes,
      std::vector<node_ptr>& auto_archive_nodes
      )
{
	if (begun_) {

//		cout << "Suite::updateCalendar " << debugNodePath()   << " serverRunning = " << calParams.serverRunning() << " jobSubInterval = " << to_simple_string(calParams.serverPollPeriod());
//		if ( clockAttr_ )  cout << " clockAttr_->startStopWithServer() = " << clockAttr_->startStopWithServer();
//		cout << "\n";

		/// Some suite can elect to avoid updating the calendar when the server is stopped.
		/// This allows normal and relative time dependencies to be honoured even
		/// if the server is started/stopped many times.
		if ( clockAttr_.get() &&  clockAttr_->startStopWithServer() && !calParams.serverRunning()) {
			return;
		}

      SuiteChanged1 changed(this);

		/// The calendar_ will cache server poll period/job submission interval, as calendar increment for easy access
		calendar_.update( calParams );
	   calendar_change_no_ = Ecf::state_change_no() + 1; // ** See: collateChanges **

		update_generated_variables();

 		calendarChanged(calendar_,auto_cancelled_nodes,auto_archive_nodes,get_late());
	}
}

bool Suite::resolveDependencies(JobsParam& jobsParam)
{
 	if (begun_) {

 	   if (jobsParam.check_for_job_generation_timeout()) return false;

 	   SuiteChanged1 changed(this);
  		return NodeContainer::resolveDependencies(jobsParam);
 	}
 	return true;
}

bool Suite::operator==(const Suite& rhs) const
{
	if (begun_ != rhs.begun_) {
#ifdef DEBUG
		if (Ecf::debug_equality()) {
			std::cout << "Suite::operator==  (begun_(" <<  begun_ << ") != rhs.begun_(" << rhs.begun_ << ")) " << debugNodePath() << "\n";
		}
#endif
		return false;
	}

 	if ((clockAttr_.get() && !rhs.clockAttr_.get()) || (!clockAttr_.get() && rhs.clockAttr_.get()) ){
#ifdef DEBUG
		if (Ecf::debug_equality()) {
         std::cout << "Suite::operator==(clockAttr_.get() && !rhs.clockAttr_.get()) || (!clockAttr_.get() && rhs.clockAttr_.get() " << debugNodePath() << "\n";
 		}
#endif
		return false;
	}
	if (clockAttr_.get() && rhs.clockAttr_.get() && !(*clockAttr_ == *rhs.clockAttr_)) {
#ifdef DEBUG
		if (Ecf::debug_equality()) {
			std::cout << "Suite::operator== (clockAttr_ && rhs.clockAttr_ && !(*clockAttr_ == *rhs.clockAttr_)) " << debugNodePath() << "\n";
			std::cout << "clockAttr_: " << clockAttr_->toString() << "  rhs.clockAttr_: " << rhs.clockAttr_->toString() << "\n";
		}
#endif
		return false;
	}

 	return NodeContainer::operator==(rhs);
}

std::ostream& Suite::print(std::ostream& os) const
{
   Indentor::indent(os) << "suite " << name();
   if (!PrintStyle::defsStyle()) {
      std::string st = write_state();
      if (!st.empty()) os << " #" << st;
   }
   os << "\n";

	Node::print(os);

	// make sure clock attribute is written before
   if (clockAttr_.get()) clockAttr_->print(os);
   if (clock_end_attr_.get()) clock_end_attr_->print(os);
	if (!PrintStyle::defsStyle()) {
	   std::string calendar_state = calendar_.write_state();
	   if (!calendar_state.empty()) {
	      Indentor indent;
	      Indentor::indent(os) << "calendar" << calendar_state << "\n";
	   }
	}

	NodeContainer::print(os);
	Indentor::indent(os) << "endsuite\n";

	return os;
}

std::string Suite::write_state() const
{
   // *IMPORTANT* we *CANT* use ';' character, since is used in the parser, when we have
   //             multiple statement on a single line i.e.
   //                 task a; task b;
   std::string ret;
   if (begun_) ret += "  begun:1";
   ret += NodeContainer::write_state();
   return ret;
}
void Suite::read_state(const std::string& line,const std::vector<std::string>& lineTokens) {

   // suite s1 # begun:1 state:queued flag:edit_failed suspended:1
   if (lineTokens.size() >= 4 && lineTokens[3] == "begun:1") begun_ = true;
   NodeContainer::read_state(line,lineTokens);
}

const std::string& Suite::debugType() const { return ecf::Str::SUITE();}

std::ostream& operator<<(std::ostream& os, const Suite& d) { return d.print(os); }

void Suite::addClock( const ClockAttr& c,bool initialize_calendar)
{
	if ( clockAttr_.get()) {
 		throw std::runtime_error("Add Clock failed: Suite can only have one clock " + absNodePath());
 	}
   if (clock_end_attr_.get()) {
       if (clock_end_attr_->ptime() <= c.ptime()) {
          throw std::runtime_error("Add Clock failed:: End time must be greater than start time " + absNodePath());
       }
    }

	clockAttr_ = boost::make_shared<ClockAttr>(c);
	if (initialize_calendar) clockAttr_->init_calendar(calendar_);

   // clock_end_attr_ is always same type as clock
   if (clock_end_attr_.get())  clock_end_attr_->hybrid(clockAttr_->hybrid());
}

void Suite::changeClock( const ClockAttr& c)
{
   // When changing the clock, *WAIT* till requeue/begin to init the calendar
	clockAttr_.reset();
 	addClock( c , false);
}

void Suite::add_end_clock( const ClockAttr& c)
{
   // end clock is for for simulator only
   if ( clock_end_attr_.get()) {
      throw std::runtime_error("Add end Clock failed: Suite can only have one end clock " + absNodePath());
   }
   if (clockAttr_.get()) {
      if (c.ptime() <=  clockAttr_->ptime()) {
         throw std::runtime_error("Add end Clock failed: End time must be greater than start time " + absNodePath());
      }
   }

   clock_end_attr_ = boost::make_shared<ClockAttr>(c);
   clock_end_attr_->set_end_clock();

   // clock_end_attr_ is always same type as clock
   if (clockAttr_.get())  clock_end_attr_->hybrid(clockAttr_->hybrid());
}

void Suite::changeClockType(const std::string& clockType)
{
   // ISSUES:
   // Whenever the user *alters* the clock attributes, it needs to be followed by a re-queue of the suite, because:
   //   o/ if we change from real ->hybrid, then we need to set cron, etc time based nodes to complete
   //      Since we could have running tasks, it is up to user to decide when.
   //   o/ If we change from hybrid ->real, then Node with cron attributes etc, need to be requeued.
   //   o/ Any relative times are no longer valid
   //   o/ Time attributes will be incorrect, and hence may fail/pass incorrectly during dependency evaluation.
   //   o/ Why command may be wrong
   //
   //
   // *IF* the user *forgets* to do this, it can cause spurious errors, hence to *minimise* these
   // the best we can do is to :
   //   o/ re-sync suite calendar for clock attribute
   //   o/ re-queue all time based attributes, *avoiding*
   //      change of state when switching to hybrid clock (i.e due to day,date,cron time attrs)
   // This is handled in handle_clock_attribute_change()
   //

   if (clockType != "hybrid" && clockType != "real") {
      throw std::runtime_error("Suite::changeClockType: expected clock type to be 'hybrid' or 'real'  but found " + clockType);
   }

   SuiteChanged1 changed(this);
   if (clockAttr_.get()) {
      clockAttr_->hybrid( clockType == "hybrid" ); // will update state change_no
   }
   else {
      addClock( ClockAttr( clockType == "hybrid") ); // will update state change_no
   }

   // clock_end_attr_ is always same type as clock
   if (clock_end_attr_.get()) clock_end_attr_->hybrid(clockAttr_->hybrid());

   // re-sync suite calendar for clock attribute, re-queue all time based attributes
   handle_clock_attribute_change();
}

void Suite::changeClockDate(const std::string& theDate)
{
   // See ISSUES: Suite::changeClockType
   int dayy,month,year;
   DateAttr::getDate(theDate,dayy,month,year);
   if (dayy == 0 || month == 0 || year == 0)  throw std::runtime_error("Suite::changeClockDate Invalid clock date:" + theDate );

   // ECFLOW-417
   // By default the user *IS* expected to requeue afterwards. However in the case where we
   // have a hybrid clock *AND* repeat day, the calendar date will be updated after the requeue.
   // This will update calendar by repeat days.
   // Hence take this into account by decrementing by number of days
   if ( clockAttr_.get() && clockAttr_->hybrid() && repeat().is_repeat_day()) {
      boost::gregorian::date theDate( year, month, dayy );
      theDate -= date_duration(repeat().step());
      dayy = theDate.day();
      month = theDate.month();
      year = theDate.year();
   }


   SuiteChanged1 changed(this);
   if (clockAttr_.get())  {
      clockAttr_->date(dayy,month,year);      // this will check the date and update state change_no
   }
   else {
      addClock( ClockAttr(dayy,month,year) ); // will update state change_no
   }

   handle_clock_attribute_change();
}

void Suite::changeClockGain(const std::string& gain)
{
   // See: ISSUES on Suite::changeClockType
   long theGain = 0;
   try { theGain = boost::lexical_cast< long >( gain ); }
   catch ( boost::bad_lexical_cast& ) {
      throw std::runtime_error( "Suite::changeClockGain: value '" + gain + "' is not convertible to an long, for suite " + name());
   }

   SuiteChanged1 changed(this);
   if (!clockAttr_.get())  {
      addClock( ClockAttr() ); // will update state change_no
   }

   if (theGain > 0) {
      clockAttr_->set_gain_in_seconds( theGain, true);  // will update state change_no
   }
   else {
      clockAttr_->set_gain_in_seconds( theGain, false); // will update state change_no
   }

   handle_clock_attribute_change();
}

void Suite::changeClockSync()
{
   // See: ISSUES on Suite::changeClockType
   SuiteChanged1 changed(this);
   if (clockAttr_.get()) {
      clockAttr_->sync();     // clear so that on re-queue we sync with computer, + will update state change_no
   }
   else {
      addClock( ClockAttr() ); // will update state change_no
   }

   handle_clock_attribute_change();
}

void Suite::handle_clock_attribute_change()
{
   // re-queue time could cause thousands of mementos to be created, to avoid this we
   // update the modify change number.
   Ecf::incr_modify_change_no();

   // Since the suite clock attribute has changed, re-sync the suite calendar
   begin_calendar();

   // re-queue all the time attributes, since clock attribute has changed, avoid changing node state.
   // Note: when switching to hybrid clock the re-queue of time dependencies will
   //       *not* mark hybrid (day,date,cron) as complete
   //       since these nodes could be in a active/submitted state.
   NodeContainer::requeue_time_attrs();

   // make sure we regenerate all suite variables, i.e like ECF_DATE, etc
   if (suite_gen_variables_)  suite_gen_variables_->force_update();

   update_generated_variables();
}


bool Suite::checkInvariants(std::string& errorMsg) const
{
	if (!calendar_.checkInvariants(errorMsg)) {
		return false;
	}
   if (clockAttr_.get()) {
      if ( calendar().hybrid() != clockAttr_->hybrid()) {
         std::stringstream ss;
         ss << "Suite:" << name() << " Calendar(hybrid(" << calendar().hybrid() << ")) and Clock attribute(hybrid(" << clockAttr_->hybrid() << ")) must be in sync, clock types differs";
         errorMsg += ss.str();
         return false;
      }
   }

	if (Ecf::server()) {
	   if (state_change_no_ > Ecf::state_change_no() ) {
	      std::stringstream ss;
	      ss << "Suite::checkInvariants: suite_change_no(" << state_change_no_ << ") > Ecf::state_change_no(" << Ecf::state_change_no() << ")\n";
	      errorMsg += ss.str();
	      return false;
	   }
      if (begun_change_no_ > Ecf::state_change_no() ) {
         std::stringstream ss;
         ss << "Suite::checkInvariants: begun_change_no_(" << begun_change_no_ << ") > Ecf::state_change_no(" << Ecf::state_change_no() << ")\n";
         errorMsg += ss.str();
         return false;
      }
      if (calendar_change_no_ > Ecf::state_change_no() ) {
         std::stringstream ss;
         ss << "Suite::checkInvariants: calendar_change_no_(" << calendar_change_no_ << ") > Ecf::state_change_no(" << Ecf::state_change_no() << ")\n";
         errorMsg += ss.str();
         return false;
      }
      if (modify_change_no_ > Ecf::modify_change_no() ) {
         std::stringstream ss;
         ss << "Suite::checkInvariants: modify_change_no_(" << modify_change_no_ << ") > Ecf::modify_change_no(" << Ecf::modify_change_no() << ")\n";
         errorMsg += ss.str();
         return false;
      }
	}
	return NodeContainer::checkInvariants(errorMsg);
}

void Suite::collateChanges(DefsDelta& changes) const
{
	/// The suite hold the max state change no, for all its children and attributes

	// Optimising updates:
	// Problem:
	//    User has requested 1 second updated in the viewer. We used add SuiteCalendarMemento
	//    when ever there were changes in the suite. However this causes the suite in the
	//    viewer to *refresh* to often.
	//
	// Soln 1:
	//   Use:
	//      calendar_change_no_ = Ecf::incr_state_change_no();
	//
	//   plus only create SuiteCalendarMemento, where the suite changes *AND*
	//   calendar has actually changed.
	//   - This fixes the problem, at the expense of *always* creating a SuiteCalendarMemento
	//     every 60 seconds. Thus adding to network traffic.
	//   - The regression tests will fail, since a change is made in the server,
	//     for which the sync does nothing. *************************************
	//     This could be fixed by creating a SuiteCalendarMemento when calendar changes
	//     However we then go back always creating SuiteCalendarMemento every 60 seconds
	//     even when there are **no other** changes
	//
   // Soln 2:
	//    Use:
	//       calendar_change_no_ = Ecf::state_change_no() + 1
	//
	//    We mimick updating Ecf::state_change_no(), thus we can create memento when required
	//    They should however not be recognised as state change.
	//    + This fixes the problem, and the regression test will also work
	//    This is the solution that has been implemented
	//
	//    + ECFLOW-631 updated, to allow client to sync clock, before calling why cmd.

	// ********************************************************************
	// Note: we separate determining incremental changes from the traversal
	// ********************************************************************

#ifdef DEBUG_MEMENTO
      std::cout << "Suite::collateChanges()\n";
      std::cout << "  " << debugNodePath() << "\n";
      std::cout << "  client_state_change_no(" << changes.client_state_change_no() << ")\n";
      std::cout << "  suite state_change_no (" << state_change_no() << ")\n";
      std::cout << "  calendar_change_no_   (" << calendar_change_no_ << ")\n";
      std::cout << "  sync_suite_clock      (" << changes.sync_suite_clock() << ")\n";
#endif

	if (state_change_no() > changes.client_state_change_no() ||
	      (changes.sync_suite_clock() && calendar_change_no_ > changes.client_state_change_no() ) ) {

	   // *TREAT* All changes to *a* Node, in a single compound_memento_ptr
	   size_t before = changes.size();

	   compound_memento_ptr suite_compound_mememto;
	   if (clockAttr_.get() && clockAttr_->state_change_no() > changes.client_state_change_no()) {
	      if (!suite_compound_mememto.get()) suite_compound_mememto = boost::make_shared<CompoundMemento>(absNodePath());
	      suite_compound_mememto->add( boost::make_shared<SuiteClockMemento>(  *clockAttr_ ) );
	   }
	   if (begun_change_no_ > changes.client_state_change_no()) {
	      if (!suite_compound_mememto.get()) suite_compound_mememto = boost::make_shared<CompoundMemento>(absNodePath());
	      suite_compound_mememto->add( boost::make_shared<SuiteBeginDeltaMemento>( begun_) );
	   }

	   /// Collate NodeContainer and Node changes into *SAME* compound_memento_ptr
	   NodeContainer::incremental_changes(changes, suite_compound_mememto);

	   // Traversal, we have finished with this node:
	   // Traverse children : *SEPARATE* compound_memento_ptr created on demand
	   NodeContainer::collateChanges(changes);

	   /// *ONLY* create SuiteCalendarMemento, if something changed in the suite.
	   /// *OR* if it has been specifically requested. see ECFLOW-631
	   /// Additionally calendar_change_no_ updates should not register as a state change, i.e for tests
	   /// SuiteCalendarMemento is needed so that WhyCmd can work on the client side.
	   /// Need to use new compound since the suite may not have change, but it children may have.
	   /// Hence as side affect why command with reference to time will only be accurate
	   /// after some kind of state change. Fixed with ECFLOW-631 (Client must do sync_clock, before calling why)
	   size_t after = changes.size();
	   if ((before != after || changes.sync_suite_clock() ) && calendar_change_no_ > changes.client_state_change_no() ) {
	      compound_memento_ptr compound_ptr =  boost::make_shared<CompoundMemento>(absNodePath());
	      compound_ptr->add( boost::make_shared<SuiteCalendarMemento>( calendar_ ) );
	      changes.add( compound_ptr );
	   }
	}
}

void Suite::set_memento( const SuiteClockMemento* memento,std::vector<ecf::Aspect::Type>& aspects,bool aspect_only) {
#ifdef DEBUG_MEMENTO
	std::cout << "Suite::set_memento( const SuiteClockMemento*) " << debugNodePath() << "\n";
#endif

	if (aspect_only) aspects.push_back(ecf::Aspect::SUITE_CLOCK);
	else             changeClock(memento->clockAttr_);
}

void Suite::set_memento( const SuiteBeginDeltaMemento* memento,std::vector<ecf::Aspect::Type>& aspects,bool aspect_only) {
#ifdef DEBUG_MEMENTO
	std::cout << "Suite::set_memento( const SuiteBeginDeltaMemento* ) " << debugNodePath() << "\n";
#endif

	if (aspect_only) aspects.push_back(ecf::Aspect::SUITE_BEGIN);
	else             begun_ = memento->begun_;
}

void Suite::set_memento( const SuiteCalendarMemento* memento,std::vector<ecf::Aspect::Type>& aspects,bool aspect_only) {
#ifdef DEBUG_MEMENTO
	std::cout << "Suite::set_memento( const SuiteCalendarMemento* ) " << debugNodePath() << "\n";
#endif

	if (aspect_only) {
	   aspects.push_back(ecf::Aspect::SUITE_CALENDAR);
	   return;
	}

	// The calendar does *NOT* persist the calendar type (hybrid/real) since we can derive this for clock attribute
	// Hence make sure calendar/clock are in sync. part of the suite invariants
	calendar_ = memento->calendar_;
   if  (clockAttr_.get()) {
      if (clockAttr_->hybrid()) calendar_.set_clock_type(ecf::Calendar::HYBRID);
      else                      calendar_.set_clock_type(ecf::Calendar::REAL);
   }
}

// generated variables ---------------------------------------------------------------------
void Suite::update_generated_variables() const
{
   // This function is called during:
   //   o begin()
   //   o requeue()
   //   o when calendar changes
   if (!suite_gen_variables_) suite_gen_variables_ = new SuiteGenVariables(this);
   suite_gen_variables_->update_generated_variables();
   update_repeat_genvar();
}

const Variable& Suite::findGenVariable(const std::string& name) const
{
   if (!suite_gen_variables_) update_generated_variables();

   const Variable& gen_var = suite_gen_variables_->findGenVariable(name);
   if (!gen_var.empty()) return gen_var;
   return NodeContainer::findGenVariable(name);
}

void Suite::gen_variables(std::vector<Variable>& vec) const
{
   if (!suite_gen_variables_) update_generated_variables();

   vec.reserve(vec.size() + 13);
   NodeContainer::gen_variables(vec);
   suite_gen_variables_->gen_variables(vec);
}

// =======================================================================================

// The false below is used as a dummy argument to call the Variable constructor that does not
// check the variable names. i.e we know they are valid
SuiteGenVariables::SuiteGenVariables(const Suite* s)
:  suite_(s),
   genvar_suite_("SUITE", "", false),
   genvar_ecf_time_("ECF_TIME", "", false ),
   genvar_time_("TIME", "", false ),
   genvar_yyyy_("YYYY","", false),
   genvar_dow_("DOW", "", false),
   genvar_doy_("DOY", "", false),
   genvar_date_("DATE", "", false),
   genvar_day_("DAY", "", false),
   genvar_dd_("DD", "", false ),
   genvar_mm_("MM", "", false ),
   genvar_month_("MONTH", "", false ),
   genvar_ecf_date_("ECF_DATE", "", false ),
   genvar_ecf_clock_("ECF_CLOCK", "", false ),
   genvar_ecf_julian_("ECF_JULIAN", "", false ),
   force_update_(false){}

void SuiteGenVariables::update_generated_variables() const
{
   genvar_suite_.set_value(suite_->name());

   // The calendar_ is only initialised once the suite has begun
   if (!suite_->begun_) {
      return;
   }

   // The code below ASSUMES calendar has been initialised
   boost::posix_time::time_duration time_of_day = suite_->calendar_.suiteTime().time_of_day();

   //#ifdef DEBUG
   //    using namespace boost::gregorian;
   //    tm t = to_tm(suite_->calendar_.suiteTime());  // to_tm can be a bit of a performance hog
   ////  cerr << "\ntm_year = " << t.tm_year << "\n";  /* year - 1900              */
   ////  cerr << "tm_mon = " << t.tm_mon << "\n";      /* month of year (0 - 11)   */
   ////  cerr << "tm_mday = " << t.tm_mday << "\n";    /* day of month (1 - 31)    */
   ////  cerr << "tm_wday = " << t.tm_wday << "\n";    /* day of week (Sunday = 0) */
   ////  cerr << "tm_yday = " << t.tm_yday << "\n";    /* day of year (0 - 365)    */
   ////  cerr << "tm_hour = " << t.tm_hour << "\n";    /* hours (0 - 23)           */
   ////  cerr << "tm_min = " << t.tm_min << "\n";      /* minutes (0 - 59)         */
   ////  cerr << "tm_sec = " << t.tm_sec << "\n";      /* seconds (0 - 59)         */
   //
   //    // ***IMPORTANT*** suiteTime is only valid for real clock, note that
   //    // *************** for hybrid the day does not change. hence assertion
   //    // *************** needs to take into account calendar type
   //       if (!suite_->calendar_.hybrid()) {
   //          assert( t.tm_wday   == calendar_.day_of_week());
   //          assert( t.tm_mday   == calendar_.day_of_month());
   //          assert( t.tm_yday+1 == calendar_.day_of_year());
   //          assert( t.tm_mon+1  == calendar_.month());
   //       }
   //    assert( time_of_day.hours() == t.tm_hour);
   //    assert( time_of_day.minutes() == t.tm_min);
   //    assert( t.tm_year + 1900 == calendar_.year());
   //#endif

   char smstime[255];
   sprintf(smstime,"%02d%02d", time_of_day.hours(),time_of_day.minutes());
   genvar_time_.set_value( smstime );

   sprintf(smstime,"%02d:%02d", time_of_day.hours(),time_of_day.minutes());
   genvar_ecf_time_.set_value( smstime );

   //cout << "genvar_time_ = " << genvar_time_.theValue() << "\n";
   //cout << "genvar_ecf_time_ = " << genvar_ecf_time_.theValue() << "\n";


   // **********************************************************************
   // The following generated variable need only be updated if NULL or if day changed
   // Under: HYBRID the day will never change, hence a one time update
   // **********************************************************************
   if (suite_->calendar_.dayChanged() || genvar_yyyy_.theValue().empty() || force_update_) {

      force_update_ = false;
      genvar_yyyy_.set_value(boost::lexical_cast<std::string>(suite_->calendar_.year()));
      genvar_dow_.set_value( boost::lexical_cast<std::string>(suite_->calendar_.day_of_week()) );
      genvar_doy_.set_value( boost::lexical_cast<std::string>(suite_->calendar_.day_of_year()) );
      //cout << "genvar_yyyy_ = " << genvar_yyyy_.theValue() << "\n";
      //cout << "genvar_dow_ = " << genvar_dow_.theValue() << "\n";
      //cout << "genvar_doy_ = " << genvar_doy_.theValue() << "\n";

      char ddmmyyyyBuffer[255];
      sprintf(ddmmyyyyBuffer,"%02d.%02d.%04d", suite_->calendar_.day_of_month(), suite_->calendar_.month(), suite_->calendar_.year());
      genvar_date_.set_value( ddmmyyyyBuffer );
      //cout << "genvar_date_ = " << genvar_date_.theValue() << "\n";

      char *day_name[]= { const_cast<char*>("sunday"),   const_cast<char*>("monday"),
                          const_cast<char*>("tuesday"),  const_cast<char*>("wednesday"),
                          const_cast<char*>("thursday"), const_cast<char*>("friday"),
                          const_cast<char*>("saturday"), NULL };
      genvar_day_.set_value( day_name[suite_->calendar_.day_of_week()]  );
      //cout << "genvar_day_ = " << genvar_day_.theValue() << "\n";

      char dd[255];
      sprintf(dd,"%02d",suite_->calendar_.day_of_month());
      genvar_dd_.set_value( dd  );
      //cout << "genvar_dd_ = " << genvar_dd_.theValue() << "\n";

      char mm[255];
      sprintf(mm,"%02d",suite_->calendar_.month());
      genvar_mm_.set_value( mm  );
      //cout << "genvar_mm_ = " << genvar_mm_.theValue() << "\n";

      char *month_name[]
                       = { const_cast<char*>("january"),   const_cast<char*>("february"),  const_cast<char*>("march"),
                           const_cast<char*>("april"),     const_cast<char*>("may"),       const_cast<char*>("june"),
                           const_cast<char*>("july"),      const_cast<char*>("august"),    const_cast<char*>("september"),
                           const_cast<char*>("october"),   const_cast<char*>("november"),  const_cast<char*>("december"),
                                NULL } ;
      genvar_month_.set_value( month_name[suite_->calendar_.month()-1]  );
      //cout << "genvar_month_ = " << genvar_month_.theValue() << "\n";

      char smsdate[255];
      sprintf(smsdate,"%04d%02d%02d", suite_->calendar_.year(), suite_->calendar_.month() , suite_->calendar_.day_of_month());
      genvar_ecf_date_.set_value( smsdate );
      //cout << "genvar_ecf_date_ = " << genvar_ecf_date_.theValue() << "\n";

      char smsclock[255];
      sprintf(smsclock,"%s:%s:%d:%d", day_name[suite_->calendar_.day_of_week()], month_name[suite_->calendar_.month()-1],suite_->calendar_.day_of_week(),suite_->calendar_.day_of_year());
      genvar_ecf_clock_.set_value( smsclock );
      //cout << "genvar_ecf_clock_ = " << genvar_ecf_clock_.theValue() << "\n";


      genvar_ecf_julian_.set_value( boost::lexical_cast<std::string>( suite_->calendar_.suiteTime().date().julian_day()) );
   }
}

const Variable& SuiteGenVariables::findGenVariable(const std::string& name) const
{
   if (genvar_suite_.name() == name) return genvar_suite_;
   if (genvar_ecf_date_.name() == name) return genvar_ecf_date_;
   if (genvar_yyyy_.name() == name) return genvar_yyyy_;
   if (genvar_dow_.name() == name) return genvar_dow_;
   if (genvar_doy_.name() == name) return genvar_doy_;
   if (genvar_date_.name() == name) return genvar_date_;
   if (genvar_day_.name() == name) return genvar_day_;
   if (genvar_dd_.name() == name) return genvar_dd_;
   if (genvar_mm_.name() == name) return genvar_mm_;
   if (genvar_month_.name() == name) return genvar_month_;
   if (genvar_ecf_clock_.name() == name) return genvar_ecf_clock_;
   if (genvar_ecf_time_.name() == name) return genvar_ecf_time_;
   if (genvar_ecf_julian_.name() == name) return genvar_ecf_julian_;
   if (genvar_time_.name() == name) return genvar_time_;
   return Variable::EMPTY();
}

void SuiteGenVariables::gen_variables(std::vector<Variable>& vec) const
{
   vec.push_back(genvar_suite_);
   vec.push_back(genvar_ecf_date_);
   vec.push_back(genvar_yyyy_);
   vec.push_back(genvar_dow_);
   vec.push_back(genvar_doy_);
   vec.push_back(genvar_date_);
   vec.push_back(genvar_day_);
   vec.push_back(genvar_dd_);
   vec.push_back(genvar_mm_);
   vec.push_back(genvar_month_);
   vec.push_back(genvar_ecf_clock_);
   vec.push_back(genvar_ecf_time_);
   vec.push_back(genvar_ecf_julian_);
   vec.push_back(genvar_time_);
}
