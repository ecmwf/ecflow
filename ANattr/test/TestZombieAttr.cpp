/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #14 $
//
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
// Description :
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8

#include <iostream>
#include <stdio.h>
#include <boost/test/unit_test.hpp>

#include "ZombieAttr.hpp"
#include "Str.hpp"

using namespace std;
using namespace ecf;

BOOST_AUTO_TEST_SUITE( ANattrTestSuite )

BOOST_AUTO_TEST_CASE( test_zombie_attr )
{
   cout << "ANattr:: ...test_zombie_attr\n";
   {
      ZombieAttr ecf(ecf::Child::ECF,std::vector<ecf::Child::CmdType>(), ecf::User::FAIL );
      BOOST_CHECK_MESSAGE(ecf.zombie_lifetime() == ZombieAttr::default_ecf_zombie_life_time(),"expected " << ZombieAttr::default_ecf_zombie_life_time() << " but got " << ecf.zombie_lifetime());

      ZombieAttr user(ecf::Child::USER,std::vector<ecf::Child::CmdType>(), ecf::User::FAIL );
      BOOST_CHECK_MESSAGE(user.zombie_lifetime() == ZombieAttr::default_user_zombie_life_time(),"Zombie life time not as expected");

      ZombieAttr path(ecf::Child::PATH,std::vector<ecf::Child::CmdType>(), ecf::User::FAIL );
      BOOST_CHECK_MESSAGE(path.zombie_lifetime() == ZombieAttr::default_path_zombie_life_time(),"Zombie life time not as expected");
   }
   {
      int zombie_life_time = 0;
      ZombieAttr ecf(ecf::Child::ECF,std::vector<ecf::Child::CmdType>(), ecf::User::FAIL,zombie_life_time);
      BOOST_CHECK_MESSAGE(ecf.zombie_lifetime() == ZombieAttr::default_ecf_zombie_life_time(),"Zombie life time not as expected");

      ZombieAttr user(ecf::Child::USER,std::vector<ecf::Child::CmdType>(), ecf::User::FAIL,zombie_life_time);
      BOOST_CHECK_MESSAGE(user.zombie_lifetime() == ZombieAttr::default_user_zombie_life_time(),"Zombie life time not as expected");

      ZombieAttr path(ecf::Child::PATH,std::vector<ecf::Child::CmdType>(), ecf::User::FAIL,zombie_life_time);
      BOOST_CHECK_MESSAGE(path.zombie_lifetime() == ZombieAttr::default_path_zombie_life_time(),"Zombie life time not as expected");
   }
   {
      int zombie_life_time = -1;
      ZombieAttr ecf(ecf::Child::ECF,std::vector<ecf::Child::CmdType>(), ecf::User::FAIL,zombie_life_time);
      BOOST_CHECK_MESSAGE(ecf.zombie_lifetime() == ZombieAttr::default_ecf_zombie_life_time(),"Zombie life time not as expected");

      ZombieAttr user(ecf::Child::USER,std::vector<ecf::Child::CmdType>(), ecf::User::FAIL,zombie_life_time);
      BOOST_CHECK_MESSAGE(user.zombie_lifetime() == ZombieAttr::default_user_zombie_life_time(),"Zombie life time not as expected");

      ZombieAttr path(ecf::Child::PATH,std::vector<ecf::Child::CmdType>(), ecf::User::FAIL,zombie_life_time);
      BOOST_CHECK_MESSAGE(path.zombie_lifetime() == ZombieAttr::default_path_zombie_life_time(),"Zombie life time not as expected");
   }
   {
      int zombie_life_time = 29;
      ZombieAttr ecf(ecf::Child::ECF,std::vector<ecf::Child::CmdType>(), ecf::User::FAIL,zombie_life_time);
      BOOST_CHECK_MESSAGE(ecf.zombie_lifetime() == ZombieAttr::minimum_zombie_life_time(),"Zombie life time not as expected");

      ZombieAttr user(ecf::Child::USER,std::vector<ecf::Child::CmdType>(), ecf::User::FAIL,zombie_life_time);
      BOOST_CHECK_MESSAGE(user.zombie_lifetime() == ZombieAttr::minimum_zombie_life_time(),"Zombie life time not as expected");

      ZombieAttr path(ecf::Child::PATH,std::vector<ecf::Child::CmdType>(), ecf::User::FAIL,zombie_life_time);
      BOOST_CHECK_MESSAGE(path.zombie_lifetime() == ZombieAttr::minimum_zombie_life_time(),"Zombie life time not as expected");
   }
}

