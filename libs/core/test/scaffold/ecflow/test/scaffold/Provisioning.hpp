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

#include <array>
#include <fstream>
#include <random>
#include <string>

#include <sys/param.h>

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

class NamedTestFile {
public:
    explicit NamedTestFile(const std::string& name) : name_{name} {}

    constexpr const fs::path& path() const { return name_; }

private:
    fs::path name_;
};

class AutomaticTestFile {
public:
    AutomaticTestFile() : random_name_{randomized_name_n(MAX_CHARS)} {}
    explicit AutomaticTestFile(const std::string& name) : random_name_{suffixed_name_n(name, MAX_CHARS)} {}

    constexpr const fs::path& path() const { return random_name_; }

private:
    fs::path random_name_;

    static constexpr size_t MIN_CHARS = 8;
    static constexpr size_t MAX_CHARS = 24;

    static std::string randomized_name_n(size_t n) {
        assert(n >= MIN_CHARS);
        return random_n(n);
    }

    static std::string suffixed_name_n(const std::string& name, size_t n) {
        assert(name.size() > 0);
        assert(n >= MIN_CHARS);
        return name + "." + random_n(n);
    }

    static std::string random_n(size_t n) {
        std::random_device device;
        std::mt19937 generator(device());
        std::uniform_int_distribution<int> distribution(0, charset.size() - 1);

        auto random = [&]() -> char { return charset[distribution(generator) % charset.size()]; };

        fs::path::string_type str(n, 0);
        std::generate_n(str.begin(), n, random);
        return str;
    }

    static constexpr std::array charset = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B',
                                           'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N',
                                           'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z'};
};

class WithTestFile {
public:
    template <typename TestName>
    explicit WithTestFile(const TestName& location, const std::string& content = "This is a dummy test file.\n")
        : location_{location.path()} {
        // Caution: We assume that existing test files can be overwritten

        {
            std::ofstream os(location_.string(), std::ios::out | std::ios::trunc);
            os << content;
        }

        // Now that the file actually exists, we update the location to the canonical path
        location_ = fs::canonical(location_);
    }

    explicit WithTestFile() : WithTestFile(AutomaticTestFile{}) {}

    WithTestFile(const WithTestFile&)                = default;
    WithTestFile& operator=(const WithTestFile&)     = default;
    WithTestFile(WithTestFile&&) noexcept            = default;
    WithTestFile& operator=(WithTestFile&&) noexcept = default;

    ~WithTestFile() { fs::remove(location_); }

    const fs::path& path() const { return location_; }

private:
    fs::path location_;
    std::string content_;
};

#endif /* ecflow_test_scaffold_Provisioning_HPP */
