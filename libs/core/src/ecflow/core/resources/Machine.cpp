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
};

template <typename T>
static auto parse = [](const std::string& line) -> uint64_t {
    std::istringstream iss(line);
    std::string key;
    T value = 0;
    iss >> key >> value;
    return value;
};

namespace detail {

struct proc_self_status
{
    inline static const char* PROC_SELF_STATUS = "/proc/self/status";

    uint64_t vm_size;
    uint64_t vm_rss;
    uint64_t threads;

    static proc_self_status process() {
        std::ifstream status(PROC_SELF_STATUS);
        if (!status) {
            throw ResourceUnavailable("Unable to read " + std::string(PROC_SELF_STATUS));
        }
        proc_self_status ps;

        std::string line;
        while (std::getline(status, line)) {
            if (line.rfind("VmSize:", 0) == 0) {
                ps.vm_size = parse<uint64_t>(line);
                continue;
            }
            if (line.rfind("VmRSS:", 0) == 0) {
                ps.vm_rss = parse<uint64_t>(line);
                continue;
            }
            if (line.rfind("Threads:", 0) == 0) {
                ps.threads = parse<long>(line);
                continue;
            }
        }

        return ps;
    }

    uint64_t virtual_memory_kb() const { return vm_size; }
    uint64_t resident_memory_kb() const { return vm_rss; }
    int32_t n_threads() const { return static_cast<int32_t>(threads); }
};

struct proc_self_stat
{
    inline static const char* PROC_SELF_STAT = "/proc/self/stat";

    uint64_t proc_utime         = 0;
    uint64_t proc_stime         = 0;
    int64_t proc_cutime         = 0;
    int64_t proc_cstime         = 0;
    int32_t threads             = 0;
    uint64_t virtual_memory_kb  = 0;
    uint64_t resident_memory_kb = 0;

    static proc_self_stat process() {
        std::ifstream stat(PROC_SELF_STAT);
        if (!stat) {
            throw ResourceUnavailable("Unable to read " + std::string(PROC_SELF_STAT));
        }

        proc_self_stat ps;

        // List of "relevant" fields in the 1st line of /proc/self/stat:
        //  - (1) pid %d -- The process ID.
        //
        //  - (2) comm %s --  The filename of the executable, in parentheses.
        //
        //  ...
        //
        //  - (12) majflt %lu
        //       The number of major faults the process has made
        //       which have required loading a memory page from disk.
        //
        //  - (13) cmajflt %lu
        //       The number of major faults that the process's
        //       waited-for children have made.
        //
        //  - (14) utime %lu
        //       Amount of time that this process has been scheduled
        //       in user mode, measured in clock ticks (divide by
        //       sysconf(_SC_CLK_TCK)).  This includes guest time,
        //       guest_time (time spent running a virtual CPU, see
        //       below), so that applications that are not aware of
        //       the guest time field do not lose that time from
        //       their calculations.
        //
        //  - (15) stime %lu
        //       Amount of time that this process has been scheduled
        //       in kernel mode, measured in clock ticks (divide by
        //       sysconf(_SC_CLK_TCK)).
        //
        //  - (16) cutime %ld
        //       Amount of time that this process's waited-for
        //       children have been scheduled in user mode, measured
        //       in clock ticks (divide by sysconf(_SC_CLK_TCK)).
        //       (See also times(2).)  This includes guest time,
        //       cguest_time (time spent running a virtual CPU, see
        //       below).
        //
        //  - (17) cstime %ld
        //       Amount of time that this process's waited-for
        //       children have been scheduled in kernel mode,
        //       measured in clock ticks (divide by
        //       sysconf(_SC_CLK_TCK)).
        //
        //  ...
        //
        //  - (20) num_threads %ld
        //
        //  ...
        //
        //  - (23) vsize %lu
        //       Virtual memory size in bytes.
        //
        //  - (24) rss %ld
        //       Resident Set Size: number of pages the process has
        //       in real memory.  This is just the pages which count
        //       toward text, data, or stack space.  This does not
        //       include pages which have not been demand-loaded in,
        //       or which are swapped out.  This value is inaccurate.
        //

        // Read 1st line
        std::string line;
        std::getline(stat, line);

        // Skip fields 1-2
        auto found = line.rfind(')');
        if (found == std::string::npos) {
            throw ResourceUnavailable("Unable to parse " + std::string(PROC_SELF_STAT));
        }
        found = found + 2; // skip ") "

        auto trimmed = line.substr(found);
        std::istringstream remaining(trimmed);

        // Skip fields 3-13
        std::string dummy;
        for (int i = 3; i <= 13; ++i) {
            remaining >> dummy;
        }

        // Retrieve fields 14, 15, 16, 17
        remaining >> ps.proc_utime >> ps.proc_stime >> ps.proc_cutime >> ps.proc_cstime;

        // Skip fields 18-19
        for (int i = 18; i <= 19; ++i) {
            remaining >> dummy;
        }

        // Retrieve fields 20
        remaining >> ps.threads;

        // Skip fields 21-22
        for (int i = 21; i <= 22; ++i) {
            remaining >> dummy;
        }

        // Retrieve fields 23-24
        remaining >> ps.virtual_memory_kb >> ps.resident_memory_kb;

        return ps;
    }
};

struct proc_stat
{
    inline static const char* PROC_STAT = "/proc/stat";

