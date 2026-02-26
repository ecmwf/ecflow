/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/core/resources/Machine.hpp"

#include <iostream>
#include <unistd.h>

#if defined(__linux__)
    #include <fstream>
    #include <sstream>
    #include <string>

    #include <sys/sysinfo.h>
#elif defined(__APPLE__)
    #include <libproc.h>

    #include <mach/mach.h>
#endif

namespace ecf::resources {

namespace detail {

#if defined(__linux__)

struct LinuxMachine : public MachineBase
{
    ProcessMeter get_process_meter() const override;

    inline static const char* PROC_SELF_STATUS = "/proc/self/status";
};

template <typename T>
static auto parse = [](const std::string& line) -> uint64_t {
    std::istringstream iss(line);
    std::string key;
    T value = 0;
    iss >> key >> value;
    return value; // /proc reports kB
};

ProcessMeter LinuxMachine::get_process_meter() const {

    // Retrieve process ID
    const pid_t pid = getpid();

    // Retrieve maximum memory
    struct sysinfo info{};
    if (0 != sysinfo(&info)) {
        throw ResourceUnavailable("Unable to retrieve sysinfo");
    }
    const uint64_t maximum_memory_kb = static_cast<uint64_t>(info.totalram) * info.mem_unit / 1024;

    // Retrieve page size
    const long page_size = sysconf(_SC_PAGESIZE);
    if (page_size <= 0) {
        throw ResourceUnavailable("Unable to retrieve page size");
    }

    // Retrieve CPU counts
    const long n_cpus_online  = sysconf(_SC_NPROCESSORS_ONLN);
    const long n_cpus_maximum = sysconf(_SC_NPROCESSORS_CONF);
    if (n_cpus_online <= 0 || n_cpus_maximum <= 0) {
        throw ResourceUnavailable("Unable to retrieve CPU counts");
    }

    // Retrieve Memory and Thread counts from /proc/self/status
    std::ifstream status(PROC_SELF_STATUS);
    if (!status) {
        throw ResourceUnavailable("Unable to read /proc/self/status");
    }

    uint64_t virtual_memory_kb  = 0;
    uint64_t resident_memory_kb = 0;
    int32_t threads             = 0;

    for (std::string line; std::getline(status, line);) {
        if (line.rfind("VmSize:", 0) == 0) {
            virtual_memory_kb = parse<uint64_t>(line);
        }
        else if (line.rfind("VmRSS:", 0) == 0) {
            resident_memory_kb = parse<uint64_t>(line);
        }
        else if (line.rfind("Threads:", 0) == 0) {
            threads = parse<long>(line);
        }
    }

    return ProcessMeter::make()
        .with_pid(pid)
        .with_maximum_memory(maximum_memory_kb)
        .with_virtual_memory(virtual_memory_kb)
        .with_resident_memory(resident_memory_kb)
        .with_page_size(page_size)
        .with_n_cpu_online(n_cpus_online)
        .with_n_cpu_maximum(n_cpus_maximum)
        .with_n_threads(threads);
}

#elif defined(__APPLE__)

struct DarwinMachine : public MachineBase
{
    ProcessMeter get_process_meter() const override;

    host_basic_info_data_t host_nfo;
    vm_statistics64_data_t vm_stats;
};

ProcessMeter DarwinMachine::get_process_meter() const {
    mach_msg_type_number_t info_size;

    info_size = HOST_BASIC_INFO_COUNT;
    auto host_port = mach_host_self();
    if (0 != host_info(host_port, HOST_BASIC_INFO, (host_info_t) & this->host_nfo, &info_size)) {
        mach_port_deallocate(mach_task_self(), host_port);
        throw ResourceUnavailable("Unable to retrieve host info");
    }
    mach_port_deallocate(mach_task_self(), host_port);

    pid_t pid = getpid();

    struct proc_taskinfo pti;
    if (PROC_PIDTASKINFO_SIZE != proc_pidinfo(pid, PROC_PIDTASKINFO, 0, &pti, PROC_PIDTASKINFO_SIZE)) {
        throw ResourceUnavailable("Unable to retrieve proc_pidinfo");
    }

    return ProcessMeter::make()
        .with_pid(pid)
        .with_maximum_memory(this->host_nfo.max_mem / 1024)
        .with_page_size(vm_kernel_page_size)
        .with_n_cpu_online(this->host_nfo.physical_cpu)
        .with_n_cpu_maximum(this->host_nfo.physical_cpu_max)
        .with_virtual_memory(pti.pti_virtual_size / 1024)
        .with_resident_memory(pti.pti_resident_size / 1024)
        .with_n_threads(pti.pti_threadnum);
}

#endif

} // namespace detail

Machine Machine::make() {
#if defined(__linux__)
    return Machine(std::make_unique<detail::LinuxMachine>());
#elif defined(__APPLE__)
    return Machine(std::make_unique<detail::DarwinMachine>());
#else
    throw UnsupportedPlatform("Unsupported platform");
#endif
}

} // namespace ecf::resources
