/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #29 $ 
//
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description :
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8

#include <iostream>
#include <memory>
#include <boost/foreach.hpp>
#include <boost/algorithm/string/trim.hpp>

#include "ClientToServerCmd.hpp"
#include "GroupSTCCmd.hpp"
#include "AbstractServer.hpp"
#include "AbstractClientEnv.hpp"
#include "Str.hpp"
#include "CtsApi.hpp"
#include "CtsCmdRegistry.hpp"
#include "ArgvCreator.hpp"
#include "Log.hpp"

using namespace ecf;
using namespace std;
using namespace boost;
namespace po = boost::program_options;

//#define DEBUG_GROUP_CMD 1

//======================================================================================

GroupCTSCmd::GroupCTSCmd(const std::string& cmdSeries,AbstractClientEnv* clientEnv)
{
   std::vector<std::string> individualCmdVec;
   Str::split(cmdSeries,individualCmdVec,";");
   if ( individualCmdVec.empty())  throw std::runtime_error("GroupCTSCmd::GroupCTSCmd: Please provide a list of ';' separated commands\n" );
   if (clientEnv->debug()){
      for(const auto & i : individualCmdVec) { cout << "  CHILD COMMAND = " << i << "\n";}
   }


   // Create a list of allowable commands for a group. i.e excludes help, group
   po::options_description desc( "Allowed group options" );
   CtsCmdRegistry cmdRegistry( false /* don't add group option */);
   cmdRegistry.addCmdOptions(desc);


   std::string subCmd;
   for(auto aCmd : individualCmdVec){
      // massage the commands so that, we add -- at the start of each command.
      // This is required by the boost program options.
      boost::algorithm::trim(aCmd);

      subCmd.clear();
      if (aCmd.find("--") == std::string::npos)  subCmd = "--";
      subCmd += aCmd;

      // handle case like: alter add variable FRED "fre d ddy" /suite
      // If we have quote marks, then treat as one string,
      // by replacing spaces with /b, then replacing back after the split
      // This can only handle one level of quotes  hence can't cope with "fred \"joe fred\"
      bool start_quote = false;
      bool replaced_spaces =  false;
      for(char & i : subCmd) {
         if (start_quote) {
            if (i == '"' || i == '\'') start_quote = false;
            else if (i == ' ') {
               i = '\b';  // "fre d ddy"  => "fre\bd\bddy"
               replaced_spaces = true;
            }
         }
         else {
            if (i == '"' || i == '\'') start_quote = true;
         }
      }

      // Each sub command can have, many args
      std::vector<std::string> subCmdArgs;
      Str::split(subCmd,subCmdArgs);

      if (replaced_spaces) {
         for(auto & str : subCmdArgs) {
            for(char & j : str) {
               if (j == '\b') j = ' ';     // "fre\bd\bddy"  => "fre d ddy"
            }
         }
      }

      // The first will be the command, then the args. However from boost 1.59
      // we must use --cmd=value, instead of --cmd value
      if (!subCmdArgs.empty() && subCmdArgs.size() > 1 && subCmdArgs[0].find("=") == std::string::npos) {
         subCmdArgs[0] += "=";
         subCmdArgs[0] += subCmdArgs[1];
         subCmdArgs.erase( subCmdArgs.begin() + 1); // remove, since we have added to first
      }

      /// Hack because we *can't* create program option with vector of strings, which can be empty
      /// Hence if command is just show, add a dummy arg.
      //if (aCmd == "show")  subCmdArgs.push_back("<dummy_arg>");

      std::vector<std::string> theArgs;  theArgs.push_back("ClientInvoker");
      std::copy( subCmdArgs.begin(), subCmdArgs.end(), std::back_inserter(theArgs));

      // Create a Argv array from a vector of strings
      ArgvCreator argvCreator(theArgs);

      if (clientEnv->debug()) {
         cout << "  PROCESSING COMMAND = '" << subCmd << "' argc(" << argvCreator.argc() << ")";
         cout << argvCreator.toString() << "\n";
      }

      // Treat each sub command  separately
      boost::program_options::variables_map group_vm;
      po::store( po::parse_command_line( argvCreator.argc(), argvCreator.argv(), desc ), group_vm );
      po::notify( group_vm );

      Cmd_ptr childCmd;
      cmdRegistry.parse( childCmd,group_vm,clientEnv);
      addChild( childCmd );
   }
}


