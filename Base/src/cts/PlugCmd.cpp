/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #32 $ 
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

#include "ClientToServerCmd.hpp"
#include "AbstractServer.hpp"
#include "AbstractClientEnv.hpp"
#include "Str.hpp"
#include "CtsApi.hpp"
#include "Defs.hpp"
#include "Suite.hpp"
#include "Task.hpp"
#include "Family.hpp"
#include "NodePath.hpp"
#include "SuiteChanged.hpp"
#include "Client.hpp"
#ifdef ECF_OPENSSL
#include "SslClient.hpp"
#include "Openssl.hpp"
#endif

using namespace ecf;
using namespace std;
using namespace boost;
namespace po = boost::program_options;

//=======================================================================================

bool PlugCmd::equals(ClientToServerCmd* rhs) const
{
   auto* the_rhs = dynamic_cast<PlugCmd*>(rhs);
   if (!the_rhs)  return false;
   if ( source_ != the_rhs->source()) { return false; }
   if ( dest_   != the_rhs->dest())   { return false; }
   return UserCmd::equals(rhs);
}

std::ostream& PlugCmd::print(std::ostream& os) const
{
   return user_cmd(os,CtsApi::to_string(CtsApi::plug(source_,dest_)));
}
std::ostream& PlugCmd::print_only(std::ostream& os) const
{
   os << CtsApi::to_string(CtsApi::plug(source_,dest_));return os;
}

/// Class to manage locking: Only unlock if acquired the lock,
class Lock {
public:
   Lock(const std::string& user, AbstractServer* as) : as_(as) { ok_ = as->lock(user); }
   ~Lock() { if (ok_) as_->unlock(); }
   bool ok() const { return ok_;}
private:
   bool ok_;
   AbstractServer* as_;
};

STC_Cmd_ptr PlugCmd::doHandleRequest(AbstractServer* as) const
{
   as->update_stats().plug_++;

   Lock lock(user(),as);
   if (!lock.ok()) {
      std::string errorMsg = "Plug command failed. User "; errorMsg += as->lockedUser();
      errorMsg += " already has an exclusive lock";
      throw std::runtime_error( errorMsg ) ;
   }

   node_ptr sourceNode = as->defs()->findAbsNode(source_);
   if (!sourceNode.get()) throw std::runtime_error( "Plug command failed. Could not find source path " + source_  ) ;

   // Moving a node which is active, or submitted, will lead to zombie's. hence prevent
   if (sourceNode->state() == NState::ACTIVE || sourceNode->state() == NState::SUBMITTED) {
      std::string errorMsg = "Plug command failed. The source node "; errorMsg += source_;
      errorMsg += " is ";
      errorMsg += NState::toString(sourceNode->state());
      throw std::runtime_error( errorMsg ) ;
   }

   if (sourceNode->isAlias()) {
       std::string errorMsg = "Plug command failed. The source node "; errorMsg += source_;
       errorMsg += " is a Alias. Alias can not be moved";
       throw std::runtime_error( errorMsg ) ;
   }


   // Check to see if dest node is on the same server
   std::string host,port,destPath;
   node_ptr destNode =  as->defs()->findAbsNode(dest_);
   if (!destNode.get()) {

      // Dest could still be on the same server. Extract host and port
      // expect: host:port/suite/family/node
      if (!NodePath::extractHostPort(dest_,host,port)) {
         std::string errorMsg = "Plug command failed. The destination path "; errorMsg += dest_;
         errorMsg += " does not exist on server, and could not extract host/port from the destination path";
         throw std::runtime_error( errorMsg ) ;
      }

      // Remove the host:port from the path
      destPath = NodePath::removeHostPortFromPath(dest_);

      std::pair<std::string,std::string> hostPortPair = as->hostPort();
      if ( hostPortPair.first == host  && hostPortPair.second == port) {

         // Matches local server, try to find dest node again.
         destNode =  as->defs()->findAbsNode(destPath);
         if (!destNode) {
            std::string errorMsg = "Plug command failed. The destination path "; errorMsg += dest_;
            errorMsg += " does not exist on server "; errorMsg += hostPortPair.first;
            throw std::runtime_error( errorMsg ) ;
         }
      }
      // dest_ is on another server.
   }

   if (!destNode.get()) {

      // Since host/port does not match local server, move source node to remote server
      try {
         if (destPath.empty()) {
            // Note destPath can be empty, when moving a suite
            if (!sourceNode->isSuite()) {
               throw std::runtime_error( "Destination path can only be empty when moving a whole suite to a new server" ) ;
            }
         }

         {
            // MoveCmd should inherit user and passwd, from PlugCmd
            Cmd_ptr cts_cmd = Cmd_ptr(new MoveCmd(as->hostPort(),sourceNode.get(), destPath));
            cts_cmd->setup_user_authentification(user(),passwd());

            // Server is acting like a client, Send MoveCmd to another server
            // The source should end up being copied, when sent to remote server
            ServerReply server_reply;
            boost::asio::io_service io_service;
#ifdef ECF_OPENSSL
            if (as->ssl()) {
                ecf::Openssl openssl;
                openssl.init_for_client(host,port);

                SslClient theClient( io_service, openssl.context(), cts_cmd,  host, port );
                io_service.run();
                theClient.handle_server_response( server_reply, false /* debug */ );
                if (server_reply.client_request_failed()) {
                   throw std::runtime_error( server_reply.error_msg() ) ;
                }
            }
            else {
#endif
               Client theClient( io_service, cts_cmd,  host, port );
               io_service.run();
               theClient.handle_server_response( server_reply, false /* debug */ );
               if (server_reply.client_request_failed()) {
                  throw std::runtime_error( server_reply.error_msg() ) ;
               }
            }
#ifdef ECF_OPENSSL
         }
#endif

         // The move command was ok, remove the source node, and delete its memory
         sourceNode->remove();

         // Updated defs state
         as->defs()->set_most_significant_state();

         return PreAllocatedReply::ok_cmd();
      }
      catch (std::exception& e) {
         std::stringstream ss; ss << "MoveCmd Failed for " << host << ":" << port << "  " << e.what() << "\n";
         throw std::runtime_error( ss.str() ) ;
      }
   }

   // source and destination on same defs file

   // If the destination is task, replace with its parent

   Node* theDestNode = destNode.get();
   if (theDestNode->isTask()) theDestNode = theDestNode->parent();
   SuiteChanged1 suiteChanged(destNode->suite());

   // Before we do remove the source node, check its ok to add it as a child
   std::string errorMsg;
   if (!theDestNode->isAddChildOk(sourceNode.get(),errorMsg) ) {
      throw std::runtime_error(  "Plug command failed. " +  errorMsg ) ;
   }

   if (!theDestNode->addChild(  sourceNode->remove()  ) ) {
      // This should never fail !!!! else we have lost/ and leaked source node !!!!
      throw std::runtime_error("Fatal error plug command failed.") ;
   }

   add_node_for_edit_history(destNode);

   // Updated defs state
   as->defs()->set_most_significant_state();

   return PreAllocatedReply::ok_cmd();
}

