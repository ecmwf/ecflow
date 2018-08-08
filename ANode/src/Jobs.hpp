#ifndef JOBS_HPP_
#define JOBS_HPP_
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #7 $ 
//
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description :
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
#include <boost/noncopyable.hpp>
#include "NodeFwd.hpp"

/// Job generation involves:
///  1/ resolving dependencies. This means we look at day,date,time and triggers,
///     and check to to see if a node is free or still holding.
///     When a node is free of its dependencies, a job can be created.
///     Note: for a node that is suspended, job generation is disabled.
///           In this case the time dependencies are still checked.
///           and if free are marked as such. later on if the node is resumed
///           we check dependencies and create the jobs
///  2/ Creating jobs. Pre processing and variable substitution
///  3/ Changing state of task to submitted.
///  4/ Increment the inlimit references, for successful job submission
///  5/ Error/Complete must decrement limits
///  6/ Set up signal handlers to monitor child job, so that on failure
//      Change state to ABORTED and decrement limit references
///
///  Job submission *MUST* be done sequentially,as each job submission could
///  consume a resource(i.e like a limit), which can affect subsequent jobs.
///
/// The process of resolving dependencies and submitting all the tasks, must take
/// less than 60 seconds. As this is resolution of the clock.
/// For testing purposes this can be changed and also we do not always want
/// to create jobs.
///
/// Return true, if job submission ok, else false and error message in JobsParam
///
/// The jobs file are shell scripts, which have IPC(child commands) which talk to
/// to the server. Since the scripts are user created, they can include, errors:
///   o multiple call to complete
///     To guard against this, we should *not* clear reset password in the complete
///     Otherwise we will registered as a zombie.
///   o Failure to call complete (maybe due to early exit in the job file)
///     There is not much we can do here, the job will stay active.
///   o Path do not match, since the node tree, in the server has been deleted
///     Typically the job will hold on the child commands.
///
/// Note: in real life test 99% of job generation is done after child command
class Jobs  : private boost::noncopyable  {
public:
   explicit Jobs(const defs_ptr& d) : defs_(d.get()), node_(nullptr) {}
   explicit Jobs(Defs* d) : defs_(d), node_(nullptr) {}
   explicit Jobs(Node* d) : defs_(nullptr), node_(d) {}

 	bool generate( JobsParam& ) const;
 	bool generate() const;

private:
	Defs* defs_;
	Node* node_;
};
#endif
