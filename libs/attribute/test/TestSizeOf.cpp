/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include <fstream>
#include <iostream>

#include <boost/test/unit_test.hpp>

#include "ecflow/attribute/DayAttr.hpp"
#include "ecflow/core/TimeSeries.hpp"
#include "ecflow/test/scaffold/Naming.hpp"

#define STRINGIFY(x) #x

template <typename T>
inline const char* typeName(void) {
    return "unknown";
}

#define TYPE_STRING(T)                 \
    template <>                        \
    inline const char* typeName<T>() { \
        return STRINGIFY(T);           \
    }

TYPE_STRING(std::ofstream)
TYPE_STRING(std::string)
TYPE_STRING(std::vector<int>)
TYPE_STRING(std::vector<std::string>)
TYPE_STRING(std::weak_ptr<int>)
TYPE_STRING(std::nullptr_t)
TYPE_STRING(std::unique_ptr<int>)
TYPE_STRING(double)
TYPE_STRING(long)
TYPE_STRING(int)
TYPE_STRING(unsigned int)
TYPE_STRING(bool)
TYPE_STRING(ecf::TimeSeries)
TYPE_STRING(DayAttr)

template <typename T>
void inspect_size_of(T t = T{}) {
#if PRINT_SIZEOF_RESULTS
    ECF_TEST_DBG(<< "   * sizeof(" << typeName<T>() << ") = " << sizeof(T));
#endif
    BOOST_REQUIRE_EQUAL(sizeof(T), sizeof(T));
}

BOOST_AUTO_TEST_SUITE(U_Attributes)

BOOST_AUTO_TEST_SUITE(T_SizeOf)

BOOST_AUTO_TEST_CASE(test_size_of) {
    ECF_NAME_THIS_TEST();

    inspect_size_of<std::ofstream>();
    inspect_size_of<std::string>();
    inspect_size_of<std::vector<int>>();
    inspect_size_of<std::vector<std::string>>();
    inspect_size_of<std::weak_ptr<int>>();
    inspect_size_of<std::nullptr_t>();
    inspect_size_of(nullptr);
    inspect_size_of<std::unique_ptr<int>>();
    inspect_size_of<double>();
    inspect_size_of<long>();
    inspect_size_of<int>();
    inspect_size_of<unsigned int>();
    inspect_size_of<bool>();
    inspect_size_of<ecf::TimeSeries>();
    inspect_size_of<DayAttr>();
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