bool GroupCTSCmd::isWrite() const
{
 	BOOST_FOREACH(Cmd_ptr subCmd, cmdVec_) { if (subCmd->isWrite()) return true; }
 	return false;
}

bool GroupCTSCmd::get_cmd() const
{
 	BOOST_FOREACH(Cmd_ptr subCmd, cmdVec_) { if (subCmd->get_cmd()) return true; }
 	return false;
}

PrintStyle::Type_t GroupCTSCmd::show_style() const
{
   // Only return non default style( PrintStyle::NOTHING ) if sub command
   // contains a show cmd
 	BOOST_FOREACH(Cmd_ptr subCmd, cmdVec_) {
 	   if ( subCmd->show_cmd() ) return subCmd->show_style();
 	}
 	return PrintStyle::NOTHING;
}

bool GroupCTSCmd::task_cmd() const
{
 	BOOST_FOREACH(Cmd_ptr subCmd, cmdVec_) { if (subCmd->task_cmd()) return true; }
 	return false;
}

bool GroupCTSCmd::terminate_cmd() const
{
 	BOOST_FOREACH(Cmd_ptr subCmd, cmdVec_) { if (subCmd->terminate_cmd()) return true; }
 	return false;
}

bool GroupCTSCmd::why_cmd( std::string& nodePath) const
{
 	BOOST_FOREACH(Cmd_ptr subCmd, cmdVec_) { if (subCmd->why_cmd(nodePath)) return true; }
 	return false;
}

std::ostream& GroupCTSCmd::print(std::ostream& os) const
{
   std::stringstream ss;
   size_t the_size = cmdVec_.size();
	for(size_t i = 0; i < the_size; i++) {
 		cmdVec_[i]->print(ss);
 		ss <<"; ";
 	}
	return user_cmd(os,CtsApi::group(ss.str()));
}

bool GroupCTSCmd::equals(ClientToServerCmd* rhs) const
{
	GroupCTSCmd* the_rhs = dynamic_cast< GroupCTSCmd* > ( rhs );
	if ( !the_rhs ) return false;

 	const std::vector<Cmd_ptr>& rhsCmdVec = the_rhs->cmdVec();
	if (cmdVec_.size() != rhsCmdVec.size()) return false;

	for(size_t i = 0; i < cmdVec_.size(); i++) {
		if ( !cmdVec_[i]->equals( rhsCmdVec[i].get() ) ) {
			return false;
		}
	}

	return UserCmd::equals(rhs);
}

void GroupCTSCmd::addChild(Cmd_ptr childCmd)
{
	assert(childCmd.get()); // Dont add NULL children
	cmdVec_.push_back(childCmd);
}

void GroupCTSCmd::setup_user_authentification(const std::string& user, const std::string& passwd)
{
   UserCmd::setup_user_authentification(user,passwd);
 	for(auto & i : cmdVec_) {
 		i->setup_user_authentification(user,passwd);
 	}
}

void GroupCTSCmd::setup_user_authentification(AbstractClientEnv& env)
{
   UserCmd::setup_user_authentification(env);
   for(auto & i : cmdVec_) {
      i->setup_user_authentification(env);
   }
}

void GroupCTSCmd::setup_user_authentification()
{
   UserCmd::setup_user_authentification();
   for(auto & i : cmdVec_) {
      i->setup_user_authentification();
   }
}

bool GroupCTSCmd::authenticate(AbstractServer* as, STC_Cmd_ptr& errorMsg) const
{
	// Can only run Group cmd if all child commands authenticate
   size_t cmd_vec_size = cmdVec_.size();
 	for(size_t i = 0; i < cmd_vec_size; i++) {
 		if (!cmdVec_[i]->authenticate(as,errorMsg)) {

 		   // Log authentication failure:
 		   std::stringstream ss;
 		   ss << "GroupCTSCmd::authenticate failed: for ";
 		   cmdVec_[i]->print(ss);
 		   ss << errorMsg;
 		   log(Log::ERR,ss.str());    // will automatically add end of line

#ifdef	DEBUG_GROUP_CMD
 			std::cout << "GroupCTSCmd::authenticate failed for "; cmdVec_[i]->print(std::cout); std::cout << errorMsg << "\n";
#endif
 			return false;
 		}
 	}
 	return true;
}

