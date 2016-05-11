#ifndef CLIENT_SUITES_MGR_HPP_
#define CLIENT_SUITES_MGR_HPP_
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #24 $ 
//
// Copyright 2009-2016 ECMWF. 
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description :
//
// The client may only want to view a small subset of the suites available
// in a defs file. This class manages the client handles
//
// Users are allowed to register interest in suite that have not yet been added
// This will only work provided we have a definition
//
// Only handle with value > 0 are valid. This is because sync() and news() command
// take a client_handle. By reserving a client handle of zero, we can sync with the
// full defs.
//
// ***************************************************************************
// Note: Change of suite order is handled by OrderMemento
//       and *NOT* by the ClientSuiteMgr, however whenever suites are
//       registered and added/deleted:
//       then:
//           defs_ptr create_defs(unsigned int client_handle) const;
//
//       Will return the suites in the same order as the defs
// ****************************************************************************
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8

#include <boost/noncopyable.hpp>
#include "ClientSuites.hpp"
class DefsDelta;

class ClientSuiteMgr : private boost::noncopyable {
public:
 	ClientSuiteMgr(Defs*);

 	/// Create a client suite, and return the handle associated with the created object
 	unsigned int create_client_suite(bool auto_add_new_suites, const std::vector<std::string>& suites, const std::string& user);


 	/// These function can throw std::runtime_error if the handle can not be found
   void remove_client_suite(unsigned int client_handle);
   void remove_client_suites(const std::string& user_to_drop);
 	void add_suites(unsigned int client_handle, const std::vector<std::string>& suites);
 	void remove_suites(unsigned int client_handle, const std::vector<std::string>& suites);
 	void auto_add_new_suites(unsigned int client_handle, bool auto_add_new_suites);

 	/// Return true if the input handle is valid
 	bool valid_handle(unsigned int client_handle) const;

 	/// returns true if the handle was created, or suites added or removed from it
 	/// The handle change flag is reset when create_defs is called
   bool handle_changed( unsigned int client_handle );


 	/// Collect all the state changes, so that only small subset is returned to client
  	/// When nodes are added/deleted we use the modify_change_no_, in this case
  	/// the whole defs is returned. Both integers are returned back to the client
  	/// so that, the client then sends the integers back to server, so we can determine
  	/// what's changed.
 	void collateChanges(unsigned int client_handle,DefsDelta&) const;


   // Only return the defs state and suites that the client has registered in the client handle
   // *HOWEVER* if the client has registered all the suites, just return the server defs
   //           *with* the updated change numbers
 	// *OTHERWISE*
	/// This will also compute the **maximum** state and modify change numbers over
	/// the suites managed by the client handle.
	/// and then set it on the newly created defs.
	/// It also takes special precaution *NOT* to change Ecf::state_change_no() and Ecf::modify_change_no()
 	/// This will clear the handle_changed flag
 	defs_ptr create_defs(unsigned int client_handle, defs_ptr server_defs) const;

 	/// Used to determine the change, will throw if handle not found
 	void max_change_no(unsigned int client_handle,unsigned int& max_state_change_no , unsigned int& max_modify_change_no);

 	/// Accessor
 	const std::vector<ecf::ClientSuites>& clientSuites() const { return clientSuites_;}

   /// returns the list of suites associated with a handle, Used by ecFlowview
   void suites(unsigned int client_handle, std::vector<std::string>& names) const;

   /// A suite is being added in the definition.
   /// If the suite was previously registered *UPDATE* its suite_ptr
 	/// Otherwise if any ClientSuites registered for automatic inclusion of new suite, add them in
	void suite_added_in_defs(suite_ptr);

	/// The suite is being deleted in the definition, reset the suite_ptr
	/// Deleted suites STAY registered, until explicitly dropped.
	void suite_deleted_in_defs(suite_ptr);

   /// Update suites to be in same order as Defs.
	void update_suite_order();

	/// remove all client Suites
	void clear() { clientSuites_.clear(); }

	/// Returns a string which has the max state change and modify numbers for each handle
	std::string dump_max_change_no() const;

	/// For debug dumps
	std::string dump() const;

private:
	std::vector<ecf::ClientSuites> clientSuites_;
	Defs* defs_;
};
#endif
