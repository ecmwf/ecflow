#ifndef CHANGE_MGR_SINGLETON_HPP_
#define CHANGE_MGR_SINGLETON_HPP_

/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #10 $ 
//
// Copyright 2009-2012 ECMWF. 
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description :
//               Used on the client side, to notify any registered observers of
//               *incremental* changes to the node tree.
// This is a variation of the observer pattern. In that we have avoided added
// the observers directly to the Node. This was done because the observer
// mechanism is only required by the client side, plus it avoids bloating the
// Node with data members that are only used on the client side.
// The disadvantage of this approach is it requires a look up for the Node:
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8

#include <map>
#include <boost/noncopyable.hpp>
#include "NodeFwd.hpp"
#include "Aspect.hpp"

class ChangeMgrSingleton : private boost::noncopyable {
public:
	static ChangeMgrSingleton* instance();

	// Returns the ChangeMgr singleton without creating it
   static ChangeMgrSingleton* exists();

	/// Attach/detatch interest in incremental change
	void attach(Node*,AbstractObserver*);
	void attach(Defs*,AbstractObserver*);
	void detach(Node*,AbstractObserver*);
	void detach(Defs*,AbstractObserver*);

	/// Used in debug:
   size_t no_of_node_observers() const { return node_obs_map_.size(); }
   size_t no_of_def_observers() const { return defs_obs_vec_.size(); }

	/// returns true if we are in a notification. Help client code to avoid infinite cycles
	bool in_notification() const { return in_notification_;}

	/// The cumulated aspect are sent, when we do a the real notification
   void add_aspect(ecf::Aspect::Type aspect) { aspects_vec_.push_back(aspect); }
   void clear_aspects() { aspects_vec_.clear(); }

	/// Notify observer of a incremental change to Node
	/// Will return true for in_notification()
	void notify(node_ptr);

	/// Notify observer of a incremental change to Defs,
	/// Currently state or suspended attribute changes
   /// Will return true for in_notification()
	void notify(defs_ptr);

	/// Inform observers when the subject is about to be deleted
	/// The Observers *MUST* remember to call detach
   void notify_delete(Node*);
   void notify_delete(Defs*);

	static void destroy();

private:

	ChangeMgrSingleton();
	~ChangeMgrSingleton();

   /// Start of notifications:
   ///   Aspect sent upon  notify(node_ptr); notify(defs_ptr);
   void notify_start() { in_notification_ = true; }
	void notify_end() { in_notification_ = false; }

   void update_start() { updating_ = true; }
   void update_end() { updating_ = false; }

private:
	// Allow multiple observers per subject
	std::multimap<Node*,AbstractObserver*> node_obs_map_;

	std::vector< std::pair<Defs*,AbstractObserver*> > defs_obs_vec_;

	std::vector<ecf::Aspect::Type> aspects_vec_;
   static ChangeMgrSingleton* instance_;

   bool in_notification_;
   bool updating_;
   friend class ChangeMgrStartNotification;
};

// Start notification. End notification automatically signalled, Even if exception raised.
// Note: notify_end() is also called in notify(node_ptr), notify(defs_ptr)
class ChangeMgrStartNotification : private boost::noncopyable {
public:
   ChangeMgrStartNotification() { ChangeMgrSingleton::instance()->notify_start();}
   ~ChangeMgrStartNotification() { ChangeMgrSingleton::instance()->notify_end();}
};

// Start notification. End notification automatically signalled, Even if exception raised.
// Note: notify_end() is also called in notify(node_ptr), notify(defs_ptr)
class ChangeMgrStartUpdating : private boost::noncopyable {
public:
   ChangeMgrStartUpdating() { ChangeMgrSingleton::instance()->update_start();}
   ~ChangeMgrStartUpdating() { ChangeMgrSingleton::instance()->update_end();}
};

#endif
