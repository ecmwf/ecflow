#ifndef SERVER_REPLY_HPP_
#define SERVER_REPLY_HPP_
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #18 $ 
//
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description :
//
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
#include <boost/noncopyable.hpp>
#include "NodeFwd.hpp"
#include "Zombie.hpp"
#include "Stats.hpp"

/// This class is used to hold the replies back from the server
/// *Note* server_reply_.client_handle_ is kept until the next call to register a new client_handle
/// The client invoker can be used multiple times, hence keep value of defs, and client handle in server reply

class ServerReply : private boost::noncopyable {
public:
   enum News_t { NO_NEWS, NEWS, DO_FULL_SYNC };
 	ServerReply()= default;

	/// *Note* server_reply_.client_handle_ is kept until the next call to register a new client_handle
	/// The client invoker can be used multiple times, hence keep value of defs, and client handle in server reply
 	/// and reset everything else
	void clear_for_invoke(bool command_line_interface); // true if calling from command line

 	/// task based replies
 	bool block_client_on_home_server() const { return block_client_on_home_server_;}
 	void set_block_client_on_home_server() { block_client_on_home_server_ = true;}

	bool block_client_server_halted() const { return block_client_server_halted_;}
	void set_block_client_server_halted() { block_client_server_halted_ = true;}

	bool block_client_zombie_detected() const { return block_client_zombie_detected_;}
	void set_block_client_zombie_detected() { block_client_zombie_detected_ = true;}

	void set_host_port(const std::string& host, const std::string& port) { host_ = host; port_ = port;}
   const std::string& host() const { return host_;}
   const std::string& port() const { return port_;}

	bool client_request_failed() const { return !error_msg_.empty(); }
	void set_error_msg(const std::string& e) { error_msg_ = e;}
	const std::string& error_msg() const { return error_msg_; }
	std::string& get_error_msg() { return error_msg_; }

	const std::vector<Zombie>& zombies() const { return zombies_; }
	void set_zombies( const std::vector<Zombie>& z) { zombies_ = z;}

	/// For uses with the suites cmd, or any other command that needs a vector of strings form server
   const std::vector<std::string>& get_string_vec() const { return str_vec_; }
   void set_string_vec( const std::vector<std::string>& z) { str_vec_ = z;}

   const std::vector<std::pair<unsigned int, std::vector<std::string> > >& get_client_handle_suites() const { return client_handle_suites_; }
   void set_client_handle_suites(const std::vector<std::pair<unsigned int, std::vector<std::string> > >& s) { client_handle_suites_ = s; }

	/// Only valid when the client requested the CFileCmd or log file from the server
	/// Other times this will be empty.
	const std::string& get_string() const { return str_; }
	void set_string(  const std::string& f) { str_ = f;}

	/// Only valid when Stats command called.
   const Stats& stats() const { return stats_; }
   void set_stats(  const Stats& s) { stats_ = s;}

	/// Return true if client is using Command Level Interface. Set via clear_for_invoke()
 	bool cli() const { return cli_; }

	/// Only valid after a call to sync()
 	/// in_sync() should be called after call to sync() Returns true if client defs is now in_sync with server
 	bool in_sync() const { return in_sync_;}
 	void set_sync(bool b) { in_sync_ = b;}

 	/// The SSyncCmd will set if we have a full sync(ie retrieved full node tree from server) or incremental sync
 	bool full_sync() const { return full_sync_; }
 	void set_full_sync(bool f) { full_sync_ =  f; }

 	/// Only valid after a call to news()
 	/// return true if the server has changed
 	News_t get_news() const { return news_; }
	void set_news(News_t n) { news_ = n;}


 	/// Only valid when the client requested the defs node tree from the server
	/// The defs *WILL* be retained until the next call to get the server defs
	/// **Hence is NOT reset in clear_for_invoke()
	defs_ptr client_defs() const { return client_defs_; }
	void set_client_defs(defs_ptr d) { client_defs_ = d;}

   /// Only valid when the client requested the node from the server
   /// The node *WILL* be retained until the next call to get the node
   /// **Hence is NOT reset in clear_for_invoke()
   node_ptr client_node() const { return client_node_; }
   void set_client_node(node_ptr d) { client_node_ = d;}

	/// A client handle of value 0 is not valid. Created in the server
	/// We should keep a reference to the client handle, for use with client handle commands
	/// **Hence is NOT reset in clear_for_invoke()
	void set_client_handle(int handle) { client_handle_ = handle;}
	int client_handle() const { return client_handle_;}

	// During incremental sync, record list of changed nodes, used by python api
	std::vector<std::string>& changed_nodes() { return changed_nodes_; }

private:
	friend class SSyncCmd;
	bool cli_{false};
 	bool in_sync_{false};                          // clear at the start of invoke
 	bool full_sync_{false};                        // clear at the start of invoke
	News_t news_{NO_NEWS};                           // clear at the start of invoke
	bool block_client_on_home_server_{false};      // clear at the start of invoke
	bool block_client_server_halted_{false};       // clear at the start of invoke
	bool block_client_zombie_detected_{false};     // clear at the start of invoke
	bool delete_all_{false};                       // clear at the start of invoke
   std::string host_;                      // clear at the start of invoke
   std::string port_;                      // clear at the start of invoke
	std::string str_;                       // clear at the start of invoke
	std::string error_msg_;                 // clear at the start of invoke
   std::vector<Zombie> zombies_;           // clear at the start of invoke
   std::vector<std::string> str_vec_;      // clear at the start of invoke
   std::vector<std::string> changed_nodes_;// clear at the start of invoke
   std::vector<std::pair<unsigned int, std::vector<std::string> > > client_handle_suites_; // clear at the start of invoke
   Stats stats_;                        // Used for test only, ideally need to clear

	int client_handle_{0};                  // set locally when suites are registered, and kept for reference
   defs_ptr    client_defs_;            // server reply stored as the client side defs, kept locally
   node_ptr    client_node_;            // server reply stored as the client side node, kept locally
};
#endif
