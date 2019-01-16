#ifndef ZOMBIE_ATTR_HPP_
#define ZOMBIE_ATTR_HPP_
//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #9 $ 
//
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description :
//============================================================================
#include <iosfwd>
#include <cereal/access.hpp>
#include "Child.hpp"

// Class ZombieAttr:
// Use compiler , generated destructor, assignment, copy constructor
// ZombieAttr does *not* have any changeable state
class ZombieAttr {
public:
	ZombieAttr(ecf::Child::ZombieType t, const std::vector<ecf::Child::CmdType>& c, ecf::User::Action a, int zombie_lifetime = 0);
	ZombieAttr()= default;

 	bool operator==(const ZombieAttr& rhs) const;
	std::ostream& print(std::ostream&) const;
	bool empty() const { return zombie_type_ == ecf::Child::NOT_SET; }

	ecf::Child::ZombieType zombie_type() const { return zombie_type_;}
	ecf::User::Action      action() const { return action_; }
	int                    zombie_lifetime() const { return zombie_lifetime_; }
	const std::vector<ecf::Child::CmdType>& child_cmds() const { return child_cmds_; }

	std::vector<ecf::Child::CmdType>::const_iterator child_begin() const { return child_cmds_.begin();} // for python
	std::vector<ecf::Child::CmdType>::const_iterator child_end()   const { return child_cmds_.end();  } // for python

	std::string toString() const;

	bool fob( ecf::Child::CmdType ) const;
	bool fail( ecf::Child::CmdType ) const;
	bool adopt( ecf::Child::CmdType ) const;
	bool block( ecf::Child::CmdType ) const;
   bool remove( ecf::Child::CmdType ) const;
   bool kill( ecf::Child::CmdType ) const;

	/// Create from a string. Will throw std::runtime_error of parse errors
	/// expects <zombie_type>:<user_action>:child_cmds:zombie_lifetime
	static ZombieAttr create(const std::string& str);

	// Added to support return by reference
	static const ZombieAttr& EMPTY();

	// Provide the default behaviour
	static ZombieAttr get_default_attr(ecf::Child::ZombieType);

	static int default_ecf_zombie_life_time()  { return 3600; }
	static int default_user_zombie_life_time() { return 300; }
   static int default_path_zombie_life_time() { return 900; }
   static int minimum_zombie_life_time()      { return 60; }

private:
	ecf::Child::ZombieType           zombie_type_{ecf::Child::NOT_SET};      // User,path or ecf
	ecf::User::Action                action_{ecf::User::BLOCK};           // fob, fail,remove, adopt, block, kill
	int                              zombie_lifetime_{0};  // How long zombie lives in server
	std::vector<ecf::Child::CmdType> child_cmds_;       // init, event, meter,label, complete

   friend class cereal::access;
   template<class Archive>
   void serialize(Archive & ar, std::uint32_t const version );
};

#endif
