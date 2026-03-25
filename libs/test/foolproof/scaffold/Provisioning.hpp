/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_test_foolproof_scaffold_Provisioning_HPP
#define ecflow_test_foolproof_scaffold_Provisioning_HPP

#include <array>
#include <chrono>
#include <fstream>
#include <optional>
#include <random>
#include <regex>
#include <sstream>
#include <string>
#include <thread>
#include <variant>

#include <sys/param.h>

#include "Process.hpp"
#include "ecflow/client/ClientInvoker.hpp"
#include "ecflow/core/EcfPortLock.hpp"
#include "ecflow/core/Filesystem.hpp"
#include "ecflow/core/PasswordEncryption.hpp"
#include "ecflow/core/ecflow_source_build_dir.h"
#include "ecflow/core/ecflow_version.h"

namespace foolproof::scaffold {

/**
 * The following classes help to provision the test environment, by handling
 * the automatic creation and cleanup of test artifacts (e.g. files and
 * environment variables).
 */

inline std::string pretty_print_path(const fs::path& path) {
    auto canonical  = fs::canonical(path);
    std::string in  = canonical.string();
    std::string fst = std::regex_replace(in, std::regex(CMAKE_ECFLOW_BUILD_DIR()), "/~~~build~~~");
    std::string snd = std::regex_replace(fst, std::regex(CMAKE_ECFLOW_SOURCE_DIR()), "/~~~source~~~");
    return snd;
}

class Directory {
public:
    explicit Directory(fs::path location)
        : location_{std::move(location)} {
        ECF_TEST_DBG("Created working directory at: " << location_.string());
    }

    Directory(const Directory&)            = delete;
    Directory& operator=(const Directory&) = delete;
    Directory(Directory&&)                 = delete;
    Directory& operator=(Directory&&)      = delete;

    ~Directory() {
        fs::remove_all(location_);
        ECF_TEST_DBG("Removed working directory at: " << location_.string());
    }

    [[nodiscard]] const fs::path& path() const { return location_; }

private:
    fs::path location_;
};

class MakeDirectory {
public:
    struct Information
    {
    private:
        static std::string get_compiler() {
#if defined(_AIX)
            return "aix";
#elif defined(HPUX)
            return "hpux";
#elif defined(__clang__)
            return "clang";
#elif defined(__INTEL_COMPILER)
            return "intel";
#elif defined(_CRAYC)
            return "cray";
#elif defined(__GNUC__)
            return "gnu";
#else
            return "unknown";
#endif
        };

        static std::string get_architecture() {
#if defined(__x86_64__) || defined(_M_X64)
            return "x86_64";
#elif defined(__aarch64__) || defined(_M_ARM64)
            return "aarch64";
#else
            return "unknown";
#endif
        }

        static std::string get_build_type() {
            auto type = CMAKE_ECFLOW_BUILD_TYPE();
            std::transform(type.begin(), type.end(), type.begin(), [](unsigned char c) { return std::tolower(c); });
            return type;
        }

        static std::string get_test_name() {
            return std::string{boost::unit_test::framework::current_test_case().p_name};
        }

        static std::string get_pid() {
            auto pid = getpid();
            return std::to_string(pid);
        }

    public:
        static std::string generate_relative_path() {
            const std::string compiler     = get_compiler();
            const std::string architecture = get_architecture();
            const std::string build_type   = get_build_type();
            const std::string test_name    = get_test_name();
            const std::string pid          = get_pid();

            return "data/ECF_HOME__b_" + build_type + "__c_" + compiler + "__a_" + architecture + "__t_" + test_name +
                   "__p_" + pid + "__";
        }
    };

    MakeDirectory()
        : location_{make_working_directory()} {}

    Directory create() const {
        // ensure the directory exists
        fs::create_directories(location_);
        assert(fs::exists(location_));
        assert(fs::is_directory(location_));
        return Directory{location_};
    }

    MakeDirectory(const MakeDirectory&)            = delete;
    MakeDirectory& operator=(const MakeDirectory&) = delete;
    MakeDirectory(MakeDirectory&&)                 = delete;
    MakeDirectory& operator=(MakeDirectory&&)      = delete;

