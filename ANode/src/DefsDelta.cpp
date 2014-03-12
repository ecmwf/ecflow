/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #23 $ 
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
#include <boost/bind.hpp>
#include "DefsDelta.hpp"
#include "ChangeMgrSingleton.hpp"
using namespace std;

//===============================================================
// DefsDelta

/// Defs delta can be re-used. reset all data members
void DefsDelta::init(unsigned int client_state_change_no)
{
	client_state_change_no_ = client_state_change_no;

	server_state_change_no_ = 0;
	server_modify_change_no_ = 0;
	compound_mementos_.clear();
}


bool DefsDelta::incremental_sync(defs_ptr client_def) const
{
	if (!client_def.get()) return false;

   /// - Sets notification flag, so that observers can also query if they are in
   ///   the middle of notification.
   ChangeMgrStartNotification start_notification;

	// Update the client defs with latest server *handle* based state change/modify number
	// to keep pace with the state changes. Passed back later on, to get further changes
	client_def->set_state_change_no( server_state_change_no_);
	client_def->set_modify_change_no( server_modify_change_no_ );

	try {
#ifdef DEBUG_MEMENTO
		std::cout << "DefsDelta::incremental_sync compound_mementos_.size() = " << compound_mementos_.size() << "\n";
#endif
		std::for_each(compound_mementos_.begin(),compound_mementos_.end(), boost::bind(&CompoundMemento::incremental_sync,_1,client_def));
	}
	catch ( std::exception& e) {
		throw std::runtime_error("Could not apply incremental server changes to client defs, because: " + string(e.what()));
	}

	// return true if there were any changes made
	return !compound_mementos_.empty();
}

void DefsDelta::add(compound_memento_ptr memento)
{
	compound_mementos_.push_back(memento);
}

