#ifndef BASESERVER_HPP_
#define BASESERVER_HPP_

/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        : Server.cpp
// Author      : Avi
// Revision    : $Revision: #62 $
//
// Copyright 2009-2019 ECMWF.
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
#include "NodeTreeTraverser.hpp"
#include "CheckPtSaver.hpp"
#include "AbstractServer.hpp"

class ServerEnvironment;

class BaseServer : public AbstractServer {
public:
   /// Constructor opens the acceptor and starts waiting for the first incoming
   /// connection.
   explicit BaseServer(boost::asio::io_service& io_service,ServerEnvironment&);
   ~BaseServer() override;


   void handle_terminate();

   // abort server if check pt files exist, but can't be loaded
   bool load_check_pt_file_on_startup();
   void loadCheckPtFile();
   bool restore_from_checkpt(const std::string& filename, bool& failed);
   void update_defs_server_state();
   void set_server_state(SState::State);


   /// AbstractServer functions
   SState::State state() const override { return serverState_; }
   std::pair<std::string,std::string> hostPort() const override;
   defs_ptr defs() const override { return defs_;}
   void updateDefs(defs_ptr,bool force) override;
   void clear_defs() override;
   void checkPtDefs(ecf::CheckPt::Mode m = ecf::CheckPt::UNDEFINED,
                               int check_pt_interval = 0,
                               int check_pt_save_time_alarm = 0) override;
   void restore_defs_from_checkpt() override;
   void nodeTreeStateChanged() override;
   bool allowTaskCommunication() const override;
   void shutdown() override;
   void halted() override;
   void restart() override;
   bool reloadWhiteListFile(std::string& errorMsg) override;
   bool reloadPasswdFile(std::string& errorMsg) override;
   bool reloadCustomPasswdFile(std::string& errorMsg) override;
   bool authenticateReadAccess(const std::string& user,bool custom_user,const std::string& passwd) override;
   bool authenticateReadAccess(const std::string& user,bool custom_user,const std::string& passwd,const std::string& path) override;
   bool authenticateReadAccess(const std::string& user,bool custom_user,const std::string& passwd,const std::vector<std::string>& paths) override;
   bool authenticateWriteAccess(const std::string& user) override;
   bool authenticateWriteAccess(const std::string& user, const std::string& path) override;
   bool authenticateWriteAccess(const std::string& user, const std::vector<std::string>& paths) override;
   bool lock(const std::string& user) override;
   void unlock() override;
   const std::string& lockedUser() const override;
   void traverse_node_tree_and_job_generate(const boost::posix_time::ptime& time_now, bool user_cmd_context) const override;
   int poll_interval() const override;
   void debug_server_on() override;
   void debug_server_off() override;
   bool debug() const override;

   // used in signal, for emergency check point during system session
   void sigterm_signal_handler();

protected:

   /// The io_service used to perform asynchronous operations.
   boost::asio::io_service& io_service_;

   /// The signal_set is used to register for automatic check pointing
   boost::asio::signal_set signals_;

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
