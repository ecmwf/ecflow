/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #10 $
//
// Copyright 2009-2016 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
#include <stdlib.h>
#include <iostream>
#include <boost/test/unit_test.hpp>

#include "MyDefsFixture.hpp"
#include "Ecf.hpp"

using namespace std;
using namespace ecf;

BOOST_AUTO_TEST_SUITE( NodeTestSuite )

BOOST_AUTO_TEST_CASE( test_assignment_operator )
{
   cout << "ANode:: ...test_assignment_operator\n";
   MyDefsFixture theDefsFixture;

   Ecf::set_debug_equality(true);
   Defs defs;
   defs = theDefsFixture.defsfile_;
   BOOST_CHECK_MESSAGE( defs == theDefsFixture.defsfile_,"assignment failed");

   Defs empty;
   defs = empty;
   BOOST_CHECK_MESSAGE( defs == empty,"assignment failed");
   Ecf::set_debug_equality(false);

   BOOST_CHECK_MESSAGE( !(defs == theDefsFixture.defsfile_),"assignment failure EXPECTED");

   theDefsFixture.defsfile_ =  empty;
   Ecf::set_debug_equality(true);
   BOOST_CHECK_MESSAGE( theDefsFixture.defsfile_ == empty,"assignment failed");
   Ecf::set_debug_equality(false);
}

BOOST_AUTO_TEST_SUITE_END()
