/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #37 $ 
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

#include <boost/make_shared.hpp>

#include "ClientToServerCmd.hpp"
#include "AbstractServer.hpp"
#include "AbstractClientEnv.hpp"
#include "CtsApi.hpp"
#include "Defs.hpp"
#include "DefsStructureParser.hpp"

using namespace ecf;
using namespace std;
using namespace boost;
namespace po = boost::program_options;

ReplaceNodeCmd::ReplaceNodeCmd(const std::string& node_path, bool createNodesAsNeeded, defs_ptr defs, bool force )
: createNodesAsNeeded_(createNodesAsNeeded), force_(force), pathToNode_(node_path), clientDefs_(defs)
{
   if (!clientDefs_.get()) {
      throw std::runtime_error( "ReplaceNodeCmd::ReplaceNodeCmd: client definition is empty"  );
   }

   // Client defs has been created in memory.
   // warn about naff expression and unresolved in-limit references to Limit's
   std::string errMsg, warningMsg;
   if (!clientDefs_->check(errMsg, warningMsg)) {
      throw std::runtime_error(errMsg);
   }

   // Make sure pathToNode exists in the client defs
   node_ptr nodeToReplace = clientDefs_->findAbsNode( node_path );
   if (! nodeToReplace.get() ) {
      std::stringstream ss;
      ss << "ReplaceNodeCmd::ReplaceNodeCmd: Can not replace child since path " << node_path;
      ss << " does not exist in the client definition ";
      throw std::runtime_error( ss.str() );
   }

   // Out put any warning's to standard output
   cout << warningMsg;
}

ReplaceNodeCmd::ReplaceNodeCmd(const std::string& node_path, bool createNodesAsNeeded, const std::string& path_to_defs, bool force )
: createNodesAsNeeded_(createNodesAsNeeded),
  force_(force),
  pathToNode_(node_path),
  path_to_defs_(path_to_defs),
  clientDefs_(Defs::create())
{
   // Parse the file and load the defs file into memory.
   DefsStructureParser checkPtParser( clientDefs_.get(), path_to_defs );
   std::string errMsg, warningMsg;
   if ( ! checkPtParser.doParse( errMsg , warningMsg) ) {
      std::stringstream ss;
      ss << "ReplaceNodeCmd::ReplaceNodeCmd: Could not parse file " << path_to_defs << " : " << errMsg;
      throw std::runtime_error( ss.str() );
   }

   // Make sure pathToNode exists in the client defs
   node_ptr nodeToReplace = clientDefs_->findAbsNode( node_path );
   if (! nodeToReplace.get() ) {
      std::stringstream ss;
      ss << "ReplaceNodeCmd::ReplaceNodeCmd: Can not replace child since path " << node_path;
      ss << ", does not exist in the client definition " << path_to_defs;
      throw std::runtime_error( ss.str() );
   }

   // Out put any warning's to standard output
   cout << warningMsg;
}

bool ReplaceNodeCmd::equals(ClientToServerCmd* rhs) const
{
	ReplaceNodeCmd* the_rhs = dynamic_cast<ReplaceNodeCmd*>(rhs);
	if (!the_rhs)  return false;

	if (!UserCmd::equals(rhs))  return false;

	if (createNodesAsNeeded_ != the_rhs->createNodesAsNeeded()) { return false; }
	if (force_        != the_rhs->force())        return false;
   if (pathToNode_   != the_rhs->pathToNode())   return false;
   if (path_to_defs_ != the_rhs->path_to_defs()) return false;

	if (clientDefs_ == NULL && the_rhs->theDefs() == NULL) return true;
	if (clientDefs_ == NULL && the_rhs->theDefs() != NULL) return false;
	if (clientDefs_ != NULL && the_rhs->theDefs() == NULL) return false;

	return (*clientDefs_ == *(the_rhs->theDefs()));
}

