/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_core_resources_Machine_HPP
#define ecflow_core_resources_Machine_HPP

#include <cstdint>
#include <memory>
#include <optional>
#include <stdexcept>
#include <variant>
#include <vector>

#include "ecflow/core/Serialization.hpp"

namespace ecf::resources {

///
/// @brief ResourceUnavailable indicates that an attempt to measure a specific resource has failed.
///
struct ResourceUnavailable : public std::runtime_error
{
    using std::runtime_error::runtime_error;
};

///
/// @brief UnsupportedPlatform indicates an attempt to measure resources on an unsupported platform
///
struct UnsupportedPlatform : public std::runtime_error
{
    using std::runtime_error::runtime_error;
};

struct NamedValue
{
    using name_t  = std::string;
    using value_t = std::variant<int32_t, uint32_t, int64_t, uint64_t, double>;
    using unit_t  = std::string;

    name_t n_;
    value_t v_;
    unit_t u_;

    NamedValue() = default;

    template <typename T>
    NamedValue(std::string n, T v)
        : n_{std::move(n)},
          v_{v},
          u_{} {}

    template <typename T>
    NamedValue(std::string n, T v, std::string u)
        : n_{std::move(n)},
          v_{v},
          u_{std::move(u)} {}

    const auto& name() const { return n_; }
    const auto& unit() const { return u_; }

    template <typename T>
    bool operator==(T v) const {
        return std::holds_alternative<T>(v_) && std::get<T>(v_) == v;
    }

    template <typename T>
    bool operator<(T v) const {
        return std::holds_alternative<T>(v_) && std::get<T>(v_) < v;
    }

    template <typename T>
    bool operator!=(T v) const {
        return !(*this == v);
    }

    template <typename T>
    bool operator>(T v) const {
        return !(*this < v) && !(*this == v);
    }

    template <typename T>
    bool operator>=(T v) const {
        return !(*this < v);
    }

    template <typename T>
    bool operator<=(T v) const {
        return (*this < v) || (*this == v);
    }

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar, std::uint32_t const version) {
        ar & n_;
        ar & v_;
        ar & u_;
    }

    friend std::ostream& operator<<(std::ostream& o, const NamedValue& nv) {
        std::visit([&o](auto&& arg) { o << arg; }, nv.v_);
        return o;
    }
};

///
/// @brief ProcessMeter is a snapshot of the resources available for/used by the process at a given time.
///
struct ProcessMeter
{
    using pid_t       = int32_t;
    using memory_t    = uint64_t;
    using page_size_t = uint64_t;
    using n_cpu_t     = int32_t;
    using n_threads_t = int32_t;

    std::vector<NamedValue> values_;

    static ProcessMeter make() { return {}; }

    ProcessMeter() = default;

    ProcessMeter(const ProcessMeter&)            = default;
    ProcessMeter& operator=(const ProcessMeter&) = default;

    ~ProcessMeter() = default;

    ProcessMeter& with_pid(pid_t value) {
        this->values_.emplace_back("pid", value);
        return *this;
    }

    ProcessMeter& with_maximum_memory(memory_t value) {
        this->values_.emplace_back("maximum_memory_available", value, "MB");
        return *this;
    }

    ProcessMeter& with_virtual_memory(memory_t value) {
        this->values_.emplace_back("virtual_memory_used", value, "MB");
        return *this;
    }

    ProcessMeter& with_resident_memory(memory_t value) {
        this->values_.emplace_back("resident_memory_used", value, "MB");
        return *this;
    }

    ProcessMeter& with_page_size(page_size_t value) {
        this->values_.emplace_back("page_size", value, "kB");
        return *this;
    }

    ProcessMeter& with_n_cpu_online(n_cpu_t value) {
        this->values_.emplace_back("n_cpu_online", value);
        return *this;
    }

    ProcessMeter& with_n_cpu_maximum(n_cpu_t value) {
        this->values_.emplace_back("n_cpu_maximum", value);
        return *this;
    }

    ProcessMeter& with_n_threads(n_threads_t value) {
        this->values_.emplace_back("n_threads", value);
        return *this;
    }

    ProcessMeter& with_arena_memory(memory_t value) {
        this->values_.emplace_back("arena_memory", value, "kB");
        return *this;
    }

    ProcessMeter& with_tracked_memory(memory_t value) {
        this->values_.emplace_back("tracked_memory", value, "kB");
        return *this;
    }

    ProcessMeter& with_freed_memory(memory_t value) {
        this->values_.emplace_back("freed_memory", value, "kB");
        return *this;
    }

    std::optional<NamedValue> get(const std::string& name) const {
        for (const auto& value : values_) {
            if (value.name() == name) {
                return value;
            }
        }
        return std::nullopt;
    }

    friend class cereal::access;
    template <class Archive>
    void serialize(Archive& ar, std::uint32_t const version) {
        ar & values_;
    }
};

namespace detail {

class MachineBase {
public:
    virtual ~MachineBase() = default;

    virtual ProcessMeter get_process_meter() const = 0;
};

} // namespace detail

///
/// @brief A representation of the current machine's resources.
///
/// Note: This is heavily dependent on the underlying operating system.
///
class Machine {
public:
    using base_t     = detail::MachineBase;
    using base_ptr_t = std::unique_ptr<base_t>;

    ///
    /// @brief Creates a new Machine instance.
    ///
    /// The currently supported platforms are: macOS and Linux.
    ///
    /// @return The Machine instance, considering the current runtime platform.
    /// @throws UnsupportedPlatform if the current platform is not supported.
    ///
    static Machine make();

    Machine(const Machine&)            = delete;
    Machine(Machine&&)                 = delete;
    Machine& operator=(const Machine&) = delete;
    Machine& operator=(Machine&&)      = delete;

    ~Machine() = default;

    ///
    /// @brief Retrieves the process/machine resources measurements for the current process.
    ///
    /// @return The process resources measurements for the current process.
    /// @throws ResourceUnavailable if any of the machine or process resources cannot be determined.
    ///
    [[nodiscard]] ProcessMeter get_process_meter() const { return machine_->get_process_meter(); }

private:
    Machine(base_ptr_t&& base)
        : machine_(std::move(base)) {};

    std::unique_ptr<detail::MachineBase> machine_;
};

} // namespace ecf::resources

#endif /* ecflow_core_resources_Machine_HPP */
