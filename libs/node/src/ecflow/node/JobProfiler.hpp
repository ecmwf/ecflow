/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_node_JobProfiler_HPP
#define ecflow_node_JobProfiler_HPP

///
/// \brief This class is used to aid profiling of the job generation step.
///
/// This will be used to identify those suite/family/tasks that take the most
/// amount of time, *when* we exceed the jobs generation interval.
/// In particular if we have output that is many megabytes, it can affect
/// the performance of the server, especially when the server is running
/// on virtual machines
///

#include "ecflow/core/Chrono.hpp"

class JobsParam;
class Task;

namespace ecf {

class JobProfiler {
private:
public:
    // Note: 1000 milliseconds = 1 second
    JobProfiler(Task*, JobsParam&, size_t threshold /* expected to be milliseconds */);
    // Disable copy (and move) semantics
    JobProfiler(const JobProfiler&)                  = delete;
    const JobProfiler& operator=(const JobProfiler&) = delete;

    ~JobProfiler();

    static void set_task_threshold(size_t threshold);
    static size_t task_threshold();

    static int task_threshold_default();

private:
    Task* node_;
    JobsParam& jobsParam_;
    boost::posix_time::ptime start_time_;
    size_t threshold_;
};

} // namespace ecf

#endif /* ecflow_node_JobProfiler_HPP */
