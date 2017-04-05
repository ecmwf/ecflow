/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #48 $ 
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
#include <boost/make_shared.hpp>

#include "Suite.hpp"
#include "Ecf.hpp"
#include "DefsDelta.hpp"
#include "ExprAst.hpp"
#include "Stl.hpp"

using namespace ecf;
using namespace std;

//#define DEBUG_MEMENTO 1

/// CompoundMemento relies all clearing all attributes with state
void Node::clear()
{
   delete lateAttr_; lateAttr_ = NULL;
   delete completeExpr_; completeExpr_ = NULL;
   delete triggerExpr_; triggerExpr_ = NULL;

 	// ************************************************************
 	// Note: auto cancel, auto restore, auto archive does not have any
   //       changeable state, Hence it is not cleared.
   //       Hence no need for memento
  	// ************************************************************

   if (time_dep_attrs_) time_dep_attrs_->clear();
   if (child_attrs_) child_attrs_->clear();
   if (misc_attrs_) misc_attrs_->clear_attributes_with_state();    // zombies can be added/removed via AlterCmd
  	repeat_.clear();
	varVec_.clear();
	limitVec_.clear();
	inLimitMgr_.clear();
}

void Node::incremental_changes( DefsDelta& changes, compound_memento_ptr& comp) const
{
#ifdef DEBUG_MEMENTO
	std::cout << "Node::incremental_changes(DefsDelta& changes) " << debugNodePath() << "\n";
#endif

	unsigned int client_state_change_no = changes.client_state_change_no();

	// determine if state changed
	if (state_.first.state_change_no() > client_state_change_no) {
		if (!comp.get()) comp =  boost::make_shared<CompoundMemento>(absNodePath());
		comp->add( boost::make_shared<StateMemento>( state_.first.state()) );
	}

	// determine if def status changed
	if (defStatus_.state_change_no() > client_state_change_no) {
		if (!comp.get()) comp =  boost::make_shared<CompoundMemento>(absNodePath());
		comp->add( boost::make_shared<NodeDefStatusDeltaMemento>( defStatus_.state()) );
	}

	// determine if node suspend changed
	if (suspended_change_no_  > client_state_change_no) {
		if (!comp.get()) comp =  boost::make_shared<CompoundMemento>(absNodePath());
		comp->add( boost::make_shared<SuspendedMemento>( suspended_) );
	}

	// Determine if node attributes DELETED or ADDED, We copy **all** internal state
	// When applying on the client side, we clear all node attributes( ie Node::clear())
	// and re-add
	if (state_change_no_ > client_state_change_no) {

	   /// *****************************************************************************************
	   /// Node attributes DELETED or ADDED, i.e we call comp->clear_attributes()
	   /// *****************************************************************************************

#ifdef DEBUG_MEMENTO
	std::cout << "    Node::incremental_changes()    Attributes added or deleted\n";
#endif
	    // Note: auto-cancel does not have any alterable state hence, *NO* memento

	   if (!comp.get()) comp =  boost::make_shared<CompoundMemento>(absNodePath());
 		comp->clear_attributes();

		if (child_attrs_) {
         const std::vector<Meter>& meter_attrs = child_attrs_->meters();
         BOOST_FOREACH(const Meter& m, meter_attrs )  { comp->add( boost::make_shared<NodeMeterMemento>( m) ); }

         const std::vector<Event>& event_attrs = child_attrs_->events();
         BOOST_FOREACH(const Event& e, event_attrs ) { comp->add( boost::make_shared<NodeEventMemento>( e) ); }

         const std::vector<Label>& label_attrs = child_attrs_->labels();
         BOOST_FOREACH(const Label& l, label_attrs )  { comp->add( boost::make_shared<NodeLabelMemento>(  l) ); }
		}
		if (time_dep_attrs_) {
		   const std::vector<ecf::TodayAttr>& today_attrs = time_dep_attrs_->todayVec();
	      BOOST_FOREACH(const ecf::TodayAttr& attr, today_attrs){ comp->add( boost::make_shared<NodeTodayMemento>(  attr) ); }

	      const std::vector<ecf::TimeAttr>& time_attrs = time_dep_attrs_->timeVec();
         BOOST_FOREACH(const ecf::TimeAttr& attr, time_attrs ) { comp->add( boost::make_shared<NodeTimeMemento>(  attr) ); }

         const std::vector<DayAttr>& day_attrs = time_dep_attrs_->days();
         BOOST_FOREACH(const DayAttr& attr, day_attrs )     { comp->add( boost::make_shared<NodeDayMemento>(  attr) ); }

         const std::vector<DateAttr>& dates_attrs = time_dep_attrs_->dates();
         BOOST_FOREACH(const DateAttr& attr, dates_attrs )   { comp->add( boost::make_shared<NodeDateMemento>(  attr) ); }

         const std::vector<ecf::CronAttr>& cron_attrs = time_dep_attrs_->crons();
         BOOST_FOREACH(const CronAttr& attr, cron_attrs )   { comp->add( boost::make_shared<NodeCronMemento>(  attr) ); }
		}
		if (misc_attrs_) {
         const std::vector<VerifyAttr>& verify_attrs = misc_attrs_->verifys();
         if (!verify_attrs.empty()) comp->add( boost::make_shared<NodeVerifyMemento>( verify_attrs) );

         const std::vector<ZombieAttr>& zombie_attrs = misc_attrs_->zombies();
         BOOST_FOREACH(const ZombieAttr& attr, zombie_attrs){ comp->add( boost::make_shared<NodeZombieMemento>( attr) ); }

         const std::vector<QueueAttr>& queue_attrs = misc_attrs_->queues();
         BOOST_FOREACH(const QueueAttr& attr, queue_attrs){ comp->add( boost::make_shared<NodeQueueMemento>( attr) ); }
		}

		BOOST_FOREACH(limit_ptr l, limitVec_ )         { comp->add( boost::make_shared<NodeLimitMemento>(  *l) ); }
  		BOOST_FOREACH(const Variable& v, varVec_ )     { comp->add( boost::make_shared<NodeVariableMemento>( v) ); }

 		inLimitMgr_.get_memento(comp);

 		if (triggerExpr_)     comp->add( boost::make_shared<NodeTriggerMemento>(  *triggerExpr_) );
	 	if (completeExpr_)    comp->add( boost::make_shared<NodeCompleteMemento>(  *completeExpr_ ) );
 		if (!repeat_.empty()) comp->add( boost::make_shared<NodeRepeatMemento>(  repeat_) );
 		if (lateAttr_)        comp->add( boost::make_shared<NodeLateMemento>(  *lateAttr_) );

  		changes.add( comp );
		return;
	}

	/// *****************************************************************************************
	/// Node attributes CHANGED
   /// *****************************************************************************************

	// ** if start to Change ZombieAttr then it needs to be added here, currently we only add/delete.

   if (child_attrs_) {

      // determine if event value changed.
      const std::vector<Event>& event_attrs = child_attrs_->events();
      BOOST_FOREACH(const Event& e, event_attrs ) {
         if (e.state_change_no() > client_state_change_no) {
            if (!comp.get()) comp =  boost::make_shared<CompoundMemento>(absNodePath());
            comp->add( boost::make_shared<NodeEventMemento>(e) );
         }
      }

      // determine if Meter changed.
      const std::vector<Meter>& meter_attrs = child_attrs_->meters();
      BOOST_FOREACH(const Meter& m, meter_attrs ) {
         if (m.state_change_no() > client_state_change_no) {
            if (!comp.get()) comp =  boost::make_shared<CompoundMemento>(absNodePath());
            comp->add( boost::make_shared<NodeMeterMemento>( m) );
         }
      }

      // determine if labels changed.
      const std::vector<Label>& label_attrs = child_attrs_->labels();
      BOOST_FOREACH(const Label& l, label_attrs ) {
         if (l.state_change_no() > client_state_change_no) {
            if (!comp.get()) comp =  boost::make_shared<CompoundMemento>(absNodePath());
            comp->add( boost::make_shared<NodeLabelMemento>(  l) );
         }
      }
   }

	// Determine if the time related dependency changed
	if (time_dep_attrs_) {

      const std::vector<ecf::TodayAttr>& today_attrs = time_dep_attrs_->todayVec();
	   BOOST_FOREACH(const TodayAttr& attr, today_attrs ) {
	      if (attr.state_change_no() > client_state_change_no) {
	         if (!comp.get()) comp =  boost::make_shared<CompoundMemento>(absNodePath());
	         comp->add( boost::make_shared<NodeTodayMemento>( attr) );
	      }
	   }

      const std::vector<ecf::TimeAttr>& time_attrs = time_dep_attrs_->timeVec();
      BOOST_FOREACH(const TimeAttr& attr, time_attrs ) {
	      if (attr.state_change_no() > client_state_change_no) {
	         if (!comp.get()) comp =  boost::make_shared<CompoundMemento>(absNodePath());
	         comp->add( boost::make_shared<NodeTimeMemento>( attr) );
	      }
	   }

      const std::vector<DayAttr>& day_attrs = time_dep_attrs_->days();
	   BOOST_FOREACH(const DayAttr& attr, day_attrs ) {
	      if (attr.state_change_no() > client_state_change_no) {
	         if (!comp.get()) comp =  boost::make_shared<CompoundMemento>(absNodePath());
	         comp->add( boost::make_shared<NodeDayMemento>( attr) );
	      }
	   }

      const std::vector<DateAttr>& dates_attrs = time_dep_attrs_->dates();
	   BOOST_FOREACH(const DateAttr& attr, dates_attrs ) {
	      if (attr.state_change_no() > client_state_change_no) {
	         if (!comp.get()) comp =  boost::make_shared<CompoundMemento>(absNodePath());
	         comp->add( boost::make_shared<NodeDateMemento>( attr) );
	      }
	   }

      const std::vector<ecf::CronAttr>& cron_attrs = time_dep_attrs_->crons();
      BOOST_FOREACH(const CronAttr& attr, cron_attrs ) {
	      if (attr.state_change_no() > client_state_change_no) {
	         if (!comp.get()) comp =  boost::make_shared<CompoundMemento>(absNodePath());
	         comp->add( boost::make_shared<NodeCronMemento>( attr) );
	      }
	   }
	}

   if (misc_attrs_) {

      const std::vector<QueueAttr>& queue_attrs = misc_attrs_->queues();
      BOOST_FOREACH(const QueueAttr& attr, queue_attrs ) {
         if (attr.state_change_no() > client_state_change_no) {
            if (!comp.get()) comp =  boost::make_shared<CompoundMemento>(absNodePath());
            comp->add( boost::make_shared<NodeQueueIndexMemento>( attr.name(), attr.index() ) );
         }
      }

      // zombies have no state that changes
      // If one verify changes then copy all. Avoids having to work out which one changed
      const std::vector<VerifyAttr>& verify_attrs = misc_attrs_->verifys();
      BOOST_FOREACH(const VerifyAttr& v, verify_attrs ) {
         if (v.state_change_no() > client_state_change_no) {
            if (!comp.get()) comp =  boost::make_shared<CompoundMemento>(absNodePath());
            comp->add( boost::make_shared<NodeVerifyMemento>( verify_attrs) );
            break;
         }
      }
   }

	// determine if the trigger or complete changed
	if (triggerExpr_ && triggerExpr_->state_change_no() > client_state_change_no) {
		if (!comp.get()) comp =  boost::make_shared<CompoundMemento>(absNodePath());
		comp->add( boost::make_shared<NodeTriggerMemento>(  *triggerExpr_) );
	}
 	if (completeExpr_ && completeExpr_->state_change_no() > client_state_change_no) {
		if (!comp.get()) comp =  boost::make_shared<CompoundMemento>(absNodePath());
		comp->add( boost::make_shared<NodeCompleteMemento>(  *completeExpr_) );
	}

	// determine if the repeat changed
	if (!repeat_.empty() && repeat_.state_change_no() > client_state_change_no) {
		if (!comp.get()) comp =  boost::make_shared<CompoundMemento>(absNodePath());
		comp->add( boost::make_shared<NodeRepeatIndexMemento>( repeat_) );
	}

	// determine if limits changed.
	BOOST_FOREACH(limit_ptr l, limitVec_ ) {
		if (l->state_change_no() > client_state_change_no) {
			if (!comp.get()) comp =  boost::make_shared<CompoundMemento>(absNodePath());
			comp->add( boost::make_shared<NodeLimitMemento>(  *l) );
		}
	}

	// determine if variable values changed. Copy all variables. Save on having variable_change_no_ per variable
	if (variable_change_no_ > client_state_change_no) {
      if (!comp.get()) comp =  boost::make_shared<CompoundMemento>(absNodePath());
	   BOOST_FOREACH(const Variable& v, varVec_ )  { comp->add( boost::make_shared<NodeVariableMemento>( v) ); }
	}

	// Determine if the late attribute has changed
	if (lateAttr_ && lateAttr_->state_change_no() > client_state_change_no) {
		if (!comp.get()) comp =  boost::make_shared<CompoundMemento>(absNodePath());
		comp->add( boost::make_shared<NodeLateMemento>( *lateAttr_) );
	}

	// Determine if the flag changed
	if (flag_.state_change_no() > client_state_change_no) {
		if (!comp.get()) comp =  boost::make_shared<CompoundMemento>(absNodePath());
		comp->add( boost::make_shared<FlagMemento>( flag_ ) );
	}


	if (comp.get() ) {
		changes.add( comp );
	}
}

