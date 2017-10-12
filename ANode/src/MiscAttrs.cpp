/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #286 $
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
#include "Defs.hpp"
#include "Suite.hpp"

#include "MiscAttrs.hpp"
#include "Str.hpp"
#include "Log.hpp"
#include "Ecf.hpp"
#include "Memento.hpp"
#include "AutoRestoreAttr.hpp"
#include "AutoCancelAttr.hpp"
#include "AutoArchiveAttr.hpp"

using namespace ecf;
using namespace std;

///////////////////////////////////////////////////////////////////////////////////////////

MiscAttrs::MiscAttrs(const MiscAttrs& rhs)
: node_(NULL),auto_cancel_(NULL),auto_archive_(NULL),auto_restore_(NULL),
  zombies_(rhs.zombies_),verifys_(rhs.verifys_),queues_(rhs.queues_), generics_(rhs.generics_)
{
   if (rhs.auto_cancel_) auto_cancel_= new AutoCancelAttr(*rhs.auto_cancel_);
   if (rhs.auto_archive_) auto_archive_= new AutoArchiveAttr(*rhs.auto_archive_);
   if (rhs.auto_restore_) auto_restore_= new AutoRestoreAttr(*rhs.auto_restore_);
}

MiscAttrs::~MiscAttrs()
{
   delete auto_cancel_;
   delete auto_archive_;
   delete auto_restore_;
}

// needed by node serialisation
void MiscAttrs::set_node(Node* n)
{
   node_ = n;
   if (auto_restore_) auto_restore_->set_node(n);
}

void MiscAttrs::begin()
{
   // reset verification
   for(size_t i = 0; i < verifys_.size(); i++) { verifys_[i].reset(); }
   for(size_t i = 0; i < queues_.size(); i++)  { queues_[i].reset(); }
}

void MiscAttrs::requeue()
{
   for(size_t i = 0; i < queues_.size(); i++) { queues_[i].reset(); }
}

void MiscAttrs::do_autorestore()
{
   if ( auto_restore_ ) auto_restore_->do_autorestore();
}

void MiscAttrs::check(std::string& errorMsg) const
{
   if ( auto_restore_ ) auto_restore_->check(errorMsg);
}

std::ostream& MiscAttrs::print(std::ostream& os) const
{
   if (auto_cancel_) auto_cancel_->print(os);
   if (auto_archive_) auto_archive_->print(os);
   if (auto_restore_) auto_restore_->print(os);
   BOOST_FOREACH(const ZombieAttr& z, zombies_) { z.print(os); }
   BOOST_FOREACH(const VerifyAttr& v, verifys_ ){ v.print(os);  }
   BOOST_FOREACH(const QueueAttr& q, queues_ )  { q.print(os);  }
   BOOST_FOREACH(const GenericAttr& g, generics_){ g.print(os);  }
   return os;
}

