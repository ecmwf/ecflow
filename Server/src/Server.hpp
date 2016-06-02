#ifndef SERVER_HPP_
#define SERVER_HPP_

/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        : Server.cpp
// Author      : Avi
// Revision    : $Revision: #62 $ 
//
// Copyright 2009-2016 ECMWF. 
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description : ECFLOW Server. Based on ASIO
//
// The port numbers are divided into three ranges:
//  o the Well Known Ports, (require root permission)  0   -1023
//  o the Registered Ports,                            1024-49151
//  o Dynamic and/or Private Ports.                    49151-65535
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8

#include <memory>
#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>

// ECFLOW_MT See doc/multi-threaded-server.tar/ddoc
//#define ECFLOW_MT 1
#ifdef ECFLOW_MT
#include "CConnection.hpp" // Must come before boost/serialisation headers.
#else
#include "Connection.hpp"  // Must come before boost/serialisation headers.
#include "ClientToServerRequest.hpp"
#include "ServerToClientResponse.hpp"
#endif

#include "NodeTreeTraverser.hpp"
#include "CheckPtSaver.hpp"
#include "AbstractServer.hpp"

class ServerEnvironment;


class Server : public AbstractServer {
public:
   /// Constructor opens the acceptor and starts waiting for the first incoming
   /// connection.
   Server(ServerEnvironment&);
   virtual ~Server();

   /// Start the server
   /// The Server::run/io_service::run() call will block until all asynchronous operations
   /// have finished. While the server is running, there is always at least one
   /// asynchronous operation outstanding: the asynchronous accept call waiting
   /// for new incoming connections.
   void run();

   /// Terminate the server gracefully. Need to cancel all timers, close all sockets
   /// Server will hang if there are any pending async handlers
   void terminate();

private:

#ifdef ECFLOW_MT
   /// Handle completion of a accept operation.
   void handle_accept(const boost::system::error_code& e);
#else
   /// Handle completion of a accept operation.
   void handle_accept(const boost::system::error_code& e, connection_ptr conn);

   /// Handle completion of a write operation.
   void handle_write(const boost::system::error_code& e, connection_ptr conn);

   /// Handle completion of a read operation.
   void handle_read(const boost::system::error_code& e, connection_ptr conn);
#endif

   void handle_terminate();
   void start_accept();
   bool shutdown_socket(connection_ptr conn, const std::string& msg) const;

private:

   // abort server if check pt files exist, but can't be loaded
   bool load_check_pt_file_on_startup();
   void loadCheckPtFile();
   bool restore_from_checkpt(const std::string& filename, bool& failed);
   void update_defs_server_state();
   void set_server_state(SState::State);

protected: // Allow test to override

   /// AbstractServer functions
   virtual SState::State state() const { return serverState_; }
   virtual std::pair<std::string,std::string> hostPort() const;
   virtual defs_ptr defs() const { return defs_;}
   virtual void updateDefs(defs_ptr,bool force);
   virtual void clear_defs();
   virtual void checkPtDefs(ecf::CheckPt::Mode m = ecf::CheckPt::UNDEFINED,
                               int check_pt_interval = 0,
                               int check_pt_save_time_alarm = 0);
   virtual void restore_defs_from_checkpt();
   virtual void nodeTreeStateChanged();
   virtual bool allowTaskCommunication() const;
   virtual void shutdown();
   virtual void halted();
   virtual void restart();
   virtual bool reloadWhiteListFile(std::string& errorMsg);
   virtual bool authenticateReadAccess(const std::string& user);
   virtual bool authenticateReadAccess(const std::string& user, const std::string& path);
   virtual bool authenticateReadAccess(const std::string& user, const std::vector<std::string>& paths);
   virtual bool authenticateWriteAccess(const std::string& user);
   virtual bool authenticateWriteAccess(const std::string& user, const std::string& path);
   virtual bool authenticateWriteAccess(const std::string& user, const std::vector<std::string>& paths);
   virtual bool lock(const std::string& user);
   virtual void unlock();
   virtual const std::string& lockedUser() const;
   virtual void traverse_node_tree_and_job_generate(const boost::posix_time::ptime& time_now, bool user_cmd_context) const;
   virtual int poll_interval() const;
   virtual void debug_server_on();
   virtual void debug_server_off();
   virtual bool debug() const;

   // used in signal, for emergency check point during system session
   void sigterm_signal_handler();

private:

   /// The io_service used to perform asynchronous operations.
   boost::asio::io_service io_service_;

   /// The signal_set is used to register for automatic check pointing
   boost::asio::signal_set signals_;

   /// The acceptor object used to accept incoming socket connections.
   boost::asio::ip::tcp::acceptor acceptor_;

#ifdef ECFLOW_MT
   /// Strand to ensure the connection's handlers are not called concurrently.
   boost::asio::io_service::strand strand_;
   size_t  thread_pool_size_;
   CConnection_ptr new_connection_;
   friend class CConnection;
#else
   /// The data, typically loaded once, and then sent to many clients
   ClientToServerRequest  inbound_request_;
   ServerToClientResponse outbound_response_;
#endif

   defs_ptr defs_;             // shared because is deleted in Test, and used in System::instance()
   NodeTreeTraverser traverser_;
   friend class NodeTreeTraverser;

   CheckPtSaver checkPtSaver_;
   friend class CheckPtSaver;

   SState::State serverState_;
   ServerEnvironment& serverEnv_;
   std::string userWhoHasLock_;
};

#endif
