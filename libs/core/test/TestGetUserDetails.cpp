/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include <cstdio>
#include <iostream>
#include <pwd.h> /* getpwdid */
#include <string>
#include <unistd.h>

#include <boost/test/unit_test.hpp>
#include <sys/types.h>

#include "TestNaming.hpp"

using namespace std;

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
