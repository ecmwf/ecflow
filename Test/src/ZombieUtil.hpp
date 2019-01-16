#ifndef ZOMBIE_UTIL_HPP_
#define ZOMBIE_UTIL_HPP_
//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #57 $
//
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "Child.hpp"
class ClientInvoker;

class ZombieUtil {
private:
  ZombieUtil(const ZombieUtil&) = delete;
  const ZombieUtil& operator=(const ZombieUtil&) = delete;
public:
   static void test_clean_up(int timeout);
   static int do_zombie_user_action(ecf::User::Action uc,
                                    int expected_action_cnt,
                                    int max_time_to_wait,
                                    bool fail_if_to_long = true);
};

class TestClean {
private:
  TestClean(const TestClean&) = delete;
  const TestClean& operator=(const TestClean&) = delete;
public:
   explicit TestClean(int timeout = 25) : timeout_(timeout)  { ZombieUtil::test_clean_up(timeout);}
   ~TestClean() { ZombieUtil::test_clean_up(timeout_);}
private:
   int timeout_;
};

#endif
