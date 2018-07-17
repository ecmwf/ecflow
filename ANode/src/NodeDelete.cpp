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
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
#include "Node.hpp"
#include "ExprAst.hpp"
#include "Stl.hpp"
#include "Ecf.hpp"

using namespace ecf;
using namespace std;

void Node::delete_time_dep_attrs_if_empty()
{
   if (time_dep_attrs_ && time_dep_attrs_->empty()) {
      time_dep_attrs_.reset(nullptr);
   }
}

void Node::deleteTime(const std::string& name )
{
   if (time_dep_attrs_)  {
      time_dep_attrs_->deleteTime(name);
      delete_time_dep_attrs_if_empty();
      return;
   }
   throw std::runtime_error("Node::delete_time: Can not find time attribute: ");
}

void Node::delete_time( const ecf::TimeAttr& attr )
{
   if (time_dep_attrs_)  {
      time_dep_attrs_->delete_time(attr);
      delete_time_dep_attrs_if_empty();
      return;
   }
   throw std::runtime_error("Node::delete_time: Can not find time attribute: ");
}

void Node::deleteToday(const std::string& name)
{
   if (time_dep_attrs_)  {
      time_dep_attrs_->deleteToday(name);
      delete_time_dep_attrs_if_empty();
      return;
   }
   throw std::runtime_error("Node::delete_today: Can not find today attribute: ");
}

void Node::delete_today(const ecf::TodayAttr& attr)
{
   if (time_dep_attrs_)  {
      time_dep_attrs_->delete_today(attr);
      delete_time_dep_attrs_if_empty();
      return;
   }
   throw std::runtime_error("Node::delete_today: Can not find today attribute: " + attr.toString());
}

void Node::deleteDate(const std::string& name)
{
   if (time_dep_attrs_)  {
      time_dep_attrs_->deleteDate(name);
      delete_time_dep_attrs_if_empty();
      return;
   }
   throw std::runtime_error("Node::delete_date: Can not find date attribute: ");
}

void Node::delete_date(const DateAttr& attr)
{
   if (time_dep_attrs_)  {
      time_dep_attrs_->delete_date(attr);
      delete_time_dep_attrs_if_empty();
      return;
   }
   throw std::runtime_error("Node::delete_date: Can not find date attribute: " + attr.toString());
}

void Node::deleteDay(const std::string& name)
{
   if (time_dep_attrs_)  {
      time_dep_attrs_->deleteDay(name);
      delete_time_dep_attrs_if_empty();
      return;
   }
   throw std::runtime_error("Node::delete_day: Can not find day attribute: ");
}

void Node::delete_day(const DayAttr& attr)
{
   if (time_dep_attrs_)  {
      time_dep_attrs_->delete_day(attr);
      delete_time_dep_attrs_if_empty();
      return;
   }
   throw std::runtime_error("Node::delete_day: Can not find day attribute: " + attr.toString());
}

void Node::deleteCron(const std::string& name)
{
   if (time_dep_attrs_)  {
      time_dep_attrs_->deleteCron(name);
      delete_time_dep_attrs_if_empty();
      return;
   }
   throw std::runtime_error("Node::delete_cron: Can not find cron attribute: ");
}

void Node::delete_cron(const ecf::CronAttr& attr)
{
   if (time_dep_attrs_)  {
      time_dep_attrs_->delete_cron(attr);
      delete_time_dep_attrs_if_empty();
      return;
   }
   throw std::runtime_error("Node::delete_cron: Can not find cron attribute: " + attr.toString());
}

void Node::deleteVariable( const std::string& name)
{
	if (name.empty()) {
		vars_.clear(); // delete all
		state_change_no_ = Ecf::incr_state_change_no();

#ifdef DEBUG_STATE_CHANGE_NO
		std::cout << "Node::deleteVariable\n";
#endif
		return;
	}

	size_t theSize = vars_.size();
	for(size_t i = 0; i < theSize; i++) {
		if (vars_[i].name() == name) {
 			vars_.erase( vars_.begin() + i );
 			state_change_no_ = Ecf::incr_state_change_no();

#ifdef DEBUG_STATE_CHANGE_NO
 			std::cout << "Node::deleteVariable\n";
#endif
			return;
		}
	}
	throw std::runtime_error("Node::deleteVariable: Can not find 'user' variable of name " + name);
}

void Node::deleteEvent(const std::string& name)
{
   if (name.empty()) {
      events_.clear();
      state_change_no_ = Ecf::incr_state_change_no();
#ifdef DEBUG_STATE_CHANGE_NO
      std::cout << "Node::deleteEvent\n";
#endif
      return;
   }

   size_t theSize = events_.size();
   for(size_t i = 0; i < theSize; i++) {
      if (events_[i].name_or_number() == name) {
         events_.erase( events_.begin() + i );
         state_change_no_ = Ecf::incr_state_change_no();
#ifdef DEBUG_STATE_CHANGE_NO
         std::cout << "Node::deleteEvent\n";
#endif
         return;
      }
   }
	throw std::runtime_error("Node::deleteEvent: Can not find event: " + name);
}

