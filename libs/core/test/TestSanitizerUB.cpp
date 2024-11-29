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

#include "ecflow/core/Environment.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

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

    bool is_available = ecf::environment::has("ECF_TEST_SANITIZER_UB");
    return is_available;
}

BOOST_AUTO_TEST_SUITE(U_Core)

BOOST_AUTO_TEST_SUITE(T_SanitizerUB)

struct Base
{
    int pad1{0};
};
struct Derived : Base
{
    int pad2{0};
};
Derived* getDerived() {
    return static_cast<Derived*>(new Base); // Error: invalid downcast
}

#if defined(__GNUC__) and !defined(__clang__)
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Warray-bounds"
#endif

BOOST_AUTO_TEST_CASE(test_sanitizer_invalid_object_size) {
    ECF_NAME_THIS_TEST();

    if (!is_sanitizer_available()) {
        return;
    }

    // This check detects pointer casts in which the size of the source type is less than the size of the
    // destination type. Using the result of such a cast to access out-of-bounds data has undefined behaviour
    cout << getDerived()->pad2 << "\n";
    BOOST_CHECK_MESSAGE(true, "stop boost test from complaining");
}

#if defined(__GNUC__) and !defined(__clang__)
    #pragma GCC diagnostic pop
#endif

BOOST_AUTO_TEST_CASE(test_sanitizer_misaligned_structure_pointer_assignment) {
    ECF_NAME_THIS_TEST();

    if (!is_sanitizer_available()) {
        return;
    }

    // In the following example, the pointer variable is required to have 8-byte alignment, but is only 1-byte

    struct A
    {
        int32_t i32;
        int64_t i64;
    };
    int8_t* buffer    = (int8_t*)malloc(32);
    struct A* pointer = (struct A*)(buffer + 1);
    pointer->i32      = 7; // Error: pointer is misaligned

    BOOST_CHECK_MESSAGE(pointer->i32 == 7, "expected error");

    // solution
    // One solution is to mark the struct as packed. In the following example, the A structure is packed,
    // preventing the compiler from adding padding between members.
    // struct A { ... } __attribute__((packed));
    BOOST_CHECK_MESSAGE(true, "stop boost test from complaining");
}

BOOST_AUTO_TEST_CASE(test_sanitizer_out_of_bounds_array_access) {
    ECF_NAME_THIS_TEST();

    if (!is_sanitizer_available()) {
        return;
    }

    // This check detects out-of-bounds access of arrays with fixed or variable-length sizes.
    // Out-of-bounds array accesses have undefined behaviour, and can result in crashes or incorrect program output.
    int array[5] = {0, 0, 0, 0, 0};
    for (int i = 0; i <= 5; ++i) {
        array[i] += 1; // Error: out-of-bounds access on the last iteration
        BOOST_REQUIRE_NO_THROW(array[i] += 1);
    }
}

struct A
{
    int x;
    int getX() {
        //        if (!this) { // Warning: redundant null check may be removed
        //            return 0;
        //        }
        return x; // Warning: 'this' pointer is null, but is dereferenced here
    }
};

BOOST_AUTO_TEST_CASE(test_sanitizer_member_access_through_null_pointer) {
    ECF_NAME_THIS_TEST();

    if (!is_sanitizer_available()) {
        return;
    }

    A* a  = nullptr;
    int x = a->getX(); // Error: member access through null pointer
    BOOST_CHECK_MESSAGE(x, "stop boost test from complaining");
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