    ~MakeDirectory() = default;

private:
    static fs::path make_working_directory() {
        auto base_path = fs::current_path();
        return base_path / Information::generate_relative_path();
    }

    fs::path location_;
};

class EnvironmentVariable {
public:
    EnvironmentVariable(std::string name, std::string value)
        : name_{std::move(name)},
          value_{std::move(value)} {
        ECF_TEST_DBG("Setting environment variable: " << name_ << "=" << value_);
        setenv(name_.c_str(), value_.c_str(), true);
    }

    EnvironmentVariable(const EnvironmentVariable&)                = delete;
    EnvironmentVariable& operator=(const EnvironmentVariable&)     = delete;
    EnvironmentVariable(EnvironmentVariable&&) noexcept            = delete;
    EnvironmentVariable& operator=(EnvironmentVariable&&) noexcept = delete;

    [[nodiscard]] std::string name() const { return name_; }
    [[nodiscard]] std::string value() const { return value_; }

    ~EnvironmentVariable() {
        ECF_TEST_DBG("Unsetting environment variable: " << name_);
        unsetenv(name_.c_str());
    }

    static std::optional<std::string> get_environment_variable(const std::string& name) {
        ECF_TEST_DBG("Getting environment variable: " << name);
        char* value = getenv(name.c_str());
        if (value == nullptr) {
            return std::nullopt;
        }
        return std::string{value};
    }

private:
    std::string name_;
    std::string value_;
};

class MakeEnvironmentVariable {
public:
    struct InvalidVariableName : public std::runtime_error
    {
        explicit InvalidVariableName(std::string message)
            : std::runtime_error(message) {}
    };

    MakeEnvironmentVariable()
        : name_{},
          value_{} {}

    MakeEnvironmentVariable(const MakeEnvironmentVariable&)                = delete;
    MakeEnvironmentVariable& operator=(const MakeEnvironmentVariable&)     = delete;
    MakeEnvironmentVariable(MakeEnvironmentVariable&&) noexcept            = delete;
    MakeEnvironmentVariable& operator=(MakeEnvironmentVariable&&) noexcept = delete;

    ~MakeEnvironmentVariable() = default;

    MakeEnvironmentVariable& with(const std::string& name, const std::string& value) {
        name_  = name;
        value_ = value;
        return *this;
    }

    [[nodiscard]] EnvironmentVariable create() const {
        if (name_.empty()) {
            throw InvalidVariableName("Environment variable name cannot be empty");
        }
        return EnvironmentVariable{name_, value_};
    }

private:
    std::string name_;
    std::string value_;
};

class SpecificFileLocation {
public:
    explicit SpecificFileLocation(const std::string& name, const Directory& cwd)
        : SpecificFileLocation(name, cwd.path()) {}

    explicit SpecificFileLocation(const std::string& name, const fs::path& location)
        : path_{location / name} {}

    constexpr const fs::path& path() const { return path_; }

private:
    fs::path path_;
};

class AutomaticFileLocation {
public:
    explicit AutomaticFileLocation(const fs::path& location = fs::current_path())
        : path_{location / randomized_name_n(MAX_CHARS)} {}
    explicit AutomaticFileLocation(const std::string& name, const fs::path& location = fs::current_path())
        : path_{location / suffixed_name_n(name, MAX_CHARS)} {}

    constexpr const fs::path& path() const { return path_; }

private:
    fs::path path_;

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

class File {
public:
    explicit File(fs::path location)
        : location_{std::move(location)} {
        assert(fs::exists(location_));
        assert(fs::is_regular_file(location_));
        ECF_TEST_DBG("Created file: " << location_);
    }

    File(const File&)                = delete;
    File& operator=(const File&)     = delete;
    File(File&&) noexcept            = default;
    File& operator=(File&&) noexcept = default;

    ~File() {
        fs::remove(location_);
        ECF_TEST_DBG("Removed file: " << location_);
    }

    [[nodiscard]] const fs::path& path() const { return location_; }
    [[nodiscard]] fs::path filename() const { return location_.filename(); }

private:
    fs::path location_;
};

class MakeTestFile {
public:
    explicit MakeTestFile()
        : location_{},
          content_{} {}

