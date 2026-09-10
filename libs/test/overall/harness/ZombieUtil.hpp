/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ecflow_test_harness_ZombieUtil_HPP
#define ecflow_test_harness_ZombieUtil_HPP

#include "ecflow/core/Child.hpp"
#include "ecflow/core/ZombieCtrlAction.hpp"

class ClientInvoker;

class ZombieUtil {
public:
    ZombieUtil() = delete;

    static void test_clean_up(int timeout);
    static int do_zombie_user_action(ecf::ZombieCtrlAction uc,
                                     int expected_action_cnt,
                                     int max_time_to_wait,
                                     bool fail_if_to_long = true);
};

class TestClean {
public:
    explicit TestClean(int timeout = 25)
        : timeout_(timeout) {
        ZombieUtil::test_clean_up(timeout);
    }

    TestClean(const TestClean&)            = delete;
    TestClean& operator=(const TestClean&) = delete;
    TestClean(TestClean&&)                 = delete;
    TestClean& operator=(TestClean&&)      = delete;

    ~TestClean() { ZombieUtil::test_clean_up(timeout_); }

private:
    int timeout_;
};

#endif /* ecflow_test_harness_ZombieUtil_HPP */