    uint64_t system_total_ticks = 0;

    static proc_stat process() {
        std::ifstream stat(PROC_STAT);
        if (!stat) {
            throw ResourceUnavailable("Unable to read " + std::string(PROC_STAT));
        }

        proc_stat ps;

        std::string line;
        std::getline(stat, line);

        // Retrieve system-wide CPU time from /proc/stat (first "cpu" line)
        // Format: "cpu  user nice system idle iowait irq softirq steal guest guest_nice"
        std::istringstream stream(line);

        // Skip "cpu" label
        std::string dummy;
        stream >> dummy;

        // Sum up all ticks
        uint64_t val = 0;
        while (stream >> val) {
            ps.system_total_ticks += val;
        }

        return ps;
    }
};

} // namespace detail

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
    uint64_t virtual_memory_kb  = 0;
    uint64_t resident_memory_kb = 0;
    int32_t threads             = 0;
    {
        auto nfo           = detail::proc_self_status::process();
        virtual_memory_kb  = nfo.virtual_memory_kb();
        resident_memory_kb = nfo.resident_memory_kb();
        threads            = nfo.n_threads();
    }

    // Retrieve CPU usage from /proc/self/stat
    //   Fields (1-indexed): pid(1) comm(2) state(3) ... utime(14) stime(15) cutime(16) cstime(17) ...
    uint64_t proc_utime = 0;
    uint64_t proc_stime = 0;
    int64_t proc_cutime = 0;
    int64_t proc_cstime = 0;
    try {
        auto ps            = detail::proc_self_stat::process();
        proc_utime         = ps.proc_utime;
        proc_stime         = ps.proc_stime;
        proc_cutime        = ps.proc_cutime;
        proc_cstime        = ps.proc_cstime;
        threads            = ps.threads;
        virtual_memory_kb  = ps.virtual_memory_kb;
        resident_memory_kb = ps.resident_memory_kb;
    }
    catch (const std::exception& e) {
        throw ResourceUnavailable("Failed to retrieve process statistics: " + std::string(e.what()));
    }

    // Total process CPU time in clock ticks
    const uint64_t process_total_ticks = proc_utime + proc_stime +
                                         static_cast<uint64_t>(proc_cutime < 0 ? 0 : proc_cutime) +
                                         static_cast<uint64_t>(proc_cstime < 0 ? 0 : proc_cstime);

    uint64_t system_total_ticks = 0;
    try {
        auto ps            = detail::proc_stat::process();
        system_total_ticks = ps.system_total_ticks;
    }
    catch (const ecf::resources::ResourceUnavailable& e) {
        throw ResourceUnavailable("Failed to retrieve system CPU statistics: " + std::string(e.what()));
    }

    // CPU usage as a percentage of total system CPU time (across all CPUs)
    const double cpu_usage =
        (system_total_ticks > 0)
            ? (static_cast<double>(process_total_ticks) / static_cast<double>(system_total_ticks)) * 100.0 *
                  n_cpus_online
            : 0.0;

    return ProcessMeter::make()
        .with_pid(pid)
        .with_maximum_memory(maximum_memory_kb)
        .with_virtual_memory(virtual_memory_kb)
        .with_resident_memory(resident_memory_kb)
        .with_page_size(page_size)
        .with_n_cpu_online(n_cpus_online)
        .with_n_cpu_maximum(n_cpus_maximum)
        .with_n_threads(threads)
        .with_cpu_usage(cpu_usage);
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

    info_size      = HOST_BASIC_INFO_COUNT;
    auto host_port = mach_host_self();
    if (0 != host_info(host_port, HOST_BASIC_INFO, (host_info_t) & this->host_nfo, &info_size)) {
        mach_port_deallocate(mach_task_self(), host_port);
        throw ResourceUnavailable("Unable to retrieve host info");
    }

    pid_t pid = getpid();

    struct proc_taskinfo pti;
    if (PROC_PIDTASKINFO_SIZE != proc_pidinfo(pid, PROC_PIDTASKINFO, 0, &pti, PROC_PIDTASKINFO_SIZE)) {
        mach_port_deallocate(mach_task_self(), host_port);
        throw ResourceUnavailable("Unable to retrieve proc_pidinfo");
    }

    // --- CPU usage calculation ---
    //
    // Process CPU time from proc_taskinfo:
    //   pti_total_user   -- total user time (nanoseconds)
    //   pti_total_system -- total system time (nanoseconds)
    //
    // These correspond conceptually to (utime + cutime) and (stime + cstime) on Linux.
    //
    const uint64_t proc_cpu_ns = pti.pti_total_user + pti.pti_total_system;

    // System-wide CPU time via host_processor_info (per-CPU tick counts).
    // Each CPU reports: CPU_STATE_USER, CPU_STATE_SYSTEM, CPU_STATE_IDLE, CPU_STATE_NICE ticks.
    natural_t n_cpus                      = 0;
    processor_info_array_t cpu_info       = nullptr;
    mach_msg_type_number_t cpu_info_count = 0;

    kern_return_t kr = host_processor_info(host_port, PROCESSOR_CPU_LOAD_INFO, &n_cpus, &cpu_info, &cpu_info_count);
    mach_port_deallocate(mach_task_self(), host_port);

    double cpu_usage = 0.0;
    if (kr == KERN_SUCCESS && n_cpus > 0) {
        // Sum all CPU ticks across every processor
        uint64_t total_ticks = 0;
        auto* cpu_load       = reinterpret_cast<processor_cpu_load_info_data_t*>(cpu_info);
        for (natural_t i = 0; i < n_cpus; ++i) {
            total_ticks += cpu_load[i].cpu_ticks[CPU_STATE_USER];
            total_ticks += cpu_load[i].cpu_ticks[CPU_STATE_SYSTEM];
            total_ticks += cpu_load[i].cpu_ticks[CPU_STATE_IDLE];
            total_ticks += cpu_load[i].cpu_ticks[CPU_STATE_NICE];
        }

        // Convert system total ticks to nanoseconds (each tick = 10 ms = 10'000'000 ns on macOS,
        // but we can use the ratio directly).
        // CPU usage % = (process_cpu / system_busy) is misleading; instead compute:
        //   usage % = process_cpu_ns / (total_ticks * tick_duration_ns) * 100
        //
        // On macOS the tick rate matches CLK_TCK (typically 100 Hz => 10 ms per tick).
        const long clk_tck = sysconf(_SC_CLK_TCK);
        if (clk_tck > 0 && total_ticks > 0) {
            const double ns_per_tick  = 1'000'000'000.0 / static_cast<double>(clk_tck);
            const double total_cpu_ns = static_cast<double>(total_ticks) * ns_per_tick;
            cpu_usage = (static_cast<double>(proc_cpu_ns) / total_cpu_ns) * 100.0 * static_cast<double>(n_cpus);
        }

        // Free the memory allocated by host_processor_info
        vm_deallocate(mach_task_self(), reinterpret_cast<vm_address_t>(cpu_info), cpu_info_count * sizeof(natural_t));
    }
    else {
        if (cpu_info) {
            vm_deallocate(
                mach_task_self(), reinterpret_cast<vm_address_t>(cpu_info), cpu_info_count * sizeof(natural_t));
        }
    }

    return ProcessMeter::make()
        .with_pid(pid)
        .with_maximum_memory(this->host_nfo.max_mem / 1024)
        .with_page_size(vm_kernel_page_size)
        .with_n_cpu_online(this->host_nfo.physical_cpu)
        .with_n_cpu_maximum(this->host_nfo.physical_cpu_max)
        .with_virtual_memory(pti.pti_virtual_size / 1024)
        .with_resident_memory(pti.pti_resident_size / 1024)
        .with_n_threads(pti.pti_threadnum)
        .with_cpu_usage(cpu_usage);
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
