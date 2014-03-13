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

using namespace std;

//#define DEBUG_DEFS 1
//#define DEBUG_NODE 1

ChangeMgrSingleton* ChangeMgrSingleton::instance_ =  NULL;

ChangeMgrSingleton* ChangeMgrSingleton::instance()
{
	if (instance_ == NULL) instance_ =  new ChangeMgrSingleton();
	return instance_;
}

ChangeMgrSingleton* ChangeMgrSingleton::exists() { return instance_; }


void ChangeMgrSingleton::attach(Node* node,AbstractObserver* a)
{
	assert( node );
	assert( a );

	node_obs_map_.insert(std::make_pair(node,a));

#ifdef DEBUG_NODE
	cout << "ChangeMgrSingleton::attach(Node*,AbstractObserver*) " << node->absNodePath() << "  obs " << a << " node_obs_map_.size() " << node_obs_map_.size() << "\n";
#endif
}

void ChangeMgrSingleton::attach(Defs* defs,AbstractObserver* a)
{
	assert( defs );
	assert( a );
	defs_obs_vec_.push_back(std::make_pair(defs,a));

#ifdef DEBUG_DEFS
   cout << "ChangeMgrSingleton::attach(Defs*,AbstractObserver*) obs " << a << " defs_obs_vec_.size() " << defs_obs_vec_.size() << "\n";
#endif
}

void ChangeMgrSingleton::detach(Node* node,AbstractObserver* a)
{
   assert( node);
   assert( a );
#ifdef DEBUG_NODE
   cout << "ChangeMgrSingleton::detach(Node*) observer=" << a;
   if (node) cout << " node " << node->absNodePath() << "\n";
   else   cout << " **NULL** node\n";
#endif


   // equal_range(b) returns pair<iterator,iterator> representing the range of elements with key(Node*)
   typedef multimap<Node*, AbstractObserver*>::iterator iterator;
   std::pair<iterator, iterator> iter_pair = node_obs_map_.equal_range(node);

   // Loop through range of maps of key Node
   for (iterator it = iter_pair.first; it != iter_pair.second; ++it) {
       if (it->second == a) {
          // We do not delete, to avoid iterator invalidation, during notify_delete()
          // node_obs_map_.erase(it);
          it->second = NULL;
          return;
       }
   }

#ifdef DEBUG_NODE
	if (!node_obs_map_.empty()) {
		if (node) std::cout << "ChangeMgrSingleton::detach could not detach node " << node->debugNodePath() << "\n";
		else     std::cout << "ChangeMgrSingleton::detach given a NULL Node\n";
	}
#endif
}

void ChangeMgrSingleton::detach(Defs* defs,AbstractObserver* a)
{
   assert( defs );
   assert( a );

   std::vector< std::pair<Defs*,AbstractObserver*> >::iterator i;
   for(i = defs_obs_vec_.begin(); i != defs_obs_vec_.end(); ++i) {
      if ((*i).first == defs && (*i).second == a) {
         defs_obs_vec_.erase(i);

#ifdef DEBUG_DEFS
         cout << "ChangeMgrSingleton::detach(Defs*,AbstractObserver*)  obs " << a << " defs_obs_vec_.size()= " << defs_obs_vec_.size() << "\n";
#endif
         return;
      }
   }

   if (!defs_obs_vec_.empty()) {
      if (defs) std::cout << "ChangeMgrSingleton::detach(Defs*) Error: could not detach Defs, was it observred ?\n";
      else      std::cout << "ChangeMgrSingleton::detach(Defs*) Error: given a NULL Defs ???\n";
   }
}

void ChangeMgrSingleton::notify(node_ptr node)
{
   assert(node.get());

#ifdef DEBUG_NODE
   cout << "ChangeMgrSingleton::notify(Node*)";
   if (node) cout << " node " << node->absNodePath() << "\n";
   else   cout << " **NULL** node\n";
#endif

   // equal_range(b) returns pair<iterator,iterator> representing the range of element with key node
   typedef multimap<Node*, AbstractObserver*>::iterator iterator;
   std::pair<iterator, iterator> iter_pair = node_obs_map_.equal_range(node.get());

   // Loop through range of maps of key Node
   for (iterator it = iter_pair.first; it != iter_pair.second; ++it) {
      it->second->update(node.get(),aspects_vec_);
   }

	// ************************************************************************
	// The GUI only attach's to nodes that are displayed. WE may well get
	// notification to nodes that are not attached. Hence this is not an error
	// ************************************************************************
}

