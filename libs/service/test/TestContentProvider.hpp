// SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
// SPDX-License-Identifier: Apache-2.0

#pragma once

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