BOOST_AUTO_TEST_CASE( test_zombie_attr_parsing )
{
   cout << "ANattr:: ...test_zombie_attr_parsing\n";
   {
      ZombieAttr zombie = ZombieAttr::create("user:fob::");
      BOOST_CHECK_MESSAGE( zombie.zombie_type() == ecf::Child::USER,"Type not as expected");
      BOOST_CHECK_MESSAGE( zombie.action() == ecf::User::FOB,"action not as expected");
      BOOST_CHECK_MESSAGE( zombie.zombie_lifetime() == ZombieAttr::default_user_zombie_life_time(),"Zombie life time not as expected");
      BOOST_CHECK_MESSAGE( zombie.child_cmds().empty(),"Expected no children");
   }
   {
      ZombieAttr zombie = ZombieAttr::create("ecf:fail::");
      BOOST_CHECK_MESSAGE( zombie.zombie_type() == ecf::Child::ECF,"Type not as expected");
      BOOST_CHECK_MESSAGE( zombie.action() == ecf::User::FAIL,"action not as expected");
      BOOST_CHECK_MESSAGE( zombie.zombie_lifetime() == ZombieAttr::default_ecf_zombie_life_time(),"Zombie life time not as expected");
      BOOST_CHECK_MESSAGE( zombie.child_cmds().empty(),"Expected no children");
   }
   {
      ZombieAttr zombie = ZombieAttr::create("path:fail::");
      BOOST_CHECK_MESSAGE( zombie.zombie_type() == ecf::Child::PATH,"Type not as expected");
      BOOST_CHECK_MESSAGE( zombie.action() == ecf::User::FAIL,"action not as expected");
      BOOST_CHECK_MESSAGE( zombie.zombie_lifetime() == ZombieAttr::default_path_zombie_life_time(),"Zombie life time not as expected");
      BOOST_CHECK_MESSAGE( zombie.child_cmds().empty(),"Expected no children");
   }

   {
      ZombieAttr zombie = ZombieAttr::create("user:fob::29");
      BOOST_CHECK_MESSAGE( zombie.zombie_type() == ecf::Child::USER,"Type not as expected");
      BOOST_CHECK_MESSAGE( zombie.action() == ecf::User::FOB,"action not as expected");
      BOOST_CHECK_MESSAGE( zombie.zombie_lifetime() == ZombieAttr::minimum_zombie_life_time(),"Zombie life time < 60, should default to 60");
      BOOST_CHECK_MESSAGE( zombie.child_cmds().empty(),"Expected no children");
   }
   {
      ZombieAttr zombie = ZombieAttr::create("user:fob:init:29");
      BOOST_CHECK_MESSAGE( zombie.zombie_type() == ecf::Child::USER,"Type not as expected");
      BOOST_CHECK_MESSAGE( zombie.action() == ecf::User::FOB,"action not as expected");
      BOOST_CHECK_MESSAGE( zombie.zombie_lifetime() == ZombieAttr::minimum_zombie_life_time(),"Zombie life time < 60, should default to 60");
      BOOST_CHECK_MESSAGE( zombie.child_cmds().size()==1,"Expected one child");
   }
}

BOOST_AUTO_TEST_SUITE_END()