void Node::set_memento(const StateMemento* memento,std::vector<ecf::Aspect::Type>& aspects,bool aspect_only) {

#ifdef DEBUG_MEMENTO
	std::cout << "Node::set_memento(const StateMemento* memento) " << debugNodePath() << "  " << NState::toString(memento->state_) << "\n";
#endif

	if (aspect_only) aspects.push_back(ecf::Aspect::STATE);
	else             setStateOnly( memento->state_ );
}

void Node::set_memento( const NodeDefStatusDeltaMemento* memento,std::vector<ecf::Aspect::Type>& aspects,bool aspect_only ) {

   #ifdef DEBUG_MEMENTO
	std::cout << "Node::set_memento(const NodeDefStatusDeltaMemento* memento) " << debugNodePath() << "\n";
#endif

	if (aspect_only) aspects.push_back(ecf::Aspect::DEFSTATUS);
	else             defStatus_.setState( memento->state_ );
}

void Node::set_memento( const SuspendedMemento* memento,std::vector<ecf::Aspect::Type>& aspects,bool aspect_only ) {
#ifdef DEBUG_MEMENTO
	std::cout << "Node::set_memento(const SuspendedMemento* memento) " << debugNodePath() << "\n";
#endif
	if (aspect_only) {
	   aspects.push_back(ecf::Aspect::SUSPENDED);
	   return;
	}

	if (memento->suspended_) suspend();
	else                     clearSuspended();
}

