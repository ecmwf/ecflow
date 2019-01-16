#ifndef MOCK_SERVER_HPP_
#define MOCK_SERVER_HPP_
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #26 $ 
//
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description :
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8

#include <assert.h>
#include "Defs.hpp"
#include "SuiteChanged.hpp"
#include "AbstractServer.hpp"
#include "JobsParam.hpp"
#include "Jobs.hpp"
#include "Log.hpp"
#include "Ecf.hpp"  // In server we increment modify and state change numbers,

/// Act as stand in for a server since Request require at least a AbstractServer
class MockServer : public AbstractServer {
public:
	// For the MockServer don't delete the Defs. since we past in a fixture defs
	struct null_deleter {
	    void operator()(void const *) const{}
	};

	// Only in server side do we increment state/modify numbers, controlled by: Ecf::set_server(true)
	explicit MockServer(Defs* defs) : defs_(defs_ptr(defs,MockServer::null_deleter())) { Ecf::set_server(true); }
	explicit MockServer(defs_ptr defs) : defs_(defs)                                   { Ecf::set_server(true); }
	~MockServer() { Ecf::set_server(false); }

	virtual SState::State state() const { return  SState::RUNNING;}
	virtual std::pair<std::string,std::string> hostPort() const { assert(defs_.get()); return defs_->server().hostPort(); }
	virtual defs_ptr defs() const { return defs_;}
	virtual void updateDefs(defs_ptr d, bool force) { assert(defs_.get()); defs_->absorb(d.get(),force); }
 	virtual void clear_defs() { if (defs_.get()) defs_->clear(); } // dont delete since we pass in Fixture defs. Otherwise it will crash
   virtual void checkPtDefs(ecf::CheckPt::Mode m = ecf::CheckPt::UNDEFINED,
                               int check_pt_interval = 0,
                               int check_pt_save_time_alarm = 0) {}
 	virtual void restore_defs_from_checkpt() {}
	virtual void nodeTreeStateChanged() {}
	virtual bool allowTaskCommunication() const { return true;}
 	virtual void shutdown() {}
 	virtual void halted() {}
 	virtual void restart() {}
	virtual bool reloadWhiteListFile(std::string&) { return true;}
   virtual bool reloadPasswdFile(std::string& errorMsg) { return true;}
 	virtual bool authenticateReadAccess(const std::string&,const std::string& passwd) { return true;}
   virtual bool authenticateReadAccess(const std::string&,const std::string& passwd, const std::string&){ return true;}
   virtual bool authenticateReadAccess(const std::string&,const std::string& passwd, const std::vector<std::string>&) { return true;}
 	virtual bool authenticateWriteAccess(const std::string&) { return true;}
   virtual bool authenticateWriteAccess(const std::string&, const std::string&){ return true;}
   virtual bool authenticateWriteAccess(const std::string&, const std::vector<std::string>&){ return true;}

 	virtual bool lock(const std::string& user) {
 		if (userWhoHasLock_.empty()) {
 			userWhoHasLock_ = user;
 			shutdown();
 			return true;
 		}
 		return false;
 	}
 	virtual void unlock() { userWhoHasLock_.clear(); restart(); }
 	virtual const std::string& lockedUser() const { return userWhoHasLock_;}
   virtual void traverse_node_tree_and_job_generate(const boost::posix_time::ptime& time_now,bool user_cmd_context) const {
      if (state() == SState::RUNNING && defs_.get()) {
          JobsParam jobsParam(poll_interval(), false /* as->allow_job_creation_during_tree_walk() */ );
          Jobs jobs(defs_);
          if (!jobs.generate(jobsParam))  ecf::log(ecf::Log::ERR,jobsParam.getErrorMsg());    // will automatically add end of line
       }
   }
   virtual int poll_interval() const { return 60; }
 	virtual void debug_server_on(){}
 	virtual void debug_server_off(){}
   virtual bool debug() const { return true; }

private:
	defs_ptr defs_;
 	std::string userWhoHasLock_;
};

/// This class is used to create a Mock Server, so that we can make direct
/// data model changes without using commands.
/// In particular it will:
///   o Ecf::set_server(true): This controls incrementing of state/modify change numbers
///                            which should *only* be done on the server side
///   o Update Suite state/modify change number
class MockSuiteChangedServer  : private boost::noncopyable {
public:
   explicit MockSuiteChangedServer(suite_ptr suite) : suiteChanged_(suite) { Ecf::set_server(true);}
	~MockSuiteChangedServer() { Ecf::set_server(false); }
private:
	ecf::SuiteChanged suiteChanged_;
};
#endif