bool MiscAttrs::operator==(const MiscAttrs& rhs) const
{
   if (auto_restore_ && rhs.auto_restore_) {
      if (*auto_restore_ == *rhs.auto_restore_) {
         return true;
      }
#ifdef DEBUG
      if (Ecf::debug_equality()) {
         std::cout << "MiscAttrs::operator== auto_restore_ && rhs.auto_restore_   " << node_->debugNodePath() << "\n";
      }
#endif
      return false;
   }
   else if ( !auto_restore_ && rhs.auto_restore_) {
#ifdef DEBUG
      if (Ecf::debug_equality()) {
         std::cout << "MiscAttrs::operator==  !auto_restore_ && rhs.auto_restore_ " << node_->debugNodePath() << "\n";
      }
#endif
      return false;
   }
   else if ( auto_restore_ && !rhs.auto_restore_ ) {
#ifdef DEBUG
      if (Ecf::debug_equality()) {
         std::cout << "MiscAttrs::operator==  auto_restore_ && !rhs.auto_restore_  " << node_->debugNodePath() << "\n";
      }
#endif
      return false;
   }

   if (auto_cancel_ && !rhs.auto_cancel_) {
#ifdef DEBUG
      if (Ecf::debug_equality()) {
         std::cout << "Node::operator==  if (auto_cancel_ && !rhs.auto_cancel_)  " << node_->debugNodePath() << "\n";
      }
#endif
      return false;
   }
   if (!auto_cancel_ && rhs.auto_cancel_) {
#ifdef DEBUG
      if (Ecf::debug_equality()) {
         std::cout << "Node::operator==  if (!auto_cancel_ && rhs.auto_cancel_)  " << node_->debugNodePath() << "\n";
      }
#endif
      return false;
   }
   if (auto_cancel_ && rhs.auto_cancel_ && !(*auto_cancel_ == *rhs.auto_cancel_)) {
#ifdef DEBUG
      if (Ecf::debug_equality()) {
         std::cout << "Node::operator==  (auto_cancel_ && rhs.auto_cancel_ && !(*auto_cancel_ == *rhs.auto_cancel_)) " << node_->debugNodePath() << "\n";
      }
#endif
      return false;
   }

   if (auto_archive_ && !rhs.auto_archive_) {
#ifdef DEBUG
      if (Ecf::debug_equality()) {
         std::cout << "Node::operator==  if (auto_archive_ && !rhs.auto_archive_)  " << node_->debugNodePath() << "\n";
      }
#endif
      return false;
   }
   if (!auto_archive_ && rhs.auto_archive_) {
#ifdef DEBUG
      if (Ecf::debug_equality()) {
         std::cout << "Node::operator==  if (!auto_archive_ && rhs.auto_archive_)  " << node_->debugNodePath() << "\n";
      }
#endif
      return false;
   }
   if (auto_archive_ && rhs.auto_archive_ && !(*auto_archive_ == *rhs.auto_archive_)) {
#ifdef DEBUG
      if (Ecf::debug_equality()) {
         std::cout << "Node::operator==  (auto_archive_ && rhs.auto_archive_ && !(*auto_archive_ == *rhs.auto_archive_)) " << node_->debugNodePath() << "\n";
      }
#endif
      return false;
   }

   if (zombies_.size() != rhs.zombies_.size()) {
#ifdef DEBUG
      if (Ecf::debug_equality()) {
         std::cout << "MiscAttrs::operator==   (zombies_.size() != rhs.zombies_.size()) " << node_->debugNodePath() << "\n";
      }
#endif
      return false;
   }
   for(unsigned i = 0; i < zombies_.size(); ++i) {
      if (!(zombies_[i] == rhs.zombies_[i]) ) {
#ifdef DEBUG
         if (Ecf::debug_equality()) {
            std::cout << "MiscAttrs::operator==   (!(zombies_[i] == rhs.zombies_[i]) " << node_->debugNodePath() << "\n";
         }
#endif
         return false;
      }
   }

   if (queues_.size() != rhs.queues_.size()) {
#ifdef DEBUG
      if (Ecf::debug_equality()) {
         std::cout << "MiscAttrs::operator==   (queues_.size() != rhs.queues_.size()) " << node_->debugNodePath() << "\n";
      }
#endif
      return false;
   }
   for(unsigned i = 0; i < queues_.size(); ++i) {
      if (!(queues_[i] == rhs.queues_[i]) ) {
#ifdef DEBUG
         if (Ecf::debug_equality()) {
            std::cout << "MiscAttrs::operator==   (!(queues_[i] == rhs.queues_[i]) " << node_->debugNodePath() << "\n";
         }
#endif
         return false;
      }
   }

   if (verifys_.size() != rhs.verifys_.size()) {
#ifdef DEBUG
      if (Ecf::debug_equality()) {
         std::cout << "MiscAttrs::operator==  (verifys_.size() != rhs.verifys_.size()) " << node_->debugNodePath() << "\n";
      }
#endif
      return false;
   }
   for(unsigned i = 0; i < verifys_.size(); ++i) {
      if (!(verifys_[i] == rhs.verifys_[i] )) {
#ifdef DEBUG
         if (Ecf::debug_equality()) {
            std::cout << "MiscAttrs::operator==  (!(verifys_[i] == rhs.verifys_[i] ))  " << node_->debugNodePath() << "\n";
         }
#endif
         return false;
      }
   }

   if (generics_.size() != rhs.generics_.size()) {
#ifdef DEBUG
      if (Ecf::debug_equality()) {
         std::cout << "MiscAttrs::operator==  (generics_.size() != rhs.generics_.size()) " << node_->debugNodePath() << "\n";
      }
#endif
      return false;
   }
   for(unsigned i = 0; i < generics_.size(); ++i) {
      if (!(generics_[i] == rhs.generics_[i] )) {
#ifdef DEBUG
         if (Ecf::debug_equality()) {
            std::cout << "MiscAttrs::operator==  (!(generics_ [i] == rhs.generics_[i] ))  " << node_->debugNodePath() << "\n";
         }
#endif
         return false;
      }
   }
   return true;
}

void MiscAttrs::verification(std::string& errorMsg) const
{
   BOOST_FOREACH(const VerifyAttr& v, verifys_) {
      if (v.expected() != v.actual()) {
         std::stringstream ss;
         ss << node_->debugNodePath() << " expected " << v.expected() << " " << NState::toString(v.state()) << " but found " << v.actual() << "\n";
         errorMsg += ss.str();
      }
   }
}

