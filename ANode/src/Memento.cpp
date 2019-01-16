/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #32 $ 
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

#include "Memento.hpp"
#include "Str.hpp"
#include "Serialization.hpp"

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
      if ( aspect_size != aspects_.size()) {
         assert(aspect_size == aspects_.size()); // aspect size should not change, when making data model changes
      }
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
      if ( aspect_size != aspects_.size()) {
         assert(aspect_size == aspects_.size()); // aspect size should not change, when making data model changes
      }

 		/// Notify any interested parties that Node has made incremental changes
 		/// Aspects records the kind of changes.
 		node->notify( aspects_ );
 	}
}

// ===================================================================================================

template<class Archive>
void Memento::serialize(Archive & ar, std::uint32_t const version ){}

template<class Archive>
void CompoundMemento::serialize(Archive & ar, std::uint32_t const version )
{
   CEREAL_OPTIONAL_NVP(ar, clear_attributes_, [this](){return clear_attributes_; });  // conditionally save
   ar(CEREAL_NVP(absNodePath_),
      CEREAL_NVP(vec_));
}

template<class Archive>
void StateMemento::serialize(Archive & ar, std::uint32_t const version )
{
   ar(cereal::base_class<Memento>(this),
      CEREAL_NVP(state_));
}

template<class Archive>
void OrderMemento::serialize(Archive & ar, std::uint32_t const version )
{
   ar(cereal::base_class<Memento>(this),
      CEREAL_NVP(order_));
}

template<class Archive>
void ChildrenMemento::serialize(Archive & ar, std::uint32_t const version )
{
   ar(cereal::base_class<Memento>(this),
      CEREAL_NVP(children_));
}

template<class Archive>
void AliasChildrenMemento::serialize(Archive & ar, std::uint32_t const version )
{
   ar(cereal::base_class<Memento>(this),
      CEREAL_NVP(children_));
}

template<class Archive>
void AliasNumberMemento::serialize(Archive & ar, std::uint32_t const version )
{
   ar(cereal::base_class<Memento>(this),
      CEREAL_NVP(alias_no_));
}

template<class Archive>
void SuspendedMemento::serialize(Archive & ar, std::uint32_t const version )
{
   ar(cereal::base_class<Memento>(this),
      CEREAL_NVP(suspended_));
}

template<class Archive>
void ServerStateMemento::serialize(Archive & ar, std::uint32_t const version )
{
   ar(cereal::base_class<Memento>(this),
      CEREAL_NVP(state_));
}

template<class Archive>
void ServerVariableMemento::serialize(Archive & ar, std::uint32_t const version )
{
   ar(cereal::base_class<Memento>(this),
      CEREAL_NVP(serverEnv_));
}

template<class Archive>
void NodeDefStatusDeltaMemento::serialize(Archive & ar, std::uint32_t const version )
{
   ar(cereal::base_class<Memento>(this),
      CEREAL_NVP(state_));
}

template<class Archive>
void NodeEventMemento::serialize(Archive & ar, std::uint32_t const version )
{
   ar(cereal::base_class<Memento>(this),
      CEREAL_NVP(event_));
}

template<class Archive>
void NodeMeterMemento::serialize(Archive & ar, std::uint32_t const version )
{
   ar(cereal::base_class<Memento>(this),
      CEREAL_NVP(meter_));
}

template<class Archive>
void NodeLabelMemento::serialize(Archive & ar, std::uint32_t const version )
{
   ar(cereal::base_class<Memento>(this),
      CEREAL_NVP(label_));
}

template<class Archive>
void NodeQueueMemento::serialize(Archive & ar, std::uint32_t const version )
{
   ar(cereal::base_class<Memento>(this),
      CEREAL_NVP(queue_));
}

template<class Archive>
void NodeGenericMemento::serialize(Archive & ar, std::uint32_t const version )
{
   ar(cereal::base_class<Memento>(this),
      CEREAL_NVP(generic_));
}

