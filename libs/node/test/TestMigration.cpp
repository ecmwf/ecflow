/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include <boost/test/unit_test.hpp>

#include "MyDefsFixture.hpp"
#include "ecflow/core/Ecf.hpp"
#include "ecflow/core/File.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Family.hpp"
#include "ecflow/node/Limit.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/node/Task.hpp"
#include "ecflow/test/scaffold/Naming.hpp"
#include "ecflow/test/scaffold/Serialisation.hpp"

using namespace std;
using namespace ecf;
using namespace boost::posix_time;
using namespace boost::gregorian;

// #define UPDATE_TESTS 1

BOOST_AUTO_TEST_SUITE(U_Node)

BOOST_AUTO_TEST_SUITE(T_Migration)

BOOST_AUTO_TEST_CASE(test_default_constructor_persistence) {
    ECF_NAME_THIS_TEST();

    std::string file_name = File::test_data("libs/node/test/data/migration/", "libs/node");

    Defs defs;
    Suite suite;
    Family family;
    Task task;

    // Can't persist server variable are dependent on HOST.i.e ECF_LISTS,ECF_CHECK,etc
    // Hence is not cross-platform
    doSave(file_name + "Defs.def", defs);

#ifdef UPDATE_TESTS
    doSave(file_name + "Suite.def", suite);
    doSave(file_name + "Family.def", family);
    doSave(file_name + "Task.def", task);
    doSave(file_name + "Limit.def", Limit());
#endif

    DebugEquality debug_equality; // only as affect in DEBUG build
    do_restore<Defs>(file_name + "Defs.def", defs);
    do_restore<Suite>(file_name + "Suite.def", suite);
    do_restore<Family>(file_name + "Family.def", family);
    do_restore<Task>(file_name + "Task.def", task);
    do_restore<Limit>(file_name + "Limit.def", Limit());

    fs::remove(file_name + "Defs.def"); // Remove the file. Comment out for debugging
}

BOOST_AUTO_TEST_CASE(test_compare_cereal_and_defs_checkpt_file) {
    ECF_NAME_THIS_TEST();

    std::string file_name = File::test_data("libs/node/test/data/migration/", "libs/node");

    // Cannot save these tests since server variable use HOST which is different for each platform
    MyDefsFixture fixture;
    doSave(file_name + "cereal.checkpt", fixture.fixtureDefsFile());
    fixture.fixtureDefsFile().save_as_checkpt(file_name + "defs.checkpt");

    DebugEquality debug_equality; // only as affect in DEBUG build
    do_restore<Defs>(file_name + "cereal.checkpt", fixture.fixtureDefsFile());

    Defs defs;
    defs.restore(file_name + "defs.checkpt");
    BOOST_CHECK_MESSAGE(defs == fixture.fixtureDefsFile(), " ");

    fs::remove(file_name + "defs.checkpt");   // Remove the file. Comment out for debugging
    fs::remove(file_name + "cereal.checkpt"); // Remove the file. Comment out for debugging
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
