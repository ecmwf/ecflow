//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #57 $
//
// Copyright 2009-2012 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include <iostream>
#include <boost/test/unit_test.hpp>

#include "ZombieUtil.hpp"

#include "Defs.hpp"
#include "Suite.hpp"
#include "Family.hpp"
#include "Task.hpp"
#include "ClientInvoker.hpp"
#include "PrintStyle.hpp"
#include "ClientToServerCmd.hpp"
#include "AssertTimer.hpp"
#include "Zombie.hpp"

using namespace std;
using namespace ecf;

void ZombieUtil::test_clean_up(int timeout) {

   // try to tidy up. Avoid leaving zombie process that mess up tests
   ClientInvoker theClient;
   theClient.zombieGet();
   std::vector<Zombie> zombies = theClient.server_reply().zombies();
   if (!zombies.empty()) {
      cout << "\n***** test_clean_up: found\n" << Zombie::pretty_print( zombies , 9) << "\n, attempting to *fob* then *remove* ...\n";

      int no_fobed = do_zombie_user_action(User::FOB, zombies.size(), timeout,theClient, false /* don't fail if it takes to long */);
      if (no_fobed) { cout << "   Fobed " << no_fobed << " left over zombies. Attempting to remove, sleep first\n"; sleep(5);}
      (void) do_zombie_user_action(User::REMOVE, no_fobed, timeout,theClient, false /* don't fail if it takes to long */);
   }
}

int ZombieUtil::do_zombie_user_action(User::Action uc, int expected_action_cnt, int max_time_to_wait,ClientInvoker& theClient, bool fail_if_to_long)
{
   /// return the number of zombies set to user action;
#ifdef DEBUG_ZOMBIE
   cout << "\n   do_zombie_user_action " << User::to_string(uc) << " expected_action_cnt " << expected_action_cnt << "\n";
#endif

   int action_set =  0;
   std::vector<Zombie> action_set_zombies;
   AssertTimer assertTimer(max_time_to_wait,false); // Bomb out after n seconds, fall back if test fail
   while (1) {
      BOOST_REQUIRE_MESSAGE(theClient.zombieGet() == 0, "zombieGet failed should return 0\n" << theClient.errorMsg());
      std::vector<Zombie> zombies = theClient.server_reply().zombies();
      bool continue_looping = false;
      BOOST_FOREACH(const Zombie& z, zombies) {
         switch (uc) {
            case User::FOB: {
               if (!z.fob()) {
                  theClient.zombieFob(z); // UNBLOCK, child commands, allow zombie to complete, will clear server_reply().zombies()
                  continue_looping = true;
                  action_set++;
                  action_set_zombies.push_back(z);
               }
               break;
            }
            case User::FAIL: {
               if (!z.fail()) {
                  theClient.zombieFail(z); // UNBLOCK, child commands, allow zombie to complete, will clear server_reply().zombies()
                  continue_looping = true;
                  action_set++;
                  action_set_zombies.push_back(z);
               }
               break;
            }
            case User::ADOPT: {
               if (!z.adopt()) {
                  theClient.zombieAdopt(z); // UNBLOCK, child commands, allow zombie to complete, will clear server_reply().zombies()
                  continue_looping = true;
                  action_set++;
                  action_set_zombies.push_back(z);
               }
               break;
            }
            case User::REMOVE: {
               if (!z.remove()) {             // should always return false
                  theClient.zombieRemove(z);  // This should be immediate, and is not remembered
                  continue_looping = true;
                  action_set++;
                  action_set_zombies.push_back(z);
               }
               break;
            }
            case User::BLOCK:  {
               if (!z.block()) {
                  theClient.zombieBlock(z);
                  continue_looping = true;
                  action_set++;
                  action_set_zombies.push_back(z);
               }
               break;
            }
            case User::KILL:  {
                if (!z.kill()) {
                   theClient.zombieKill(z);
                   continue_looping = true;
                   action_set++;
                   action_set_zombies.push_back(z);
                }
                break;
             }
         }
      }

      if (expected_action_cnt == action_set) {
         break; // return
      }

      if ( !continue_looping  && action_set > 0) {
         if (expected_action_cnt == 0) break;          // return, some clients set this as 0
         if (expected_action_cnt == action_set) break; // return
      }

      // make sure test does not take too long.
      if ( assertTimer.duration() >=  assertTimer.timeConstraint() ) {
         std::stringstream ss;
         ss << "do_zombie_user_action:\nExpected " << expected_action_cnt
            << " zombies with user action " << User::to_string(uc) << " but found " << action_set << "\naction set zombies\n"
            << Zombie::pretty_print( action_set_zombies , 6)
            << ", Test taking longer than time constraint of "
            << assertTimer.timeConstraint();
         if (fail_if_to_long) {
            BOOST_REQUIRE_MESSAGE(false,ss.str() << " aborting\n" << Zombie::pretty_print( zombies , 6));
         }
         else {
            cout << ss.str() << " breaking out\n" << Zombie::pretty_print( zombies , 6) << "\n";
            break;
         }
      }
      sleep(1);
   }

   // return the real state of the server zombies.
   // *** Note: remove is immediate hence z.remove() below is not valid
   action_set = 0;
   BOOST_REQUIRE_MESSAGE(theClient.zombieGet() == 0, "zombieGet failed should return 0\n" << theClient.errorMsg());
   std::vector<Zombie> zombies = theClient.server_reply().zombies();
   BOOST_FOREACH(const Zombie& z, zombies) {
      switch (uc) {
         case User::FOB:    { if (z.fob())    action_set++; break; }
         case User::FAIL:   { if (z.fail())   action_set++; break; }
         case User::ADOPT:  { if (z.adopt())  action_set++; break; }
         case User::REMOVE: { if (z.remove()) action_set++; break; }
         case User::BLOCK:  { if (z.block())  action_set++; break; }
         case User::KILL:   { if (z.kill())   action_set++; break; }
      }
   }
#ifdef DEBUG_ZOMBIE
   cout << "   " << action_set << " zombies set to user action " << User::to_string(uc) << " returning\n";
   cout << Zombie::pretty_print( zombies , 6);
#endif
   return action_set;
}
