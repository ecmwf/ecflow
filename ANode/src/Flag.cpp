/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #12 $ 
//
// Copyright 2009-2016 ECMWF. 
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description :
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8

#include <stdexcept>
#include <iostream>
#include "Flag.hpp"
#include "Ecf.hpp"
#include "Str.hpp"

namespace  ecf {

void Flag::set(Flag::Type flag)
{
   if (!is_set(flag)) {
      // minimize changes to state_change_no_
      flag_ |= (1<<flag);
      state_change_no_ = Ecf::incr_state_change_no();
   }
}

void Flag::clear(Flag::Type flag )
{
   if (is_set(flag)) {
      // minimize changes to state_change_no_
      flag_ &= ~(1<<flag);
      state_change_no_ = Ecf::incr_state_change_no();
   }
}

void Flag::reset() {
   flag_ = 0;
   state_change_no_ = Ecf::incr_state_change_no();
}

std::vector<Flag::Type> Flag::list()
{
   std::vector<Flag::Type> ret; ret.reserve(15);
   ret.push_back(Flag::FORCE_ABORT);
   ret.push_back(Flag::USER_EDIT);
   ret.push_back(Flag::TASK_ABORTED);
   ret.push_back(Flag::EDIT_FAILED);
   ret.push_back(Flag::JOBCMD_FAILED);
   ret.push_back(Flag::NO_SCRIPT);
   ret.push_back(Flag::KILLED);
   ret.push_back(Flag::MIGRATED);
   ret.push_back(Flag::LATE);
   ret.push_back(Flag::MESSAGE);
   ret.push_back(Flag::BYRULE);
   ret.push_back(Flag::QUEUELIMIT);
   ret.push_back(Flag::WAIT);
   ret.push_back(Flag::LOCKED);
   ret.push_back(Flag::ZOMBIE);
   ret.push_back(Flag::NO_REQUE_IF_SINGLE_TIME_DEP);
   return ret;
}

std::string Flag::enum_to_string(Flag::Type flag) {

   switch ( flag ) {
      case Flag::FORCE_ABORT:  return "force_aborted"; break;
      case Flag::USER_EDIT :   return "user_edit"; break;
      case Flag::TASK_ABORTED: return "task_aborted"; break;
      case Flag::EDIT_FAILED:  return "edit_failed"; break;
      case Flag::JOBCMD_FAILED:return "ecfcmd_failed"; break;
      case Flag::NO_SCRIPT:    return "no_script"; break;
      case Flag::KILLED:       return "killed"; break;
      case Flag::MIGRATED:     return "migrated"; break;
      case Flag::LATE:         return "late"; break;
      case Flag::MESSAGE:      return "message"; break;
      case Flag::BYRULE:       return "by_rule"; break;
      case Flag::QUEUELIMIT:   return "queue_limit"; break;
      case Flag::WAIT:         return "task_waiting"; break;
      case Flag::LOCKED:       return "locked"; break;
      case Flag::ZOMBIE:       return "zombie"; break;
      case Flag::NO_REQUE_IF_SINGLE_TIME_DEP: return "no_reque"; break;
      case Flag::NOT_SET:      return "not_set"; break;
      default: break;
   };
   return std::string();
}


Flag::Type Flag::string_to_flag_type(const std::string& s)
{
   if (s == "force_aborted") return Flag::FORCE_ABORT;
   if (s == "user_edit") return Flag::USER_EDIT;
   if (s == "task_aborted") return Flag::TASK_ABORTED;
   if (s == "edit_failed") return Flag::EDIT_FAILED;
   if (s == "ecfcmd_failed") return Flag::JOBCMD_FAILED;
   if (s == "no_script") return Flag::NO_SCRIPT;
   if (s == "killed") return Flag::KILLED;
   if (s == "migrated") return Flag::MIGRATED;
   if (s == "late") return Flag::LATE;
   if (s == "message") return Flag::MESSAGE;
   if (s == "by_rule") return Flag::BYRULE;
   if (s == "queue_limit") return Flag::QUEUELIMIT;
   if (s == "task_waiting") return Flag::WAIT;
   if (s == "locked") return Flag::LOCKED;
   if (s == "zombie") return Flag::ZOMBIE;
   if (s == "no_reque") return Flag::NO_REQUE_IF_SINGLE_TIME_DEP;
   return Flag::NOT_SET;
}

void Flag::valid_flag_type(std::vector<std::string>& vec)
{
   vec.reserve(15);
   vec.push_back("force_aborted");
   vec.push_back("user_edit");
   vec.push_back("task_aborted");
   vec.push_back("edit_failed");
   vec.push_back("ecfcmd_failed");
   vec.push_back("no_script");
   vec.push_back("killed");
   vec.push_back("migrated");
   vec.push_back("late");
   vec.push_back("message");
   vec.push_back("by_rule");
   vec.push_back("queue_limit");
   vec.push_back("task_waiting");
   vec.push_back("locked");
   vec.push_back("zombie");
   vec.push_back("no_reque");
}

std::string Flag::to_string() const
{
   std::string ret;
   std::vector<Flag::Type> flag_list =  Flag::list();
   for (size_t i = 0; i < flag_list.size(); ++i) {
      if ( is_set( flag_list[i] ) ) {
         if (!ret.empty()) ret += ',';
         ret += enum_to_string( flag_list[i]);
      }
   }
   return ret;
}

void Flag::set_flag(const std::string& flags)
{
   std::vector< std::string > the_flags_vec;
   Str::split(flags,the_flags_vec,",");

   for(size_t i =0; i < the_flags_vec.size(); i++) {
      Flag::Type ft = string_to_flag_type(the_flags_vec[i]);
      if (ft == Flag::NOT_SET) {
         throw std::runtime_error("Flag::set_flag: Unknown flag types found: " + the_flags_vec[i]);
      }
      set(ft);
   }
}


}
