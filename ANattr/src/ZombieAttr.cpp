//============================================================================
// Name        : NodeTree.cpp
// Author      : Avi
// Revision    : $Revision: #13 $ 
//
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description :
//============================================================================
#include <sstream>
#include <iostream>
#include <assert.h>
#include <boost/tokenizer.hpp>
#include <boost/token_functions.hpp>
#include <boost/lexical_cast.hpp>

#include "ZombieAttr.hpp"
#include "Indentor.hpp"
#include "Str.hpp"

using namespace ecf;
using namespace boost;
using namespace std;

const ZombieAttr& ZombieAttr::EMPTY() { static const ZombieAttr ZOMBIEATTR = ZombieAttr(); return ZOMBIEATTR; }

// Constructor ==============================================================================
ZombieAttr::ZombieAttr(ecf::Child::ZombieType t, const std::vector<ecf::Child::CmdType>& c, ecf::User::Action a, int zombie_lifetime)
	: zombie_type_(t), action_(a), zombie_lifetime_(zombie_lifetime), child_cmds_(c)
{
   /// Server typically checks every 60 seconds, hence this is lowest valid value for
   if (zombie_lifetime_ <= 0) {
      // default constructor. Set defaults
      switch (zombie_type_) {
         case Child::USER: zombie_lifetime_ = default_user_zombie_life_time() ; break;
         case Child::PATH: zombie_lifetime_ = default_path_zombie_life_time() ; break;
         case Child::ECF:  zombie_lifetime_ = default_ecf_zombie_life_time() ; break;
         case Child::ECF_PID:       zombie_lifetime_ = default_ecf_zombie_life_time() ; break;
         case Child::ECF_PID_PASSWD:zombie_lifetime_ = default_ecf_zombie_life_time() ; break;
         case Child::ECF_PASSWD:    zombie_lifetime_ = default_ecf_zombie_life_time() ; break;
         case Child::NOT_SET: assert(false); break;
      }
   }
   else if (zombie_lifetime_ < minimum_zombie_life_time() ) {
      zombie_lifetime_ = minimum_zombie_life_time()  ;
   }
}

bool ZombieAttr::operator==(const ZombieAttr& rhs) const
{
   if (zombie_type_ != rhs.zombie_type_) return false;
   if (action_ != rhs.action_) return false;
   if (zombie_lifetime_ != rhs.zombie_lifetime_) return false;
   if (child_cmds_ != rhs.child_cmds_) return false;
   return true;
}

std::ostream& ZombieAttr::print(std::ostream& os) const
{
   Indentor in;
   Indentor::indent(os) << toString();
   os << "\n";
   return os;
}

std::string ZombieAttr::toString() const
{
	/// format is  zombie_type : child_cmds(optional) : action : zombie_lifetime_(optional)
	std::string ret = "zombie ";
	ret += Child::to_string(zombie_type_);
	ret += Str::COLON();
	ret += User::to_string(action_);
	ret += Str::COLON();
	ret += Child::to_string(child_cmds_);
	ret += Str::COLON();
	ret += boost::lexical_cast<std::string>(zombie_lifetime_);
	return ret;
}

bool ZombieAttr::fob( ecf::Child::CmdType child_cmd) const
{
	if ( action_ != User::FOB) return false;
	if ( child_cmds_.empty()) return true;

	// If we have child commands specified, then the action is only applicable for that child cmd
	// for all other child cmds we block
 	for(size_t i = 0; i < child_cmds_.size(); i++) {
		if (child_cmds_[i] == child_cmd ) {
			return true;
		}
	}
	return false;
}

bool ZombieAttr::fail( ecf::Child::CmdType child_cmd) const
{
	if ( action_ != User::FAIL) return false;
	if ( child_cmds_.empty()) return true;

	// If we have child commands specified, then the action is only applicable for that child cmd
	// for all other child cmds we block
 	for(size_t i = 0; i < child_cmds_.size(); i++) {
		if (child_cmds_[i] == child_cmd ) {
			return true;
		}
	}
	return false;
}

bool ZombieAttr::adopt( ecf::Child::CmdType child_cmd) const
{
	if ( action_ != User::ADOPT) return false;
	if ( child_cmds_.empty()) return true;

	// If we have child commands specified, then the action is only applicable for that child cmd
	// for all other child cmds we block
 	for(size_t i = 0; i < child_cmds_.size(); i++) {
		if (child_cmds_[i] == child_cmd ) {
			return true;
		}
	}
	return false;
}

bool ZombieAttr::remove( ecf::Child::CmdType child_cmd) const
{
	if ( action_ != User::REMOVE) return false;
	if ( child_cmds_.empty()) return true;

	// If we have child commands specified, then the action is only applicable for that child cmd
	// for all other child cmds we block
 	for(size_t i = 0; i < child_cmds_.size(); i++) {
		if (child_cmds_[i] == child_cmd ) {
			return true;
		}
	}
	return false;
}

bool ZombieAttr::block( ecf::Child::CmdType child_cmd) const
{
	if ( action_ != User::BLOCK) return false;
	if ( child_cmds_.empty()) return true;

	// If we have child commands specified, then the action is only applicable for that child cmd
	// for all other child cmds we block
 	for(size_t i = 0; i < child_cmds_.size(); i++) {
		if (child_cmds_[i] == child_cmd ) {
			return true;
		}
	}
	return false;
}

