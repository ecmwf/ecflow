/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include <iostream>

#include <boost/test/unit_test.hpp>

#include "TestFixture.hpp"
#include "ZombieUtil.hpp"
#include "ecflow/attribute/Zombie.hpp"
#include "ecflow/core/AssertTimer.hpp"
#include "ecflow/core/Environment.hpp"

using namespace std;
using namespace ecf;

void ZombieUtil::test_clean_up(int timeout) {

    // try to tidy up. Avoid leaving zombie process that mess up tests
    TestFixture::client().zombieGet();
    std::vector<Zombie> zombies = TestFixture::client().server_reply().zombies();
    if (!zombies.empty()) {
        cout << "\nZombieUtil::test_clean_up: Found Zombies:\n";
        cout << "Client Environment:\n" << TestFixture::client().to_string() << "\n";
        cout << Zombie::pretty_print(zombies, 9) << "\n, attempting to *fob* then *remove* ...\n";

        int no_fobed =
            do_zombie_user_action(User::FOB, zombies.size(), timeout, false /* don't fail if it takes to long */);

        // In order to FOB, we must wait, till a child command, talks to the server.
        if (no_fobed) {
            int wait = 5;
#if defined(HPUX) || defined(_AIX)
            wait += 5; // On these platforms wait longer,
#endif
            cout << "   Fobed " << no_fobed << " left over zombies. sleeping for " << wait
                 << "s before attempting to remove\n";
            sleep(wait);
        }
        (void)do_zombie_user_action(User::REMOVE, no_fobed, timeout, false /* don't fail if it takes to long */);
    }
}

int ZombieUtil::do_zombie_user_action(User::Action uc,
                                      int expected_action_cnt,
                                      int max_time_to_wait,
                                      bool fail_if_to_long) {
    /// return the number of zombies set to user action;
    bool ecf_debug_zombies = false;
    if (ecf::environment::has("ECF_DEBUG_ZOMBIES")) {
        ecf_debug_zombies = true;
        cout << "\n   do_zombie_user_action " << User::to_string(uc) << " expected_action_cnt " << expected_action_cnt
             << "\n";
    }

    int action_set = 0;
    std::vector<Zombie> action_set_zombies;
    AssertTimer assertTimer(max_time_to_wait, false); // Bomb out after n seconds, fall back if test fail
    while (true) {
        BOOST_REQUIRE_MESSAGE(TestFixture::client().zombieGet() == 0,
                              "zombieGet failed should return 0\n"
                                  << TestFixture::client().errorMsg());
        std::vector<Zombie> zombies = TestFixture::client().server_reply().zombies();
        bool continue_looping       = false;
        for (const Zombie& z : zombies) {
            switch (uc) {
                case User::FOB: {
                    if (!z.fob()) {
                        TestFixture::client().zombieFob(z); // UNBLOCK, child commands, allow zombie to complete, will
                                                            // clear server_reply().zombies()
                        continue_looping = true;
                        action_set++;
                        action_set_zombies.push_back(z);
                    }
                    break;
                }
                case User::FAIL: {
                    if (!z.fail()) {
                        TestFixture::client().zombieFail(z); // UNBLOCK, child commands, allow zombie to complete, will
                                                             // clear server_reply().zombies()
                        continue_looping = true;
                        action_set++;
                        action_set_zombies.push_back(z);
                    }
                    break;
                }
                case User::ADOPT: {
                    if (!z.adopt()) {
                        TestFixture::client().zombieAdopt(z); // UNBLOCK, child commands, allow zombie to complete, will
                                                              // clear server_reply().zombies()
                        continue_looping = true;
                        action_set++;
                        action_set_zombies.push_back(z);
                    }
                    break;
                }
                case User::REMOVE: {
                    if (!z.remove()) {                         // should always return false
                        TestFixture::client().zombieRemove(z); // This should be immediate, and is not remembered
                        continue_looping = true;
                        action_set++;
                        action_set_zombies.push_back(z);
                    }
                    break;
                }
                case User::BLOCK: {
                    if (!z.block()) {
                        TestFixture::client().zombieBlock(z);
                        continue_looping = true;
                        action_set++;
                        action_set_zombies.push_back(z);
                    }
                    break;
                }
                case User::KILL: {
                    if (!z.kill()) {
                        TestFixture::client().zombieKill(z);
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

        if (!continue_looping && action_set > 0) {
            if (expected_action_cnt == 0)
                break; // return, some clients set this as 0
        }

        // make sure test does not take too long.
        if (assertTimer.duration() >= assertTimer.timeConstraint()) {

            if (expected_action_cnt > 0 && action_set > 0) {
                if (ecf_debug_zombies)
                    cout << "   timeing out after action_set = " << action_set
                         << " expected_action_cnt = " << expected_action_cnt << "\n";
                break;
            }

            std::stringstream ss;
            ss << "do_zombie_user_action:\nExpected " << expected_action_cnt << " zombies with user action "
               << User::to_string(uc) << " but found " << action_set << "\naction set zombies\n"
               << Zombie::pretty_print(action_set_zombies, 6) << ", Test taking longer than time constraint of "
               << assertTimer.timeConstraint();
            if (fail_if_to_long) {
                BOOST_REQUIRE_MESSAGE(false, ss.str() << " aborting\n" << Zombie::pretty_print(zombies, 6));
            }
            else {
                cout << ss.str() << " breaking out\n" << Zombie::pretty_print(zombies, 6) << "\n";
                break;
            }
        }
        sleep(1);
    }

    if (ecf_debug_zombies) {
        cout << "   " << action_set << " zombies set to user action " << User::to_string(uc) << " returning\n";
        cout << Zombie::pretty_print(action_set_zombies, 6);
    }
    return action_set;
}