    MakeTestFile(const MakeTestFile&)                = delete;
    MakeTestFile& operator=(const MakeTestFile&)     = delete;
    MakeTestFile(MakeTestFile&&) noexcept            = delete;
    MakeTestFile& operator=(MakeTestFile&&) noexcept = delete;

    ~MakeTestFile() = default;

    MakeTestFile& with(const SpecificFileLocation& location) {
        location_ = location.path();
        return *this;
    }

    MakeTestFile& with(const AutomaticFileLocation& location) {
        location_ = location.path();
        return *this;
    }

    MakeTestFile& with(const char* content) {
        content_ = content;
        return *this;
    }

    MakeTestFile& with(const std::string& content) {
        content_ = content;
        return *this;
    }

    template <typename Content>
    MakeTestFile& with(const Content& content) {
        content_ = content.data();
        return *this;
    }

    [[nodiscard]] File create() const {
        { // Caution: We assume that existing test files can be overwritten
            std::ofstream os(location_.string(), std::ios::out | std::ios::trunc);
            os << content_;
        }

        // Now that the file actually exists, we update the location to the canonical path
        auto actual_location = fs::canonical(location_);

        return File{actual_location};
    }

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
    LockFile(const fs::path& lock_file)
        : lock_file_(lock_file) {
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

    [[nodiscard]] const fs::path& path() const { return lock_file_; }

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

struct User
{
    std::string username;
    std::string password;
    std::string permission;

    bool can_read() const { return permission.find("r") != std::string::npos; }
    bool can_write() const { return permission.find("w") != std::string::npos; }
};

struct Host
{
    using host_t = std::string;

    Host() = default;
    Host(host_t host)
        : host_{host} {
        if (host_.empty()) {
            throw std::runtime_error("host name cannot be empty\n");
        }

        if (host_ == default_host) {
            host_ = resolve_hostname();
        }
    }

    Host(const Host&)                = default;
    Host& operator=(const Host&)     = default;
    Host(Host&&) noexcept            = default;
    Host& operator=(Host&&) noexcept = default;

    ~Host() = default;

    [[nodiscard]] bool is_valid() const { return !host_.empty(); }

    [[nodiscard]] const host_t& value() const { return host_; }

    static inline const host_t default_host = "localhost";

    friend bool operator==(const Host& lhs, const Host& rhs) { return lhs.host_ == rhs.host_; }
    friend bool operator!=(const Host& lhs, const Host& rhs) { return !(lhs == rhs); }

    friend std::ostream& operator<<(std::ostream& os, const Host& host) { return os << host.host_; }

    static std::string resolve_hostname() {
        std::array<char, 255> buffer;
        if (gethostname(buffer.data(), buffer.size()) != -1) {
            return buffer.data();
        }
        else {
            throw std::runtime_error("could not get host name\n");
        }
    }

private:
    host_t host_{default_host};
};

struct Port
{
    using port_t     = uint32_t;
    using location_t = fs::path;

    Port(port_t port, LockFile&& lock)
        : port_{port},
          lock_{std::move(lock)} {}

    Port(const Port&)                = delete;
    Port& operator=(const Port&)     = delete;
    Port(Port&&) noexcept            = delete;
    Port& operator=(Port&&) noexcept = delete;

    ~Port() { ECF_TEST_DBG("Port " << port_ << " is unlocked, lock file removed from " << lock_.path()); }

    [[nodiscard]] port_t value() const { return port_; }

    [[nodiscard]] location_t lock_location() const { return lock_.path(); }

    static constexpr port_t default_port = 3141;
    static constexpr port_t maximum_port = 65535;

    friend bool operator==(const Port& lhs, const Port& rhs) { return lhs.port_ == rhs.port_; }
    friend bool operator!=(const Port& lhs, const Port& rhs) { return !(lhs == rhs); }

    friend std::ostream& operator<<(std::ostream& os, const Port& port) { return os << port.port_; }

private:
    port_t port_;
    LockFile lock_;
};

struct PasswordsFile
{
    template <typename... Entry>
    PasswordsFile(const Host& host, const Port& port, const Entry&... entries)
        : content_{} {
        content_ += ECFLOW_VERSION;
        content_ += "\n";
        auto print = [this, host, &port](auto& user) {
            content_ += user.username;
            content_ += " ";
            content_ += host.value();
            content_ += " ";
            content_ += std::to_string(port.value());
            content_ += " ";
            content_ += user.password;
            content_ += "\n";
        };
        (print(entries), ...);
    }

    const std::string& data() const { return content_; }

private:
    std::string content_;
};

struct WhitelistFile
{
    template <typename... Entry>
    WhitelistFile(Entry... entries)
        : content_{} {
        content_ += ECFLOW_VERSION;
        content_ += "\n";
        auto print = [this](auto& user) {
            content_ += user.can_write() ? "" : "-";
            content_ += user.username;
            content_ += "\n";
        };
        (print(entries), ...);
    }

    const std::string& data() const { return content_; }

private:
    std::string content_;
};

struct ServerEnvironmentFile
{
    template <typename... Entry>
    ServerEnvironmentFile(Entry... entries)
        : content_{} {
        content_ += "\n";
        auto print = [this](auto& entry) {
            auto [variable, value] = entry;
            content_ += variable;
            content_ += "=";
            content_ += value;
            content_ += "\n";
        };
        (print(entries), ...);
    }

    const std::string& data() const { return content_; }

private:
    std::string content_;
};

class MakeHost {
public:
    MakeHost() = default;

    MakeHost(const MakeHost&)                = delete;
    MakeHost& operator=(const MakeHost&)     = delete;
    MakeHost(MakeHost&&) noexcept            = delete;
    MakeHost& operator=(MakeHost&&) noexcept = delete;

    ~MakeHost() = default;

    MakeHost& with(const Host::host_t name) {
        host_ = name;
        return *this;
    }

    [[nodiscard]] Host create() const { return Host{host_}; }

private:
    Host::host_t host_{Host::default_host};
};

struct SpecificPortValue
{
    using port_t = Port::port_t;

    explicit SpecificPortValue(port_t port = Port::default_port)
        : base_port{port} {}

    port_t base_port;

    static std::optional<std::pair<port_t, LockFile>> attempt_to_lock_port(const fs::path lock_dir, port_t port) {
        // define name of 'lock' file
        auto lock_name     = std::to_string(port) + ".lock";
        fs::path lock_file = lock_dir / lock_name;

        // attempt to create 'lock' file
        if (auto lock = LockFile::make_lock(lock_file); lock.has_value()) {
            return std::make_pair(port, std::move(lock.value()));
        }

        return std::nullopt;
    }
};

struct AutomaticPortValue
{
    using port_t = Port::port_t;

    explicit AutomaticPortValue(port_t port = Port::default_port)
        : base_port{port} {}

    port_t base_port;

    static std::optional<std::pair<port_t, LockFile>> attempt_to_lock_port(const fs::path lock_dir, port_t port) {

        for (port_t current = port; current <= Port::maximum_port; ++current) {
            if (auto found = SpecificPortValue::attempt_to_lock_port(lock_dir, current); found.has_value()) {
                return found;
            }
        }

        return std::nullopt;
    }
};

class MakePort {
public:
    class UnableToLockPort : public std::runtime_error {
    public:
        explicit UnableToLockPort(std::string msg)
            : std::runtime_error(std::move(msg)) {}
    };

    class NoLockCurrentlyAvailable : public std::runtime_error {
    public:
        explicit NoLockCurrentlyAvailable(std::string msg)
            : std::runtime_error(std::move(msg)) {}
    };

    using location_t = fs::path;

    MakePort()
        : strategy_{SpecificPortValue{}} {}

    MakePort(const MakePort&)                = delete;
    MakePort& operator=(const MakePort&)     = delete;
    MakePort(MakePort&&) noexcept            = delete;
    MakePort& operator=(MakePort&&) noexcept = delete;

    ~MakePort() = default;

    template <typename Strategy>
    MakePort& with(Strategy strategy) {
        strategy_ = strategy;
        return *this;
    }

    [[nodiscard]] Port create() const {
        // define location to store 'lock' files
        // 1) by default, use project build directory
        // 2) overriden by ECF_PORT_LOCK_DIR environment variable
        fs::path lock_dir = CMAKE_ECFLOW_SOURCE_DIR();
        if (const char* env = std::getenv("ECF_PORT_LOCK_DIR")) {
            lock_dir = env;
        };

        return std::visit(
            [&lock_dir](auto&& strategy) {
                // attemp to lock port (i.e. create the lock file)
                using Strategy = std::decay_t<decltype(strategy)>;
                if (auto found = Strategy::attempt_to_lock_port(lock_dir, strategy.base_port); found.has_value()) {
                    auto port_ = found.value().first;
                    auto lock_ = std::move(found.value().second);
                    ECF_TEST_DBG("Port " << port_ << " is locked, lock file created at " << lock_.path());
                    return Port{port_, std::move(lock_)};
                }
                else {
                    throw UnableToLockPort("Failed to find an available port");
                };
            },
            strategy_);
    }

    std::variant<SpecificPortValue, AutomaticPortValue> strategy_;
};

template <typename V, typename E = std::string>
class Outcome {

    struct Error
    {
        E reason_;
    };

public:
    using value_t = V;
    using error_t = E;

    Outcome() = delete;

    Outcome(const Outcome&)            = default;
    Outcome& operator=(const Outcome&) = default;
    Outcome(Outcome&&)                 = default;
    Outcome& operator=(Outcome&&)      = default;

private:
    explicit Outcome(const V& value)
        : success_{true},
          data_{value} {}
    explicit Outcome(V&& value)
        : success_{true},
          data_{std::move(value)} {}
    explicit Outcome(const Error& error)
        : success_{false},
          data_{error} {}

public:
    ~Outcome() = default;

    static Outcome success(const V& value) { return Outcome(value); }
    static Outcome success(V&& value) { return Outcome(std::move(value)); }
    static Outcome failure(const E& error) { return Outcome{Error{error}}; }

    [[nodiscard]] bool ok() const { return success_; }
    [[nodiscard]] value_t get() { return std::move(std::get<value_t>(data_)); }
    [[nodiscard]] const value_t& value() const { return std::get<value_t>(data_); }
    [[nodiscard]] const error_t& reason() const { return std::get<Error>(data_).reason_; }

private:
    bool success_;
    std::variant<V, Error> data_;
};

struct Client
{
    int exit_code;
    std::string stdout_buffer;
    std::string stderr_buffer;

    bool stdout_contains(const std::string& expected) const {
        return stdout_buffer.find(expected) != std::string::npos;
    }
};

class RunClient {
public:
    using host_t = Host;
    using port_t = Port;

    struct CommandPing
    {
        std::vector<std::string> options() const { return {"--ping", "-d"}; }

        void process_output(const std::string& output) const {}
    };

    struct CommandGet
    {
        std::vector<std::string> options() const { return {"--get"}; }
    };

    struct CommandGetState
    {
        std::vector<std::string> options() const { return {"--get_state"}; }
    };

    struct CommandLoad
    {
        explicit CommandLoad(fs::path defs)
            : defs_{defs} {}

        std::vector<std::string> options() const { return {"--load", defs_.string(), "-d"}; }

        fs::path defs_;
    };

    struct CommandReplace
    {
        explicit CommandReplace(fs::path defs, std::string node)
            : defs_{defs},
              node_{std::move(node)} {}

        std::vector<std::string> options() const { return {"--replace", node_, defs_.string()}; }

        fs::path defs_;
        std::string node_;
    };

    struct CommandDelete
    {
        explicit CommandDelete(std::string path)
            : path_{path} {}

        std::vector<std::string> options() const { return {"--delete", "force", "yes", path_}; }

        std::string path_;
    };

    struct CommandUpdateLabel
    {
        explicit CommandUpdateLabel(std::string path, std::string label, std::string value)
            : path_{path},
              label_{label},
              value_{value} {}

        std::vector<std::string> options() const { return {"--alter", "change", "label", label_, value_, path_}; }

        std::string path_;
        std::string label_;
        std::string value_;
    };

    struct CommandReloadWhitelist
    {
        explicit CommandReloadWhitelist() {}

        std::vector<std::string> options() const { return {"--reloadwsfile"}; }
    };

    explicit RunClient()
        : host_{nullptr},
          port_{nullptr},
          user_{nullptr},
          cwd_{nullptr} {};

    RunClient(const RunClient&)            = delete;
    RunClient& operator=(const RunClient&) = delete;
    RunClient(RunClient&&)                 = delete;
    RunClient& operator=(RunClient&&)      = delete;

    ~RunClient() = default;

    RunClient& with(const Host& host) {
        host_ = &host;
        return *this;
    }

    RunClient& with(const Port& port) {
        port_ = &port;
        return *this;
    }

    RunClient& with(const User& user) {
        user_ = &user;
        return *this;
    }

    RunClient& with(const Directory& cwd) {
        cwd_ = &cwd;
        return *this;
    }

    template <typename Command>
    Outcome<Client> execute(const Command& command) {
        return launch_ecflow_client(host_, port_, user_, cwd_, command);
    }

private:
    static fs::path find_ecflow_client_path() { return fs::path{CMAKE_ECFLOW_BUILD_DIR()} / "bin" / "ecflow_client"; }

    template <typename Command>
    static Outcome<Client> launch_ecflow_client(const Host* host,
                                                const Port* port,
                                                const User* user,
                                                const Directory* cwd,
                                                const Command& command) {
        BOOST_REQUIRE_MESSAGE(host != nullptr, "The server host is non-null");
        BOOST_REQUIRE_MESSAGE(port != nullptr, "The server port is non-null");
        BOOST_REQUIRE_MESSAGE(cwd != nullptr, "The working directory is non-null");

        auto client_path = find_ecflow_client_path();

        BOOST_REQUIRE_MESSAGE(!client_path.empty(), "The ecflow client path is non-empty");
        BOOST_REQUIRE_MESSAGE(fs::exists(client_path), "The ecflow client executable exist at " << client_path);

        auto options = std::vector<std::string>{"--host", host->value(), "--port", std::to_string(port->value())};
        if (user != nullptr) {
            options.push_back("--user");
            options.push_back(user->username);
            options.push_back("--password");
            options.push_back(user->password);
        }
        for (auto option : command.options()) {
            options.push_back(option);
        }

        auto ecflow_client = Process(client_path, options, cwd->path());

        auto print = [](const auto& executable, const auto& options) {
            std::string buffer = "[ " + pretty_print_path(executable) + (options.empty() ? "" : ", ");
            for (size_t i = 0; i < options.size(); ++i) {
                buffer += options[i];
                if (i != options.size() - 1) {
                    buffer += ", ";
                }
            }
            return buffer + " ]";
        };

        if (auto r = ecflow_client.wait(); r == 0) {
            ECF_TEST_DBG("Executed " << print(client_path, options));
            ECF_TEST_DBG("         result: [OK]");
            ECF_TEST_DBG("         pid: " << ecflow_client.pid());
            ECF_TEST_DBG("         cwd: " << cwd->path().string());
            auto [stdout_buffer, stderr_buffer] = dump_client_execution_report(*cwd, ecflow_client);
            return Outcome<Client>::success(Client{r, stdout_buffer, stderr_buffer});
        }
        else {
            ECF_TEST_DBG("Executed " << print(client_path, options));
            ECF_TEST_DBG("         result: [FAIL]");
            ECF_TEST_DBG("         pid: " << ecflow_client.pid());
            ECF_TEST_DBG("         cwd: " << cwd->path().string());
            auto [stdout_buffer, stderr_buffer] = dump_client_execution_report(*cwd, ecflow_client);
            return Outcome<Client>::failure("ecflow_client failed, return code: " + std::to_string(r) +
                                            ", stdout: " + stdout_buffer + ", stderr: " + stderr_buffer);
        }
    }

    static std::tuple<std::string, std::string> dump_client_execution_report(const Directory& cwd,
                                                                             const Process& server) {
        auto out = server.read_stdout();
        auto err = server.read_stderr();

        std::string report_stdout_file =
            std::string{"ecflow_client__execution_report."} + std::to_string(server.pid()) + ".stdout.txt";
        std::string report_stderr_file =
            std::string{"ecflow_client__execution_report."} + std::to_string(server.pid()) + ".stderr.txt";
        fs::path report_stdout_path = cwd.path() / report_stdout_file;
        fs::path report_stderr_path = cwd.path() / report_stderr_file;

        {
            std::ofstream ofs(report_stdout_path.c_str());
            ofs << out;
        }
        {
            std::ofstream ofs(report_stderr_path.c_str());
            ofs << err;
        }

        ECF_TEST_DBG("         stdout: " << pretty_print_path(report_stdout_path));
        ECF_TEST_DBG("         stderr: " << pretty_print_path(report_stderr_path));

        return std::make_tuple(out, err);
    }

    const Host* host_;
    const Port* port_;
    const User* user_;
    const Directory* cwd_;
};

class Server {
public:
    using streams_t = std::tuple<std::string, std::string>;

    struct UnableToShutdownServer : public std::runtime_error
    {
        explicit UnableToShutdownServer(std::string msg)
            : std::runtime_error(std::move(msg)) {}
    };

    explicit Server(const Host& host, const Port& port, const Directory& cwd, Process&& process)
        : host_{host},
          port_{port},
          cwd_{cwd},
          process_{std::move(process)} {}

    Server(const Server&)            = delete;
    Server& operator=(const Server&) = delete;
    Server(Server&& other) noexcept
        : host_{other.host_},
          port_{other.port_},
          cwd_{other.cwd_},
          process_{std::move(other.process_)} {}
    Server& operator=(Server&& other) = delete;

    ~Server() {
        if (process_.is_running()) {
            shutdown_ecflow_server();
        }
    }

    streams_t shutdown() {
        if (process_.is_running()) {
            auto [out, err] = shutdown_ecflow_server();
            return {out, err};
        }
        return {"", ""};
    }

    pid_t pid() const { return process_.pid(); }
    const Host& host() const { return host_; }
    const Port& port() const { return port_; }
    const Directory& cwd() const { return cwd_; }

private:
    streams_t shutdown_ecflow_server() {
        if (auto o = process_.terminate(); o == 0) {
            ECF_TEST_DBG("Shutdown server: [OK]");
            return dump_server_execution_report(cwd_, process_);
        }
        else {
            ECF_TEST_DBG("Shutdown server: [FAIL]");
            return dump_server_execution_report(cwd_, process_);
        }

        if (auto o = ensure_ecflow_server_is_shutdown(host_, port_, cwd_); !o.ok()) {
            throw UnableToShutdownServer(o.reason());
        }
    }

    static Outcome<std::string>
    ensure_ecflow_server_is_shutdown(const Host& host,
                                     const Port& port,
                                     const Directory& cwd,
                                     std::chrono::seconds timeout = std::chrono::seconds(10)) {
        auto start = std::chrono::system_clock::now();
        for (;;) {
            auto r = RunClient{}.with(host).with(port).with(cwd).execute(RunClient::CommandPing{});
            if (!r.ok()) {
                // Important: this checks that the client finished with FAILURE!
                // As this means the ping failed, and thus the server is no longer responding!
                break;
            }

            auto now = std::chrono::system_clock::now();
            if (now - start > timeout) {
                ECF_TEST_DBG("ecflow server is ***NOT*** shutdown");
                return Outcome<std::string>::failure("Timed out waiting for ecflow server to shutdown");
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(250));
        }
        return Outcome<std::string>::success("ecflow server is shutdown");
    }

    static streams_t dump_server_execution_report(const Directory& cwd, const Process& server) {
        std::string report_file =
            std::string{"ecflow_server__execution_report."} + std::to_string(server.pid()) + ".txt";
        fs::path report_path = cwd.path() / report_file;
        std::ofstream ofs(report_path.c_str());
        auto out = server.read_stdout();
        ofs << "Report STDOUT \n <<< STDOUT BEGIN >>>\n" << out << " <<< STDOUT END >>>\n";
        auto err = server.read_stderr();
        ofs << "Report STDERR \n <<< STDERR BEGIN >>>\n" << err << " <<< STDERR END >>>\n";

        ECF_TEST_DBG("Server execution report written to " << pretty_print_path(report_path));

        return std::make_tuple(out, err);
    }

    const Host& host_;
    const Port& port_;
    const Directory& cwd_;
    Process process_;
};

class MakeServer {
public:
    explicit MakeServer()
        : host_{nullptr},
          port_{nullptr},
          cwd_{nullptr} {};

    MakeServer(const MakeServer&)            = delete;
    MakeServer& operator=(const MakeServer&) = delete;
    MakeServer(MakeServer&&)                 = delete;
    MakeServer& operator=(MakeServer&&)      = delete;

    MakeServer& with(const Host& host) {
        host_ = &host;
        return *this;
    }

    MakeServer& with(const Port& port) {
        port_ = &port;
        return *this;
    }

    MakeServer& with(const Directory& cwd) {
        cwd_ = &cwd;
        return *this;
    }

    Outcome<Server> launch() {
        BOOST_REQUIRE_MESSAGE(host_ != nullptr, "The server host is non-null");
        BOOST_REQUIRE_MESSAGE(port_ != nullptr, "The server port is non-null");
        BOOST_REQUIRE_MESSAGE(cwd_ != nullptr, "The server working directory is non-null");

        if (auto o = launch_ecflow_server(*host_, *port_, *cwd_); o.ok()) {
            return Outcome<Server>::success(Server{*host_, *port_, *cwd_, o.get()});
        }
        else {
            return Outcome<Server>::failure(o.reason());
        }
    }

    ~MakeServer() = default;

private:
    static fs::path find_ecflow_server_path() { return fs::path{CMAKE_ECFLOW_BUILD_DIR()} / "bin" / "ecflow_server"; }

    static Outcome<Process> launch_ecflow_server(const Host& host, const Port& port, const Directory& cwd) {
        auto server_path = find_ecflow_server_path();

        BOOST_REQUIRE_MESSAGE(!server_path.empty(), "The ecflow server path could not be found");
        BOOST_REQUIRE_MESSAGE(fs::exists(server_path),
                              "The ecflow server executable does not exist at " << server_path);

        auto options = std::vector<std::string>{"--port", std::to_string(port.value()), "-d"};

        auto ecflow_server = Process(server_path, options, cwd.path());

        auto print = [](const auto& executable, const auto& options) {
            std::string buffer = "[ " + pretty_print_path(executable) + (options.empty() ? "" : ", ");
            for (size_t i = 0; i < options.size(); ++i) {
                buffer += options[i];
                if (i != options.size() - 1) {
                    buffer += ", ";
                }
            }
            return buffer + " ]";
        };

        ECF_TEST_DBG("Launched " << print(server_path, options) << "; now waiting for it to be ready...");

        if (auto found = ensure_ecflow_server_is_running(host, port, cwd); found.ok()) {
            ECF_TEST_DBG("Executed " << print(server_path, options));
            ECF_TEST_DBG("         result: [OK]");
            ECF_TEST_DBG("         pid: " << ecflow_server.pid());
            ECF_TEST_DBG("         cwd: " << cwd.path().string());
            return Outcome<Process>::success(std::move(ecflow_server));
        }
        else {
            ECF_TEST_DBG("Executed " << print(server_path, options));
            ECF_TEST_DBG("         result: [FAIL], due to: " << found.reason());
            ECF_TEST_DBG("         pid: " << ecflow_server.pid());
            ECF_TEST_DBG("         cwd: " << cwd.path().string());
            return Outcome<Process>::failure(found.reason());
        }
    }

    static Outcome<std::string>
    ensure_ecflow_server_is_running(const Host& host,
                                    const Port& port,
                                    const Directory& cwd,
                                    std::chrono::seconds timeout = std::chrono::seconds(10)) {
        auto start = std::chrono::system_clock::now();
        for (;;) {
            auto r = RunClient{}.with(host).with(port).with(cwd).execute(RunClient::CommandPing{});
            if (r.ok()) {
                // Important: this checks that the client finished with SUCCESS!
                // As this means the ping succeeded, and thus the server is now responding!
                break;
            }

            auto now = std::chrono::system_clock::now();
            if (now - start > timeout) {
                return Outcome<std::string>::failure("Timed out waiting for ecflow server to start");
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(250));
        }
        return Outcome<std::string>::success("ecflow server is running");
    }

private:
    const Host* host_;
    const Port* port_;
    const Directory* cwd_;
};

} // namespace foolproof::scaffold

#endif /* ecflow_test_foolproof_scaffold_Provisioning_HPP */