void Node::set_memento( const NodeEventMemento* memento,std::vector<ecf::Aspect::Type>& aspects,bool aspect_only ) {

#ifdef DEBUG_MEMENTO
	std::cout << "Node::set_memento(const NodeEventMemento* memento) " << debugNodePath() << "\n";
#endif

   if (aspect_only) {
      // For attribute add/delete Should have already added ecf::Aspect::ADD_REMOVE_ATTR to aspects
      aspects.push_back(ecf::Aspect::EVENT);
      return;
   }

	if (child_attrs_) {
	   child_attrs_->set_memento(memento);
	   return;
	}
	addEvent( memento->event_);
}

void Node::set_memento( const NodeMeterMemento* memento,std::vector<ecf::Aspect::Type>& aspects,bool aspect_only ) {

#ifdef DEBUG_MEMENTO
	std::cout << "Node::set_memento(const NodeMeterMemento* memento) " << debugNodePath() << "\n";
#endif

   if (aspect_only) {
      // For attribute add/delete Should have already added ecf::Aspect::ADD_REMOVE_ATTR to aspects
      aspects.push_back(ecf::Aspect::METER);
      return;
   }

   if (child_attrs_) {
      child_attrs_->set_memento(memento);
      return;
   }
	addMeter(memento->meter_);
}