void MiscAttrs::addZombie( const ZombieAttr& z)
{
   const ZombieAttr& theFndOne = findZombie( z.zombie_type() );
   if (!theFndOne.empty()) {
      std::stringstream ss;
      ss << "MiscAttrs::addZombie: Node " << node_->absNodePath() << " already has a zombie attribute of type " << Child::to_string(theFndOne.zombie_type()) << "\n";
      throw std::runtime_error(ss.str());
   }
   zombies_.push_back( z );
   node_->state_change_no_ = Ecf::incr_state_change_no(); // Only add where used in AlterCmd

#ifdef DEBUG_STATE_CHANGE_NO
   std::cout << "MiscAttrs::addZombie()\n";
#endif
}

void MiscAttrs::deleteZombie(const std::string& zombie_type)
{
   if (zombie_type.empty()) {
      zombies_.clear();
      node_->state_change_no_ = Ecf::incr_state_change_no();
      return;
   }

   if (!Child::valid_zombie_type(zombie_type)) {
      throw std::runtime_error("MiscAttrs::deleteZombie failed: Expected one of [ ecf | path | user ] or empty string but found " + zombie_type);
   }
   delete_zombie( Child::zombie_type(zombie_type) );
}

void MiscAttrs::delete_zombie(Child::ZombieType zt)
{
   for(size_t i = 0; i < zombies_.size(); ++i) {
      if (zombies_[i].zombie_type() == zt) {
         zombies_.erase( zombies_.begin() + i );
         node_->state_change_no_ = Ecf::incr_state_change_no();
         return;
      }
   }
}

const ZombieAttr& MiscAttrs::findZombie( ecf::Child::ZombieType zombie_type) const
{
   /// There should only be one of each type
   for(size_t i = 0; i < zombies_.size(); i++) {
      if ( zombies_[i].zombie_type() == zombie_type ) {
         return zombies_[i];
      }
   }
   return ZombieAttr::EMPTY();
}

void MiscAttrs::add_generic( const GenericAttr& z)
{
   const GenericAttr& theFndOne = find_generic( z.name() );
   if (!theFndOne.empty()) {
      std::stringstream ss;
      ss << "MiscAttrs::add_generic : Node " << node_->absNodePath() << " already has a generic attribute of name " << z.name() << "\n";
      throw std::runtime_error(ss.str());
   }
   generics_.push_back( z );
   node_->state_change_no_ = Ecf::incr_state_change_no(); // Only add where used in AlterCmd
}

void MiscAttrs::addVerify( const VerifyAttr& v )
{
   if (findVerify(v)) {
      std::stringstream ss;
      ss << "Add Verify failed: Duplicate '" << v.toString() << "' already exist for node " << node_->debugNodePath();
      throw std::runtime_error( ss.str() );
   }
   verifys_.push_back(v);
   node_->state_change_no_ = Ecf::incr_state_change_no();
}

bool MiscAttrs::findVerify(const VerifyAttr& v) const
{
   size_t theSize = verifys_.size();
   for(size_t i = 0; i < theSize; i++) {
      if (verifys_[i].state() == v.state() ) {
         return true;
      }
   }
   return false;
}

void MiscAttrs::add_autorestore( const ecf::AutoRestoreAttr& auto_restore)
{
   if (auto_restore_) {
      std::stringstream ss;
      ss << "MiscAttrs::add_auto_restore: Can only have one autorestore per node " << node_->debugNodePath();
      throw std::runtime_error( ss.str() );
   }
   auto_restore_ =  new ecf::AutoRestoreAttr(auto_restore);
   auto_restore_->set_node(node_);
   node_->state_change_no_ = Ecf::incr_state_change_no(); // Only add where used in AlterCmd
}

void MiscAttrs::add_autocancel( const AutoCancelAttr& ac)
{
   if (auto_archive_) {
      std::stringstream ss;
      ss << "Node::addAutoCancel: Can not add autocancel and autoarchive on the same node " << node_->debugNodePath();
      throw std::runtime_error( ss.str() );
   }
   if (auto_cancel_) {
      std::stringstream ss;
      ss << "Node::addAutoCancel: A node can only have one autocancel, see node " << node_->debugNodePath();
      throw std::runtime_error( ss.str() );
   }
   auto_cancel_ = new ecf::AutoCancelAttr(ac);
   node_->state_change_no_ = Ecf::incr_state_change_no();
}

