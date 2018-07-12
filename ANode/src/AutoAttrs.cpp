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
#include "Task.hpp"

#include "AutoAttrs.hpp"
#include "Str.hpp"
#include "Log.hpp"
#include "Ecf.hpp"
#include "AutoRestoreAttr.hpp"
#include "AutoCancelAttr.hpp"
#include "AutoArchiveAttr.hpp"

using namespace ecf;
using namespace std;

///////////////////////////////////////////////////////////////////////////////////////////

AutoAttrs::AutoAttrs(const AutoAttrs& rhs) : node_(NULL)
{
   if (rhs.auto_cancel_) auto_cancel_ = std::make_unique<AutoCancelAttr>(*rhs.auto_cancel_);
   if (rhs.auto_archive_) auto_archive_ = std::make_unique<AutoArchiveAttr>(*rhs.auto_archive_);
   if (rhs.auto_restore_) auto_restore_ = std::make_unique<AutoRestoreAttr>(*rhs.auto_restore_);
}

AutoAttrs::~AutoAttrs(){}

//void AutoAttrs::delete_autorestore()
//{
//   auto_restore_.reset(nullptr);
//}
//
//void AutoAttrs::delete_autocancel()
//{
//   auto_cancel_.reset(nullptr);
//}
//
//void AutoAttrs::delete_autoarchive()
//{
//   auto_archive_.reset(nullptr);
//}

// needed by node serialisation
void AutoAttrs::set_node(Node* n)
{
   node_ = n;
   if (auto_restore_) auto_restore_->set_node(n);
}

bool AutoAttrs::checkInvariants(std::string& errorMsg) const
{
   if (!node_) {
      errorMsg += "AutoAttrs::checkInvariants: failed node_ is NULL";
      return false;
   }
   return true;
}

void AutoAttrs::do_autorestore()
{
   if ( auto_restore_ ) auto_restore_->do_autorestore();
}

void AutoAttrs::check(std::string& errorMsg) const
{
   if ( auto_restore_ ) auto_restore_->check(errorMsg);
}

std::ostream& AutoAttrs::print(std::ostream& os) const
{
   if (auto_cancel_) auto_cancel_->print(os);
   if (auto_archive_) auto_archive_->print(os);
   if (auto_restore_) auto_restore_->print(os);
   return os;
}

bool AutoAttrs::operator==(const AutoAttrs& rhs) const
{
   if (auto_restore_ && rhs.auto_restore_) {
      if (*auto_restore_ == *rhs.auto_restore_) {
         return true;
      }
#ifdef DEBUG
      if (Ecf::debug_equality()) {
         std::cout << "AutoAttrs::operator== auto_restore_ && rhs.auto_restore_   " << node_->debugNodePath() << "\n";
      }
#endif
      return false;
   }
   else if ( !auto_restore_ && rhs.auto_restore_) {
#ifdef DEBUG
      if (Ecf::debug_equality()) {
         std::cout << "AutoAttrs::operator==  !auto_restore_ && rhs.auto_restore_ " << node_->debugNodePath() << "\n";
      }
#endif
      return false;
   }
   else if ( auto_restore_ && !rhs.auto_restore_ ) {
#ifdef DEBUG
      if (Ecf::debug_equality()) {
         std::cout << "AutoAttrs::operator==  auto_restore_ && !rhs.auto_restore_  " << node_->debugNodePath() << "\n";
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

   return true;
}

void AutoAttrs::add_autorestore( const ecf::AutoRestoreAttr& auto_restore)
{
   if (auto_restore_) {
      std::stringstream ss;
      ss << "AutoAttrs::add_auto_restore: Can only have one autorestore per node " << node_->debugNodePath();
      throw std::runtime_error( ss.str() );
   }
   auto_restore_ = std::make_unique<ecf::AutoRestoreAttr>(auto_restore);
   auto_restore_->set_node(node_);
   node_->state_change_no_ = Ecf::incr_state_change_no(); // Only add where used in AlterCmd
}

void AutoAttrs::add_autocancel( const AutoCancelAttr& ac)
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
   auto_cancel_ = std::make_unique<ecf::AutoCancelAttr>(ac);
   node_->state_change_no_ = Ecf::incr_state_change_no();
}

void AutoAttrs::add_autoarchive( const AutoArchiveAttr& ac)
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
   auto_archive_ = std::make_unique<ecf::AutoArchiveAttr>(ac);
   node_->state_change_no_ = Ecf::incr_state_change_no();
}

bool AutoAttrs::checkForAutoCancel(const ecf::Calendar& calendar) const
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

bool AutoAttrs::check_for_auto_archive(const ecf::Calendar& calendar) const
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