void Node::set_memento( const NodeLabelMemento* memento,std::vector<ecf::Aspect::Type>& aspects,bool aspect_only ) {

#ifdef DEBUG_MEMENTO
	std::cout << "Node::set_memento(const NodeLabelMemento* memento) " << debugNodePath() << "\n";
#endif

   if (aspect_only) {
      // For attribute add/delete Should have already added ecf::Aspect::ADD_REMOVE_ATTR to aspects
      aspects.push_back(ecf::Aspect::LABEL);
      return;
   }

	if (child_attrs_) {
	   child_attrs_->set_memento(memento);
	   return;
	}
	addLabel(memento->label_);
}

void Node::set_memento(const NodeQueueMemento* m,std::vector<ecf::Aspect::Type>& aspects,bool aspect_only)
{
   if (aspect_only) {
      // For attribute add/delete Should have already added ecf::Aspect::ADD_REMOVE_ATTR to aspects
      aspects.push_back(ecf::Aspect::QUEUE);
      return;
   }
   if (misc_attrs_) {
      misc_attrs_->set_memento(m);
      return;
   }
   add_queue(m->queue_);
}

void Node::set_memento(const NodeQueueIndexMemento* m,std::vector<ecf::Aspect::Type>& aspects,bool aspect_only )
{
   if (aspect_only) {
      // For attribute add/delete Should have already added ecf::Aspect::ADD_REMOVE_ATTR to aspects
      aspects.push_back(ecf::Aspect::QUEUE_INDEX);
      return;
   }

   // The queue must exist
   if (misc_attrs_) {
      misc_attrs_->set_memento(m);
      return;
   }
}

