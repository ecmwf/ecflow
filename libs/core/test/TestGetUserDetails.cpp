/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#include <cstdio>
#include <pwd.h> /* getpwdid */
#include <string>
#include <unistd.h>

#include <boost/test/unit_test.hpp>
#include <sys/types.h>

#include "ecflow/test/scaffold/Naming.hpp"

BOOST_AUTO_TEST_SUITE(U_Core)

BOOST_AUTO_TEST_SUITE(T_GetUserDetails)

BOOST_AUTO_TEST_CASE(test_get_user_details, *boost::unit_test::disabled()) {
    ECF_NAME_THIS_TEST();

    /* Get the uid of the running processand use it to get a record from /etc/passwd */
    struct passwd* passwd = getpwuid(getuid());

    printf("\n The Real User Name is %s ", passwd->pw_gecos);
    printf("\n The Login Name is %s ", passwd->pw_name);
    printf("\n The Home Directory is %s", passwd->pw_dir);
    printf("\n The Login Shell is %s ", passwd->pw_shell);
    printf("\n The Passwd is %s ", getpwuid(getuid())->pw_passwd);
    printf("\n The uid is %lu ", (unsigned long)getpwuid(getuid())->pw_uid);
    printf("\n The gid is %lu \n\n", (unsigned long)getpwuid(getuid())->pw_gid);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
