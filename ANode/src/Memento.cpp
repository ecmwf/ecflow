/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #32 $ 
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

#include "Memento.hpp"
#include "Str.hpp"

using namespace std;
using namespace ecf;

//#define DEBUG_MEMENTO 1

// ===============================================================
Memento::~Memento() {}

// ===============================================================
void CompoundMemento::incremental_sync(defs_ptr client_def,std::vector<std::string>& changed_nodes) const
{
   /// Clear out aspects, for this Memento.
   ///   Aspects are added via do_incremental_* / set_mememto functions
   ///   AND in *this* function when node attributes have been added or deleted.
   aspects_.clear();

   // Record changes nodes for the Python interface
   changed_nodes.push_back(absNodePath_);

	node_ptr node = client_def->findAbsNode(absNodePath_);
 	if (!node.get()) {
 		if ( absNodePath_ != Str::ROOT_PATH()) throw std::runtime_error("CompoundMemento::incremental_sync: could not find path " + absNodePath_ );

#ifdef DEBUG_MEMENTO
 		cout << "CompoundMemento::incremental_sync: ROOT_PATH   changed_nodes.size()=" << changed_nodes.size() << "\n";
#endif
 		BOOST_FOREACH(memento_ptr m, vec_) {
  			m->do_incremental_defs_sync( client_def.get(), aspects_);
 		}

 		/// Notify any interested parties incremental changes
      /// Aspects records the kind of changes.
 		client_def->notify( aspects_);
	}
 	else {

#ifdef DEBUG_MEMENTO
 		cout << "CompoundMemento::incremental_sync: " << node->debugNodePath() << "  changed_nodes.size()=" << changed_nodes.size() << "\n";
#endif

 		if (clear_attributes_) {
 		   aspects_.push_back(ecf::Aspect::ADD_REMOVE_ATTR);
 			node->clear();
 		}

      Task* task = node->isTask();
      Alias* alias = node->isAlias();
      Suite* suite = node->isSuite();
      Family* family = node->isFamily();

 		BOOST_FOREACH(memento_ptr m, vec_) {
// 		std::cout << "memento = " << typeid(*m.get()).name() << "\n";
 			if (task)        m->do_incremental_task_sync( task, aspects_ );
         else if (alias)  m->do_incremental_alias_sync( alias, aspects_ );
         else if (suite)  m->do_incremental_suite_sync( suite , aspects_);
 			else if (family) m->do_incremental_family_sync( family, aspects_ );
 			m->do_incremental_node_sync( node.get(), aspects_ );
 		}

 		/// Notify any interested parties that Node has made incremental changes
 		/// Aspects records the kind of changes.
 		node->notify( aspects_ );
 	}
}

BOOST_CLASS_EXPORT_IMPLEMENT(StateMemento);
BOOST_CLASS_EXPORT_IMPLEMENT(NodeDefStatusDeltaMemento);
BOOST_CLASS_EXPORT_IMPLEMENT(SuspendedMemento);
BOOST_CLASS_EXPORT_IMPLEMENT(ServerStateMemento);
BOOST_CLASS_EXPORT_IMPLEMENT(ServerVariableMemento);
BOOST_CLASS_EXPORT_IMPLEMENT(NodeEventMemento);
BOOST_CLASS_EXPORT_IMPLEMENT(NodeMeterMemento);
BOOST_CLASS_EXPORT_IMPLEMENT(NodeLabelMemento);
BOOST_CLASS_EXPORT_IMPLEMENT(NodeTriggerMemento);
BOOST_CLASS_EXPORT_IMPLEMENT(NodeCompleteMemento);
BOOST_CLASS_EXPORT_IMPLEMENT(NodeRepeatMemento);
BOOST_CLASS_EXPORT_IMPLEMENT(NodeLimitMemento);
BOOST_CLASS_EXPORT_IMPLEMENT(NodeInLimitMemento);
BOOST_CLASS_EXPORT_IMPLEMENT(NodeVariableMemento);
BOOST_CLASS_EXPORT_IMPLEMENT(NodeLateMemento);
BOOST_CLASS_EXPORT_IMPLEMENT(NodeTodayMemento);
BOOST_CLASS_EXPORT_IMPLEMENT(NodeTimeMemento);
BOOST_CLASS_EXPORT_IMPLEMENT(NodeDayMemento);
BOOST_CLASS_EXPORT_IMPLEMENT(NodeCronMemento);
BOOST_CLASS_EXPORT_IMPLEMENT(NodeDateMemento);
BOOST_CLASS_EXPORT_IMPLEMENT(NodeZombieMemento);
BOOST_CLASS_EXPORT_IMPLEMENT(NodeVerifyMemento);
BOOST_CLASS_EXPORT_IMPLEMENT(FlagMemento);
BOOST_CLASS_EXPORT_IMPLEMENT(SubmittableMemento);
BOOST_CLASS_EXPORT_IMPLEMENT(SuiteClockMemento);
BOOST_CLASS_EXPORT_IMPLEMENT(SuiteBeginDeltaMemento);
BOOST_CLASS_EXPORT_IMPLEMENT(SuiteCalendarMemento);
BOOST_CLASS_EXPORT_IMPLEMENT(OrderMemento);
BOOST_CLASS_EXPORT_IMPLEMENT(ChildrenMemento);
BOOST_CLASS_EXPORT_IMPLEMENT(AliasChildrenMemento);
BOOST_CLASS_EXPORT_IMPLEMENT(AliasNumberMemento);
