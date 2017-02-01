#ifndef CLIENT_SUITES_HPP_
#define CLIENT_SUITES_HPP_
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #26 $ 
//
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description :
// The definition file could hold hundreds of suites, however the client
// may only be interested in a small subset. By allowing the client to register
// the suites they are interested in, we can reduce the network traffic
// when they ask for updates.(ie via sync or news)
// When the Client registers an interest in suites it is returned a handle,
// this handle must is passed back to the server as a reference, with the sync'ing commands
//
// Users are allowed to register interest in suite that have not yet been added
// This will only work provided we have a definition
//
// ***************************************************************************
// Note: Change of suite order is handled by OrderMemento
//       and *NOT* by the ClientSuites, however whenever suites are:
//       registered and added/deleted:
//       then:
//           defs_ptr create_defs(Defs* server_defs) const;
//
//       Will return the suites in the same order as the defs
// ****************************************************************************
//
// Uses compiler generated copy constructor and destructor
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8

#include <string>
#include <vector>
#include <limits>
#include "NodeFwd.hpp"

namespace ecf {

struct HSuite {
   HSuite(const std::string& name, weak_suite_ptr p, int index = std::numeric_limits<int>::max())
   : name_(name), weak_suite_ptr_(p),index_(index) {}

   HSuite(const std::string& name )
   : name_(name),index_(std::numeric_limits<int>::max()) {}

   std::string    name_;           // suite name
   weak_suite_ptr weak_suite_ptr_; // does suite exist in defs, need to lock, to find out
   int            index_;          // suites in handles must be in same order as defs
};

class ClientSuites {
public:
	/// Will automatically create a unique handle
	ClientSuites(Defs*,unsigned int handle, bool auto_add_new_suites, const std::vector<std::string>& suites , const std::string& user);

	/// Return the handle, returned to the client, and required for all correspondence
	/// between client server. This value is always > 0
	unsigned int handle() const { return handle_;}

	/// return the user who requested this handle
	const std::string& user()  const { return user_; }

	/// The handle changed flag is set to true whenever a ClinetSuites is created
	/// or when suites are added/removed from it. The alternative way would have
	/// been to update the modify change number, but this would have affected all handles
	bool handle_changed() const { return handle_changed_; }

	/// Add an interest to this suite
   void add_suite(const std::string&);
   void add_suite(suite_ptr);

	/// no longer interested in this suite. Explicit remove
   void remove_suite(const std::string&);
   bool remove_suite(suite_ptr);

   /// A new suite is being added in the definition.
   /// If it was already registered update the suite ptr
   /// If auto add new suite enabled,register it
   void suite_added_in_defs(suite_ptr);

   /// The suite is being deleted, update modify_change_no. So we do a full sync
   /// RESETs suite ptr. Deleted suites are *NOT* automatically removed
   void suite_deleted_in_defs(suite_ptr);

	/// Collate the incremental changes, made to my suites
	void collateChanges(DefsDelta& changes) const;

   // Only return the defs state and suites that the client has registered in this suite
   // *HOWEVER* if the client has registered all the suites, just return the server defs
   //           *with* the updated change numbers
   // *OTHERWISE*
	/// Creates a new defs, by adding the suites to the defs
	/// The defs is to be transferred to the client
	/// Suites are returned in the same order as the defs
	/// We avoid copying, but need to adjust suites defs pointer
	/// This will clear the handle_changed_  flag
	defs_ptr create_defs(defs_ptr server_defs) const;

	// iterates overs its suites and return max state and modify change numbers
	void max_change_no(unsigned int& state_change_no,unsigned int& modify_change_no ) const;

	/// Enable/disable interest in new suites.
	/// if the flag is true, when ever new suites are added to the defs
	/// The internal list is updated.
	void add_new_suite( bool flag ) { auto_add_new_suites_ = flag;}
	bool auto_add_new_suites() const { return auto_add_new_suites_;}

	/// returns the list of suites
	void suites(std::vector<std::string>& names) const;

	/// Update suites to be in same order as Defs.
	/// This should be done externally to avoid update after individual add
	void update_suite_order();

	/// For debug dumps
	std::string dump() const;

private:
	std::vector<HSuite>::iterator find_suite(const std::string& name);

private:
   Defs* defs_;
   unsigned int handle_;                   // This must be unique

   // The modify_change_no_ is required specifically when a registered suite is deleted
   // Both modify_change_no_ & state_change_no_ are required when user has registered
   // with *ALL* the suites. In this case we need to ensure that after a SYNC/create_defs
   // the NewsCmd call to max_change_no() is in sync with global change numbers.
   mutable unsigned int state_change_no_;
   mutable unsigned int modify_change_no_;

	bool auto_add_new_suites_;
	mutable bool handle_changed_;           // set when handle created, or when suites added/removed
	std::string user_;                        // user who create this handle
	std::vector<HSuite> suites_;
};
}
#endif
