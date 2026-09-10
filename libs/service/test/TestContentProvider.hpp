/*
 * SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef test_ecflow_service_aviso_TestContentProvider_HPP
#define test_ecflow_service_aviso_TestContentProvider_HPP

#include <string>

namespace ecf::test {

class TestContentProvider {
public:
    TestContentProvider() = delete;

    explicit TestContentProvider(const std::string& file_name_prefix);
    explicit TestContentProvider(const std::string& file_name_prefix, const std::string& content);

    TestContentProvider(const TestContentProvider&)             = delete;
    TestContentProvider& operator=(const TestContentProvider&)  = delete;
    TestContentProvider(TestContentProvider&&)                  = delete;
    TestContentProvider& operator=(const TestContentProvider&&) = delete;

    ~TestContentProvider();

    [[nodiscard]] const std::string& file() const { return file_; }

private:
    std::string file_;
};

} // namespace ecf::test

#endif /* test_ecflow_service_aviso_TestContentProvider_HPP */
