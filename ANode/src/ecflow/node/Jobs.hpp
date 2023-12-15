/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_node_Jobs_HPP
#define ecflow_node_Jobs_HPP

#include "ecflow/node/NodeFwd.hpp"

///
/// Job generation implies the following activities:
///
/// 1/ Resolve dependencies.
/// This implies inspecting day, date, time and triggers; and determine if
/// each node is free or still holding.
/// For task nodes that are free of its dependencies, a job can be created.
/// Note: Job generation is not performed for Nodes that are suspended.
///       In this case, time dependencies are still checked, and marked as
///       free, accordingly. When the Nodes are later resumed, dependencies
///       are (re)checked and job scripts are eventually created.
///
/// 2/ Create jobs scripts.
/// Perform pre-processing of .ecf files including variable substitution.
///
/// 3/ Change state of task to submitted.
///
/// 4/ Increment the inlimit references (for successful job submissions).
///
/// 5/ Update limits according to Error/Complete
///
/// 6/ Set up signal handlers to monitor child jobs.
/// This ensures that on failure the Task state is changed to ABORTED and
/// related limit references are updated accordingly.
///
/// Important: Job submissions *MUST* be done sequentially, as each job
/// submission can potentially consume a resource (e.g. a limit), which can
/// affect the ability to perform subsequent job submissions.
///
/// The process of resolving dependencies (and submitting all the tasks), must
/// take less than 60 seconds, as this is resolution of the clock.
/// For testing purposes, this can be changed and we might not always want
/// to actually create jobs.
///
/// @returns true, if job submission succeeds; false, otherwise (with error message in JobsParam)
///
/// The job files are shell scripts, which have IPC (child commands) which talk to
/// to the server. Since the scripts are user created, they can include, errors:
///   o multiple call to complete
///     To guard against this, we should *not* clear/reset password in the complete.
///     Otherwise, we will registered as a zombie.
///   o Failure to call complete (maybe due to early exit in the job file).
///     There is not much we can do here, the job will stay active.
///   o Path do not match, since the node tree, in the server has been deleted
///     Typically the job will hold on the child commands.
///
/// Note: in real life test 99% of job generation is done after child command.
///

class Jobs {
private:
    Jobs(const Jobs&)                  = delete;
    const Jobs& operator=(const Jobs&) = delete;

public:
    explicit Jobs(const defs_ptr& d) : defs_(d.get()) {}
    explicit Jobs(Defs* d) : defs_(d) {}
    explicit Jobs(Node* d) : node_(d) {}

    bool generate(JobsParam&) const;
    bool generate() const;

private:
    Defs* defs_{nullptr};
    Node* node_{nullptr};
};

#endif /* ecflow_node_Jobs_HPP */
