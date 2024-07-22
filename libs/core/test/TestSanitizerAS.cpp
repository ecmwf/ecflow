/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include <cstdlib>

#include <boost/test/unit_test.hpp>

#include "TestNaming.hpp"
#include "ecflow/core/Environment.hpp"

using namespace boost;
using namespace std;

static bool is_sanitizer_available() {

    /*
     * This condition is used to skip the test if the sanitizer is not available.
     *
     * The implementation to skip a test is currently to short-circuit into early termination.
     * A better approach would be to use the boost::unit_test::precondition() decorator to skip the test based on
     * a runtime condition.
     * Unfortunatelly this approach does not work when using Boost 1.66 @ Rocky 8.6.
     */

    bool is_available = ecf::environment::has("ECF_TEST_SANITIZER_AS");
    return is_available;
}

BOOST_AUTO_TEST_SUITE(U_Core)

BOOST_AUTO_TEST_SUITE(T_SanitizerAS)

bool bool_returning_function() {
    return true;
}
int integer_returning_function() {
    return 3;
}

#if defined(__GNUC__) and !defined(__clang__)
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif

// *** This test does not seem work with address sanitiser ****
BOOST_AUTO_TEST_CASE(test_sanitizer_use_of_out_of_scope_stack_memory) {
    ECF_NAME_THIS_TEST();

    if (!is_sanitizer_available()) {
        return;
    }

    int* pointer = NULL;
    if (bool_returning_function()) {
        int value = integer_returning_function();
        pointer   = &value;
    }
    cout << "dodgy pointer:" << *pointer << "\n"; // Error: invalid access of stack memory out of declaration scope
    *pointer = 42;
    cout << "dodgy pointer:" << *pointer << "\n";
    BOOST_CHECK_MESSAGE(true, "stop boost test from complaining");
}

#if defined(__GNUC__) and !defined(__clang__)
    #pragma GCC diagnostic pop
#endif

#if defined(__GNUC__) and !defined(__clang__)
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Warray-bounds"
#endif

BOOST_AUTO_TEST_CASE(test_sanitizer_vector_overflow) {
    ECF_NAME_THIS_TEST();

    if (!is_sanitizer_available()) {
        return;
    }

    // This check detects when a libc++ container is accessed beyond the region [container.begin(), container.end())
    // — even when the accessed memory is in a heap-allocated buffer used internally by a container.

    // the following example, the vector variable has valid indexes in the range [0, 2], but is accessed at index 3,
    // causing an overflow.
    std::vector<int> vector{0, 1, 2};
    auto* pointer = &vector[0];
    cout << pointer[3]; // Error: out of bounds access for vector
    BOOST_CHECK_MESSAGE(true, "stop boost test from complaining");
}

#if defined(__GNUC__) and !defined(__clang__)
    #pragma GCC diagnostic pop
#endif

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