void ChangeMgrSingleton::notify(defs_ptr defs)
{
#ifdef DEBUG_DEFS
   cout << "ChangeMgrSingleton::notify(Defs*)\n";
#endif
   assert(defs.get());
   std::vector< std::pair<Defs*,AbstractObserver*> >::iterator i;
   for(i = defs_obs_vec_.begin(); i != defs_obs_vec_.end(); ++i) {
      if ((*i).first == defs.get()) {
         (*i).second->update(defs.get(),aspects_vec_);
      }
   }
}

void ChangeMgrSingleton::notify_delete(Node* node)
{
#ifdef DEBUG_NODE
   cout << "ChangeMgrSingleton::notify_delete(Node*)  node " << node->absNodePath() << "\n";
#endif

   // *************************************************************************
   // When we call update_delete() it will call detach(..) and hence modify
   // the container we are iterator over.
   // How do we avoid avoid iterator invaldation ?
   //   a/ Make a copy of the container in this function.
   //      This works, however for large design, it is a performance killer
   //   b/ Allow detach(..) to NULL out the observer, hence we don't invalidate
   //      the iterators. The key will then we deleted in this function.
   // *************************************************************************

   // equal_range(b) returns pair<iterator,iterator> representing the range of element with key node
   typedef multimap<Node*, AbstractObserver*>::iterator iterator;
   std::pair<iterator, iterator> iter_pair = node_obs_map_.equal_range(node);

   // Loop through range of maps of key Node: Client code should detach.
   for (iterator it = iter_pair.first; it != iter_pair.second; ++it) {
      if (it->second) it->second->update_delete(node);
   }

   /// Check to make sure that derived Observer called detach
   /// We can not call detach ourselves, since the the client needs to
   /// call detach in the case where the graphical tree is destroyed by user
   /// In this case the Subject/Node is being deleted.
#ifdef DEBUG_NODE
   iter_pair = node_obs_map_.equal_range(node);
   for (iterator it = iter_pair.first; it != iter_pair.second; ++it) {
      assert( it->second == NULL);
   }
#endif

   // Delete the key
   node_obs_map_.erase(node);

#ifdef DEBUG_NODE
   cout << "   ChangeMgrSingleton::notify_delete(Node*) node_obs_map_.size()= " << node_obs_map_.size() << "\n";
#endif
}

void ChangeMgrSingleton::notify_delete(Defs* defs)
{
#ifdef DEBUG_DEFS
   cout << "ChangeMgrSingleton::notify_delete(Defs*)\n";
#endif
   assert(defs);

   // make a copy, so that observers avoid modifying the vector we are iterating over
   std::vector< std::pair<Defs*,AbstractObserver*> > defs_obs_vec_copy = defs_obs_vec_;

   std::vector< std::pair<Defs*,AbstractObserver*> >::iterator i;
   for(i = defs_obs_vec_copy.begin(); i != defs_obs_vec_copy.end(); ++i) {
      if ((*i).first == defs) {

         /// This must call ChangeMgrSingleton::detach(Defs* defs,AbstractObserver* a)
         (*i).second->update_delete(defs);
      }
   }

   /// Check to make sure that the Observer called detach
   /// We can not call detach ourselves, since the the client needs to
   /// call detach in the case where the graphical tree is destroyed by user
   /// In this case the Subject/Node is being deleted.
   for(i = defs_obs_vec_.begin(); i != defs_obs_vec_.end(); ++i) {
      if ((*i).first == defs) {
         assert(false); // client code forgot to detach
      }
   }
#ifdef DEBUG_DEFS
   cout << "   ChangeMgrSingleton::notify_delete(Defs*) defs_obs_vec_.size()= " << defs_obs_vec_.size() << "\n";
#endif
}

void ChangeMgrSingleton::destroy()
{
	delete instance_;
	instance_ = NULL;
}

ChangeMgrSingleton::ChangeMgrSingleton() : in_notification_(false) {}
ChangeMgrSingleton::~ChangeMgrSingleton() {}
