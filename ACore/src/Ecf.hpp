#ifndef ECF_HPP_
#define ECF_HPP_
//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #13 $ 
//
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description : Provides globals used by server for determining change
//============================================================================

#include <boost/noncopyable.hpp>
#include <string>

// class Ecf: This class is used in the server to determine incremental changes
//            to the data model. Each Node/attribute stores a state change no
//            When ever there is a change, we increment local state change
//            number with this global.
// When making large scale changes, ie nodes added or deleted we use modify change no
// Note: The client will need to at some point copy over the full defs
//       at this point the state change no add modify number is also copied.
//       The client passes these two number back to server, the server then
//       uses these two numbers to determine what's changed.
//
class Ecf : private boost::noncopyable {
public:
	/// Increment and then return state change no
	static unsigned int incr_state_change_no() ;
	static unsigned int state_change_no() { return state_change_no_; }
	static void set_state_change_no(unsigned int x) { state_change_no_ = x;}

	/// The modify_change_no_ is used for node addition and deletion and re-ordering
	static unsigned int incr_modify_change_no();
 	static unsigned int modify_change_no() { return modify_change_no_; }
	static void set_modify_change_no(unsigned int x) { modify_change_no_ = x;}

	/// Returns true if we are on the server side.
	/// Only in server side do we increment state/modify numbers
	/// Also used in debug/test: Allows print to add know if in server/client
	static bool server() { return server_;}

	/// Should only be set by the server, made public so that testing can also set it
   /// Only in server side do we increment state/modify numbers
	static void set_server(bool f) { server_ = f;}

	static bool debug_equality() { return debug_equality_;}
	static void set_debug_equality(bool f) { debug_equality_ = f;}

	// ECFLOW-99
   static unsigned int debug_level() { return debug_level_;}
   static void set_debug_level(unsigned int level) { debug_level_ = level;}

   static const char* SERVER_NAME();
   static const char* CLIENT_NAME();

   static const std::string& LOG_FILE();
   static const std::string& CHECKPT();
   static const std::string& BACKUP_CHECKPT();
   static const std::string& MICRO();
   static const std::string& JOB_CMD();
   static const std::string& KILL_CMD();
   static const std::string& STATUS_CMD(); // Typical run on the server
   static const std::string& CHECK_CMD();  // Typical run on the client
   static const std::string& URL_CMD();
   static const std::string& URL_BASE();
   static const std::string& URL();

private:

	Ecf(){}
	static bool server_;
	static bool debug_equality_;
   static unsigned int debug_level_;
   static unsigned int state_change_no_;
	static unsigned int modify_change_no_;
};

/// Make sure the Ecf number don't change
class EcfPreserveChangeNo {
public:
	EcfPreserveChangeNo();
	~EcfPreserveChangeNo();
private:
	unsigned int state_change_no_;
	unsigned int modify_change_no_;
};

class DebugEquality : private boost::noncopyable {
public:
   DebugEquality() { Ecf::set_debug_equality(true); }
   ~DebugEquality(){ Ecf::set_debug_equality(false); set_ignore_server_variables(false);}
   static bool ignore_server_variables() { return ignore_server_variables_;}
   static void set_ignore_server_variables(bool flg) { ignore_server_variables_ = flg;}
private:
   static bool ignore_server_variables_;
};
#endif
