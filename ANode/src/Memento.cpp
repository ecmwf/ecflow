/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #32 $ 
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

#include "Memento.hpp"
#include "Str.hpp"

using namespace std;
using namespace ecf;

//#define DEBUG_MEMENTO 1

// ===============================================================
Memento::~Memento() = default;

// ===============================================================
void CompoundMemento::incremental_sync(defs_ptr client_def) const
{
   /// Clear out aspects, for this Memento.
   ///   Aspects are added via do_incremental_* / set_mememto functions
   ///   AND in *this* function when node attributes have been added or deleted.
   aspects_.clear();

	node_ptr node = client_def->findAbsNode(absNodePath_);
 	if (!node.get()) {
 		if ( absNodePath_ != Str::ROOT_PATH()) throw std::runtime_error("CompoundMemento::incremental_sync: could not find path " + absNodePath_ );

#ifdef DEBUG_MEMENTO
 		cout << "CompoundMemento::incremental_sync: ROOT_PATH   changed_nodes.size()=" << changed_nodes.size() << "\n";
#endif
 		//
      // Notify observers what aspect is going to change, before make-ing data model changes
 		//
 		BOOST_FOREACH(memento_ptr m, vec_) {
  			m->do_incremental_defs_sync( client_def.get(), aspects_,true/* collect aspects only, don't make any changes*/);
 		}
 		size_t aspect_size = aspects_.size();
      client_def->notify_start( aspects_);


      /// make data model change.
 		/// Notify any interested parties incremental changes
      /// Aspects records the kind of changes.
      BOOST_FOREACH(memento_ptr m, vec_) {
          m->do_incremental_defs_sync( client_def.get(), aspects_,false/*Data model changes only*/);
      }
      assert(aspect_size == aspects_.size()); // aspect size should not change, when making data model changes
 		client_def->notify(aspects_);
	}
 	else {

#ifdef DEBUG_MEMENTO
 		cout << "CompoundMemento::incremental_sync: " << node->debugNodePath() << "  changed_nodes.size()=" << changed_nodes.size() << "\n";
#endif
 		// Notify observers what aspect, is going to change.
      Task* task = node->isTask();
      Alias* alias = node->isAlias();
      Suite* suite = node->isSuite();
      Family* family = node->isFamily();

 		if (clear_attributes_)  aspects_.push_back(ecf::Aspect::ADD_REMOVE_ATTR);

 		BOOST_FOREACH(memento_ptr m, vec_) {
 			if (task)        m->do_incremental_task_sync( task, aspects_,true/* collect aspects only, don't make any changes*/ );
         else if (alias)  m->do_incremental_alias_sync( alias, aspects_,true/* collect aspects only, don't make any changes*/ );
         else if (suite)  m->do_incremental_suite_sync( suite , aspects_,true/* collect aspects only, don't make any changes*/);
 			else if (family) m->do_incremental_family_sync( family, aspects_,true/* collect aspects only, don't make any changes*/ );
 			m->do_incremental_node_sync( node.get(), aspects_,true/* collect aspects only, don't make any changes*/ );
 		}
      size_t aspect_size = aspects_.size();
      node->notify_start( aspects_ );

      //
      // data model changes only, aspects should not change
      //
      if (clear_attributes_) node->clear();

      BOOST_FOREACH(memento_ptr m, vec_) {
         if (task)        m->do_incremental_task_sync( task, aspects_, false/*Data model changes only*/);
         else if (alias)  m->do_incremental_alias_sync( alias, aspects_,false/*Data model changes only*/);
         else if (suite)  m->do_incremental_suite_sync( suite , aspects_,false/*Data model changes only*/);
         else if (family) m->do_incremental_family_sync( family, aspects_,false/*Data model changes only*/);
         m->do_incremental_node_sync( node.get(), aspects_,false/*Data model changes only*/);
      }
      assert(aspect_size == aspects_.size()); // aspect size should not change, when making data model changes

 		/// Notify any interested parties that Node has made incremental changes
 		/// Aspects records the kind of changes.
 		node->notify( aspects_ );
 	}
}

CEREAL_REGISTER_TYPE(StateMemento);
CEREAL_REGISTER_TYPE(NodeDefStatusDeltaMemento);
CEREAL_REGISTER_TYPE(SuspendedMemento);
CEREAL_REGISTER_TYPE(ServerStateMemento);
CEREAL_REGISTER_TYPE(ServerVariableMemento);
CEREAL_REGISTER_TYPE(NodeEventMemento);
CEREAL_REGISTER_TYPE(NodeMeterMemento);
CEREAL_REGISTER_TYPE(NodeLabelMemento);
CEREAL_REGISTER_TYPE(NodeQueueMemento);
CEREAL_REGISTER_TYPE(NodeGenericMemento);
CEREAL_REGISTER_TYPE(NodeQueueIndexMemento);
CEREAL_REGISTER_TYPE(NodeTriggerMemento);
CEREAL_REGISTER_TYPE(NodeCompleteMemento);
CEREAL_REGISTER_TYPE(NodeRepeatMemento);
CEREAL_REGISTER_TYPE(NodeRepeatIndexMemento);
CEREAL_REGISTER_TYPE(NodeLimitMemento);
CEREAL_REGISTER_TYPE(NodeInLimitMemento);
CEREAL_REGISTER_TYPE(NodeVariableMemento);
CEREAL_REGISTER_TYPE(NodeLateMemento);
CEREAL_REGISTER_TYPE(NodeTodayMemento);
CEREAL_REGISTER_TYPE(NodeTimeMemento);
CEREAL_REGISTER_TYPE(NodeDayMemento);
CEREAL_REGISTER_TYPE(NodeCronMemento);
CEREAL_REGISTER_TYPE(NodeDateMemento);
CEREAL_REGISTER_TYPE(NodeZombieMemento);
CEREAL_REGISTER_TYPE(NodeVerifyMemento);
CEREAL_REGISTER_TYPE(FlagMemento);
CEREAL_REGISTER_TYPE(SubmittableMemento);
CEREAL_REGISTER_TYPE(SuiteClockMemento);
CEREAL_REGISTER_TYPE(SuiteBeginDeltaMemento);
CEREAL_REGISTER_TYPE(SuiteCalendarMemento);
CEREAL_REGISTER_TYPE(OrderMemento);
CEREAL_REGISTER_TYPE(ChildrenMemento);
CEREAL_REGISTER_TYPE(AliasChildrenMemento);
CEREAL_REGISTER_TYPE(AliasNumberMemento);