const char* PlugCmd::arg()  { return CtsApi::plugArg();}
const char* PlugCmd::desc() {
   return
            "Plug command is used to move nodes.\n"
            "The destination node can be on another server In which case the destination\n"
            "path should be of the form '<host>:<port>/suite/family/task\n"
            "  arg1 = path to source node\n"
            "  arg2 = path to the destination node\n"
            "This command can fail because:\n"
            "- Source node is in a 'active' or 'submitted' state\n"
            "- Another user already has an lock\n"
            "- source/destination paths do not exist on the corresponding servers\n"
            "- If the destination node path is empty, i.e. only host:port is specified,\n"
            "  then the source node must correspond to a suite.\n"
            "- If the source node is added as a child, then its name must be unique\n"
            "  amongst its peers\n"
            "Usage:\n"
            "  --plug=/suite macX:3141  # move the suite to ecFlow server on host(macX) and port(3141)"
            ;
}

void PlugCmd::addOption(boost::program_options::options_description& desc) const {
   desc.add_options()( PlugCmd::arg(),po::value< vector<string> >()->multitoken(), PlugCmd::desc() );
}

void PlugCmd::create( 	Cmd_ptr& cmd,
         boost::program_options::variables_map& vm,
         AbstractClientEnv* ace) const
{
   vector<string> args = vm[  arg() ].as< vector<string> >();

   if (ace->debug())  dumpVecArgs(PlugCmd::arg(),args);

   if (args.size() != 2 ) {
      std::stringstream ss;
      ss << "PlugCmd: Two arguments are expected, found " << args.size() << "\n" << PlugCmd::desc() << "\n";
      throw std::runtime_error( ss.str() );
   }

   std::string sourceNode = args[0];
   std::string destNode = args[1];

   cmd = std::make_shared<PlugCmd>(sourceNode, destNode);
}

// ===================================================================================

