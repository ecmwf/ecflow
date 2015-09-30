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


void ChangeMgrSingleton::attach(Node* n,AbstractObserver* a)
{
   assert( n != NULL);
   assert( a != NULL);
   map_.insert( std::make_pair(n,a) );
   
#ifdef DEBUG_NODE
	cout << "ChangeMgrSingleton::attach(Node*,AbstractObserver*) " << n->absNodePath() << "  obs " << a << " map_.size() " << map_.size() << "\n";
#endif
}

void ChangeMgrSingleton::attach(Defs* n,AbstractObserver* a)
{
   assert( n != NULL);
   assert( a != NULL);
   defs_map_.insert( std::make_pair(n,a) );
   
#ifdef DEBUG_DEFS
   cout << "ChangeMgrSingleton::attach(Defs*,AbstractObserver*) obs " << a << " defs_map_.size() " << defs_map_.size() << "\n";
#endif
}

void ChangeMgrSingleton::detach(Node* n,AbstractObserver* a)
{
 assert( n != NULL);
#ifdef DEBUG_NODE
   cout << "ChangeMgrSingleton::detach(Node*) observer=" << a;
   if (n) cout << " node " << n->absNodePath() << "\n";
   else   cout << " **NULL** node\n";
#endif

  
   NodeObserverMap_t::iterator i = map_.find(n);
   if (i != map_.end()) {
      map_.erase(i);
      return;
   }
   
#ifdef DEBUG_NODE
	if (!map_.empty()) {
		if (n) std::cout << "ChangeMgrSingleton::detach could not detach node " << n->debugNodePath() << "\n";
		else   std::cout << "ChangeMgrSingleton::detach given a NULL Node\n";
	}
#endif
}

void ChangeMgrSingleton::detach(Defs* n,AbstractObserver* a)
{
#ifdef DEBUG_DEFS
   cout << "ChangeMgrSingleton::detach(Defs*,AbstractObserver*)  obs " << a << "\n";
#endif

   assert( n != NULL);
   DefsObserverMap_t::iterator i = defs_map_.find(n);
   if (i != defs_map_.end()) {
      defs_map_.erase(i);

#ifdef DEBUG_DEFS
      cout << "ChangeMgrSingleton::detach(Defs*,AbstractObserver*) defs_map_.size()= " << defs_map_.size() << "\n";
#endif
      return;
   }
}

void ChangeMgrSingleton::detach(AbstractObserver* a)
{
    DefsObserverMap_t::iterator itD=defs_map_.begin();
    while(itD != defs_map_.end())
    {
        if(itD->second == a)
        {
            DefsObserverMap_t::iterator itErase=itD;
            ++itD;
            defs_map_.erase(itErase);
        }
        else
        {
            ++itD;
        }    
    }
    
    NodeObserverMap_t::iterator itN=map_.begin();
    while(itN != map_.end())
    {
        if(itN->second == a)
        {
            NodeObserverMap_t::iterator itErase=itN;
            ++itN;
            map_.erase(itErase);
        }
        else
        {
            ++itN;
        }    
    }
        
    
}
//static void dump_aspect_vec(std::vector<ecf::Aspect::Type>& aspects_vec, const std::string& desc)
//{
//   cout << desc;
//   for(size_t i = 0; i <  aspects_vec.size(); i++) {
//      switch (aspects_vec[i]) {
//         case ecf::Aspect::NOT_DEFINED: cout << "NOT_DEFINED "; break;
//         case ecf::Aspect::ORDER: cout << "ORDER "; break;
//         case ecf::Aspect::ADD_REMOVE_NODE: cout << "ADD_REMOVE_NODE "; break;
//         case ecf::Aspect::ADD_REMOVE_ATTR:cout << "ADD_REMOVE_ATTR "; break;
//         case ecf::Aspect::METER:cout << "METER "; break;
//         case ecf::Aspect::EVENT:cout << "EVENT "; break;
//         case ecf::Aspect::LABEL:cout << "LABEL "; break;
//         case ecf::Aspect::LIMIT:cout << "LIMIT "; break;
//         case ecf::Aspect::STATE:cout << "STATE "; break;
//         case ecf::Aspect::DEFSTATUS:cout << "DEFSTATUS "; break;
//         case ecf::Aspect::SUSPENDED:cout << "SUSPENDED "; break;
//         case ecf::Aspect::SERVER_STATE:cout << "SERVER_STATE "; break;
//         case ecf::Aspect::SERVER_VARIABLE:cout << "SERVER_VARIABLE "; break;
//         case ecf::Aspect::EXPR_TRIGGER:cout << "EXPR_TRIGGER "; break;
//         case ecf::Aspect::EXPR_COMPLETE:cout << "EXPR_COMPLETE "; break;
//         case ecf::Aspect::REPEAT:cout << "REPEAT "; break;
//         case ecf::Aspect::NODE_VARIABLE:cout << "NODE_VARIABLE "; break;
//         case ecf::Aspect::LATE:cout << "LATE "; break;
//         case ecf::Aspect::TODAY:cout << "TODAY "; break;
//         case ecf::Aspect::TIME:cout << "TIME "; break;
//         case ecf::Aspect::DAY:cout << "DAY "; break;
//         case ecf::Aspect::CRON:cout << "CRON "; break;
//         case ecf::Aspect::DATE:cout << "DATE "; break;
//         case ecf::Aspect::FLAG:cout << "FLAG "; break;
//         case ecf::Aspect::SUBMITTABLE:cout << "SUBMITTABLE "; break;
//         case ecf::Aspect::SUITE_CLOCK:cout << "SUITE_CLOCK "; break;
//         case ecf::Aspect::SUITE_BEGIN:cout << "SUITE_BEGIN "; break;
//         case ecf::Aspect::SUITE_CALENDAR:cout << "SUITE_CALENDAR "; break;
//         case ecf::Aspect::ALIAS_NUMBER:cout << "ALIAS_NUMBER "; break;
//         default: cout << "OOPS-unknown!! "; break;
//      }
//   }
//   cout << "\n";
//}

void ChangeMgrSingleton::notify(node_ptr n)
{
   NodeObserverMap_t::iterator i = map_.find(n.get());
   if (i != map_.end()) {
//#ifdef DEBUG
//      dump_aspect_vec(aspects_vec_,"ChangeMgrSingleton::notify(node_ptr n)******************************\n");
//#endif

      (*i).second->update(n.get(),aspects_vec_);
      return;
   }
   // ************************************************************************
   // The GUI only attach's to nodes that are displayed. WE may well get
   // notification to nodes that are not attached. Hence this is not an error
   // ************************************************************************

#ifdef DEBUG_NODE
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
//#ifdef DEBUG
//      dump_aspect_vec(aspects_vec_,"ChangeMgrSingleton::notify(defs_ptr defs)******************************\n");
//#endif

      (*i).second->update(defs.get(),aspects_vec_);
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
#ifdef DEBUG_NODE
   cout << "ChangeMgrSingleton::notify_delete(Node*)  node " << n->absNodePath() << "\n";
#endif

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

#ifdef DEBUG_NODE
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
#ifdef DEBUG_DEFS
   cout << "ChangeMgrSingleton::notify_delete(Defs*)\n";
#endif
   assert(defs);
   
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

#ifdef DEBUG_DEFS
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
