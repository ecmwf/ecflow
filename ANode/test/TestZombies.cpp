/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #10 $
//
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
#include <iostream>
#include <cstdlib>

#include <boost/test/unit_test.hpp>

#include "Defs.hpp"
#include "Suite.hpp"
#include "Family.hpp"
#include "Task.hpp"
#include "System.hpp"

using namespace std;
using namespace ecf;

BOOST_AUTO_TEST_SUITE( NodeTestSuite )

BOOST_AUTO_TEST_CASE( test_zombies )
{
   cout << "ANode:: ...test_zombies\n";
   Defs theDefs;
   suite_ptr s = theDefs.add_suite("s");
   task_ptr t = s->add_family("f")->add_task("t");

   // SANITY
   {
      BOOST_REQUIRE_MESSAGE(s->zombies().size() == 0, "Expected 0 zombies but found " << s->zombies().size());
      BOOST_REQUIRE_MESSAGE(s->findZombie(ecf::Child::USER).empty(), "Expected no zombies");
      BOOST_REQUIRE_MESSAGE(s->findZombie(ecf::Child::ECF).empty(), "Expected no zombies");
      BOOST_REQUIRE_MESSAGE(s->findZombie(ecf::Child::ECF_PID).empty(), "Expected no zombies");
      BOOST_REQUIRE_MESSAGE(s->findZombie(ecf::Child::ECF_PASSWD).empty(), "Expected no zombies");
      BOOST_REQUIRE_MESSAGE(s->findZombie(ecf::Child::ECF_PID_PASSWD).empty(), "Expected no zombies");
      BOOST_REQUIRE_MESSAGE(s->findZombie(ecf::Child::PATH).empty(), "Expected no zombies");
      ZombieAttr attr;
      BOOST_REQUIRE_MESSAGE(!t->findParentZombie(ecf::Child::PATH,attr) && attr.empty(), "Expected to NOT find PATH zombies on parent");
   }

   // ADD
   std::vector<ecf::Child::CmdType> child_cmds = ecf::Child::list();
   {
      s->addZombie( ZombieAttr(ecf::Child::USER, child_cmds, ecf::User::FOB,10) );
      BOOST_REQUIRE_MESSAGE(s->zombies().size() == 1, "Expected 1 zombie but found " << s->zombies().size());
      s->addZombie( ZombieAttr(ecf::Child::ECF, child_cmds, ecf::User::FAIL,100) );
      BOOST_REQUIRE_MESSAGE(s->zombies().size() == 2, "Expected 2 zombie but found " << s->zombies().size());
      s->addZombie( ZombieAttr(ecf::Child::ECF_PID, child_cmds, ecf::User::FAIL,100) );
      BOOST_REQUIRE_MESSAGE(s->zombies().size() == 3, "Expected 3 zombie but found " << s->zombies().size());
      s->addZombie( ZombieAttr(ecf::Child::ECF_PID_PASSWD, child_cmds, ecf::User::FAIL,100) );
      BOOST_REQUIRE_MESSAGE(s->zombies().size() == 4, "Expected 4 zombie but found " << s->zombies().size());
      s->addZombie( ZombieAttr(ecf::Child::ECF_PASSWD, child_cmds, ecf::User::FAIL,100) );
      BOOST_REQUIRE_MESSAGE(s->zombies().size() == 5, "Expected 5 zombie but found " << s->zombies().size());
      s->addZombie( ZombieAttr(ecf::Child::PATH, child_cmds, ecf::User::BLOCK,100) );
      BOOST_REQUIRE_MESSAGE(s->zombies().size() == 6, "Expected 6 zombie but found " << s->zombies().size());
   }

   // FIND
   BOOST_REQUIRE_MESSAGE(!s->findZombie(ecf::Child::USER).empty(), "Expected to find USER zombies");
   BOOST_REQUIRE_MESSAGE(!s->findZombie(ecf::Child::ECF).empty(), "Expected to find ECF zombies");
   BOOST_REQUIRE_MESSAGE(!s->findZombie(ecf::Child::ECF_PID).empty(), "Expected to find ECF_PID zombies");
   BOOST_REQUIRE_MESSAGE(!s->findZombie(ecf::Child::ECF_PID_PASSWD).empty(), "Expected to find ECF_PID_PASSWD zombies");
   BOOST_REQUIRE_MESSAGE(!s->findZombie(ecf::Child::ECF_PASSWD).empty(), "Expected to find ECF_PASSWD zombies");
   BOOST_REQUIRE_MESSAGE(!s->findZombie(ecf::Child::PATH).empty(), "Expected to find PATH zombies");

   // FIND on parent
   {
      ZombieAttr path_z,ecf_z,ecf_pid_z,ecf_pid_passwd_z,ecf_passwd_z,user_z;
      BOOST_REQUIRE_MESSAGE(t->findParentZombie(ecf::Child::PATH,path_z) && !path_z.empty(), "Expected to find PATH zombies on parent");
      BOOST_REQUIRE_MESSAGE(t->findParentZombie(ecf::Child::ECF,ecf_z) && !ecf_z.empty(), "Expected to find ECF zombies on parent");
      BOOST_REQUIRE_MESSAGE(t->findParentZombie(ecf::Child::ECF_PID,ecf_pid_z) && !ecf_pid_z.empty(), "Expected to find ECF_PID zombies on parent");
      BOOST_REQUIRE_MESSAGE(t->findParentZombie(ecf::Child::ECF_PID_PASSWD,ecf_pid_passwd_z) && !ecf_pid_passwd_z.empty(), "Expected to find ECF_PID_PASSWD zombies on parent");
      BOOST_REQUIRE_MESSAGE(t->findParentZombie(ecf::Child::ECF_PASSWD,ecf_passwd_z) && !ecf_passwd_z.empty(), "Expected to find ECF_PASSWD zombies on parent");
      BOOST_REQUIRE_MESSAGE(t->findParentZombie(ecf::Child::USER,user_z) && !user_z.empty(), "Expected to find USER zombies on parent");
   }

   /// Destroy System singleton to avoid valgrind from complaining
   System::destroy();
}

BOOST_AUTO_TEST_SUITE_END()
