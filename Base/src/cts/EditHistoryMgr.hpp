#ifndef EDIT_HISTORY_MGR_HPP_
#define EDIT_HISTORY_MGR_HPP_
//============================================================================
// Name        :
// Author      : Avi
// Revision    : $Revision: #72 $
//
// Copyright 2009-2012 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
// This class manages the edit history, for the commands.
// It determines if there was a state change, if there was, it adds edit
// history to the stored nodes.
// Additionally we check that if there was an edit then the command must
// return ClientToServerCmd::isWrite() true.
//============================================================================
#include <boost/noncopyable.hpp>
#include <string>

class ClientToServerCmd;
class AbstractServer;

class EditHistoryMgr : private boost::noncopyable  {
public:
   EditHistoryMgr(const ClientToServerCmd*,AbstractServer*);
   ~EditHistoryMgr();

private:
   void add_edit_history(const std::string& path) const;
   void add_delete_edit_history(const std::string& path) const;

private:
   const ClientToServerCmd* cts_cmd_;
   AbstractServer* as_;
   mutable unsigned int state_change_no_;        // detect state change in defs
   mutable unsigned int modify_change_no_;       // detect state change in defs
};
#endif
