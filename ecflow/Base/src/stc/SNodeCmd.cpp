/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        : Cmd
// Author      : Avi
// Revision    : $Revision: #11 $ 
//
// Copyright 2009-2012 ECMWF. 
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description :
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
#include <iostream>

#include "SNodeCmd.hpp"
#include "ClientToServerCmd.hpp"
#include "Defs.hpp"
#include "Suite.hpp"
#include "Family.hpp"
#include "Task.hpp"
#include "Alias.hpp"
#include "AbstractServer.hpp"
#include "PrintStyle.hpp"

using namespace std;
using namespace boost;

//=====================================================================================
// This command returns the requested node back to the client
// Note: In the case where defs has not been loaded, it can be NULL

SNodeCmd::SNodeCmd(AbstractServer* as,node_ptr node)
{
   init(as,node);
}

void SNodeCmd::init(AbstractServer* as, node_ptr node)
{
   suite_.reset();
   family_.reset();
   task_.reset();
   alias_.reset();

   if (node.get()) {
      if (node->isSuite()) {
         suite_ = boost::dynamic_pointer_cast<Suite>(node);
      }
      else if (node->isFamily()) {
         family_ = boost::dynamic_pointer_cast<Family>(node);
      }
      else if (node->isTask()) {
         task_ = boost::dynamic_pointer_cast<Task>(node);
      }
      else if (node->isAlias()) {
         alias_ = boost::dynamic_pointer_cast<Alias>(node);
      }
   }
}

node_ptr SNodeCmd::get_node_ptr() const
{
   if (suite_.get()) {
      return boost::dynamic_pointer_cast<Node>(suite_);
   }
   else if (family_.get()) {
      return boost::dynamic_pointer_cast<Node>(family_);
   }
   else if (task_.get()) {
      return boost::dynamic_pointer_cast<Node>(task_);
   }
   else if (alias_.get()) {
      return boost::dynamic_pointer_cast<Node>(alias_);
   }
   return node_ptr();
}

bool SNodeCmd::equals(ServerToClientCmd* rhs) const
{
   SNodeCmd* the_rhs = dynamic_cast<SNodeCmd*>(rhs);
   if (!the_rhs) return false;
   if (!ServerToClientCmd::equals(rhs)) return false;
   return true;
}

std::ostream& SNodeCmd::print(std::ostream& os) const
{
   os << "cmd:SNodeCmd [ ";
   node_ptr node = get_node_ptr();
   if (node.get()) os << node->absNodePath();
   else       os << "node == NULL";
   os << " ]";
   return os;
}

// Called in client
bool SNodeCmd::handle_server_response( ServerReply& server_reply, Cmd_ptr cts_cmd,  bool debug ) const
{
   if (debug) std::cout << "SNodeCmd::handle_server_response\n";

   node_ptr node = get_node_ptr();
   if ( !node.get() ) {
      std::stringstream ss;
      ss << "SNodeCmd::handle_server_response: Error Node could not be retrieved from server. Request "; cts_cmd->print(ss); ss << " failed.\n";
      throw std::runtime_error(ss.str());
   }


   if (server_reply.cli() && !cts_cmd->group_cmd()) {
      /// This Could be part of a group command, hence ONLY show Node if NOT group command
      PrintStyle style(cts_cmd->show_style());

      Suite* suite = node->isSuite();
      if (suite) {
         if (cts_cmd->show_style() != PrintStyle::MIGRATE) {
            /// Auto generate externs, before writing to standard out. This can be expensive since
            /// All the trigger references need to to be resolved. & AST need to be created first
            /// The old spirit based parsing, horrendously, slow. Can't use Spirit QI, till IBM pull support it
            ///
            /// We need a fabricate a defs to show the externs, used by the suite
            Defs defs;
            defs.addSuite(boost::dynamic_pointer_cast<Suite>( node ));
            defs.auto_add_externs();
            std::cout << defs;
            return true;
         }
         std::cout << *suite << "\n";
         return true;
      }
      Family* fam = node->isFamily();
      if (fam) std::cout << *fam << "\n";
      Task* task = node->isTask();
      if (task) std::cout << *task << "\n";
      Alias* alias = node->isAlias();
      if (alias) std::cout << *alias << "\n";
   }
   else {
      server_reply.set_client_node( node );
   }
   return true;
}

std::ostream& operator<<(std::ostream& os, const SNodeCmd& c) { return c.print(os); }
