#ifndef FLAG_HPP_
#define FLAG_HPP_
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #15 $ 
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

#include <vector>
#include <string>
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/level.hpp>
#include <boost/serialization/tracking.hpp>

namespace  ecf {

/// Flag are used store what has happened to a node. These are shown as icon in ecFlowview
/// Uses compiler generated copy constructor, assignment operator and destructor

/// During interactive use. A Node can be *forced to complete*, or forced to *run*
/// Typically the user may want to force a node to complete, if they are trying
/// to update the repeat variable.
///
/// In either case  we need to miss a time slot, this is done by setting the
/// NO_REQUE_IF_SINGLE_TIME_DEP, then at REQUE time we query the flag, if it was set
/// we avoid resetting the time slots. effectively missing the next time slot.
///
/// This functionality is only required during interactive force or run
/// However if the job aborted, we need to clear NO_REQUE_IF_SINGLE_TIME_DEP, i.e
///     time 10:00
///     time 11:00
/// If at 9.00am use the run command, we want to miss the 10:00 time slot.
/// However if the run at 9.00 fails, and we run again, we also miss 11:00 time slot
/// to avoid this if the job aborts, we clear NO_REQUE_IF_SINGLE_TIME_DEP flag.

class Flag {
public:
   Flag() : flag_(0),state_change_no_(0) {}

   /// The BYRULE is used to distinguish between tasks that have RUN and completed
   /// and those that have completed by complete expression.
   enum Type {
      FORCE_ABORT   =  0,  // Node* do not run when try_no > ECF_TRIES, and task aborted by user
      USER_EDIT     =  1,  // task
      TASK_ABORTED  =  2,  // task*
      EDIT_FAILED   =  3,  // task*
      JOBCMD_FAILED =  4,  // task*
      NO_SCRIPT     =  5,  // task*
      KILLED        =  6,  // task* do not run when try_no > ECF_TRIES, and task killed by user
      MIGRATED      =  7,  // Node                                   ( NOT USED currently )
      LATE          =  8,  // Node attribute, Task is late, or Defs checkpt takes to long
      MESSAGE       =  9,  // Node
      BYRULE        = 10,  // Node*, set if node is set to complete by complete trigger expression
      QUEUELIMIT    = 11,  // Node                                   ( NOT USED currently)
      WAIT          = 12,  // task*  Set/cleared but never queried ? ( NOT USED currently )
      LOCKED        = 13,  // Server                                 ( NOT USED currently)
      ZOMBIE        = 14,  // task*  Set/cleared but never queried ? ( NOT USED currently )
      NO_REQUE_IF_SINGLE_TIME_DEP = 15,  //
      NOT_SET       = 16
   };

   bool operator==(const Flag& rhs) const { return flag_ == rhs.flag_; }
   bool operator!=(const Flag& rhs) const { return !operator==(rhs); }


   // Flag functions:
   void set(Type flag);
   void clear(Type flag );
   bool is_set(Type flag) const { return ( flag_ & (1 << flag)); }

   void reset();
   int flag() const { return flag_;}
   void set_flag(int f) { flag_ = f; }
   void set_flag(const std::string& flags); // these are comma separated

   /// returns a comma separated list of all flags set
   std::string to_string() const;

   /// returns the string equivalent
   static std::string enum_to_string(Flag::Type flag);

   /// Used to determine change in state relative to client
   unsigned int state_change_no() const { return state_change_no_; }

   /// returns the list of all flag types
   static std::vector<Flag::Type> list();

   /// Converts from string to flag types.
   static Flag::Type string_to_flag_type(const std::string& s);

   /// valid flag types, than can be used in AlterCmd
   static void valid_flag_type(std::vector<std::string>& vec);

private:
   int          flag_;
   unsigned int state_change_no_;  // *not* persisted, only used on server side

   friend class boost::serialization::access;
   template<class Archive>
   void serialize(Archive & ar, const unsigned int /*version*/) {
      ar & flag_;
   }
};
}

// This should ONLY be added to objects that are *NOT* serialised through a pointer
BOOST_CLASS_IMPLEMENTATION(ecf::Flag, boost::serialization::object_serializable)
BOOST_CLASS_TRACKING(ecf::Flag,boost::serialization::track_never);

#endif