STC_Cmd_ptr ReplaceNodeCmd::doHandleRequest(AbstractServer* as) const
{
	as->update_stats().replace_++;

	assert(isWrite()); // isWrite used in handleRequest() to control check pointing
	if (clientDefs_) {

	   if (as->defs()) {

	      if (as->defs().get() == clientDefs_.get()) {
	         /// Typically will only happen with test environment
	         throw std::runtime_error("ReplaceNodeCmd::doHandleRequest: The definition in the server is the same as the client provided definition??");
	      }

	      if (force_) {
	         as->zombie_ctrl().add_user_zombies( as->defs()->findAbsNode( pathToNode_ ) );
	      }

	      std::string errorMsg;
	      if (!as->defs()->replaceChild(pathToNode_, clientDefs_, createNodesAsNeeded_, force_, errorMsg)) {
	         throw std::runtime_error(errorMsg);
	      }
	   }
	   else {
	      // The server has *NO* defs. Hence create one
	      as->create_defs();

	      // Hence replace should act as a *ADD* *IF*
	      // a/ pathToNode_ exists in clientDefs_
	      // b/ createNodesAsNeeded_ is TRUE, i.e parent option used
         std::string errorMsg;
         if (!as->defs()->replaceChild(pathToNode_, clientDefs_, createNodesAsNeeded_, force_, errorMsg)) {
            throw std::runtime_error(errorMsg);
         }
	   }

	   add_node_for_edit_history(as,pathToNode_);
 	}
   return doJobSubmission( as );
}

std::ostream& ReplaceNodeCmd::print(std::ostream& os) const
{
   std::string path_to_client_defs = path_to_defs_;
   if (path_to_client_defs.empty()) path_to_client_defs = "<empty>"; // defs must have bee loaded in memory via python api
	return user_cmd(os,CtsApi::to_string(CtsApi::replace(pathToNode_,path_to_client_defs,createNodesAsNeeded_,force_)));
}

const char* ReplaceNodeCmd::arg()  { return CtsApi::replace_arg();}
const char* ReplaceNodeCmd::desc() {
            /////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
   return
            "Replaces a node in the server, with the given path\n"
            "Can also be used to add nodes in the server\n"
            "  arg1 = path to node\n"
            "         must exist in the client defs(arg2). This is also the node we want to\n"
            "         replace in the server\n"
            "  arg2 = path to client definition file\n"
            "         provides the definition of the new node\n"
            "  arg3 = (optional) [ parent | false ] (default = parent)\n"
            "         create parent families or suite as needed, when arg1 does not\n"
            "         exist in the server\n"
            "  arg4 = (optional) force (default = false) \n"
            "         Force the replacement even if it causes zombies to be created\n"
            "Replace can fail if:\n"
            "- The node path(arg1) does not exist in the provided client definition(arg2)\n"
            "- The client definition(arg2) must be free of errors\n"
            "- If the third argument is not provided, then node path(arg1) must exist in the server\n"
            "- Nodes to be replaced are in active/submitted state, in which case arg4(force) can be used\n"
            "Usage:\n"
            "  --replace=/suite/f1/t1 /tmp/client.def  parent      # Add/replace node tree /suite/f1/t1\n"
            "  --replace=/suite/f1/t1 /tmp/client.def  false force # replace t1 even if its active or submitted";
}

void ReplaceNodeCmd::addOption(boost::program_options::options_description& desc) const {
	desc.add_options()( ReplaceNodeCmd::arg(),po::value< vector<string> >()->multitoken(), ReplaceNodeCmd::desc() );
}
void ReplaceNodeCmd::create( 	Cmd_ptr& cmd,
                             	boost::program_options::variables_map& vm,
                             	AbstractClientEnv* clientEnv ) const
{
	vector<string> args = vm[  arg() ].as< vector<string> >();

	if (clientEnv->debug())  dumpVecArgs(ReplaceNodeCmd::arg(),args);

	if (args.size() < 2 ) {
		std::stringstream ss;
		ss << "ReplaceNodeCmd: At least two arguments expected, found " << args.size()
		   << " Please specify <path-to-Node>  <defs files> parent(optional) force(optional), i.e\n"
		   << "--" <<  arg() << "=/suite/fa/t AdefsFile.def  parent force\n";
	 	throw std::runtime_error( ss.str() );
	}

	std::string pathToNode     = args[0];
	std::string pathToDefsFile = args[1];
	bool createNodesAsNeeded = true; // parent arg
	bool force = false;
	if ( args.size() == 3 && args[2] == "false") createNodesAsNeeded = false;
	if ( args.size() == 4 && args[3] == "force") force = true;

	/// If path to file does not parse, we will throw an exception
	ReplaceNodeCmd* replace_cmd = new ReplaceNodeCmd(pathToNode,createNodesAsNeeded, pathToDefsFile , force);

	// For test allow the defs environment to changed, i.e. allow us to inject  ECF_CLIENT
	replace_cmd->theDefs()->set_server().add_or_update_user_variables( clientEnv->env() );

	cmd = Cmd_ptr( replace_cmd  );
}

std::ostream& operator<<(std::ostream& os, const ReplaceNodeCmd& c) { return c.print(os); }