void Node::set_memento( const NodeTriggerMemento* memento,std::vector<ecf::Aspect::Type>& aspects,bool aspect_only ) {

#ifdef DEBUG_MEMENTO
	std::cout << "Node::set_memento(const NodeTriggerMemento* memento) " << debugNodePath() << "\n";
#endif

	if (aspect_only) {
	   // For attribute add/delete, should have already added ecf::Aspect::ADD_REMOVE_ATTR to aspects
	   aspects.push_back(ecf::Aspect::EXPR_TRIGGER);
	   return;
	}

	if (triggerExpr_) {
		if (memento->exp_.isFree()) freeTrigger();
		else                        clearTrigger();
		return;
	}
	add_trigger_expression( memento->exp_);
}

void Node::set_memento( const NodeCompleteMemento* memento,std::vector<ecf::Aspect::Type>& aspects,bool aspect_only ) {

#ifdef DEBUG_MEMENTO
	std::cout << "Node::set_memento(const NodeCompleteMemento* memento) " << debugNodePath() << "\n";
#endif

	if (aspect_only) {
      // For attribute add/delete, should have already added ecf::Aspect::ADD_REMOVE_ATTR to aspects
	   aspects.push_back(ecf::Aspect::EXPR_COMPLETE);
	   return;
	}

	if (completeExpr_) {
		if (memento->exp_.isFree()) freeComplete();
		else                        clearComplete();
		return;
	}
	add_complete_expression( memento->exp_);
}

void Node::set_memento( const NodeRepeatMemento* memento,std::vector<ecf::Aspect::Type>& aspects,bool aspect_only ) {

#ifdef DEBUG_MEMENTO
	std::cout << "Node::set_memento(const NodeRepeatMemento* memento) " << debugNodePath() << "\n";
#endif

	if (aspect_only) {
      // For attribute add/delete, should have already added ecf::Aspect::ADD_REMOVE_ATTR to aspects
	   aspects.push_back(ecf::Aspect::REPEAT);
	   return;
	}

	if (!repeat_.empty()) {

      // Note: the node is incremented one past, the last value
      // In Node we increment() then check for validity
      // hence the_new_value may be outside of the valid range.
      // This can be seen when do a incremental sync,
      // *hence* allow memento to copy the value as is.
      repeat_.set_value(memento->repeat_.index_or_value());

      // Alternative, but expensive since relies on cloning and coping potentially very large vectors
		// repeat_ = memento->repeat_;
 		return;
	}

	addRepeat(memento->repeat_);
}

void Node::set_memento( const NodeRepeatIndexMemento* memento,std::vector<ecf::Aspect::Type>& aspects,bool aspect_only ) {

#ifdef DEBUG_MEMENTO
   std::cout << "Node::set_memento(const NodeRepeatIndexMemento* memento) " << debugNodePath() << "\n";
#endif

   if (aspect_only) {
      // For attribute add/delete, should have already added ecf::Aspect::ADD_REMOVE_ATTR to aspects
      aspects.push_back(ecf::Aspect::REPEAT_INDEX);
      return;
   }

   if (!repeat_.empty()) {

      // Note: the node is incremented one past, the last value
      // In Node we increment() then check for validity
      // hence the_new_value may be outside of the valid range.
      // This can be seen when do a incremental sync,
      // *hence* allow memento to copy the value as is.
      repeat_.set_value(memento->index_or_value_);
   }
}