bool ZombieAttr::kill( ecf::Child::CmdType child_cmd) const
{
   if ( action_ != User::KILL) return false;
   if ( child_cmds_.empty()) return true;

   // If we have child commands specified, then the action is only applicable for that child cmd
   // for all other child cmds we block
   for(size_t i = 0; i < child_cmds_.size(); i++) {
      if (child_cmds_[i] == child_cmd ) {
         return true;
      }
   }
   return false;
}

ZombieAttr ZombieAttr::create(const std::string& string_to_parse)
{
	/// Use boost tokenizer instead of Str::split, as it allows preservation of empty tokens
	char_separator<char> sep(":","",boost::keep_empty_tokens);
	typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
	tokenizer tokenise(string_to_parse, sep);
	std::vector<std::string> tokens;
	std::copy(tokenise.begin(), tokenise.end(), back_inserter(tokens));
	if (tokens.size() < 2) throw std::runtime_error("ZombieAttr::create failed: Invalid zombie type " + string_to_parse);

	/// expects <zombie_type>:<user_action>:child_cmds:zombie_lifetime
	string str_zombie_type;
	string action_str;
	string child_cmds;
	string lifetime;
	size_t tokens_size = tokens.size() ;
	for(size_t i =0; i < tokens_size ; i++) {
		// cout << "   token " << i << ": '" << tokens[i] << "'\n";
		if (i == 0) { str_zombie_type = tokens[i]; continue;}
		if (i == 1) { action_str = tokens[i]; continue;}
		if (i == 2) { child_cmds = tokens[i]; continue;}
		if (i == 3) { lifetime = tokens[i]; continue;}
		throw std::runtime_error("ZombieAttr::create failed: Invalid zombie tokens " + string_to_parse);
	}
	//std::cout << "   zombie_type = " << str_zombie_type << "   user_action = " << action_str <<  "   child_cmds = " << child_cmds<< "   zombie_lifetime = " << lifetime << "\n";

	if (!Child::valid_zombie_type(str_zombie_type))
		throw std::runtime_error("ZombieAttr::create failed: Invalid zombie type, expected one of [ user | ecf | ecf_pid | ecf_pid_passed | ecf_passwd | path ] but found " + str_zombie_type + string(":") + string_to_parse);

	if ( !action_str.empty() && !User::valid_user_action(action_str)) {
		throw std::runtime_error("ZombieAttr::create failed: Invalid user action, expected one of [ fob | fail | remove | block | adopt | kill ] but found " + action_str + string(":") + string_to_parse);
	}

	if ( !child_cmds.empty() && !Child::valid_child_cmds(child_cmds)) {
		throw std::runtime_error("ZombieAttr::create failed: Invalid child type, expected one or more of [ init,event,meter,label,wait,queue,abort,complete] but found " + tokens[2] + string(":") + string_to_parse);
	}

	int zombie_lifetime = -1;
	if ( !lifetime.empty() ) {
		try { zombie_lifetime = boost::lexical_cast<int>(lifetime);}
		catch ( boost::bad_lexical_cast& ) {
			throw std::runtime_error("ZombieAttr::create failed: Zombie life time must be convertible to an integer " + lifetime + string(":") + string_to_parse);
		}
	}

	if (action_str.empty() && zombie_lifetime == -1)
		throw std::runtime_error("ZombieAttr::create failed: User Action(fob,fail,remove,adopt,block) or lifetime must be specified: " + string_to_parse);

	Child::ZombieType zombie_type = Child::zombie_type(str_zombie_type);
	User::Action action =  User::user_action(action_str);
	vector<Child::CmdType> childVec = Child::child_cmds(child_cmds);

	/// If zombie_lifetime is still -1 constructor will reset to standard defaults
  	return ZombieAttr(zombie_type,childVec,action,zombie_lifetime) ;
}

ZombieAttr ZombieAttr::get_default_attr(ecf::Child::ZombieType zt)
{
	switch (zt) {
		case Child::USER:          return ZombieAttr(zt, std::vector<ecf::Child::CmdType>(), User::BLOCK, default_user_zombie_life_time() ); break;
		case Child::PATH:          return ZombieAttr(zt, std::vector<ecf::Child::CmdType>(), User::BLOCK, default_path_zombie_life_time() ); break;
        case Child::ECF:           return ZombieAttr(zt, std::vector<ecf::Child::CmdType>(), User::BLOCK, default_ecf_zombie_life_time() ); break;
        case Child::ECF_PID:       return ZombieAttr(zt, std::vector<ecf::Child::CmdType>(), User::BLOCK, default_ecf_zombie_life_time() ); break;
        case Child::ECF_PID_PASSWD:return ZombieAttr(zt, std::vector<ecf::Child::CmdType>(), User::BLOCK, default_ecf_zombie_life_time() ); break;
        case Child::ECF_PASSWD:    return ZombieAttr(zt, std::vector<ecf::Child::CmdType>(), User::BLOCK, default_ecf_zombie_life_time() ); break;
		case Child::NOT_SET: break;
	}
	return ZombieAttr(Child::ECF, std::vector<ecf::Child::CmdType>(), User::BLOCK, default_ecf_zombie_life_time());
}