template<class Archive>
void NodeQueueIndexMemento::serialize(Archive & ar, std::uint32_t const version )
{
   ar(cereal::base_class<Memento>(this),
      CEREAL_NVP(index_),
      CEREAL_NVP(name_),
      CEREAL_NVP(state_vec_));
}

template<class Archive>
void NodeTriggerMemento::serialize(Archive & ar, std::uint32_t const version )
{
   ar(cereal::base_class<Memento>(this),
      CEREAL_NVP(exp_));
}

template<class Archive>
void NodeCompleteMemento::serialize(Archive & ar, std::uint32_t const version )
{
   ar(cereal::base_class<Memento>(this),
      CEREAL_NVP(exp_));
}

template<class Archive>
void NodeRepeatMemento::serialize(Archive & ar, std::uint32_t const version )
{
   ar(cereal::base_class<Memento>(this),
      CEREAL_NVP(repeat_));
}

template<class Archive>
void NodeRepeatIndexMemento::serialize(Archive & ar, std::uint32_t const version )
{
   ar(cereal::base_class<Memento>(this),
      CEREAL_NVP(index_or_value_));
}

template<class Archive>
void NodeLimitMemento::serialize(Archive & ar, std::uint32_t const version )
{
   ar(cereal::base_class<Memento>(this),
      CEREAL_NVP(limit_));
}

template<class Archive>
void NodeInLimitMemento::serialize( Archive & ar, std::uint32_t const version  ) {
   ar(cereal::base_class<Memento>(this),
      CEREAL_NVP(inlimit_));
}

template<class Archive>
void NodeVariableMemento::serialize(Archive & ar, std::uint32_t const version )
{
   ar(cereal::base_class<Memento>(this),
      CEREAL_NVP(var_));
}

template<class Archive>
void NodeLateMemento::serialize(Archive & ar, std::uint32_t const version )
{
   ar(cereal::base_class<Memento>(this),
      CEREAL_NVP(late_));
}

template<class Archive>
void FlagMemento::serialize(Archive & ar, std::uint32_t const version )
{
   ar(cereal::base_class<Memento>(this),
      CEREAL_NVP(flag_));
}

template<class Archive>
void NodeTodayMemento::serialize(Archive & ar, std::uint32_t const version )
{
   ar(cereal::base_class<Memento>(this),
      CEREAL_NVP(attr_));
}

template<class Archive>
void NodeTimeMemento::serialize(Archive & ar, std::uint32_t const version )
{
   ar(cereal::base_class<Memento>(this),
      CEREAL_NVP(attr_));
}

template<class Archive>
void NodeDayMemento::serialize(Archive & ar, std::uint32_t const version )
{
   ar(cereal::base_class<Memento>(this),
      CEREAL_NVP(attr_));
}

template<class Archive>
void NodeCronMemento::serialize(Archive & ar, std::uint32_t const version )
{
   ar(cereal::base_class<Memento>(this),
      CEREAL_NVP(attr_));
}

template<class Archive>
void NodeDateMemento::serialize(Archive & ar, std::uint32_t const version )
{
   ar(cereal::base_class<Memento>(this),
      CEREAL_NVP(attr_));
}

template<class Archive>
void NodeZombieMemento::serialize(Archive & ar, std::uint32_t const version )
{
   ar(cereal::base_class<Memento>(this),
      CEREAL_NVP(attr_));
}

template<class Archive>
void NodeVerifyMemento::serialize(Archive & ar, std::uint32_t const version )
{
   ar(cereal::base_class<Memento>(this),
      CEREAL_NVP(verifys_));
}

template<class Archive>
void SubmittableMemento::serialize(Archive & ar, std::uint32_t const version )
{
   ar(cereal::base_class<Memento>(this),
      CEREAL_NVP(paswd_),
      CEREAL_NVP(rid_),
      CEREAL_NVP(abr_),
      CEREAL_NVP(tryNo_));
}

