//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #5 $
//
// Copyright 2009-2020 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
// Description
//============================================================================
#include <boost/test/unit_test.hpp>
#include <boost/lexical_cast.hpp>


#include <iostream>

using namespace boost;
using namespace std;

BOOST_AUTO_TEST_SUITE( CoreTestSuite )

bool bool_returning_function() { return true;}
int integer_returning_function() { return 3;}

// *** This test does not seem work with address sanitiser ****
BOOST_AUTO_TEST_CASE( test_sanitizer_use_of_out_of_scope_stack_memory )
{
   char* test_me = getenv("ECF_TEST_SANITIZER_AS");
   if (test_me) {
      cout << "ACore:: ...test_sanitizer_use_of_out_of_scope_stack_memory\n" ;

      int *pointer = NULL;
      if (bool_returning_function()) {
         int value = integer_returning_function();
         pointer = &value;
      }
      cout << "dodgy pointer:" << *pointer << "\n";  // Error: invalid access of stack memory out of declaration scope
      *pointer = 42;
      cout << "dodgy pointer:" << *pointer << "\n";
      BOOST_CHECK_MESSAGE( true,"stop boost test from complaining");
   }
}

BOOST_AUTO_TEST_CASE( test_sanitizer_vector_overflow )
{
   char* test_me = getenv("ECF_TEST_SANITIZER_AS");
   if (test_me) {
      cout << "ACore:: ...test_sanitizer_vector_overflow\n" ;
      // This check detects when a libc++ container is accessed beyond the region [container.begin(), container.end()) —
      // even when the accessed memory is in a heap-allocated buffer used internally by a container.

      // the following example, the vector variable has valid indexes in the range [0, 2], but is accessed at index 3, causing an overflow.
      std::vector<int> vector{0,1,2};
      auto *pointer = &vector[0];
      cout << pointer[3]; // Error: out of bounds access for vector
      BOOST_CHECK_MESSAGE( true,"stop boost test from complaining");
   }
}

BOOST_AUTO_TEST_SUITE_END()
