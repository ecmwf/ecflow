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
#include <stdexcept>

#include <boost/test/unit_test.hpp>

#include "ecflow/core/File.hpp"
#include "ecflow/node/ClientSuiteMgr.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Family.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

using namespace std;
using namespace ecf;

namespace {

defs_ptr load_defs(const std::string& path) {
    defs_ptr defs = std::make_shared<Defs>();
    std::string errorMsg, warningMsg;
    bool parse = defs->restore(path, errorMsg, warningMsg);
    BOOST_CHECK(parse);
    return defs;
}

} // namespace

BOOST_AUTO_TEST_SUITE(U_Node)

BOOST_AUTO_TEST_SUITE(T_ClientSuiteMgr)

BOOST_AUTO_TEST_CASE(test_client_suite_mgr_remove_user) {
    ECF_NAME_THIS_TEST();

    Defs theDefs;
    {
        suite_ptr suite = theDefs.add_suite("test_client_suite_mgr_remove_user");
        family_ptr fam  = suite->add_family("family");
        fam->add_task("t1");
    }

    std::vector<std::string> suites  = {"s1", "s2", "s3"};
    std::vector<std::string> suites1 = {"s1", "s2", "s3", "s4"};
    std::vector<std::string> names;

    ClientSuiteMgr mgr(&theDefs);

    unsigned int handle = mgr.create_client_suite(true, suites, "avi");
    BOOST_CHECK_MESSAGE(mgr.valid_handle(handle), "Expected valid handle");
    BOOST_CHECK_MESSAGE(handle == 1, "Expected handle 1 but found " << handle);
    mgr.suites(handle, names);
    BOOST_CHECK_MESSAGE(names == suites, "suite not as expected");
    BOOST_CHECK_MESSAGE(mgr.clientSuites().size() == 1,
                        "Expected client suite of size 1 but found " << mgr.clientSuites().size());

    names.clear();
    handle = mgr.create_client_suite(true, suites1, "avi");
    BOOST_CHECK_MESSAGE(mgr.valid_handle(handle), "Expected valid handle");
    BOOST_CHECK_MESSAGE(handle == 2, "Expected handle 2 but found " << handle);
    mgr.suites(handle, names);
    BOOST_CHECK_MESSAGE(names == suites1, "suite not as expected for handle_fred");
    BOOST_CHECK_MESSAGE(mgr.clientSuites().size() == 2,
                        "Expected client suite of size 2 but found " << mgr.clientSuites().size());

    names.clear();
    handle = mgr.create_client_suite(true, suites, "fred");
    BOOST_CHECK_MESSAGE(mgr.valid_handle(handle), "Expected valid handle");
    BOOST_CHECK_MESSAGE(handle == 3, "Expected handle 3 but found " << handle);
    mgr.suites(handle, names);
    BOOST_CHECK_MESSAGE(names == suites, "suite not as expected for handle_fred");
    BOOST_CHECK_MESSAGE(mgr.clientSuites().size() == 3,
                        "Expected client suite of size 3 but found " << mgr.clientSuites().size());

    mgr.remove_client_suites("avi"); // should remove two handles
    BOOST_CHECK_MESSAGE(mgr.clientSuites().size() == 1,
                        "Expected client suite of size 1 but found " << mgr.clientSuites().size());
    BOOST_CHECK_THROW(mgr.remove_client_suites("avi"), std::runtime_error);

    mgr.remove_client_suites("fred");
    BOOST_CHECK_MESSAGE(mgr.clientSuites().size() == 0,
                        "Expected client suite of size 0 but found " << mgr.clientSuites().size());
    BOOST_CHECK_THROW(mgr.remove_client_suites("fred"), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(test_client_suite_mgr_remove_handle) {
    ECF_NAME_THIS_TEST();

    Defs theDefs;
    {
        suite_ptr suite = theDefs.add_suite("test_client_suite_mgr_remove_handle");
        family_ptr fam  = suite->add_family("family");
        fam->add_task("t1");
    }

    std::vector<std::string> suites  = {"s1", "s2", "s3"};
    std::vector<std::string> suites1 = {"s1", "s2", "s3", "s4"};
    std::vector<std::string> names;

    ClientSuiteMgr mgr(&theDefs);

    unsigned int handle1 = mgr.create_client_suite(true, suites, "avi");
    BOOST_CHECK_MESSAGE(mgr.valid_handle(handle1), "Expected valid handle");
    BOOST_CHECK_MESSAGE(handle1 == 1, "Expected handle 1 but found " << handle1);
    mgr.suites(handle1, names);
    BOOST_CHECK_MESSAGE(names == suites, "suite not as expected");
    BOOST_CHECK_MESSAGE(mgr.clientSuites().size() == 1,
                        "Expected client suite of size 1 but found " << mgr.clientSuites().size());

    names.clear();
    unsigned int handle2 = mgr.create_client_suite(true, suites1, "avi");
    BOOST_CHECK_MESSAGE(mgr.valid_handle(handle2), "Expected valid handle");
    BOOST_CHECK_MESSAGE(handle2 == 2, "Expected handle 2 but found " << handle2);
    mgr.suites(handle2, names);
    BOOST_CHECK_MESSAGE(names == suites1, "suite not as expected for handle_fred");
    BOOST_CHECK_MESSAGE(mgr.clientSuites().size() == 2,
                        "Expected client suite of size 2 but found " << mgr.clientSuites().size());

    names.clear();
    unsigned int handle3 = mgr.create_client_suite(true, suites, "fred");
    BOOST_CHECK_MESSAGE(mgr.valid_handle(handle3), "Expected valid handle");
    BOOST_CHECK_MESSAGE(handle3 == 3, "Expected handle 3 but found " << handle3);
    mgr.suites(handle3, names);
    BOOST_CHECK_MESSAGE(names == suites, "suite not as expected for handle_fred");
    BOOST_CHECK_MESSAGE(mgr.clientSuites().size() == 3,
                        "Expected client suite of size 3 but found " << mgr.clientSuites().size());

    mgr.remove_client_suite(handle1);
    BOOST_CHECK_MESSAGE(mgr.clientSuites().size() == 2,
                        "Expected client suite of size 2 but found " << mgr.clientSuites().size());
    BOOST_CHECK_THROW(mgr.remove_client_suite(handle1), std::runtime_error);

    mgr.remove_client_suite(handle2);
    BOOST_CHECK_MESSAGE(mgr.clientSuites().size() == 1,
                        "Expected client suite of size 1 but found " << mgr.clientSuites().size());
    BOOST_CHECK_THROW(mgr.remove_client_suite(handle2), std::runtime_error);

    mgr.remove_client_suite(handle3);
    BOOST_CHECK_MESSAGE(mgr.clientSuites().size() == 0,
                        "Expected client suite of size 0 but found " << mgr.clientSuites().size());
    BOOST_CHECK_THROW(mgr.remove_client_suite(handle3), std::runtime_error);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(T_ClientSuiteMgr_SuiteReplacement)

auto path   = File::test_data("libs/node/test/data/replace.txt", "libs/node");
auto user   = std::string("someuser");
auto suites = std::vector<std::string>{"aa", "bb"};

std::string errormsg;

BOOST_AUTO_TEST_CASE(test_suite_filter_replace_filtered_suite_with_auto_add) {
    cout << "ANode:: ...test_suite_filter_replace_filtered_suite_with_auto_add\n";

    auto defs = load_defs(path);
    BOOST_CHECK_EQUAL(defs->suiteVec().size(), 3);

    auto handle = defs->client_suite_mgr().create_client_suite(true, suites, user);

    { // access filtered suites, before replace
        auto filtered = defs->client_suite_mgr().create_defs(handle, defs);
        BOOST_CHECK_EQUAL(filtered->suiteVec().size(), 2);
    }

    { // replace suite
        auto another = load_defs(path);
        defs->replaceChild("/xx", another, true, false, errormsg);
    }

    { // access filtered suites, after replace
        auto filtered = defs->client_suite_mgr().create_defs(handle, defs);
        BOOST_CHECK_EQUAL(filtered->suiteVec().size(), 2);
    }
}

BOOST_AUTO_TEST_CASE(test_suite_filter_replace_selected_suite_with_auto_add) {
    cout << "ANode:: ...test_suite_filter_replace_selected_suite_with_auto_add\n";

    auto defs = load_defs(path);
    BOOST_CHECK_EQUAL(defs->suiteVec().size(), 3);

    auto handle = defs->client_suite_mgr().create_client_suite(true, suites, user);

    { // access filtered suites, before replace
        auto filtered = defs->client_suite_mgr().create_defs(handle, defs);
        BOOST_CHECK_EQUAL(filtered->suiteVec().size(), 2);
    }

    { // replace suite
        auto another = load_defs(path);
        defs->replaceChild("/aa", another, true, false, errormsg);
    }

    { // access filtered suites, after replace
        auto& mng     = defs->client_suite_mgr();
        auto filtered = mng.create_defs(handle, defs);
        BOOST_CHECK_EQUAL(filtered->suiteVec().size(), 2);
    }
}

BOOST_AUTO_TEST_CASE(test_suite_filter_replace_filtered_suite_without_auto_add) {
    cout << "ANode:: ...test_suite_filter_replace_filtered_suite_without_auto_add\n";

    auto defs = load_defs(path);
    BOOST_CHECK_EQUAL(defs->suiteVec().size(), 3);

    auto handle = defs->client_suite_mgr().create_client_suite(false, suites, user);

    { // access filtered suites, before replace
        auto filtered = defs->client_suite_mgr().create_defs(handle, defs);
        BOOST_CHECK_EQUAL(filtered->suiteVec().size(), 2);
    }

    { // replace suite
        auto another = load_defs(path);
        defs->replaceChild("/xx", another, true, false, errormsg);
    }

    { // access filtered suites, after replace
        auto filtered = defs->client_suite_mgr().create_defs(handle, defs);
        BOOST_CHECK_EQUAL(filtered->suiteVec().size(), 2);
    }
}

BOOST_AUTO_TEST_CASE(test_suite_filter_replace_selected_suite_without_auto_add) {
    cout << "ANode:: ...test_suite_filter_replace_selected_suite_without_auto_add\n";

    auto defs = load_defs(path);
    BOOST_CHECK_EQUAL(defs->suiteVec().size(), 3);

    auto handle = defs->client_suite_mgr().create_client_suite(false, suites, user);

    { // access filtered suites, before replace
        auto filtered = defs->client_suite_mgr().create_defs(handle, defs);
        BOOST_CHECK_EQUAL(filtered->suiteVec().size(), 2);
    }

    { // replace suite
        auto another = load_defs(path);
        defs->replaceChild("/aa", another, true, false, errormsg);
    }

    { // access filtered suites, after replace
        auto& mng     = defs->client_suite_mgr();
        auto filtered = mng.create_defs(handle, defs);
        BOOST_CHECK_EQUAL(filtered->suiteVec().size(), 2);
    }
}

BOOST_AUTO_TEST_CASE(test_suite_filter_delete_and_add_filtered_suite_with_auto_add) {
    cout << "ANode:: ...test_suite_filter_delete_and_add_filtered_suite_with_auto_add\n";

    auto defs = load_defs(path);
    BOOST_CHECK_EQUAL(defs->suiteVec().size(), 3);

    auto handle = defs->client_suite_mgr().create_client_suite(true, suites, user);

    { // access filtered suites, before replace
        auto filtered = defs->client_suite_mgr().create_defs(handle, defs);
        BOOST_CHECK_EQUAL(filtered->suiteVec().size(), 2);
    }

    { // delete suite
        auto deleting = defs->findAbsNode("/xx");
        defs->deleteChild(deleting.get());
    }

    { // add suite
        auto local  = load_defs(path);
        auto adding = local->findSuite("xx");
        adding->set_defs(nullptr); // detach suite from 'local' defs, so it can be added to 'defs'
        defs->addSuite(adding, 0);
    }

    { // access filtered suites, after replace
        auto filtered = defs->client_suite_mgr().create_defs(handle, defs);
        BOOST_CHECK_EQUAL(filtered->suiteVec().size(), 3);
    }
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
