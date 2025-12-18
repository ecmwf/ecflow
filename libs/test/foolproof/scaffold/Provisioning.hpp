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
#include <optional>
#include <random>
#include <string>

#include <boost/beast/http/verb.hpp>
#include <sys/param.h>

#include "ecflow/core/EcfPortLock.hpp"
#include "ecflow/core/Filesystem.hpp"
#include "ecflow/core/ecflow_source_build_dir.h"

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

class LockFile {
public:
    static std::optional<LockFile> make_lock(const fs::path& lock_file) {
        if (create_file(lock_file)) {
            return LockFile{lock_file};
        }
        return std::nullopt;
    };

private:
    LockFile(const fs::path& lock_file) : lock_file_(lock_file) {
        assert(!lock_file_.empty());
        assert(fs::exists(lock_file_));
        assert(fs::is_regular_file(lock_file_));
    }

public:
    LockFile(const LockFile&)                = delete;
    LockFile& operator=(const LockFile&)     = delete;
    LockFile(LockFile&&) noexcept            = default;
    LockFile& operator=(LockFile&&) noexcept = default;

    ~LockFile() {
        if (!lock_file_.empty()) {
            fs::remove(lock_file_);
        }
    }

    const fs::path& path() const { return lock_file_; }

private:
    static bool create_file(const fs::path& file) {
        if (auto lock = fopen(file.c_str(), "wx")) {
            auto content = std::string("This is a lock file!"); // This is dummy content!
            fwrite(content.c_str(), 1, content.size(), lock);
            fclose(lock);
            return true;
        }
        return false;
    }

    fs::path lock_file_{};
};

class WithPort {
public:
    class UnableToLockPort : public std::runtime_error {
    public:
        explicit UnableToLockPort(std::string msg) : std::runtime_error(std::move(msg)) {}
    };

    class NoLockCurrentlyAvailable : public std::runtime_error {
    public:
        explicit NoLockCurrentlyAvailable(std::string msg) : std::runtime_error(std::move(msg)) {}
    };

    using port_t     = uint32_t;
    using location_t = fs::path;

    template <typename Strategy>
    WithPort(Strategy strategy) {
        // define location to store 'lock' files
        // 1) by default, use project build directory
        // 2) overriden by ECF_PORT_LOCK_DIR environment variable
        fs::path lock_dir = CMAKE_ECFLOW_SOURCE_DIR;
        if (const char* env = std::getenv("ECF_PORT_LOCK_DIR")) {
            lock_dir = env;
        };

        // attemp to lock port (i.e. create the lock file)
        if (auto found = Strategy::attempt_to_lock_port(lock_dir, strategy.base_port); found.has_value()) {
            port_      = found.value().first;
            lock_file_ = std::move(found.value().second);
            // std::cout << "Locked port: " << port_ << ", with file at " << lock_file_.value().path() << std::endl;
        }
        else {
            throw UnableToLockPort("Failed to find an available port");
        }
    }

    WithPort(const WithPort&)                = delete;
    WithPort& operator=(const WithPort&)     = delete;
    WithPort(WithPort&&) noexcept            = default;
    WithPort& operator=(WithPort&&) noexcept = default;

    ~WithPort() = default;

    port_t value() const { return port_; }

    location_t lock_location() const {
        if (lock_file_.has_value()) {
            return lock_file_.value().path();
        }
        throw NoLockCurrentlyAvailable{"No lock currently available"};
    }

    static constexpr port_t default_port = 3141;
    static constexpr port_t maximum_port = 65535;

private:
    port_t port_{default_port};
    std::optional<LockFile> lock_file_{std::nullopt};
};

struct SpecificPortValue
{
    using port_t = WithPort::port_t;

    explicit SpecificPortValue(port_t port = WithPort::default_port) : base_port{port} {}

    port_t base_port;

    static std::optional<std::pair<port_t, LockFile>> attempt_to_lock_port(const fs::path lock_dir, port_t port) {
        // define name of 'lock' file
        auto lock_name     = std::to_string(port) + ".lock";
        fs::path lock_file = lock_dir / lock_name;

        // attempt to create 'lock' file
        if (auto lock = LockFile::make_lock(lock_file); lock.has_value()) {
            return std::make_pair(port, std::move(lock.value()));
        }
        // auto lock = fopen(lock_file.c_str(), "wx");
        // if (lock) {
        //     auto content = std::string("this is a locked port: ") + std::to_string(port);
        //     fwrite(content.c_str(), 1, content.size(), lock);
        //     fclose(lock);
        //
        //     return std::make_pair(port, lock_file);
        // }

        return std::nullopt;
    }
};

struct AutomaticPortValue
{
    using port_t = WithPort::port_t;

    explicit AutomaticPortValue(port_t port = WithPort::default_port) : base_port{port} {}

    port_t base_port;

    static std::optional<std::pair<port_t, LockFile>> attempt_to_lock_port(const fs::path lock_dir, port_t port) {

        for (port_t current = port; current <= WithPort::maximum_port; ++current) {
            if (auto found = SpecificPortValue::attempt_to_lock_port(lock_dir, current); found.has_value()) {
                return found;
            }
        }

        return std::nullopt;
    }
};

#endif /* ecflow_test_scaffold_Provisioning_HPP */
