/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef test_ecflow_service_aviso_TestContentProvider_HPP
#define test_ecflow_service_aviso_TestContentProvider_HPP

#include <string>

namespace ecf::test {

class TestContentProvider {
public:
    TestContentProvider()                                       = delete;
    TestContentProvider(const TestContentProvider&)             = delete;
    TestContentProvider(TestContentProvider&&)                  = delete;
    TestContentProvider& operator=(const TestContentProvider&)  = delete;
    TestContentProvider& operator=(const TestContentProvider&&) = delete;

    explicit TestContentProvider(const std::string& file_name_prefix);
    explicit TestContentProvider(const std::string& file_name_prefix, const std::string& content);

    ~TestContentProvider();

    [[nodiscard]] const std::string& file() const { return file_; }

private:
    std::string file_;
};

} // namespace ecf::test

#endif /* test_ecflow_service_aviso_TestContentProvider_HPP */
