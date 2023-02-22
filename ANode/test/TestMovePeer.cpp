/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #10 $
//
// Copyright 2009- ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
#include <boost/test/unit_test.hpp>

#include "Defs.hpp"
#include "Family.hpp"
#include "Suite.hpp"
#include "Task.hpp"
// #include "PrintStyle.hpp"

using namespace std;
using namespace ecf;

BOOST_AUTO_TEST_SUITE(NodeTestSuite)

BOOST_AUTO_TEST_CASE(test_move_peer) {
    cout << "ANode:: ...test_move_peer\n";
    Defs defs;
    {
        std::vector<string> vec{"3", "2", "1"};
        for (const auto& str0 : vec) {
            suite_ptr s = defs.add_suite("s" + str0);
            for (const auto& str : vec) {
                family_ptr f1 = s->add_family("f" + str);
                for (const auto& str2 : vec) {
                    f1->add_task("t" + str2);
                }
            }
        }
    }
    Defs expectedDefs;
    {
        std::vector<string> vec{"1", "2", "3"};
        for (const auto& str0 : vec) {
            suite_ptr s = expectedDefs.add_suite("s" + str0);
            for (const auto& str : vec) {
                family_ptr f1 = s->add_family("f" + str);
                for (const auto& str2 : vec) {
                    f1->add_task("t" + str2);
                }
            }
        }
    }
    //   std::cout << defs << "\n";

    suite_ptr s3 = defs.findSuite("s3");
    suite_ptr s2 = defs.findSuite("s2");
    suite_ptr s1 = defs.findSuite("s1");
    // order     is s3,s2,s1
    defs.move_peer(s1.get(), s3.get()); // order now is s1,s3,s2
    defs.move_peer(s2.get(), s3.get()); // order now is s1,s2,s3

    //   std::cout << defs << "\n";

    node_ptr s1_f1 = defs.findAbsNode("/s1/f1");
    node_ptr s1_f2 = defs.findAbsNode("/s1/f2");
    node_ptr s1_f3 = defs.findAbsNode("/s1/f3");

    // order is f3,f2,f1
    s1->move_peer(s1_f1.get(), s1_f3.get()); // order is f1,f3,f2
    s1->move_peer(s1_f2.get(), s1_f3.get()); // order is f1,f3,f2

    node_ptr s2_f1 = defs.findAbsNode("/s2/f1");
    node_ptr s2_f2 = defs.findAbsNode("/s2/f2");
    node_ptr s2_f3 = defs.findAbsNode("/s2/f3");
    // order is f3,f2,f1
    s2->move_peer(s2_f1.get(), s2_f3.get()); // order is f1,f3,f2
    s2->move_peer(s2_f2.get(), s2_f3.get()); // order is f1,f3,f2

    node_ptr s3_f1 = defs.findAbsNode("/s3/f1");
    node_ptr s3_f2 = defs.findAbsNode("/s3/f2");
    node_ptr s3_f3 = defs.findAbsNode("/s3/f3");
    // order is f3,f2,f1
    s3->move_peer(s3_f1.get(), s3_f3.get()); // order is f1,f3,f2
    s3->move_peer(s3_f2.get(), s3_f3.get()); // order is f1,f3,f2

    for (auto suite : defs.suiteVec()) {
        for (auto family : suite->familyVec()) {

            node_ptr t1 = defs.findAbsNode(family->absNodePath() + "/t1");
            node_ptr t2 = defs.findAbsNode(family->absNodePath() + "/t2");
            node_ptr t3 = defs.findAbsNode(family->absNodePath() + "/t3");

            family->move_peer(t1.get(), t3.get());
            family->move_peer(t2.get(), t3.get());
        }
    }
    //   std::cout << defs << "\n";

    BOOST_CHECK_MESSAGE(defs == expectedDefs, "Defs are not the same");
}
BOOST_AUTO_TEST_SUITE_END()
