//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #18 $ 
//
// Copyright 2009-2012 ECMWF. 
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description :
//============================================================================
#include <assert.h>

#include "Jobs.hpp"
#include "Defs.hpp"
#include "Suite.hpp"
#include "Family.hpp"
#include "Task.hpp"
#include "Log.hpp"
#include "DurationTimer.hpp"
#include "JobsParam.hpp"
#include "Signal.hpp"
#include "System.hpp"
#include "SuiteChanged.hpp"
#include "JobProfiler.hpp"

using namespace ecf;
using namespace std;

//#define DEBUG_JOB_SUBMISSION 1

bool Jobs::generate( JobsParam& jobsParam) const
{
#ifdef DEBUG_JOB_SUBMISSION
   cout << "\n" << "Jobs::generate (" << jobsParam.logDebugMessage() << ") create jobs(" << jobsParam.createJobs() << ")";
   if (defs_) cout << " server_state(" << SState::to_string(defs_->server().get_state()) << ")\n";
#endif

   // dependency resolving and job submission must be less than submitJobsInterval seconds
   // Note: Duration timer makes a system call
   DurationTimer durationTimer;

#ifdef DEBUG_JOB_SUBMISSION
   LogToCout toCoutAsWell;
   LOG(Log::DBG,"-->Job submission start " << jobsParam.logDebugMessage());
#endif
   {
      // Constructor does nothing, destructor will un-block SIGCHLD
      // This will allow child process termination to handled by the signal handler in System
      // The desctructor will then re-block SIGCHLD
    	Signal unblock_on_desctruction_then_reblock;

    	// *******************************************************************
    	// **** JOB submission *MUST* be done sequentially, as each task could
    	// **** be affected by a resource/limit, and hence affect subsequent
    	// **** job submission
    	// *******************************************************************

    	if (defs_) {
    		if (defs_->server().get_state() == SState::RUNNING) {
    			const std::vector<suite_ptr>& suiteVec = defs_->suiteVec();
    			size_t theSize = suiteVec.size();
    			for(size_t i = 0; i < theSize; i++) {
    				// SuiteChanged moved internal to Suite::resolveDependencies. i.e on fast path
    			   // and when suites not begun we save a constructor/destructor calls
    				(void)suiteVec[i]->resolveDependencies(jobsParam);
    			}
    		}
    	}
    	else {
    		if (!node_->isParentSuspended()) {
    			// suite, family, task
    		   SuiteChanged1 changed(node_->suite());
    			(void)node_->resolveDependencies( jobsParam );
    		}
    	}

		// *****************************************************************
		// Should end up calling signal handler here for any pending SIGCHLD
		// *****************************************************************
    }

	// Process children that have terminated
   System::instance()->processTerminatedChildren();

#ifdef DEBUG_JOB_SUBMISSION
   LOG(Log::DBG,"-->Job submission finish interval = "
       << jobsParam.submitJobsInterval()
       <<  " time taken = " << durationTimer.duration()
       << " Tasks/Aliases submitted " << jobsParam.submitted().size()
       << " " << jobsParam.getErrorMsg()
   );
#endif

   if (durationTimer.duration() > jobsParam.submitJobsInterval()) {
      LOG(Log::ERR,"Jobs::generate: job generation time(" << durationTimer.duration() << " seconds) is greater than job submission interval of " << jobsParam.submitJobsInterval() << " seconds!!");
   }
   return jobsParam.getErrorMsg().empty();
}

bool Jobs::generate() const
{
	Defs* defs = NULL;
	if (defs_) {
		defs = defs_;
		LOG_ASSERT( defs != NULL ,"defs_ == NULL");
	}
	else {
		defs = node_->defs();
		LOG_ASSERT( defs != NULL ,"node_->defs() == NULL");
	}

	if (defs->server().get_state() == SState::RUNNING) {
	   LOG_ASSERT( defs->server().jobSubmissionInterval() != 0 ,"");
	   JobsParam jobsParam( defs->server().jobSubmissionInterval(), defs->server().jobGeneration() );
#ifdef DEBUG_JOB_SUBMISSION
	   jobsParam.logDebugMessage(" from Jobs::generate()/Server");
#endif

	   return generate( jobsParam );
	}
	return false;
}
