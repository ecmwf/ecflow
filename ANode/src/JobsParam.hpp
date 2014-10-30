#ifndef JOBSPARAM_HPP_
#define JOBSPARAM_HPP_
//============================================================================
// Name        : time
// Author      : Avi
// Revision    : $Revision: #14 $ 
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

#include <boost/noncopyable.hpp>
#include "NodeFwd.hpp"

// Used as a utility class for controlling job creation.
// Collates data during the node tree traversal
// Note: For testing purposes we do not always want to create jobs or spawn jobs
class JobsParam : private boost::noncopyable {
public:
   // This constructor is used in test
	JobsParam(bool createJobs = false)
      : createJobs_(createJobs), spawnJobs_(false), submitJobsInterval_(60){}

	JobsParam(int submitJobsInterval, bool createJobs)
	   : createJobs_(createJobs),spawnJobs_(true), submitJobsInterval_(submitJobsInterval)
	   { if (!createJobs_) spawnJobs_ = false;}

	std::string& errorMsg() { return errorMsg_;}
	const std::string& getErrorMsg() const { return errorMsg_;}

	void push_back_submittable(Submittable* t) { submitted_.push_back(t); }
	const std::vector<Submittable*>& submitted() const { return submitted_;}

	bool createJobs() const { return createJobs_;}
	bool spawnJobs() const { return spawnJobs_;}

	/// returns the number of seconds at which we should check time dependencies
	/// this includes evaluating trigger dependencies and submit the corresponding jobs.
	/// This is set at 60 seconds. But will vary for debug purposes only.
	int submitJobsInterval() const { return submitJobsInterval_;}

	/// Allow user to set the debug message that appears in log file when job submission starts
	void logDebugMessage(const std::string& s) { debugMsg_ = s;}
	const std::string& logDebugMessage() const { return debugMsg_;}

	void set_user_edit_variables(const NameValueMap& v) { user_edit_variables_ = v;}
	const NameValueMap& user_edit_variables() const { return user_edit_variables_;}

	void set_user_edit_file(const std::vector<std::string>& file) { user_edit_file_ = file;}
	const std::vector<std::string>& user_edit_file() const { return user_edit_file_; }

	// Functions to aid timing of job generation
   size_t start_profile();
   size_t last_profile_index() const;
   void add_to_profile(size_t index, const std::string& s);
   void set_to_profile(size_t index, const std::string& s,int time_taken);
   const std::string& get_text_at_profile(size_t index) const;
   const std::vector< std::pair<std::string,int> >& profiles() const { return profiles_; }
   void profile_to_log() const;
   void profile_to_cout() const;

private:
	bool createJobs_;
	bool spawnJobs_;
	int  submitJobsInterval_;
	std::string errorMsg_;
	std::string debugMsg_;
	std::vector<Submittable*> submitted_;
	std::vector<std::string> user_edit_file_;
   std::vector< std::pair<std::string,int> > profiles_;  // text,time
	NameValueMap user_edit_variables_; /// Used for User edit
};
#endif