void Node::deleteMeter(const std::string& name)
{
   if (name.empty()) {
      meters_.clear();
      state_change_no_ = Ecf::incr_state_change_no();
#ifdef DEBUG_STATE_CHANGE_NO
      std::cout << "Expression::clearFree()\n";
#endif
      return;
   }

   size_t theSize = meters_.size();
   for(size_t i = 0; i < theSize; i++) {
      if (meters_[i].name() == name) {
         meters_.erase( meters_.begin() + i );
         state_change_no_ = Ecf::incr_state_change_no();
#ifdef DEBUG_STATE_CHANGE_NO
         std::cout << "Expression::clearFree()\n";
#endif
         return;
      }
   }
	throw std::runtime_error("Node::deleteMeter: Can not find meter: " + name);
}

void Node::deleteLabel(const std::string& name)
{
   if (name.empty()) {
      labels_.clear();
      state_change_no_ = Ecf::incr_state_change_no();
#ifdef DEBUG_STATE_CHANGE_NO
      std::cout << "Node::deleteLabel\n";
#endif
      return;
   }

   size_t theSize = labels_.size();
   for(size_t i = 0; i < theSize; i++) {
      if (labels_[i].name() == name) {
         labels_.erase( labels_.begin() + i );
         state_change_no_ = Ecf::incr_state_change_no();
#ifdef DEBUG_STATE_CHANGE_NO
         std::cout << "Node::deleteLabel\n";
#endif
         return;
      }
   }
	throw std::runtime_error("Node::deleteLabel: Can not find label: " + name);
}

void Node::delete_queue(const std::string& name)
{
   if (misc_attrs_)  {
      misc_attrs_->delete_queue(name);
      return;
   }
   throw std::runtime_error("Node::delete_queue: Can not find queue: " + name);
}

void Node::delete_generic(const std::string& name)
{
   if (misc_attrs_)  {
      misc_attrs_->delete_generic(name);
      return;
   }
   throw std::runtime_error("Node::delete_generic : Can not find generic: " + name);
}

void Node::deleteTrigger()
{
	if (t_expr_)  {
	   t_expr_.reset(nullptr);
		state_change_no_ = Ecf::incr_state_change_no();
#ifdef DEBUG_STATE_CHANGE_NO
		std::cout << "Node::deleteTrigger()\n";
#endif
	}
}

void Node::deleteComplete()
{
	if (c_expr_) {
		c_expr_.reset(nullptr);
		state_change_no_ = Ecf::incr_state_change_no();
#ifdef DEBUG_STATE_CHANGE_NO
		std::cout << "Node::deleteComplete()\n";
#endif
	}
}

void Node::deleteRepeat()
{
	if (!repeat_.empty()) {
		repeat_.clear(); // will delete the pimple
 		state_change_no_ = Ecf::incr_state_change_no();
#ifdef DEBUG_STATE_CHANGE_NO
		std::cout << "Node::deleteRepeat())\n";
#endif
	}
}

void Node::deleteLimit(const std::string& name)
{
	if (name.empty()) {
		limits_.clear();
		state_change_no_ = Ecf::incr_state_change_no();
#ifdef DEBUG_STATE_CHANGE_NO
		std::cout << "Node::deleteLimit\n";
#endif
  		return;
	}

	size_t theSize = limits_.size();
	for(size_t i = 0; i < theSize; i++) {
		if (limits_[i]->name() == name) {
			limits_.erase( limits_.begin() + i );
			state_change_no_ = Ecf::incr_state_change_no();
#ifdef DEBUG_STATE_CHANGE_NO
			std::cout << "Node::deleteLimit\n";
#endif
			return;
		}
	}
	throw std::runtime_error("Node::deleteLimit: Can not find limit: " + name);
}

void Node::delete_limit_path(const std::string& name,const std::string& path)
{
   if (name.empty()) {
      throw std::runtime_error("Node::delete_limit_path: the limit name must be provided");
   }
   if (path.empty()) {
      throw std::runtime_error("Node::delete_limit_path: the limit path must be provided");
   }

   size_t theSize = limits_.size();
   for(size_t i = 0; i < theSize; i++) {
      if (limits_[i]->name() == name) {
         limits_[i]->delete_path(path); // will update state change no
         return;
      }
   }
   throw std::runtime_error("Node::delete_limit_path: Can not find limit: " + name);
}

void Node::deleteInlimit(const std::string& name)
{
	// if name exists but no corresponding in limit found raises an exception
	if (inLimitMgr_.deleteInlimit(name)) {
		state_change_no_ = Ecf::incr_state_change_no();
#ifdef DEBUG_STATE_CHANGE_NO
		std::cout << "Node::deleteInlimit\n";
#endif
	}
}

void Node::delete_misc_attrs_if_empty()
{
   if (misc_attrs_ && misc_attrs_->empty()) {
      misc_attrs_.reset(nullptr);
   }
}

void Node::deleteZombie(const std::string& zombie_type)
{
   if (misc_attrs_) {
      misc_attrs_->deleteZombie(zombie_type);
      delete_misc_attrs_if_empty();
   }
}

void Node::delete_zombie(Child::ZombieType zt)
{
   if (misc_attrs_) {
      misc_attrs_->delete_zombie(zt);
      delete_misc_attrs_if_empty();
   }
}

void Node::deleteLate()
{
   lateAttr_.reset(nullptr);
   state_change_no_ = Ecf::incr_state_change_no();
}


