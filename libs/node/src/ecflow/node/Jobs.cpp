/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/node/Jobs.hpp"

#include "ecflow/core/Log.hpp"
#include "ecflow/core/Timer.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/JobsParam.hpp"
#include "ecflow/node/Operations.hpp"
#include "ecflow/node/Signal.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/node/SuiteChanged.hpp"
#include "ecflow/node/System.hpp"

using namespace ecf;
using namespace std;

// #define DEBUG_JOB_SUBMISSION 1

bool Jobs::generate(JobsParam& jobsParam) const {
#ifdef DEBUG_JOB_SUBMISSION
    cout << "\n"
         << "Jobs::generate (" << jobsParam.logDebugMessage() << ") create jobs(" << jobsParam.createJobs() << ")";
    if (defs_) {
        cout << " server_state(" << SState::to_string(defs_->server().get_state()) << ")\n";
    }
#endif

    // dependency resolving and job submission must be less than submitJobsInterval seconds
    // Note: Duration timer makes a system call
    DurationTimer durationTimer;

#ifdef DEBUG_JOB_SUBMISSION
    LogToCout toCoutAsWell;
    LOG(Log::DBG, "-->Job submission start " << jobsParam.logDebugMessage());
#endif
    {
        // Constructor does nothing, destructor will un-block SIGCHLD
        // This allows the child process termination to be handled by the signal handler in System.
        // The destructor will then re-block SIGCHLD
        Signal unblock_on_desctruction_then_reblock;

        // *******************************************************************
        // **** JOB submission *MUST* be done sequentially, as each task could
        // **** be affected by a resource/limit, and hence affect subsequent
        // **** job submission
        // *******************************************************************

        if (defs_) {
            if (defs_->server_state().get_state() == SState::RUNNING) {
                const std::vector<suite_ptr>& suites = defs_->suiteVec();
                for (const suite_ptr& suite : suites) {
                    // SuiteChanged moved into Suite::resolveDependencies.
                    // This ensures the fast path and when suite are not begun we save a ctor/dtor call
                    ecf::visit_all(*suite, ActivateAll{});
                    (void)suite->resolveDependencies(jobsParam);
                }
            }
        }
        else {
            if (!node_->isParentSuspended()) {
                // suite, family, task
                SuiteChanged1 changed(node_->suite());
                (void)node_->resolveDependencies(jobsParam);
            }
        }

        // *****************************************************************
        // Should end up calling signal handler here for any pending SIGCHLD
        // *****************************************************************
    }

    // Process children that have terminated
    System::instance()->processTerminatedChildren();

#ifdef DEBUG_JOB_SUBMISSION
    LOG(Log::DBG,
        "-->Job submission finish interval = " << jobsParam.submitJobsInterval() << " time taken = "
                                               << durationTimer.duration() << " Tasks/Aliases submitted "
                                               << jobsParam.submitted().size() << " " << jobsParam.getErrorMsg());
#endif

    if (durationTimer.duration() > jobsParam.submitJobsInterval()) {
        LOG(Log::ERR,
            "Jobs::generate: job generation time(" << durationTimer.duration()
                                                   << " seconds) is greater than job submission interval of "
                                                   << jobsParam.submitJobsInterval() << " seconds!!");
    }
    return jobsParam.getErrorMsg().empty();
}

bool Jobs::generate() const {
    Defs* defs = nullptr;
    if (defs_) {
        defs = defs_;
        LOG_ASSERT(defs != nullptr, "defs_ == NULL");
    }
    else {
        defs = node_->defs();
        LOG_ASSERT(defs != nullptr, "node_->defs() == NULL");
    }

    if (defs && defs->server_state().get_state() == SState::RUNNING) {
        LOG_ASSERT(defs->server_state().jobSubmissionInterval() != 0, "");
        JobsParam jobsParam(defs->server_state().jobSubmissionInterval(), defs->server_state().jobGeneration());
#ifdef DEBUG_JOB_SUBMISSION
        jobsParam.logDebugMessage(" from Jobs::generate()/Server");
#endif

        return generate(jobsParam);
    }
    return false;
}
