#ifndef SERVER_STATE_HPP_
#define SERVER_STATE_HPP_
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #18 $ 
//
// Copyright 2009-2016 ECMWF. 
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description :
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8

#include <boost/serialization/serialization.hpp>
#include <boost/serialization/string.hpp>         // no need to include <string>

#include "SState.hpp"
#include "NodeFwd.hpp"
#include "Variable.hpp"

/// This class stores the server state, so that it is accessible by the node tree
///
/// The server variables could have been added/deleted or changed, hence these
/// variables are serialised, also:  it has proved useful i.e for
///    a/ Client to visualise the server environment during a call like -group="get; show state"
///    b/ Allows the test code to update environment. ie. use ECF_CLIENT
////      allows us to replace smsinit, smscomplete,etc, with the ECF client exe
//
// Note The default state here is RUNNING while in the server the default state is HALTED
// By choosing RUNNING, it allows the Defs related test, to run without explicitly setting the state
class ServerState {
public:
   ServerState();
   ServerState(const ServerState&);

   /// Check pointing, SAVES server variables, since they are visualised by client like ecflow_ui
   /// HOWEVER PrintStyle::MIGRATE does not save the server variables, since they should
   /// not take part in migration. However the testing compares migration files with check point files
   /// This would always fail. Hence we do not compare server variables.
	/// This does not compare server variables
   bool operator==(const ServerState& rhs) const;
   bool operator!=(const ServerState& rhs) const { return !operator==(rhs);}

   /// This does compare server variables. Used in testing
   bool compare(const ServerState& rhs) const;

   /// The server variable: are automatically added by the server
   /// on STARTUP and when a checkpoint file is reloaded.
   /// The variable are required by Job creation & needed in creation of generated variables
   /// The variables should NOT be modified.
   ///
   /// However user variables can be freely added,deleted and modified
   /// They will override server variables of the same name

   void add_or_update_server_variables( const NameValueVec& env);
   void delete_server_variable( const std::string&); // should only be used by test
   void set_server_variables(const std::vector<Variable>& e);
   const std::vector<Variable>& server_variables() const { return server_variables_; }

   void add_or_update_user_variables( const NameValueVec& env);
   void add_or_update_user_variables( const std::vector<Variable>& env);
 	void add_or_update_user_variables( const std::string&, const std::string&);
 	void delete_user_variable( const std::string&);

   void set_user_variables(const std::vector<Variable>& e);
   const std::vector<Variable>& user_variables() const { return user_variables_; }
   const std::string& find_user_variable(const std::string& name) const;

   // Search user variables, and then server variables
   const std::string& find_variable(const std::string& name) const;
   const Variable& findVariable(const std::string& name) const;
   bool variable_exists(const std::string& name) const;

   /// find all %VAR% and replaces with variable values, returns false on the
   /// first variable that can't be found, cmd will be left half processed.
   /// Will search for ECF_MICRO, if not found assumes % as the micro char
   bool variableSubsitution(std::string& cmd) const;

  	// These functions/data are used to during job generation
	void jobSubmissionInterval(int s) { jobSubmissionInterval_ =  s;}
	int jobSubmissionInterval() const { return jobSubmissionInterval_;}
	bool jobGeneration() const { return jobGeneration_;} // testing may disable job generation
	void jobGeneration(bool f) { jobGeneration_ = f;}

 	void set_state(SState::State s);
 	SState::State get_state() const { return server_state_; }
 	static SState::State default_state() { return SState::RUNNING; }

	// set by the server hence no need persist
	void hostPort( const std::pair<std::string,std::string>& hostPort ) { hostPort_ = hostPort;}
	std::pair<std::string,std::string> hostPort() const { return hostPort_; }

	/// Currently only SState::State server_state_ recorded
   unsigned int state_change_no() const { return state_change_no_; }
   unsigned int variable_state_change_no() const { return variable_state_change_no_; }

   /// determines why the node is not running.
   void why(std::vector<std::string>& theReasonWhy) const;

   /// Used in test
   static void setup_default_server_variables(std::vector<Variable>&  server_variables, const std::string& port);

private:
 	void setup_default_env();

   void add_or_update_server_variables( const std::string&, const std::string&);

private:

   unsigned int state_change_no_;                // *not* persisted, only used on server side
   unsigned int variable_state_change_no_;       // *not* persisted, only used on server side

 	SState::State server_state_;
   std::vector<Variable>  server_variables_;
   std::vector<Variable>  user_variables_;

 	int jobSubmissionInterval_;                   // NOT persisted, since set in the server
 	bool jobGeneration_;                          // NOT persisted, since set in the server
 	std::pair<std::string,std::string> hostPort_; // NOT persisted, set by server hence no need to persist

private:
 	friend class boost::serialization::access;
 	template<class Archive>
 	void serialize(Archive & ar, const unsigned int /*version*/)
 	{
 	   ar & server_state_;
      ar & user_variables_;
      ar & server_variables_;
 	}
};

// This should ONLY be added to objects that are *NOT* serialised through a pointer
BOOST_CLASS_IMPLEMENTATION(ServerState, boost::serialization::object_serializable)
BOOST_CLASS_TRACKING(ServerState,boost::serialization::track_never);

#endif
