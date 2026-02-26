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
#include <stdexcept>

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

///
/// @brief ProcessMeter is a snapshot of the resources available for/used by the process at a given time.
///
struct ProcessMeter
{
    using pid_t             = int32_t;
    using maximum_memory_t  = uint64_t;
    using virtual_memory_t  = uint64_t;
    using resident_memory_t = uint64_t;
    using page_size_t       = uint64_t;
    using n_cpu_online_t    = int32_t;
    using n_cpu_maximum_t   = int32_t;
    using n_threads_t       = int32_t;

    ///
    /// @brief pid is the process ID of the current process.
    ///
    pid_t pid;

    ///
    /// @brief The maximum amount of memory available (in KB) on the machine
    ///
    maximum_memory_t maximum_memory;

    ///
    /// @brief The amount of virtual memory (in KB) currently used by the process.
    ///
    virtual_memory_t virtual_memory;

    ///
    /// @brief The amount of resident memory (in KB) currently used by the process.
    ///
    resident_memory_t resident_memory;

    ///
    /// @brief The size of a memory page (in bytes) on the machine.
    ///
    page_size_t page_size;

    ///
    /// @brief The number of CPUs currently available on the machine.
    ///
    n_cpu_online_t n_cpu_online;

    ///
    /// @brief The maximum number of CPUs that can be available on the machine.
    ///
    n_cpu_maximum_t n_cpu_maximum;

    ///
    /// @brief The number of threads currently used by the process.
    ///
    n_threads_t n_threads;

    static ProcessMeter make() { return {}; }

    ProcessMeter& with_pid(pid_t value) {
        this->pid = value;
        return *this;
    }

    ProcessMeter& with_maximum_memory(maximum_memory_t value) {
        this->maximum_memory = value;
        return *this;
    }

    ProcessMeter& with_virtual_memory(virtual_memory_t value) {
        this->virtual_memory = value;
        return *this;
    }

    ProcessMeter& with_resident_memory(resident_memory_t value) {
        this->resident_memory = value;
        return *this;
    }

    ProcessMeter& with_page_size(page_size_t value) {
        this->page_size = value;
        return *this;
    }

    ProcessMeter& with_n_cpu_online(n_cpu_online_t value) {
        this->n_cpu_online = value;
        return *this;
    }

    ProcessMeter& with_n_cpu_maximum(n_cpu_maximum_t value) {
        this->n_cpu_maximum = value;
        return *this;
    }

    ProcessMeter& with_n_threads(n_threads_t value) {
        this->n_threads = value;
        return *this;
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
