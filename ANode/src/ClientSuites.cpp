/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #35 $ 
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

#include <boost/bind.hpp>

#include "ClientSuites.hpp"
#include "Suite.hpp"
#include "Defs.hpp"
#include "DefsDelta.hpp"
#include "Ecf.hpp"

//#define DEBUG_ME 1

namespace ecf {

ClientSuites::ClientSuites(Defs* defs,unsigned int handle, bool auto_add_new_suites, const std::vector<std::string>& suites,const std::string& user)
: defs_(defs),
  handle_(handle),
  state_change_no_(0),
  modify_change_no_(0),
  auto_add_new_suites_(auto_add_new_suites),
  handle_changed_(false),
  user_(user)
{
   BOOST_FOREACH(const std::string& s, suites) {
      add_suite(s);
   }
}

void ClientSuites::add_suite(const std::string& s)
{
   suite_ptr suite = defs_->findSuite(s);
   if (suite.get()) {
      add_suite(suite);
   }
   else  {
      auto i = find_suite(s);
      if (i != suites_.end()) {
         (*i).weak_suite_ptr_ = weak_suite_ptr();
      }
      else {
         suites_.emplace_back(s );
      }
   }
}

void ClientSuites::add_suite(suite_ptr suite)
{
   if (suite.get()) {

      // *IMPORTANT* update weak_suite_ptr_
      auto i = find_suite(suite->name());
      if (i != suites_.end()) {
         (*i).weak_suite_ptr_ = weak_suite_ptr(suite);
      }
      else {
         suites_.emplace_back(suite->name(),weak_suite_ptr(suite) );
      }
      handle_changed_ = true;

#ifdef DEBUG_ME
      i = suites_.find_suite(suite->name());
      assert(i != suites_.end());
      assert((*i).weak_suite_ptr_.lock().get());
#endif
   }
}

void ClientSuites::remove_suite(const std::string& s)
{
   auto i = find_suite(s);
   if (i != suites_.end()) {
      if ( (*i).weak_suite_ptr_.lock().get() ) {
         handle_changed_ = true;
      }
      suites_.erase(i);
   }
}

bool ClientSuites::remove_suite(suite_ptr suite)
{
   if (suite.get()) {
      auto i = find_suite(suite->name());
      if (i != suites_.end()) {
         handle_changed_ = true;
         suites_.erase(i);
         return true;
      }
   }
   return false;
}


void ClientSuites::suite_added_in_defs(suite_ptr suite)
{
   if (auto_add_new_suites_)  add_suite(suite);
   else {
      // *IF* and *ONLY IF* the suite was previously registered added, *UPDATE* its suite_ptr
      auto i = find_suite(suite->name());
      if (i != suites_.end()) {
         // previously registered suite, update
         add_suite(suite);
      }
   }
}

void ClientSuites::suite_deleted_in_defs(suite_ptr suite)
{
   // Deleted suites are *NOT* automatically removed
   // They have to be moved explicitly by the user. Reset to weak ptr
   if (suite.get()) {
      auto i = find_suite(suite->name());
      if (i != suites_.end()) {
         handle_changed_ = true;
         modify_change_no_ = Ecf::modify_change_no();  // need to pass back to client
         (*i).weak_suite_ptr_ = weak_suite_ptr();      // reset suite ptr, not strictly necessary
       }
   }
}

void ClientSuites::collateChanges(DefsDelta& changes) const
{
   BOOST_FOREACH(const HSuite& s, suites_) {
      suite_ptr suite = s.weak_suite_ptr_.lock();
      if (suite.get()) {
         if (suite->state_change_no() > changes.client_state_change_no()) {
            suite->collateChanges( changes );
         }
      }
   }
}

defs_ptr ClientSuites::create_defs(defs_ptr server_defs) const
{
   /// Clear handle changed, so we can detect suites added or removed for this handle
   handle_changed_ = false;

   // If the user has registered *ALL* the suites just return the server defs
   auto suites_end = suites_.end();
   if (suites_.size() == server_defs->suiteVec().size()) {
      size_t real_suite_count = 0;
      for(auto i = suites_.begin(); i != suites_end; ++i) {
         suite_ptr suite = (*i).weak_suite_ptr_.lock();
         if (suite.get()) real_suite_count++;
      }
      if ( real_suite_count == server_defs->suiteVec().size()) {

         server_defs->set_state_change_no( Ecf::state_change_no() );
         server_defs->set_modify_change_no( Ecf::modify_change_no() );

         // Update local modify_change_no_ *AND* state_change_no_
         // ***** Note: Otherwise NewsCmd which computes change numbers over
         // ***** registered suites will not be correct. causing unnecessary sync's
         modify_change_no_ = Ecf::modify_change_no();
         state_change_no_ = Ecf::state_change_no();
         return server_defs;
      }
   }


   // CREATE NEW DEFS, using the registered suites
   //
   // *** We do not use local state_change_no_. That is used ONLY when all suites ***
   // *** are registered.                                                         ***
   //
   // add_suite() below will incremented the modify_change_no.
   // We don't want to do this, as change for suites *not in* the our handle will get skewed
   // This class ensure that any changes made are reset to their original values
   EcfPreserveChangeNo preserveChangeNo;

   // Create defs to be sent to the client, with the registered suites.
   defs_ptr newly_created_defs = Defs::create();
   newly_created_defs->copy_defs_state_only(server_defs);

   // Store the state/modify change number to the newly created defs *over* the objects that have changed
   unsigned int the_max_state_change_no = server_defs->defs_only_max_state_change_no();
   unsigned int the_max_modify_change_no = 0;

   // Handle suites that get deleted. Need a full sync
   the_max_modify_change_no = std::max( the_max_modify_change_no,  modify_change_no_  );

   // Suites should already be in order
   for(auto i = suites_.begin(); i != suites_end; ++i) {
      suite_ptr suite = (*i).weak_suite_ptr_.lock();
      if (suite.get()) {

         // Preserve the change/modify numbers, as these will updated on the suite, by addSuite() below
         unsigned int suite_state_change_no = suite->state_change_no();
         unsigned int suite_modify_change_no = suite->modify_change_no();

         the_max_state_change_no = std::max( the_max_state_change_no, suite_state_change_no );
         the_max_modify_change_no = std::max( the_max_modify_change_no, suite_modify_change_no );

         // To avoid copying the suites, we will just add the suites, to the newly created defs
         // However this presents a problem with the suites defs pointer. To avoid corrupting
         // the server, we must reset the defs pointer.
         // The newly_created_defs/Defs serialisation will re-adjust the suites defs pointer
         Defs* old_defs = suite->defs();

         // This will end up setting the suite's defs pointer to 'newly_created_defs'.
         // This is wrong, since we only have a single suite
         suite->set_defs(nullptr); // otherwise addSuite, will complain
         newly_created_defs->addSuite(suite); // will update modify_change_no, see comment at top

         suite->set_defs(old_defs);                            // reset the defs, since addSuite() changed defs ptr
         suite->set_state_change_no(suite_state_change_no);    // reset in case addSuite() changed this
         suite->set_modify_change_no(suite_modify_change_no);  // reset in case addSuite() changed this
      }
   }

   // Store the max in the defs, for transmission to client. The client uses this for syncing
   newly_created_defs->set_state_change_no(the_max_state_change_no);
   newly_created_defs->set_modify_change_no(the_max_modify_change_no);
   return newly_created_defs;
}


void ClientSuites::max_change_no(unsigned int& the_max_state_change_no,unsigned int& the_max_modify_change_no ) const
{
   /// get the max state change_no due to:
   ///   o Defs state changed
   ///   o Suite order changed
   ///   o Defs flag changed
   ///   o Defs server state changed.
   ///   o Defs server variables changed
   the_max_state_change_no = defs_->defs_only_max_state_change_no();

   // Take into account case where all suites are registered
   the_max_state_change_no = std::max( the_max_state_change_no,  state_change_no_  );

   // Take into account registered suites that get deleted, and where all suites registered
   the_max_modify_change_no = 0;
   the_max_modify_change_no = std::max( the_max_modify_change_no,  modify_change_no_  );

   BOOST_FOREACH(const HSuite& p, suites_) {
      suite_ptr suite = p.weak_suite_ptr_.lock();
      if (suite.get()) {
         the_max_modify_change_no = std::max( the_max_modify_change_no, suite->modify_change_no() );
         the_max_state_change_no  = std::max( the_max_state_change_no,  suite->state_change_no() );
      }
   }
}

void ClientSuites::suites(std::vector<std::string>& names) const
{
   names.reserve(suites_.size());
   auto suites_end = suites_.end();
   for(auto i = suites_.begin(); i != suites_end; ++i) {
       names.push_back( (*i).name_ );
   }
}

std::string ClientSuites::dump() const
{
   unsigned int maxstatechangeno = 0;
   unsigned int maxmodifychangeno = 0;
   max_change_no(maxstatechangeno,maxmodifychangeno);

   std::stringstream ss;
   ss << "  handle(" << handle() << ") user(" << user_ << ") auto_add_new_suites(" << auto_add_new_suites_
      << ") suites_.size(" << suites_.size() << ") suites(";

   auto suites_end = suites_.end();
   for(auto i = suites_.begin(); i != suites_end; ++i) {
      suite_ptr suite = (*i).weak_suite_ptr_.lock();
      if (suite.get()) {
         ss << " " << suite->name();
      }
      else {
         ss << " " << (*i).name_ << ":NULL";
      }
   }
   ss << ") max(" << maxstatechangeno << "," << maxmodifychangeno << ")";
   return ss.str();
}


void ClientSuites::update_suite_order()
{
   const std::vector<suite_ptr>& server_suite_vec = defs_->suiteVec();
   size_t server_suite_vec_size = server_suite_vec.size();

   auto suites_end = suites_.end();
   for(auto i = suites_.begin(); i != suites_end; ++i) {
      for(size_t s = 0;  s < server_suite_vec_size; s++) {
         if ((*i).name_ == server_suite_vec[s]->name()) {
            (*i).index_ = static_cast<int>(s);
            break;
         }
      }
   }

   std::sort(suites_.begin(),suites_.end(),
            boost::bind(std::less<int>(),
                          boost::bind(&HSuite::index_,_1),
                          boost::bind(&HSuite::index_,_2)));

}

std::vector<HSuite>::iterator ClientSuites::find_suite(const std::string& name)
{
   auto suites_end = suites_.end();
   for(auto i = suites_.begin(); i != suites_end; ++i) {
      if ((*i).name_ == name) {
         return i;
      }
   }
   return suites_end;
}

}