void MiscAttrs::add_autoarchive( const AutoArchiveAttr& ac)
{
   if (auto_cancel_) {
      std::stringstream ss;
      ss << "Node::add_autoarchive: Can not add autocancel and autoarchive on the same node " << node_->debugNodePath();
      throw std::runtime_error( ss.str() );
   }
   if (auto_archive_) {
      std::stringstream ss;
      ss << "Node::add_autoarchive: A node can only have one autoarchive, see node " << node_->debugNodePath();
      throw std::runtime_error( ss.str() );
   }
   auto_archive_ = new ecf::AutoArchiveAttr(ac);
   node_->state_change_no_ = Ecf::incr_state_change_no();
}

void MiscAttrs::clear_attributes_with_state()
{
   // Used during incremental sync, i.e node attributes added/deleted
   // clear attributes that have memento's, the auto_* have no state, hence no need to clear
   zombies_.clear();   // can be added/removed via AlterCmd
   verifys_.clear();
   queues_.clear();
}

void MiscAttrs::add_queue( const QueueAttr& q)
{
   const QueueAttr& theFndOne = find_queue( q.name() );
    if (!theFndOne.empty()) {
       std::stringstream ss;
       ss << "MiscAttrs::add_queue: Node " << node_->absNodePath() << " already has a queue attribute of name " << q.name() << "\n";
       throw std::runtime_error(ss.str());
    }
    queues_.push_back( q );
    node_->state_change_no_ = Ecf::incr_state_change_no(); // Only add where used in AlterCmd
}

void MiscAttrs::delete_queue(const std::string& name)
{
   if (name.empty()) {
      queues_.clear();
      node_->state_change_no_ = Ecf::incr_state_change_no();
      return;
   }
   for(size_t i = 0; i < queues_.size(); ++i) {
      if (queues_[i].name() == name) {
         queues_.erase( queues_.begin() + i );
         node_->state_change_no_ = Ecf::incr_state_change_no();
         return;
      }
   }
}

const QueueAttr& MiscAttrs::find_queue( const std::string& name) const
{
   for(size_t i = 0; i < queues_.size(); i++) {
      if ( queues_[i].name() == name ) {
         return queues_[i];
      }
   }
   return QueueAttr::EMPTY();
}

QueueAttr& MiscAttrs::findQueue(const std::string& name)
{
   for(size_t i = 0; i < queues_.size(); i++) {
      if ( queues_[i].name() == name ) {
         return queues_[i];
      }
   }
   return QueueAttr::EMPTY1();
}

void MiscAttrs::delete_generic(const std::string& name)
{
   if (name.empty()) {
      generics_.clear();
      node_->state_change_no_ = Ecf::incr_state_change_no();
      return;
   }
   for(size_t i = 0; i < generics_.size(); ++i) {
      if ( generics_[i].name() == name) {
         generics_.erase( generics_.begin() + i );
         node_->state_change_no_ = Ecf::incr_state_change_no();
         return;
      }
   }
}

const GenericAttr& MiscAttrs::find_generic( const std::string& name) const
{
   for(size_t i = 0; i < generics_.size(); i++) {
      if (  generics_[i].name() == name ) {
         return generics_[i];
      }
   }
   return GenericAttr::EMPTY();
}

void MiscAttrs::set_memento(const NodeQueueMemento* m)
{
   add_queue(m->queue_);
}

void MiscAttrs::set_memento(const NodeQueueIndexMemento* m )
{
   for(size_t i = 0; i < queues_.size(); i++) {
      if (queues_[i].name() == m->name_) {
         queues_[i].set_index( m->index_);
      }
   }
}

bool MiscAttrs::checkForAutoCancel(const ecf::Calendar& calendar) const
{
   if ( auto_cancel_ && node_->state() == NState::COMPLETE) {
      if (auto_cancel_->isFree(calendar,node_->get_state().second)) {

         /// *Only* delete this node if we don't create zombies
         /// anywhere for our children
         vector<Task*> taskVec;
         node_->getAllTasks(taskVec);
         BOOST_FOREACH(Task* t, taskVec) {
            if (t->state() == NState::ACTIVE || t->state() == NState::SUBMITTED) {
               return false;
            }
         }
         return true;
      }
   }
   return false;
}

bool MiscAttrs::check_for_auto_archive(const ecf::Calendar& calendar) const
{
   if ( auto_archive_ && node_->state() == NState::COMPLETE) {
      if (!node_->isSuspended() && auto_archive_->isFree(calendar,node_->get_state().second)) {
         if (!node_->isParentSuspended()) {

            /// *Only* archive this node if we don't create zombies anywhere for our children
            vector<Task*> taskVec;
            node_->getAllTasks(taskVec);
            BOOST_FOREACH(Task* t, taskVec) {
               if (t->state() == NState::ACTIVE || t->state() == NState::SUBMITTED) {
                  return false;
               }
            }
            return true;
         }
      }
   }
   return false;
}