void Node::set_memento( const NodeLimitMemento* memento,std::vector<ecf::Aspect::Type>& aspects,bool aspect_only ) {

#ifdef DEBUG_MEMENTO
	std::cout << "Node::set_memento(const NodeLimitMemento* memento) " << debugNodePath() << "  " << memento->limit_.toString() << "\n";
#endif

	if (aspect_only) {
      // For attribute add/delete, should have already added ecf::Aspect::ADD_REMOVE_ATTR to aspects
	   aspects.push_back(ecf::Aspect::LIMIT);
	   return;
	}

	limit_ptr limit = find_limit(memento->limit_.name());
	if (limit.get())  {
	   limit->set_state( memento->limit_.theLimit(), memento->limit_.value(), memento->limit_.paths() );
		return;
	}
	addLimit(memento->limit_);
}

void Node::set_memento( const NodeInLimitMemento* memento,std::vector<ecf::Aspect::Type>& aspects,bool aspect_only ) {

#ifdef DEBUG_MEMENTO
	std::cout << "Node::set_memento(const NodeInLimitMemento* memento) " << debugNodePath() << "\n";
#endif

   if (aspect_only) {
      // ADD_REMOVE_ATTR aspect only, since no state
      // For attribute add/delete, should have already added ecf::Aspect::ADD_REMOVE_ATTR to aspects
      return;
   }

	addInLimit(memento->inlimit_);
}

void Node::set_memento( const NodeVariableMemento* memento,std::vector<ecf::Aspect::Type>& aspects,bool aspect_only ) {

#ifdef DEBUG_MEMENTO
	std::cout << "Node::set_memento(const NodeVariableMemento* memento) " << debugNodePath() << "\n";
#endif

	// If we have added/delete variables then ecf::Aspect::ADD_REMOVE_ATTR has already been added to aspects
   if (aspect_only) {
      aspects.push_back(ecf::Aspect::NODE_VARIABLE);
      return;
   }

	size_t theSize = varVec_.size();
	for(size_t i = 0; i < theSize; i++) {
		if (varVec_[i].name() == memento->var_.name()) {
			varVec_[i].set_value( memento->var_.theValue() );
			return;
		}
 	}
 	addVariable(memento->var_);
}

void Node::set_memento( const NodeLateMemento* memento,std::vector<ecf::Aspect::Type>& aspects,bool aspect_only) {

#ifdef DEBUG_MEMENTO
	std::cout << "Node::set_memento(const NodeLateMemento* memento) " << debugNodePath() << "\n";
#endif

	if (aspect_only) {
      // For attribute add/delete, should have already added ecf::Aspect::ADD_REMOVE_ATTR to aspects
	   aspects.push_back(ecf::Aspect::LATE);
	   return;
	}

	if (lateAttr_) {
		lateAttr_->setLate(memento->late_.isLate());
		return;
	}
	addLate(memento->late_);
}

void Node::set_memento( const NodeTodayMemento* memento,std::vector<ecf::Aspect::Type>& aspects,bool aspect_only ) {

#ifdef DEBUG_MEMENTO
	std::cout << "Node::set_memento(const NodeTodayMemento* memento) " << debugNodePath() << "\n";
#endif

   if (aspect_only) {
      // For attribute add/delete, should have already added ecf::Aspect::ADD_REMOVE_ATTR to aspects
      aspects.push_back(ecf::Aspect::TODAY);
      return;
   }

   if (time_dep_attrs_ && time_dep_attrs_->set_memento(memento) ) {
      return;
   }
	addToday(memento->attr_);
}

void Node::set_memento( const NodeTimeMemento* memento,std::vector<ecf::Aspect::Type>& aspects,bool aspect_only ) {

#ifdef DEBUG_MEMENTO
	std::cout << "Node::set_memento(const NodeTimeMemento* memento) " << debugNodePath() << "\n";
#endif

   if (aspect_only) {
      // For attribute add/delete, should have already added ecf::Aspect::ADD_REMOVE_ATTR to aspects
      aspects.push_back(ecf::Aspect::TIME);
      return;
   }

   if (time_dep_attrs_ && time_dep_attrs_->set_memento(memento) ) {
      return;
   }
	addTime(memento->attr_);
}

