/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_test_scaffold_Provisioning_HPP
#define ecflow_test_scaffold_Provisioning_HPP

#include <fstream>
#include <string>

#include "ecflow/core/Filesystem.hpp"

/**
 * The following classes help to provision the test environment, by handling
 * the automatic creation and cleanup of test artifacts (e.g. files and
 * environment variables).
 */

class WithTestEnvironmentVariable {
public:
    WithTestEnvironmentVariable(std::string variable, std::string value) : variable_(variable) {
        setenv(variable_.c_str(), value.c_str(), 1);
    }
    WithTestEnvironmentVariable(const WithTestEnvironmentVariable&)                = default;
    WithTestEnvironmentVariable& operator=(const WithTestEnvironmentVariable&)     = default;
    WithTestEnvironmentVariable(WithTestEnvironmentVariable&&) noexcept            = default;
    WithTestEnvironmentVariable& operator=(WithTestEnvironmentVariable&&) noexcept = default;

    ~WithTestEnvironmentVariable() { unsetenv(variable_.c_str()); }

private:
    std::string variable_;
};

class WithTestFile {
public:
    explicit WithTestFile(fs::path location, const std::string& content = "This is a dummy test file.\n")
        : location_{location} {
        // Caution: We assume that existing test files can be overwritten

        {
            std::ofstream os(location_.string(), std::ios::out | std::ios::trunc);
            os << content;
        }

        // Now that the file actually exists, we update the location to the canonical path
        location_ = fs::canonical(location_);
    }
    WithTestFile(const WithTestFile&)                = default;
    WithTestFile& operator=(const WithTestFile&)     = default;
    WithTestFile(WithTestFile&&) noexcept            = default;
    WithTestFile& operator=(WithTestFile&&) noexcept = default;

    ~WithTestFile() { fs::remove(location_); }

private:
    fs::path location_;
};

#endif /* ecflow_test_scaffold_Provisioning_HPP */