STC_Cmd_ptr GroupCTSCmd::doHandleRequest(AbstractServer* as) const
{
#ifdef	DEBUG_GROUP_CMD
	std::cout << "GroupCTSCmd::doHandleRequest cmdVec_.size() = " << cmdVec_.size() << "\n";
#endif

	as->update_stats().group_cmd_++;

	std::shared_ptr<GroupSTCCmd> theReturnedGroupCmd = std::make_shared<GroupSTCCmd>();

	// For the command to succeed all children MUST succeed
   size_t cmd_vec_size = cmdVec_.size();
	for(size_t i = 0; i < cmd_vec_size; i++) {
#ifdef	DEBUG_GROUP_CMD
		std::cout << "  GroupCTSCmd::doHandleRequest calling "; cmdVec_[i]->print(std::cout);  // std::cout << "\n";
#endif

		STC_Cmd_ptr theReturnCmd  = cmdVec_[i]->doHandleRequest(as);

#ifdef DEBUG_GROUP_CMD
		std::cout << " return Cmd = "; theReturnCmd->print(std::cout);  std::cout << "\n";
#endif

		if ( !theReturnCmd->ok() ) {
			return theReturnCmd; // The Error Command
		}

		if ( !theReturnCmd->get_string().empty() ) {
#ifdef	DEBUG_GROUP_CMD
			std::cout << "  GroupCTSCmd::doHandleRequest returning Cmd = "; theReturnCmd->print(std::cout); std::cout << "  to client\n";
#endif
			theReturnedGroupCmd->addChild( theReturnCmd );
			continue;
		}

		if ( theReturnCmd->hasDefs() ) {
#ifdef	DEBUG_GROUP_CMD
			std::cout << "  GroupCTSCmd::doHandleRequest returning Cmd = "; theReturnCmd->print(std::cout); std::cout << "  to client\n";
#endif
			theReturnedGroupCmd->addChild( theReturnCmd );
         continue;
		}

      if ( theReturnCmd->hasNode() ) {
#ifdef   DEBUG_GROUP_CMD
         std::cout << "  GroupCTSCmd::doHandleRequest returning Cmd = "; theReturnCmd->print(std::cout); std::cout << "  to client\n";
#endif
         theReturnedGroupCmd->addChild( theReturnCmd );
      }
	}

	if ( theReturnedGroupCmd->cmdVec().empty() ) {
		// Nothing to return, i.e. no Defs, Node or Log file
		return PreAllocatedReply::ok_cmd();
	}

	return theReturnedGroupCmd ;
}

const char* GroupCTSCmd::arg() { return CtsApi::groupArg() ;}
const char* GroupCTSCmd::desc() {
   return
            "Allows a series of ';' separated commands to be grouped and executed as one.\n"
            "Some commands like halt, shutdown and terminate will prompt the user. To bypass the prompt\n"
            "provide 'yes' as an additional parameter. See example below.\n"
            "  arg = string\n"
            "Usage:\n"
            "   --group=\"halt=yes; reloadwsfile; restart;\"\n"
            "                                 # halt server,bypass the confirmation prompt,\n"
            "                                 # reload white list file, restart server\n"
            "   --group=\"get; show\"           # get server defs, and write to standard output\n"
            "   --group=\"get=/s1; show state\" # get suite 's1', and write state to standard output"
            ;
}

void GroupCTSCmd::addOption(boost::program_options::options_description& desc) const {
	desc.add_options()( GroupCTSCmd::arg(),	po::value< string >(), GroupCTSCmd::desc() );
}

void GroupCTSCmd::create( 	Cmd_ptr& cmd,
							boost::program_options::variables_map& vm,
							AbstractClientEnv* clientEnv ) const
{
   if (clientEnv->debug()) cout << "  " << arg() << ": Group Cmd '" << vm[ arg() ].as< std::string > ()  << "'\n";

   // Parse and split commands and then parse individually. Assumes commands are separated by ';'
   std::string cmdSeries = vm[GroupCTSCmd::arg()].as< std::string > ();

   cmd = Cmd_ptr( new GroupCTSCmd(cmdSeries,clientEnv) );
}

std::ostream& operator<<(std::ostream& os, const GroupCTSCmd& c) { return c.print(os); }