void Node::set_memento( const NodeDayMemento* memento,std::vector<ecf::Aspect::Type>& aspects,bool aspect_only ) {

#ifdef DEBUG_MEMENTO
	std::cout << "Node::set_memento(const NodeDayMemento* memento) " << debugNodePath() << "\n";
#endif

   if (aspect_only) {
      // For attribute add/delete, should have already added ecf::Aspect::ADD_REMOVE_ATTR to aspects
      aspects.push_back(ecf::Aspect::DAY);
      return;
   }

   if (time_dep_attrs_ && time_dep_attrs_->set_memento(memento) ) {
      return;
   }
	addDay(memento->attr_);
}

void Node::set_memento( const NodeDateMemento* memento,std::vector<ecf::Aspect::Type>& aspects,bool aspect_only ) {

#ifdef DEBUG_MEMENTO
   std::cout << "Node::set_memento(const NodeDateMemento* memento) " << debugNodePath() << "\n";
#endif

   if (aspect_only) {
      // For attribute add/delete, should have already added ecf::Aspect::ADD_REMOVE_ATTR to aspects
      aspects.push_back(ecf::Aspect::DATE);
      return;
   }

   if (time_dep_attrs_ && time_dep_attrs_->set_memento(memento) ) {
      return;
   }
   addDate(memento->attr_);
}

void Node::set_memento( const NodeCronMemento* memento,std::vector<ecf::Aspect::Type>& aspects,bool aspect_only ) {

#ifdef DEBUG_MEMENTO
	std::cout << "Node::set_memento(const NodeCronMemento* memento) " << debugNodePath() << "\n";
#endif

   if (aspect_only) {
      // For attribute add/delete, should have already added ecf::Aspect::ADD_REMOVE_ATTR to aspects
      aspects.push_back(ecf::Aspect::CRON);
      return;
   }

   if (time_dep_attrs_ && time_dep_attrs_->set_memento(memento) ) {
      return;
   }
	addCron(memento->attr_);
}

void Node::set_memento( const FlagMemento* memento,std::vector<ecf::Aspect::Type>& aspects,bool aspect_only ) {

#ifdef DEBUG_MEMENTO
	std::cout << "Node::set_memento(const FlagMemento* memento) " << debugNodePath() << "\n";
#endif

	if (aspect_only)  aspects.push_back(ecf::Aspect::FLAG);
	else              flag_.set_flag( memento->flag_.flag() );
}

void Node::set_memento( const NodeZombieMemento* memento,std::vector<ecf::Aspect::Type>& aspects,bool aspect_only ) {

#ifdef DEBUG_MEMENTO
   std::cout << "Node::set_memento(const NodeZombieMemento* memento) " << debugNodePath() << "\n";
#endif

   if (aspect_only) {
      // For attribute add/delete, should have already added ecf::Aspect::ADD_REMOVE_ATTR to aspects
      // No state, only ADD_REMOVE_ATTR aspect
      return;
   }

   // Zombie attributes should always be via ADD_REMOVE_ATTR
   // See Node::incremental_changes
   // Since there is no state to change

   /// remove existing attribute of same type, as duplicate of same type not allowed
   delete_zombie( memento->attr_.zombie_type());
   addZombie(memento->attr_);
}

void Node::set_memento( const NodeVerifyMemento* memento,std::vector<ecf::Aspect::Type>& aspects,bool aspect_only ) {

#ifdef DEBUG_MEMENTO
	std::cout << "Node::set_memento(const NodeVerifyMemento* memento) " << debugNodePath() << "\n";
#endif

   if (aspect_only) {
      // For attribute add/delete, should have already added ecf::Aspect::ADD_REMOVE_ATTR to aspects
      return;
   }

	if (misc_attrs_) {
	   misc_attrs_->verifys_.clear();
	   misc_attrs_->verifys_ = memento->verifys_;
	   return;
  	}

	misc_attrs_ = new MiscAttrs(this);
   misc_attrs_->verifys_ = memento->verifys_;
}