MoveCmd::MoveCmd(const std::pair<std::string,std::string>& host_port, Node* src, const std::string& dest)
 : src_node_(src->print(PrintStyle::NET)),
   src_host_(host_port.first),
   src_port_(host_port.second),
   src_path_(src->absNodePath()),
   dest_(dest)
{}

MoveCmd::MoveCmd()= default;
MoveCmd::~MoveCmd()= default;

bool MoveCmd::equals(ClientToServerCmd* rhs) const
{
   auto* the_rhs = dynamic_cast<MoveCmd*>(rhs);
   if (!the_rhs) return false;
   if (dest_   != the_rhs->dest())   { return false; }
   if (src_node_   != the_rhs->src_node())   { return false; }
   return UserCmd::equals(rhs);
}

std::ostream& MoveCmd::print(std::ostream& os) const
{
   std::stringstream ss;
   ss << "Plug(Move) source(" << src_host_ << ":" << src_port_ << ":" << src_path_ << ") destination(" << dest_ << ")";
   return user_cmd(os,ss.str());
}

bool MoveCmd::check_source() const
{
   return !src_node_.empty();
}


STC_Cmd_ptr MoveCmd::doHandleRequest(AbstractServer* as) const
{
   Lock lock(user(),as);
   if (!lock.ok()) {
      std::string errorMsg = "Plug(Move) command failed. User "; errorMsg += as->lockedUser();
      errorMsg += " already has an exclusive lock";
      throw std::runtime_error( errorMsg);
   }

   if (!check_source()) {
      throw std::runtime_error("Plug(Move) command failed. No source specified");
   }

   std::string error_msg;
   node_ptr src_node = Node::create(src_node_,error_msg);
   if (!error_msg.empty() || !src_node) {
      throw std::runtime_error("Plug(Move) command failed. Error in source:\n" + error_msg );
   }

   // destNode can be NULL when we are moving a suite
   node_ptr destNode;
   if (!dest_.empty()) {

      destNode = as->defs()->findAbsNode(dest_);
      if (!destNode.get()) {
         std::string errorMsg = "Plug(Move) command failed. The destination path "; errorMsg += dest_;
         errorMsg += " does not exist on server";
         throw std::runtime_error( errorMsg);
      }
   }
   else {
      if (!src_node->isSuite()) {
         throw std::runtime_error("::Destination path can only be empty when moving a whole suite to a new server");
      }
   }

   if (destNode.get()) {

      // The destNode containing suite may be in a handle
      SuiteChanged0 suiteChanged(destNode);

      // If the destination is task, replace with its parent
      Node* thedestNode = destNode.get();
      if (thedestNode->isTask()) thedestNode = thedestNode->parent();

      // check its ok to add
      std::string errorMsg;
      if (!thedestNode->isAddChildOk(src_node.get(),errorMsg) ) {
         std::string msg = "Plug(Move) command failed. "; msg += errorMsg;
         throw std::runtime_error( msg) ;
      }

      // pass ownership
      if (!thedestNode->addChild( src_node )) {
         // This should never fail !!!! else we have lost/ and leaked source node !!!!
         throw std::runtime_error("Fatal error plug(move) command failed. can not addChild") ;
      }

      add_node_for_edit_history(destNode);
   }
   else {

      if (!src_node->isSuite())  throw std::runtime_error("plug(move): Source node was expected to be a suite");

      // convert node_ptr to suite_ptr
      suite_ptr the_source_suite = std::dynamic_pointer_cast<Suite>(src_node);

      // The sourceSuite may be in a handle or pre-registered suite
      SuiteChanged suiteChanged(the_source_suite);

      as->defs()->addSuite( the_source_suite ) ;

      add_node_for_edit_history(the_source_suite);
   }

   as->defs()->set_most_significant_state();

   // Ownership for src_node has been passed on.
   return PreAllocatedReply::ok_cmd();
}

const char* MoveCmd::arg()  { return "move";}
const char* MoveCmd::desc() { return "The move command is an internal cmd, Called by the plug cmd. Does not appear on public api.";}

void MoveCmd::addOption(boost::program_options::options_description& desc) const {
   desc.add_options()( MoveCmd::arg(),po::value< vector<string> >()->multitoken(), MoveCmd::desc() );
}

void MoveCmd::create(Cmd_ptr&, boost::program_options::variables_map&, AbstractClientEnv* ) const
{
   assert(false);
}

std::ostream& operator<<(std::ostream& os, const PlugCmd& c) { return c.print(os); }
std::ostream& operator<<(std::ostream& os, const MoveCmd& c) { return c.print(os); }
