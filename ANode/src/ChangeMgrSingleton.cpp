/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #14 $ 
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
#include <iostream>
#include "ChangeMgrSingleton.hpp"
#include "AbstractObserver.hpp"
#include "Node.hpp"
#include "Defs.hpp"

//#define DEBUG_ME 1

ChangeMgrSingleton* ChangeMgrSingleton::instance_ =  NULL;

ChangeMgrSingleton* ChangeMgrSingleton::instance()
{
	if (instance_ == NULL) instance_ =  new ChangeMgrSingleton();
	return instance_;
}

ChangeMgrSingleton* ChangeMgrSingleton::exists() { return instance_; }


void ChangeMgrSingleton::attach(Node* n,AbstractObserver* a)
{
	assert( n != NULL);
	assert( a != NULL);
	map_.insert( std::make_pair(n,a) );
}

void ChangeMgrSingleton::attach(Defs* n,AbstractObserver* a)
{
	assert( n != NULL);
	assert( a != NULL);
	defs_map_.insert( std::make_pair(n,a) );
}

void ChangeMgrSingleton::detach(Node* n)
{
	assert( n != NULL);
	NodeObserverMap_t::iterator i = map_.find(n);
	if (i != map_.end()) {
		map_.erase(i);
		return;
	}
#ifdef DEBUG_ME
	if (!map_.empty()) {
		if (n) std::cout << "ChangeMgrSingleton::detach could not detach node " << n->debugNodePath() << "\n";
		else   std::cout << "ChangeMgrSingleton::detach given a NULL Node\n";
	}
#endif
}

void ChangeMgrSingleton::detach(Defs* n)
{
	assert( n != NULL);
	DefsObserverMap_t::iterator i = defs_map_.find(n);
	if (i != defs_map_.end()) {
		defs_map_.erase(i);
		return;
	}
#ifdef DEBUG_ME
	if (!defs_map_.empty()) {
		if (n) std::cout << "ChangeMgrSingleton::detach(Defs*) Error: could not detach Defs ??? \n";
		else   std::cout << "ChangeMgrSingleton::detach(Defs*) Error: given a NULL Defs ???\n";
	}
#endif
}

void ChangeMgrSingleton::notify(node_ptr n)
{
	NodeObserverMap_t::iterator i = map_.find(n.get());
	if (i != map_.end()) {
		(*i).second->update(n.get(),aspects_vec_);
		notify_end();
		return;
	}
	// ************************************************************************
	// The GUI only attach's to nodes that are displayed. WE may well get
	// notification to nodes that are not attached. Hence this is not an error
	// ************************************************************************

#ifdef DEBUG_ME
	if (!map_.empty()) {
	   /// Note: we don't deference the pointer, it may be corrupted ???
		if (n.get()) std::cout << "ChangeMgrSingleton::notify : received notification for node " << n->debugNodePath() << " thats not being observed\n";
		else         std::cout << "ChangeMgrSingleton::notify : Error: given a NULL Node\n";
	}
#endif
}

void ChangeMgrSingleton::notify(defs_ptr defs)
{
	DefsObserverMap_t::iterator i = defs_map_.find(defs.get());
	if (i != defs_map_.end()) {
		(*i).second->update(defs.get(),aspects_vec_);
      notify_end();
		return;
	}

#ifdef DEBUG_ME
	if (!defs_map_.empty()) {
		if (defs.get()) std::cout << "ChangeMgrSingleton::notify could not notify Defs that is not being observed\n";
		else            std::cout << "ChangeMgrSingleton::notify given a NULL defs\n";
	}
#endif
}

void ChangeMgrSingleton::notify_delete(Node* n)
{
   NodeObserverMap_t::iterator i = map_.find(n);
   if (i != map_.end()) {

      /// This must call detach(Node*)
      (*i).second->update_delete(n);

      /// Check to make sure that the Observer called detach
      /// We can not call detach ourselves, since the the client needs to
      /// call detach in the case where the graphical tree is destroyed by user
      /// In this case the Subject/Node is being deleted.
      assert(map_.find(n) == map_.end());

      return;
   }

#ifdef DEBUG_ME
   if (!map_.empty()) {
      /// Its not safe to call debugNodePath()/absNodePath() since that will traverse the parent
      /// This may not be safe during a delete.
      if (n) std::cout << "ChangeMgrSingleton::notify_delete : Node is not observed : " << n->name() << "\n";
      else   std::cout << "ChangeMgrSingleton::notify_delete : Error: given a NULL Node\n";
   }
#endif
}

void ChangeMgrSingleton::notify_delete(Defs* defs)
{
   DefsObserverMap_t::iterator i = defs_map_.find(defs);
   if (i != defs_map_.end()) {

      /// This must call detach(Defs*)
      (*i).second->update_delete(defs);

      /// Check to make sure that the Observer called detach
      /// We can not call detach ourselves, since the the client needs to
      /// call detach in the case where the graphical tree is destroyed by user
      /// In this case the Subject/Node is being deleted.
      assert(defs_map_.find(defs) == defs_map_.end());

      return;
   }

#ifdef DEBUG_ME
   if (!defs_map_.empty()) {
      if (defs) std::cout << "ChangeMgrSingleton::notify_delete: Defs is not being observed:\n";
      else      std::cout << "ChangeMgrSingleton::notify_delete given a NULL defs\n";
   }
#endif
}

void ChangeMgrSingleton::destroy()
{
	delete instance_;
	instance_ = NULL;
}

ChangeMgrSingleton::ChangeMgrSingleton() : in_notification_(false) {}
ChangeMgrSingleton::~ChangeMgrSingleton() {}
