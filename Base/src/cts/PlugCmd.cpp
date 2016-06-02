/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #32 $ 
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
#include <boost/make_shared.hpp>

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
#include "Client.hpp"
#include "SuiteChanged.hpp"

using namespace ecf;
using namespace std;
using namespace boost;
namespace po = boost::program_options;

//=======================================================================================

bool PlugCmd::equals(ClientToServerCmd* rhs) const
{
   PlugCmd* the_rhs = dynamic_cast<PlugCmd*>(rhs);
   if (!the_rhs)  return false;
   if ( source_ != the_rhs->source()) { return false; }
   if ( dest_   != the_rhs->dest())   { return false; }
   return UserCmd::equals(rhs);
}

std::ostream& PlugCmd::print(std::ostream& os) const
{
   return user_cmd(os,CtsApi::to_string(CtsApi::plug(source_,dest_)));
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
            // Server is acting like a client, Send MoveCmd to another server
            // The source should end up being copied, when sent to remote server
            boost::asio::io_service io_service;
            Client theClient( io_service, Cmd_ptr( new MoveCmd(as->hostPort(),sourceNode.get(), destPath) ),  host, port  );
            io_service.run();

            ServerReply server_reply;
            theClient.handle_server_response( server_reply, false /* debug */ );
            if (server_reply.client_request_failed()) {
               throw std::runtime_error( server_reply.error_msg() ) ;
            }
         }

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

   cmd = Cmd_ptr( new PlugCmd(sourceNode, destNode) );
}

// ===================================================================================

MoveCmd::MoveCmd(const std::pair<std::string,std::string>& host_port, Node* src, const std::string& dest)
 : sourceSuite_(src->isSuite()),
   sourceFamily_(src->isFamily()),
   sourceTask_(src->isTask()),
   src_host_(host_port.first),
   src_port_(host_port.second),
   src_path_(src->absNodePath()),
   dest_(dest) {}

MoveCmd::MoveCmd()
: sourceSuite_(NULL),
  sourceFamily_(NULL),
  sourceTask_(NULL) {}

MoveCmd::~MoveCmd(){}

bool MoveCmd::equals(ClientToServerCmd* rhs) const
{
   MoveCmd* the_rhs = dynamic_cast<MoveCmd*>(rhs);
   if (!the_rhs) return false;
   if (dest_   != the_rhs->dest())   { return false; }

   Node* theSource  = source();
   if (theSource == NULL && the_rhs->source())         { return false; }
   if (theSource         && the_rhs->source() == NULL) { return false; }
   if (theSource == NULL && the_rhs->source() == NULL) { return true; }
   if (theSource->absNodePath() != the_rhs->source()->absNodePath()) { return false; }
   return UserCmd::equals(rhs);
}

std::ostream& MoveCmd::print(std::ostream& os) const
{
   std::stringstream ss;
   ss << "Plug(Move) source(" << src_host_ << ":" << src_port_ << ":" << src_path_ << ") destination(" << dest_ << ")";
   return user_cmd(os,ss.str());
}

Node* MoveCmd::source() const
{
   if ( sourceSuite_ ) return  sourceSuite_ ;
   if ( sourceFamily_ ) return  sourceFamily_;
   if ( sourceTask_ ) return  sourceTask_;
   return NULL;
}

bool MoveCmd::check_source() const
{
   if ( sourceSuite_ || sourceFamily_ || sourceTask_ ) return  true ;
   return false;
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

   // destNode can be NULL when we are moving a suite
   node_ptr destNode;
   if (!dest_.empty()) {

      if (!as->defs())  throw std::runtime_error( "No definition in server");

      destNode =  as->defs()->findAbsNode(dest_);
      if (!destNode.get()) {
         std::string errorMsg = "Plug(Move) command failed. The destination path "; errorMsg += dest_;
         errorMsg += " does not exist on server";
         throw std::runtime_error( errorMsg);
      }
   }
   else {
      if (!source()->isSuite()) {
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
      if (!thedestNode->isAddChildOk(source(),errorMsg) ) {
         std::string msg = "Plug(Move) command failed. "; msg += errorMsg;
         throw std::runtime_error( msg) ;
      }

      // pass ownership
      if (!thedestNode->addChild( node_ptr( source() ) )) {
         // This should never fail !!!! else we have lost/ and leaked source node !!!!
         throw std::runtime_error("Fatal error plug(move) command failed.") ;
      }

      add_node_for_edit_history(destNode);
   }
   else {

      if (!sourceSuite_)  throw std::runtime_error("Source node was expected to be a suite");

      suite_ptr the_source_suite(sourceSuite_);        // pass ownership to suite_ptr

      // The sourceSuite may be in a handle or pre-registered suite
      SuiteChanged suiteChanged(the_source_suite);

      if (!as->defs()) {
         defs_ptr newDefs = Defs::create();
         newDefs->addSuite( the_source_suite );
         as->updateDefs( newDefs, true /*force*/ );    // force is mute, since we adding a new defs in the server
      }
      else {

         if (as->defs()->findSuite(the_source_suite->name())) {
            std::stringstream ss; ss << "Suite of name " <<  the_source_suite->name() << " already exists\n";
            throw std::runtime_error( ss.str() );
         }

         as->defs()->addSuite( the_source_suite ) ;
      }

      /// A bit of hack, since need a way of getting a node_ptr from a Node*
      add_node_for_edit_history(as,the_source_suite->absNodePath());
   }

   // Updated defs state
   if (as->defs()) as->defs()->set_most_significant_state();

   // Ownership for sourceSuite_ has been passed on.
   sourceSuite_ = NULL;
   sourceFamily_ = NULL;
   sourceTask_ =  NULL;

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
