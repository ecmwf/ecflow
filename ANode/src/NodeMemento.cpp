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
   late_.reset(nullptr);
   c_expr_.reset(nullptr);
   t_expr_.reset(nullptr);

   misc_attrs_.reset(nullptr);

   todays_.clear();
   times_.clear();
   crons_.clear();
   days_.clear();
   dates_.clear();

 	// ************************************************************
 	// Note: auto cancel, auto restore, auto archive does not have any
   //       changeable state, Hence it is not cleared.
   //       Hence no need for memento
  	// ************************************************************

   meters_.clear();
   events_.clear();
   labels_.clear();

  	repeat_.clear();
	vars_.clear();
	limits_.clear();
	inLimitMgr_.clear();
}

void Node::incremental_changes( DefsDelta& changes, compound_memento_ptr& comp) const
{
#ifdef DEBUG_MEMENTO
	std::cout << "Node::incremental_changes(DefsDelta& changes) " << debugNodePath() << "\n";
#endif

	unsigned int client_state_change_no = changes.client_state_change_no();

	// determine if state changed
	if (st_.first.state_change_no() > client_state_change_no) {
		if (!comp.get()) comp =  std::make_shared<CompoundMemento>(absNodePath());
		comp->add( std::make_shared<StateMemento>( st_.first.state()) );
	}

	// determine if def status changed
	if (d_st_.state_change_no() > client_state_change_no) {
		if (!comp.get()) comp =  std::make_shared<CompoundMemento>(absNodePath());
		comp->add( std::make_shared<NodeDefStatusDeltaMemento>( d_st_.state()) );
	}

	// determine if node suspend changed
	if (suspended_change_no_  > client_state_change_no) {
		if (!comp.get()) comp =  std::make_shared<CompoundMemento>(absNodePath());
		comp->add( std::make_shared<SuspendedMemento>( suspended_) );
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

	   if (!comp.get()) comp =  std::make_shared<CompoundMemento>(absNodePath());
 		comp->clear_attributes();


      BOOST_FOREACH(const Meter& m, meters_) { comp->add( std::make_shared<NodeMeterMemento>( m) ); }
      BOOST_FOREACH(const Event& e, events_) { comp->add( std::make_shared<NodeEventMemento>( e) ); }
      BOOST_FOREACH(const Label& l, labels_) { comp->add( std::make_shared<NodeLabelMemento>( l) ); }

      BOOST_FOREACH(const ecf::TodayAttr& attr, todays_){ comp->add( std::make_shared<NodeTodayMemento>( attr) ); }
      BOOST_FOREACH(const ecf::TimeAttr& attr, times_ ) { comp->add( std::make_shared<NodeTimeMemento>( attr) ); }
      BOOST_FOREACH(const DayAttr& attr, days_ )          { comp->add( std::make_shared<NodeDayMemento>( attr) ); }
      BOOST_FOREACH(const DateAttr& attr, dates_ )        { comp->add( std::make_shared<NodeDateMemento>( attr) ); }
      BOOST_FOREACH(const CronAttr& attr, crons_ )        { comp->add( std::make_shared<NodeCronMemento>( attr) ); }

		if (misc_attrs_) {
         const std::vector<VerifyAttr>& verify_attrs = misc_attrs_->verifys();
         if (!verify_attrs.empty()) comp->add( std::make_shared<NodeVerifyMemento>( verify_attrs) );

         const std::vector<ZombieAttr>& zombie_attrs = misc_attrs_->zombies();
         BOOST_FOREACH(const ZombieAttr& attr, zombie_attrs){ comp->add( std::make_shared<NodeZombieMemento>( attr) ); }

         const std::vector<QueueAttr>& queue_attrs = misc_attrs_->queues();
         BOOST_FOREACH(const QueueAttr& attr, queue_attrs){ comp->add( std::make_shared<NodeQueueMemento>( attr) ); }

         const std::vector<GenericAttr>& generic_attrs = misc_attrs_->generics();
         BOOST_FOREACH(const GenericAttr& attr,generic_attrs){ comp->add( std::make_shared<NodeGenericMemento>( attr) ); }
		}

		BOOST_FOREACH(limit_ptr l, limits_ )         { comp->add( std::make_shared<NodeLimitMemento>(  *l) ); }
  		BOOST_FOREACH(const Variable& v, vars_ )     { comp->add( std::make_shared<NodeVariableMemento>( v) ); }

 		inLimitMgr_.get_memento(comp);

 		if (t_expr_)     comp->add( std::make_shared<NodeTriggerMemento>(  *t_expr_) );
	 	if (c_expr_)    comp->add( std::make_shared<NodeCompleteMemento>(  *c_expr_ ) );
 		if (!repeat_.empty()) comp->add( std::make_shared<NodeRepeatMemento>(  repeat_) );
 		if (late_)        comp->add( std::make_shared<NodeLateMemento>(  *late_) );

  		changes.add( comp );
		return;
	}

	/// *****************************************************************************************
	/// Node attributes CHANGED
   /// *****************************************************************************************

	// ** if start to Change ZombieAttr then it needs to be added here, currently we only add/delete.

	// determine if event, meter, label   changed.
	BOOST_FOREACH(const Event& e, events_ ) {
	   if (e.state_change_no() > client_state_change_no) {
	      if (!comp.get()) comp = std::make_shared<CompoundMemento>(absNodePath());
	      comp->add( std::make_shared<NodeEventMemento>(e) );
	   }
	}
	BOOST_FOREACH(const Meter& m, meters_) {
	   if (m.state_change_no() > client_state_change_no) {
	      if (!comp.get()) comp = std::make_shared<CompoundMemento>(absNodePath());
	      comp->add( std::make_shared<NodeMeterMemento>( m) );
	   }
	}
	BOOST_FOREACH(const Label& l, labels_ ) {
	   if (l.state_change_no() > client_state_change_no) {
	      if (!comp.get()) comp = std::make_shared<CompoundMemento>(absNodePath());
	      comp->add( std::make_shared<NodeLabelMemento>(  l) );
	   }
	}

	// Determine if the time related dependency changed
	BOOST_FOREACH(const TodayAttr& attr, todays_  ) {
	   if (attr.state_change_no() > client_state_change_no) {
	      if (!comp.get()) comp = std::make_shared<CompoundMemento>(absNodePath());
	      comp->add( std::make_shared<NodeTodayMemento>( attr) );
	   }
	}
	BOOST_FOREACH(const TimeAttr& attr, times_  ) {
	   if (attr.state_change_no() > client_state_change_no) {
	      if (!comp.get()) comp = std::make_shared<CompoundMemento>(absNodePath());
	      comp->add( std::make_shared<NodeTimeMemento>( attr) );
	   }
	}
	BOOST_FOREACH(const DayAttr& attr, days_ ) {
	   if (attr.state_change_no() > client_state_change_no) {
	      if (!comp.get()) comp = std::make_shared<CompoundMemento>(absNodePath());
	      comp->add( std::make_shared<NodeDayMemento>( attr) );
	   }
	}
	BOOST_FOREACH(const DateAttr& attr, dates_ ) {
	   if (attr.state_change_no() > client_state_change_no) {
	      if (!comp.get()) comp = std::make_shared<CompoundMemento>(absNodePath());
	      comp->add( std::make_shared<NodeDateMemento>( attr) );
	   }
	}
	BOOST_FOREACH(const CronAttr& attr, crons_ ) {
	   if (attr.state_change_no() > client_state_change_no) {
	      if (!comp.get()) comp = std::make_shared<CompoundMemento>(absNodePath());
	      comp->add( std::make_shared<NodeCronMemento>( attr) );
	   }
	}


   if (misc_attrs_) {

      const std::vector<QueueAttr>& queue_attrs = misc_attrs_->queues();
      BOOST_FOREACH(const QueueAttr& attr, queue_attrs ) {
         if (attr.state_change_no() > client_state_change_no) {
            if (!comp.get()) comp =  std::make_shared<CompoundMemento>(absNodePath());
            comp->add( std::make_shared<NodeQueueIndexMemento>( attr.name(), attr.index(), attr.state_vec() ) );
         }
      }

      // zombies have no state that changes
      // If one verify changes then copy all. Avoids having to work out which one changed
      const std::vector<VerifyAttr>& verify_attrs = misc_attrs_->verifys();
      BOOST_FOREACH(const VerifyAttr& v, verify_attrs ) {
         if (v.state_change_no() > client_state_change_no) {
            if (!comp.get()) comp =  std::make_shared<CompoundMemento>(absNodePath());
            comp->add( std::make_shared<NodeVerifyMemento>( verify_attrs) );
            break;
         }
      }
   }

	// determine if the trigger or complete changed
	if (t_expr_ && t_expr_->state_change_no() > client_state_change_no) {
		if (!comp.get()) comp =  std::make_shared<CompoundMemento>(absNodePath());
		comp->add( std::make_shared<NodeTriggerMemento>(  *t_expr_) );
	}
 	if (c_expr_ && c_expr_->state_change_no() > client_state_change_no) {
		if (!comp.get()) comp =  std::make_shared<CompoundMemento>(absNodePath());
		comp->add( std::make_shared<NodeCompleteMemento>(  *c_expr_) );
	}

	// determine if the repeat changed
	if (!repeat_.empty() && repeat_.state_change_no() > client_state_change_no) {
		if (!comp.get()) comp =  std::make_shared<CompoundMemento>(absNodePath());
		comp->add( std::make_shared<NodeRepeatIndexMemento>( repeat_) );
	}

	// determine if limits changed.
	BOOST_FOREACH(limit_ptr l, limits_ ) {
		if (l->state_change_no() > client_state_change_no) {
			if (!comp.get()) comp =  std::make_shared<CompoundMemento>(absNodePath());
			comp->add( std::make_shared<NodeLimitMemento>(  *l) );
		}
	}

	// determine if variable values changed. Copy all variables. Save on having variable_change_no_ per variable
	if (variable_change_no_ > client_state_change_no) {
      if (!comp.get()) comp =  std::make_shared<CompoundMemento>(absNodePath());
	   BOOST_FOREACH(const Variable& v, vars_ )  { comp->add( std::make_shared<NodeVariableMemento>( v) ); }
	}

	// Determine if the late attribute has changed
	if (late_ && late_->state_change_no() > client_state_change_no) {
		if (!comp.get()) comp =  std::make_shared<CompoundMemento>(absNodePath());
		comp->add( std::make_shared<NodeLateMemento>( *late_) );
	}

	// Determine if the flag changed
	if (flag_.state_change_no() > client_state_change_no) {
		if (!comp.get()) comp =  std::make_shared<CompoundMemento>(absNodePath());
		comp->add( std::make_shared<FlagMemento>( flag_ ) );
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
	else             d_st_.setState( memento->state_ );
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

   if (set_event(memento->event_.name_or_number(),  memento->event_.value())) {
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

   if (set_meter(memento->meter_.name(), memento->meter_.value())) {
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

   size_t theSize = labels_.size();
    for(size_t i = 0; i < theSize; i++) {
       if (labels_[i].name() == memento->label_.name()) {
          labels_[i] = memento->label_;
          return;
       }
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

void Node::set_memento(const NodeGenericMemento* m,std::vector<ecf::Aspect::Type>& aspects,bool aspect_only)
{
   if (aspect_only) {
      // For attribute add/delete Should have already added ecf::Aspect::ADD_REMOVE_ATTR to aspects
      aspects.push_back(ecf::Aspect::GENERIC);
      return;
   }
   if (misc_attrs_) {
      misc_attrs_->set_memento(m);
      return;
   }
   add_generic(m->generic_);
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

	if (t_expr_) {
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

	if (c_expr_) {
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

	size_t theSize = vars_.size();
	for(size_t i = 0; i < theSize; i++) {
		if (vars_[i].name() == memento->var_.name()) {
			vars_[i].set_value( memento->var_.theValue() );
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

	if (late_) {
		late_->setLate(memento->late_.isLate());
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

   for(auto & today : todays_) {
      // We need to ignore state changes in TodayAttr, (ie we don't use equality operator)
      // otherwise today will never compare
      if ( today.structureEquals(memento->attr_) ) {
         today = memento->attr_;  // need to copy over time series state
         return;
      }
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

   for(auto & time : times_) {
      // We need to ignore state changes in TimeAttr, (ie we don't use equality operator)
      // otherwise time will never compare
      if ( time.structureEquals(memento->attr_) ) {
         time = memento->attr_;    // need to copy over time series state
         return;
      }
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

   for(auto & day : days_) {
      // We need to ignore state changes (ie we don't use equality operator)
      // otherwise attributes will never compare
      if ( day.structureEquals(memento->attr_) ) {
         if (memento->attr_.isSetFree()) day.setFree();
         else                            day.clearFree();
         return;
      }
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

   for(auto & date : dates_) {
      // We need to ignore state changes (ie we don't use equality operator)
      // otherwise attributes will never compare
      if ( date.structureEquals(memento->attr_) ) {
         if (memento->attr_.isSetFree()) date.setFree();
         else                            date.clearFree();
         return;
      }
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

   for(auto & cron : crons_) {
      // We need to ignore state changes (ie we don't use equality operator)
      // otherwise attributes will never compare
      if ( cron.structureEquals(memento->attr_) ) {
         cron = memento->attr_;   // need to copy over time series state
         return ;
      }
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

	misc_attrs_ = std::make_unique<MiscAttrs>(this);
   misc_attrs_->verifys_ = memento->verifys_;
}