template<class Archive>
void SuiteClockMemento::serialize(Archive & ar, std::uint32_t const version )
{
   ar(cereal::base_class<Memento>(this),
      CEREAL_NVP(clockAttr_));
}

template<class Archive>
void SuiteBeginDeltaMemento::serialize(Archive & ar, std::uint32_t const version )
{
   ar(cereal::base_class<Memento>(this),
      CEREAL_NVP(begun_));
}

template<class Archive>
void SuiteCalendarMemento::serialize(Archive & ar, std::uint32_t const version )
{
   ar(cereal::base_class<Memento>(this),
      CEREAL_NVP(cal_));
}

CEREAL_TEMPLATE_SPECIALIZE_V(Memento);
CEREAL_TEMPLATE_SPECIALIZE_V(CompoundMemento);

CEREAL_TEMPLATE_SPECIALIZE_V(StateMemento);
CEREAL_TEMPLATE_SPECIALIZE_V(NodeDefStatusDeltaMemento);
CEREAL_TEMPLATE_SPECIALIZE_V(SuspendedMemento);
CEREAL_TEMPLATE_SPECIALIZE_V(ServerStateMemento);
CEREAL_TEMPLATE_SPECIALIZE_V(ServerVariableMemento);
CEREAL_TEMPLATE_SPECIALIZE_V(NodeEventMemento);
CEREAL_TEMPLATE_SPECIALIZE_V(NodeMeterMemento);
CEREAL_TEMPLATE_SPECIALIZE_V(NodeLabelMemento);
CEREAL_TEMPLATE_SPECIALIZE_V(NodeQueueMemento);
CEREAL_TEMPLATE_SPECIALIZE_V(NodeGenericMemento);
CEREAL_TEMPLATE_SPECIALIZE_V(NodeQueueIndexMemento);
CEREAL_TEMPLATE_SPECIALIZE_V(NodeTriggerMemento);
CEREAL_TEMPLATE_SPECIALIZE_V(NodeCompleteMemento);
CEREAL_TEMPLATE_SPECIALIZE_V(NodeRepeatMemento);
CEREAL_TEMPLATE_SPECIALIZE_V(NodeRepeatIndexMemento);
CEREAL_TEMPLATE_SPECIALIZE_V(NodeLimitMemento);
CEREAL_TEMPLATE_SPECIALIZE_V(NodeInLimitMemento);
CEREAL_TEMPLATE_SPECIALIZE_V(NodeVariableMemento);
CEREAL_TEMPLATE_SPECIALIZE_V(NodeLateMemento);
CEREAL_TEMPLATE_SPECIALIZE_V(NodeTodayMemento);
CEREAL_TEMPLATE_SPECIALIZE_V(NodeTimeMemento);
CEREAL_TEMPLATE_SPECIALIZE_V(NodeDayMemento);
CEREAL_TEMPLATE_SPECIALIZE_V(NodeCronMemento);
CEREAL_TEMPLATE_SPECIALIZE_V(NodeDateMemento);
CEREAL_TEMPLATE_SPECIALIZE_V(NodeZombieMemento);
CEREAL_TEMPLATE_SPECIALIZE_V(NodeVerifyMemento);
CEREAL_TEMPLATE_SPECIALIZE_V(FlagMemento);
CEREAL_TEMPLATE_SPECIALIZE_V(SubmittableMemento);
CEREAL_TEMPLATE_SPECIALIZE_V(SuiteClockMemento);
CEREAL_TEMPLATE_SPECIALIZE_V(SuiteBeginDeltaMemento);
CEREAL_TEMPLATE_SPECIALIZE_V(SuiteCalendarMemento);
CEREAL_TEMPLATE_SPECIALIZE_V(OrderMemento);
CEREAL_TEMPLATE_SPECIALIZE_V(ChildrenMemento);
CEREAL_TEMPLATE_SPECIALIZE_V(AliasChildrenMemento);
CEREAL_TEMPLATE_SPECIALIZE_V(AliasNumberMemento);


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
